#include "ButterflyPostProcess.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"

IMPLEMENT_GLOBAL_SHADER(ButterflyPostProcessShader, "/Project/UnrealMasterThesis/ButterflyPostProcess.usf", "MainCompute", SF_Compute);

FRDGTextureRef register_texture_(
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

void ButterflyPostProcessShader::BuildAndExecuteGraph(
  FRHICommandListImmediate &RHI_cmd_list,
	UTextureRenderTarget2D* input_output,
	float scale) {

	FRDGBuilder graph_builder(RHI_cmd_list);

	FRDGTextureRef io_tex_ref = register_texture_(graph_builder, input_output, "InputOutputRenderTarget");

	TShaderMapRef<ButterflyPostProcessShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FRDGTextureUAVRef input_texture_UAV = graph_builder.CreateUAV(io_tex_ref);

  FParameters* PassParameters = graph_builder.AllocParameters<ButterflyPostProcessShader::FParameters>();

  PassParameters->InputOutputTexture = input_texture_UAV;
  PassParameters->scale = scale;

  int N = input_output->SizeX;

  FComputeShaderUtils::AddPass(
    graph_builder,
    RDG_EVENT_NAME("Buttterfly Post-process Pass"),
    ComputeShader,
    PassParameters,
    FIntVector(N, N, 1)
  );

  // TODO: Not sure if the below is needed (apart from execute obviously)

	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget;
	graph_builder.QueueTextureExtraction(io_tex_ref, &PooledComputeTarget);

	graph_builder.Execute();

	RHI_cmd_list.CopyToResolveTarget(
    PooledComputeTarget.GetReference()->GetRenderTargetItem().TargetableTexture,
    input_output->GetRenderTargetResource()->TextureRHI,
    FResolveParams()
  );

}