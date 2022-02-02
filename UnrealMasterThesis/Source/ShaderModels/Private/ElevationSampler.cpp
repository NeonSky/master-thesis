#include "ElevationSampler.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

IMPLEMENT_GLOBAL_SHADER(ElevationSamplerShader, "/Project/UnrealMasterThesis/ElevationSampler.usf", "MainCompute", SF_Compute);

FRDGTextureRef register_texture2(
	FRDGBuilder& graph_builder,
	FTexture2DRHIRef rhi_ref,
	FString name) {

	FSceneRenderTargetItem RenderTargetItem;
	RenderTargetItem.TargetableTexture = rhi_ref;
	RenderTargetItem.ShaderResourceTexture = rhi_ref;
	FPooledRenderTargetDesc RenderTargetDesc = FPooledRenderTargetDesc::Create2DDesc(
		rhi_ref->GetSizeXY(),
		rhi_ref->GetFormat(),
		FClearValueBinding::Black,
		TexCreate_None,
		TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV,
		false);
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	GRenderTargetPool.CreateUntrackedElement(RenderTargetDesc, PooledRenderTarget, RenderTargetItem);

	FRDGTextureRef RDG_tex_ref = graph_builder.RegisterExternalTexture(PooledRenderTarget, *name);

	return RDG_tex_ref;
}

void ElevationSamplerShader::BuildAndExecuteGraph(
    FRHICommandListImmediate &RHI_cmd_list,
    UTextureRenderTarget2D* elevations,
    TArray<FVector2D> input_sample_coordinates,
    TArray<float>* output) {

	FRDGBuilder graph_builder(RHI_cmd_list);

  // Setup input parameters
	FParameters* PassParameters;
	PassParameters = graph_builder.AllocParameters<ElevationSamplerShader::FParameters>();

  // ElevationTexture
	PassParameters->ElevationTexture = register_texture2(graph_builder, elevations->GetRenderTargetResource()->TextureRHI->GetTexture2D(), "input_elevations");

  // InputSampleCoordinates
	FRDGBufferRef InputSampleCoordinates = CreateStructuredBuffer(
		graph_builder,
		TEXT("InputSampleCoordinates_StructuredBuffer"),
		sizeof(float),
		input_sample_coordinates.Num(),
		input_sample_coordinates.GetData(),
		sizeof(float) * input_sample_coordinates.Num()
	);
	FRDGBufferSRVRef InputSampleCoordinatesSRV = graph_builder.CreateSRV(InputSampleCoordinates, PF_R32_UINT); // PF_R32_FLOAT?
	PassParameters->InputSampleCoordinates = InputSampleCoordinatesSRV;

  // OutputBuffer
	FRDGBufferRef OutputBuffer = CreateStructuredBuffer(
		graph_builder,
		TEXT("OutputBuffer_StructuredBuffer"),
		sizeof(float),
		input_sample_coordinates.Num(),
		input_sample_coordinates.GetData(),
		sizeof(float) * input_sample_coordinates.Num()
	);
	FRDGBufferUAVRef OutputBufferUAV = graph_builder.CreateUAV(OutputBuffer, PF_R32_UINT); // PF_R32_FLOAT?
	PassParameters->OutputBuffer = OutputBufferUAV;

  // Call compute shader
	TShaderMapRef<ElevationSamplerShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
  FComputeShaderUtils::AddPass(
    graph_builder,
    RDG_EVENT_NAME("ElevationSampler Pass"),
    ComputeShader,
    PassParameters,
    FIntVector(input_sample_coordinates.Num(), 1, 1)
  );


  // Request output extraction
	TRefCountPtr<FRDGPooledBuffer> PooledComputeTarget;
	graph_builder.QueueBufferExtraction(OutputBuffer, &PooledComputeTarget);
	// graph_builder.QueueBufferExtraction(OutputBuffer, &PooledComputeTarget, FRDGSubresourceState::EAccess::Read);

	graph_builder.Execute();

	output->SetNum(input_sample_coordinates.Num());

	// Copy result to CPU
	void *source = RHI_cmd_list.LockStructuredBuffer(PooledComputeTarget->GetStructuredBufferRHI(), 0, sizeof(float) * input_sample_coordinates.Num(), RLM_ReadOnly);
	FMemory::Memcpy(output->GetData(), source, sizeof(float) * input_sample_coordinates.Num());
	RHI_cmd_list.UnlockStructuredBuffer(PooledComputeTarget->GetStructuredBufferRHI());

	UE_LOG(LogTemp, Warning, TEXT("First Output"));
	for (int i = 0; i < output->Num(); i++) {
		UE_LOG(LogTemp, Warning, TEXT("i = %i: %f"), i, (*output)[i]);
	}

	// FComputeShader::CopyBuffer(
	// 	RHI_cmd_list,
	// 	PooledComputeTarget,
	// 	output->GetData(),
	// 	sizeof(FVector) * parameters.verticies.Num()
	// );

  // output->SetNum(0, true);
  // output->Push(1);
  // output->Push(2);
  // output->Push(3);
  // output->Push(7);
}