#include "Obstruction.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

#define NN 256

IMPLEMENT_GLOBAL_SHADER(ObstructionShader, "/Project/UnrealMasterThesis/Obstruction.usf", "eWaveCompute", SF_Compute);

FRDGTextureRef register_texture4_obs(
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

struct CustomUAV {
    FRDGTextureRef ref;
    FRDGTextureUAVRef uav_ref;
};

CustomUAV create_UAV4_obs( // TODO: this and register texture defined in fourier components.
    FRDGBuilder& graph_builder,
    UTextureRenderTarget2D* rtt,
    FString name) {

    FRDGTextureDesc OutTextureDesc = FRDGTextureDesc::Create2D(
        FIntPoint(rtt->SizeX, rtt->SizeY),
        PF_FloatRGBA,
        FClearValueBinding(),
        TexCreate_UAV,
        1,
        1);
    FRDGTextureRef OutTextureRef = graph_builder.CreateTexture(OutTextureDesc, *name);
    FRDGTextureUAVDesc OutTextureUAVDesc(OutTextureRef);

    CustomUAV uav;
    uav.ref = OutTextureRef;
    uav.uav_ref = graph_builder.CreateUAV(OutTextureUAVDesc);

    return uav;
}

void ReadbackRTT3_obs(FRHICommandListImmediate& RHI_cmd_list, UTextureRenderTarget2D* rtt) {
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

    for (int i = 0; i < data.Num(); i++) {
        UE_LOG(LogTemp, Warning, TEXT("%i: (%f, %f, %f, %f)"), i, data[i].R.GetFloat(), data[i].G.GetFloat(), data[i].B.GetFloat(), data[i].A.GetFloat());
    }

}

void ObstructionShader::BuildAndExecuteGraph(
    FRHICommandListImmediate& RHI_cmd_list,
    UTextureRenderTarget2D* boat_rtt,
    TRefCountPtr<FRDGPooledBuffer> submerged_triangles,
    UTextureRenderTarget2D* obstructionMap_rtt,
    UTextureRenderTarget2D* hv_rtt,
    UTextureRenderTarget2D* hv_prev_rtt,
    int preFFT) {

    FRDGBuilder graph_builder(RHI_cmd_list);

    FParameters* PassParameters;
    PassParameters = graph_builder.AllocParameters<ObstructionShader::FParameters>();

    PassParameters->BoatTexture = register_texture4_obs(graph_builder, boat_rtt, "BoatRTT");

    FRDGBufferRef RDG_ref = graph_builder.RegisterExternalBuffer(submerged_triangles, TEXT("SubmergedTriangles_StructuredBuffer"), ERDGBufferFlags::MultiFrame);
    PassParameters->SubmergedTrianglesBuffer = graph_builder.CreateSRV(RDG_ref);

    PassParameters->preFFT = preFFT;

	FRDGTextureRef io_tex_ref = register_texture4_obs(graph_builder, obstructionMap_rtt, "InputOutputRenderTarget");
    auto uav = graph_builder.CreateUAV(io_tex_ref);
    FRDGTextureRef io_tex_ref2 = register_texture4_obs(graph_builder, hv_rtt, "InputOutputRenderTarget2");
    auto uav2 = graph_builder.CreateUAV(io_tex_ref2);
    FRDGTextureRef io_tex_ref3 = register_texture4_obs(graph_builder, hv_prev_rtt, "InputOutputRenderTarget3");
    auto uav3 = graph_builder.CreateUAV(io_tex_ref3);

    PassParameters->obstructionMap_rtt = uav;
    PassParameters->hv_rtt = uav2;
    PassParameters->hv_prev_rtt = uav3;

    TShaderMapRef<ObstructionShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    int x_size = preFFT == 2 ? 1 : NN / 8;

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("Obstruction Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(x_size, x_size, 1)
    );

    graph_builder.Execute();
}
