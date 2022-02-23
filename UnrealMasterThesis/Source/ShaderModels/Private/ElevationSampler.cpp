#include "ElevationSampler.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

IMPLEMENT_GLOBAL_SHADER(ElevationSamplerShader, "/Project/UnrealMasterThesis/ElevationSampler.usf", "MainCompute", SF_Compute);

// FRDGTextureRef register_texture2(
// 	FRDGBuilder& graph_builder,
// 	FTexture2DRHIRef rhi_ref,
// 	FString name) {

// 	FSceneRenderTargetItem RenderTargetItem;
// 	RenderTargetItem.TargetableTexture = rhi_ref;
// 	RenderTargetItem.ShaderResourceTexture = rhi_ref;
// 	FPooledRenderTargetDesc RenderTargetDesc = FPooledRenderTargetDesc::Create2DDesc(
// 		rhi_ref->GetSizeXY(),
// 		rhi_ref->GetFormat(),
// 		FClearValueBinding::Black,
// 		TexCreate_None,
// 		TexCreate_RenderTargetable | TexCreate_ShaderResource,
// 		false);
// 	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
// 	GRenderTargetPool.CreateUntrackedElement(RenderTargetDesc, PooledRenderTarget, RenderTargetItem);

// 	FRDGTextureRef RDG_tex_ref = graph_builder.RegisterExternalTexture(PooledRenderTarget, *name);

// 	return RDG_tex_ref;
// }
FRDGTextureRef register_texture2(
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

void ElevationSamplerShader::BuildAndExecuteGraph(
    FRHICommandListImmediate &RHI_cmd_list,
    UTextureRenderTarget2D* elevations,
    TArray<FVector2D> input_sample_coordinates,
    TArray<float>* output) {

	int N = input_sample_coordinates.Num();

	FRDGBuilder graph_builder(RHI_cmd_list);

  // Setup input parameters
	FParameters* PassParameters;
	PassParameters = graph_builder.AllocParameters<ElevationSamplerShader::FParameters>();

  // elevation_texture
	PassParameters->elevation_texture = register_texture2(graph_builder, elevations, "input_elevations");

  // input_sample_coordinates
	FRDGBufferRef InputSampleCoordinates = CreateStructuredBuffer(
		graph_builder,
		TEXT("InputSampleCoordinates_StructuredBuffer"),
		sizeof(FVector2D),
		N,
		input_sample_coordinates.GetData(),
		sizeof(FVector2D) * N
	);
	FRDGBufferSRVRef InputSampleCoordinatesSRV = graph_builder.CreateSRV(InputSampleCoordinates, PF_R32_UINT); // PF_R32_FLOAT?
	PassParameters->input_sample_coordinates = InputSampleCoordinatesSRV;

  // output_texture
	FRDGTextureDesc OutTextureDesc = FRDGTextureDesc::Create2D(
		FIntPoint(N, 1),
		PF_FloatRGBA, // TODO: R32?
		FClearValueBinding(),
		TexCreate_UAV,
		1,
		1); 
	FRDGTextureRef tex_ref = graph_builder.CreateTexture(OutTextureDesc, TEXT("output_texture_ref"));
	FRDGTextureUAVDesc OutTextureUAVDesc(tex_ref);
  FRDGTextureUAVRef tex_uav_ref = graph_builder.CreateUAV(OutTextureUAVDesc);
	PassParameters->output_texture = tex_uav_ref;

  // Call compute shader
	TShaderMapRef<ElevationSamplerShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
  FComputeShaderUtils::AddPass(
    graph_builder,
    RDG_EVENT_NAME("ElevationSampler Pass"),
    ComputeShader,
    PassParameters,
    FIntVector(N, 1, 1)
  );


  // Request output extraction
	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget;
	graph_builder.QueueTextureExtraction(tex_ref, &PooledComputeTarget);

	graph_builder.Execute();

  {
    FRHIResourceCreateInfo CreateInfo;
    FTexture2DRHIRef readback_tex = RHICreateTexture2D(
      N,
      1,
      PF_FloatRGBA,
      1,
      1,
      TexCreate_RenderTargetable,
      CreateInfo);

    RHI_cmd_list.CopyToResolveTarget(
      PooledComputeTarget.GetReference()->GetRenderTargetItem().TargetableTexture,
      readback_tex->GetTexture2D(),
      FResolveParams()
    );

    FReadSurfaceDataFlags read_flags(RCM_MinMax);
    read_flags.SetLinearToGamma(false);

    TArray<FFloat16Color> rdata;
    RHI_cmd_list.ReadSurfaceFloatData(
      readback_tex->GetTexture2D(),
      FIntRect(0, 0, N, 1),
      rdata,
      read_flags
    );

    // UE_LOG(LogTemp, Warning, TEXT("READBACK START"));
	output->SetNum(rdata.Num());
	for (int i = 0; i < rdata.Num(); i++) {
		// UE_LOG(LogTemp, Warning, TEXT("%i: %f"), i, rdata[i].R.GetFloat());
		(*output)[i] = rdata[i].R.GetFloat();
    }

  }
}