#include "Serialize.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 256

IMPLEMENT_GLOBAL_SHADER(SerializeShader, "/Project/UnrealMasterThesis/Serialize.usf", "mainCompute", SF_Compute);

FRDGTextureRef register_texture_serialize(
	FRDGBuilder& graph_builder,
	UTextureRenderTarget2D* render_target,
	FString name) {

	FRenderTarget* RenderTargetResource = render_target->GetRenderTargetResource();
	FTexture2DRHIRef RenderTargetRHI = RenderTargetResource->GetRenderTargetTexture();

	FSceneRenderTargetItem RenderTargetItem;
	RenderTargetItem.TargetableTexture = RenderTargetRHI;
	RenderTargetItem.ShaderResourceTexture = RenderTargetRHI;
	FPooledRenderTargetDesc RenderTargetDesc = FPooledRenderTargetDesc::Create2DDesc(
		RenderTargetResource->GetSizeXY(),
		RenderTargetRHI->GetFormat(),
		FClearValueBinding::Black,
		TexCreate_None,
		TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV,
		false);
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	GRenderTargetPool.CreateUntrackedElement(RenderTargetDesc, PooledRenderTarget, RenderTargetItem);

	FRDGTextureRef RDG_tex_ref = graph_builder.RegisterExternalTexture(PooledRenderTarget, *name);

	return RDG_tex_ref;
}

TArray<FFloat16Color> readback_RTT_serialize(FRHICommandListImmediate& RHI_cmd_list, UTextureRenderTarget2D* rtt) {
	FRHIResourceCreateInfo CreateInfo;
	FTexture2DRHIRef readback_tex = RHICreateTexture2D(
		rtt->SizeX,
		rtt->SizeY,
		PF_FloatRGBA,
		1,
		1,
		TexCreate_RenderTargetable,
		CreateInfo);

	RHI_cmd_list.CopyToResolveTarget(
		rtt->GetRenderTargetResource()->TextureRHI,
		readback_tex->GetTexture2D(),
		FResolveParams()
	);

	FReadSurfaceDataFlags read_flags(RCM_MinMax);
	read_flags.SetLinearToGamma(false);

	TArray<FFloat16Color> data;
	RHI_cmd_list.ReadSurfaceFloatData(
		readback_tex->GetTexture2D(),
		FIntRect(0, 0, rtt->SizeX, rtt->SizeY),
		data,
		read_flags
	);

	return data;
}

void SerializeShader::BuildAndExecuteGraph(
    FRHICommandListImmediate& RHI_cmd_list,
    UTextureRenderTarget2D* input_rtt,
    UTextureRenderTarget2D* serialize_rtt,
    TArray<FFloat16Color>* data) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<SerializeShader::FParameters>();

	FRDGTextureRef io_tex_ref = register_texture_serialize(graph_builder, input_rtt, "InputRenderTarget");
    auto uav_input = graph_builder.CreateUAV(io_tex_ref);
    FRDGTextureRef io_tex_ref2 = register_texture_serialize(graph_builder, serialize_rtt, "SerializeRenderTarget");
    auto uav_serialize = graph_builder.CreateUAV(io_tex_ref2);

    PassParameters->input_rtt = uav_input;
    PassParameters->serialize_rtt = uav_serialize;
    

    TShaderMapRef<SerializeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("Serialize Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(NN / 8, NN / 8, 1)
    );

    TRefCountPtr<IPooledRenderTarget> PooledComputeTarget;
    graph_builder.QueueTextureExtraction(io_tex_ref2, &PooledComputeTarget);
 

    graph_builder.Execute();

    RHI_cmd_list.CopyToResolveTarget(
        PooledComputeTarget.GetReference()->GetRenderTargetItem().TargetableTexture,
        serialize_rtt->GetRenderTargetResource()->TextureRHI,
        FResolveParams()
    );
    *data = readback_RTT_serialize(RHI_cmd_list, serialize_rtt);
}
