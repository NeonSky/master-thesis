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
		sizeof(FVector2D),
		input_sample_coordinates.Num(),
		input_sample_coordinates.GetData(),
		sizeof(FVector2D) * input_sample_coordinates.Num()
	);
	FRDGBufferSRVRef InputSampleCoordinatesSRV = graph_builder.CreateSRV(InputSampleCoordinates, PF_R32_UINT); // PF_R32_FLOAT?
	PassParameters->InputSampleCoordinates = InputSampleCoordinatesSRV;

  // OutputBuffer
	// FRDGBufferRef OutputBuffer = CreateStructuredBuffer(
	// 	graph_builder,
	// 	TEXT("OutputBuffer_StructuredBuffer"),
	// 	sizeof(float),
	// 	input_sample_coordinates.Num(),
	// 	input_sample_coordinates.GetData(),
	// 	sizeof(float) * input_sample_coordinates.Num()
	// );
  FRDGBufferDesc OutputBufferDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector4), input_sample_coordinates.Num());
  FRDGBufferRef OutputBuffer = graph_builder.CreateBuffer(OutputBufferDesc, TEXT("OutputBuffer"));
	FRDGBufferUAVDesc OBuffer(OutputBuffer);
  FRDGBufferUAVRef OutputBufferUAV = graph_builder.CreateUAV(OBuffer);
	// FRDGBufferUAVRef OutputBufferUAV = graph_builder.CreateUAV(OutputBuffer, PF_R32_UINT); // PF_R32_FLOAT?
	PassParameters->OutputBuffer = OutputBufferUAV;

	// Remove me
	FRDGTextureDesc OutTextureDesc = FRDGTextureDesc::Create2D(
		FIntPoint(elevations->SizeX, elevations->SizeY),
		PF_FloatRGBA,
		FClearValueBinding(),
		TexCreate_UAV,
		1,
		1); 
	FRDGTextureRef tex_ref = graph_builder.CreateTexture(OutTextureDesc, TEXT("test_output_ref"));
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
	graph_builder.QueueBufferExtraction(OutputBuffer, &PooledComputeTarget, ERHIAccess::CPURead);
	// graph_builder.QueueBufferExtraction(OutputBuffer, &PooledComputeTarget, FRDGSubresourceState::EAccess::Read);


	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget2;
	graph_builder.QueueTextureExtraction(tex_ref, &PooledComputeTarget2);

	TRefCountPtr<FRDGPooledBuffer> PooledComputeTarget3;
	graph_builder.QueueBufferExtraction(InputSampleCoordinates, &PooledComputeTarget3, ERHIAccess::CPURead);

	graph_builder.Execute();

	RHI_cmd_list.SubmitCommandsAndFlushGPU();


	// https://github.com/pramberg/DeviceRGB/blob/a2020b2b5accb1fc864c16905c35c352d3e4f39b/Source/DeviceRGB/Private/DeviceRGBViewExtension.cpp#L376
	auto r = PooledComputeTarget3->GetStructuredBufferRHI();

	FVector2D* serv_source = (FVector2D*) RHI_cmd_list.LockStructuredBuffer(r, 0, sizeof(FVector2D) * input_sample_coordinates.Num(), RLM_WriteOnly);
	serv_source[0] = FVector2D(3.7f, 1.2f);
	serv_source[1] = FVector2D(0.1f, 0.5f);
	RHI_cmd_list.UnlockStructuredBuffer(r);

	FVector2D* srv_source = (FVector2D*) RHI_cmd_list.LockStructuredBuffer(r, 0, sizeof(FVector2D) * input_sample_coordinates.Num(), RLM_ReadOnly);
	UE_LOG(LogTemp, Warning, TEXT("SRVV Readback"));
	for (int i = 0; i < input_sample_coordinates.Num(); i++) {
		UE_LOG(LogTemp, Warning, TEXT("i = %i: %f, %f"), i, srv_source[i].X, srv_source[i].Y);
	}

	TArray<FVector2D> r_output;
	r_output.SetNum(input_sample_coordinates.Num());
	uint32 sz = sizeof(FVector2D) * input_sample_coordinates.Num();
	UE_LOG(LogTemp, Warning, TEXT("sz = %u"), sz);
	FMemory::Memcpy(r_output.GetData(), srv_source, sz);
	RHI_cmd_list.UnlockStructuredBuffer(r);

	// Copy result to CPU
	FVector4* source = (FVector4*) RHI_cmd_list.LockStructuredBuffer(PooledComputeTarget->GetStructuredBufferRHI(), 0, sizeof(FVector4) * input_sample_coordinates.Num(), RLM_ReadOnly);
	UE_LOG(LogTemp, Warning, TEXT("Even First Output"));
	for (int i = 0; i < input_sample_coordinates.Num(); i++) {
		UE_LOG(LogTemp, Warning, TEXT("i = %i: %f, %f, %f"), i, source[i].X, source[i].Y, source[i].Z);
	}

	TArray<FVector4> t_output;
	t_output.SetNum(input_sample_coordinates.Num());
	FMemory::Memcpy(t_output.GetData(), source, sizeof(FVector4) * input_sample_coordinates.Num());
	RHI_cmd_list.UnlockStructuredBuffer(PooledComputeTarget->GetStructuredBufferRHI());

	UE_LOG(LogTemp, Warning, TEXT("First Output"));
	for (int i = 0; i < t_output.Num(); i++) {
		UE_LOG(LogTemp, Warning, TEXT("i = %i: %f %f %f"), i, t_output[i].X, t_output[i].Y, t_output[i].Z);
	}

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