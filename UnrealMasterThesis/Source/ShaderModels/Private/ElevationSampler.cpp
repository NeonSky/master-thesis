#include "ElevationSampler.h"

#include <algorithm>

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TextureResource.h"
#include "Engine/Texture2DArray.h"

IMPLEMENT_GLOBAL_SHADER(ElevationSamplerShader, "/Project/UnrealMasterThesis/ElevationSampler.usf", "MainCompute", SF_Compute);

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
	TArray<UTextureRenderTarget2D*> wake_rtts,
	TArray<FVector2D> ws_boat_coords,
	TArray<FVector2D> input_sample_coordinates,
    TArray<float>* output) {

	int N = input_sample_coordinates.Num();

	FRDGBuilder graph_builder(RHI_cmd_list);

  // Setup input parameters
	FParameters* PassParameters;
	PassParameters = graph_builder.AllocParameters<ElevationSamplerShader::FParameters>();

  // elevation_texture
	PassParameters->elevation_texture = register_texture2(graph_builder, elevations, "input_elevations");

	PassParameters->wake_textures[0] = register_texture2(graph_builder, wake_rtts[0], "wake_rtt");

	// See comment in ElevationSampler.usf
	if (wake_rtts.Num() > 1) {
		PassParameters->wake_textures[1] = register_texture2(graph_builder, wake_rtts[1], "wake_rtt2");
	} else {
		// Assign arbitrary valid texture to prevent crash. It will not be used anyway.
		PassParameters->wake_textures[1] = register_texture2(graph_builder, wake_rtts[0], "wake_rtt2");
	}

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
		PF_FloatRGBA,
		FClearValueBinding(),
		TexCreate_UAV,
		1,
		1); 
	FRDGTextureRef tex_ref = graph_builder.CreateTexture(OutTextureDesc, TEXT("output_texture_ref"));
	FRDGTextureUAVDesc OutTextureUAVDesc(tex_ref);
  FRDGTextureUAVRef tex_uav_ref = graph_builder.CreateUAV(OutTextureUAVDesc);
	PassParameters->output_texture = tex_uav_ref;

	for (int i = 0; i < std::min(PassParameters->ws_boat_coords.Num(), ws_boat_coords.Num()); i++) {
		PassParameters->ws_boat_coords[i] = ws_boat_coords[i];
	}

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

	output->SetNum(rdata.Num());
	for (int i = 0; i < rdata.Num(); i++) {

		uint32_t p1  = uint32_t(rdata[i].R.GetFloat());
		uint32_t p2  = uint32_t(rdata[i].G.GetFloat());
		uint32_t p3  = uint32_t(rdata[i].B.GetFloat());
		uint32_t p4  = uint32_t(rdata[i].A.GetFloat());
		uint32_t v = (p4 << 24 | p3 << 16 | p2 << 8 | p1);

		float res;
		memcpy(&res, &v, sizeof(float));
		
		(*output)[i] = res;

    }

  }
}