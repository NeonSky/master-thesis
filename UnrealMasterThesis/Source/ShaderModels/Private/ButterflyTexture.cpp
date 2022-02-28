#include "ButterflyTexture.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"

IMPLEMENT_GLOBAL_SHADER(ButterflyTextureShader, "/Project/UnrealMasterThesis/ButterflyTexture.usf", "MainCompute", SF_Compute);

uint32 reverse_bits(uint32 x, uint32 range) {
	uint32 res = 0;
	for (int i = range - 1; i >= 0; i--) {
		res |= (x & 1) << i;
		x >>= 1;
	}
	return res;
}

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

void ButterflyTextureShader::BuildAndExecuteGraph(
	FRHICommandListImmediate& RHI_cmd_list,
	UTextureRenderTarget2D* output) {

	FRDGBuilder graph_builder(RHI_cmd_list);

	FParameters* PassParameters;
	PassParameters = graph_builder.AllocParameters<ButterflyTextureShader::FParameters>();

	PassParameters->N = output->SizeY;

	int n_stages = output->SizeX;
	TArray<uint32> ReverseBits;
	for (uint32 x = 0; (int)x < output->SizeY; x++) {
		ReverseBits.Add(reverse_bits(x, n_stages));
		// UE_LOG(LogTemp, Warning, TEXT("%u -> %u"), x, reverse_bits(x, n_stages));
	}

	FRDGBufferRef ReverseBitsBuffer = CreateStructuredBuffer(
		graph_builder,
		TEXT("ReverseBits_StructuredBuffer"),
		sizeof(uint32),
		ReverseBits.Num(),
		ReverseBits.GetData(),
		sizeof(uint32) * ReverseBits.Num()
	);
	FRDGBufferSRVRef ReverseBitsSRV = graph_builder.CreateSRV(ReverseBitsBuffer, PF_R32_UINT);
	PassParameters->ReverseBits = ReverseBitsSRV;

	FRDGTextureRef output_tex_ref = register_texture3(graph_builder, output, "InputOutputRenderTarget");
	PassParameters->OutputTexture = graph_builder.CreateUAV(output_tex_ref);


	// ------ Add the compute pass ------
	// Get a reference to our global shader class
	TShaderMapRef<ButterflyTextureShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	// Add the compute shader pass to the render graph
	FComputeShaderUtils::AddPass(
		graph_builder,
		RDG_EVENT_NAME("Butterfly Texture Pass"),
		ComputeShader,
		PassParameters,
		FIntVector(output->SizeX, output->SizeY, 1) // This gets multiplied by `numthreads` in the .usf shader, kinda.
	);

	graph_builder.Execute();
}
