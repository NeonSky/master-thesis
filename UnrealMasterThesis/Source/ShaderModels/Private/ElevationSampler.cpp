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

	// Remove me
	// FRHIResourceCreateInfo CreateInfo;
	// FTexture2DRHIRef readback_tex = RHICreateTexture2D(
	// 	4,
	// 	4,
	// 	PF_FloatRGBA,
	// 	1,
	// 	1,
	// 	TexCreate_RenderTargetable,
	// 	CreateInfo);
	FRDGTextureDesc OutTextureDesc = FRDGTextureDesc::Create2D(
		FIntPoint(elevations->SizeX, elevations->SizeY),
		PF_FloatRGBA,
		FClearValueBinding(),
		TexCreate_UAV,
		1,
		1); 
	FRDGTextureRef tex_ref = graph_builder.CreateTexture(OutTextureDesc, *FString("test_output_ref"));
	FRDGTextureUAVDesc OutTextureUAVDesc(tex_ref);
  FRDGTextureUAVRef tex_uav_ref = graph_builder.CreateUAV(OutTextureUAVDesc);
	PassParameters->test_output = tex_uav_ref;

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


	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget2;
	graph_builder.QueueTextureExtraction(tex_ref, &PooledComputeTarget2);

	graph_builder.Execute();

	// Copy result to CPU
	void *source = RHI_cmd_list.LockStructuredBuffer(PooledComputeTarget->GetStructuredBufferRHI(), 0, sizeof(float) * input_sample_coordinates.Num(), RLM_ReadOnly);
	output->SetNum(input_sample_coordinates.Num());
	FMemory::Memcpy(output->GetData(), source, sizeof(float) * input_sample_coordinates.Num());
	RHI_cmd_list.UnlockStructuredBuffer(PooledComputeTarget->GetStructuredBufferRHI());

	UE_LOG(LogTemp, Warning, TEXT("First Output"));
	for (int i = 0; i < output->Num(); i++) {
		UE_LOG(LogTemp, Warning, TEXT("i = %i: %f"), i, (*output)[i]);
	}

	// RHI_cmd_list.CopyToResolveTarget(
  //   PooledComputeTarget2.GetReference()->GetRenderTargetItem().TargetableTexture,
  //   elevations->GetRenderTargetResource()->TextureRHI,
  //   FResolveParams()
  // );

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

  {
    FRHIResourceCreateInfo CreateInfo;
    FTexture2DRHIRef readback_tex = RHICreateTexture2D(
      elevations->SizeX,
      elevations->SizeY,
      PF_FloatRGBA,
      1,
      1,
      TexCreate_RenderTargetable,
      CreateInfo);

    RHI_cmd_list.CopyToResolveTarget(
      PooledComputeTarget2.GetReference()->GetRenderTargetItem().TargetableTexture,
      readback_tex->GetTexture2D(),
      FResolveParams()
    );

    // UE_LOG(LogTemp, Warning, TEXT("READBACK START"));

    FReadSurfaceDataFlags read_flags(RCM_MinMax);
    read_flags.SetLinearToGamma(false);

    TArray<FFloat16Color> rdata;
    RHI_cmd_list.ReadSurfaceFloatData(
      readback_tex->GetTexture2D(),
      FIntRect(0, 0, elevations->SizeX, elevations->SizeY),
      rdata,
      read_flags
    );

    UE_LOG(LogTemp, Warning, TEXT("Amount of pixels: %i"), rdata.Num());
    // for (int i = 0; i < rdata.Num(); i++) {
    for (int i = 0; i < 4; i++) {
      UE_LOG(LogTemp, Warning, TEXT("%i: (%f, %f, %f, %f)"), i, rdata[i].R.GetFloat(), rdata[i].G.GetFloat(), rdata[i].B.GetFloat(), rdata[i].A.GetFloat());
    }
    UE_LOG(LogTemp, Warning, TEXT("READBACK END"));
  }
}