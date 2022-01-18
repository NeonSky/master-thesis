#include "ButterflyTexture.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"

IMPLEMENT_GLOBAL_SHADER(ButterflyTextureShader, "/Project/UnrealMasterThesis/ButterflyTexture.usf", "MainCompute", SF_Compute);

uint32 reverse_bits(uint32 x, uint32 range) {
	uint32 res = 0;
	for (int i = range-1; i >= 0; i--){
			res |= (x & 1) << i;
			x >>= 1;
	}
	return res;
}

void ButterflyTextureShader::BuildAndExecuteGraph(
  FRHICommandListImmediate &RHI_cmd_list,
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

	FRDGTextureDesc OutTextureDesc = FRDGTextureDesc::Create2D(
		FIntPoint(output->SizeX, output->SizeY),
		PF_FloatRGBA,
		FClearValueBinding(),
		TexCreate_UAV,
		1,
		1); 
	FRDGTextureRef OutTextureRef = graph_builder.CreateTexture(OutTextureDesc, TEXT("Compute_Out_Texture"));
	FRDGTextureUAVDesc OutTextureUAVDesc(OutTextureRef);
	PassParameters->OutputTexture = graph_builder.CreateUAV(OutTextureUAVDesc);


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


	// ------ Extracting to pooled render target ------
	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget;
	// Copy the result of compute shader from UAV to pooled render target
	graph_builder.QueueTextureExtraction(OutTextureRef, &PooledComputeTarget);

	graph_builder.Execute();

	// Queue the UAV we wrote to for extraction 
	// I.e. copy UAV result on the GPU to our render target (on the CPU?).
	RHI_cmd_list.CopyToResolveTarget(
    PooledComputeTarget.GetReference()->GetRenderTargetItem().TargetableTexture,
    output->GetRenderTargetResource()->TextureRHI,
    FResolveParams()
  );

}