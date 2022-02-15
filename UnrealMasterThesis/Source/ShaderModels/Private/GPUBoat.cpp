#include "GPUBoat.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"

IMPLEMENT_GLOBAL_SHADER(GPUBoatShader, "/Project/UnrealMasterThesis/GPUBoat.usf", "MainCompute", SF_Compute);

FRDGTextureRef register_texture3(
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

TArray<FFloat16Color> readback_RTT(FRHICommandListImmediate &RHI_cmd_list, UTextureRenderTarget2D* rtt) {
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

void GPUBoatShader::BuildAndExecuteGraph(
    FRHICommandListImmediate &RHI_cmd_list,
    float speed_input,
    FVector2D velocity_input,
    UTextureRenderTarget2D* elevation_texture,
    UTextureRenderTarget2D* input_output,
	TArray<FFloat16Color>* readback_target) {

	// We will use this to build a Rendering Dependency Graph (RDG).
	FRDGBuilder graph_builder(RHI_cmd_list);

	FRDGTextureRef h_tex_ref = register_texture3(graph_builder, elevation_texture, "ElevationRenderTarget");
	FRDGTextureRef io_tex_ref = register_texture3(graph_builder, input_output, "InputOutputRenderTarget");
	FRDGTextureUAVRef io_tex_UAV = graph_builder.CreateUAV(io_tex_ref);

	TShaderMapRef<GPUBoatShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FParameters* PassParameters = graph_builder.AllocParameters<GPUBoatShader::FParameters>();
    PassParameters->SpeedInput         = speed_input;
    PassParameters->VelocityInput      = velocity_input;
    PassParameters->ElevationTexture   = h_tex_ref;
    PassParameters->InputOutputTexture = io_tex_UAV;

    FComputeShaderUtils::AddPass(
        graph_builder,
        RDG_EVENT_NAME("GPU Boat Pass"),
        ComputeShader,
        PassParameters,
        FIntVector(1, 1, 1)
    );

	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget;
    if (readback_target) {
        graph_builder.QueueTextureExtraction(io_tex_ref, &PooledComputeTarget);
    }

	graph_builder.Execute();

    if (readback_target) {
        RHI_cmd_list.CopyToResolveTarget(
            PooledComputeTarget.GetReference()->GetRenderTargetItem().TargetableTexture,
            input_output->GetRenderTargetResource()->TextureRHI,
            FResolveParams()
        );
       *readback_target = readback_RTT(RHI_cmd_list, input_output);
    }

}