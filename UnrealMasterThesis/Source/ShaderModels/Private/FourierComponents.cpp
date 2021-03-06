#include "FourierComponents.h"
#include "Globals/StatelessHelpers.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

IMPLEMENT_GLOBAL_SHADER(FourierComponentsShader, "/Project/UnrealMasterThesis/FourierComponents.usf", "MainCompute", SF_Compute);

FRDGTextureRef register_texture(
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

struct CustomUAV {
  FRDGTextureRef ref;
  FRDGTextureUAVRef uav_ref;
};

CustomUAV create_UAV(
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

TArray<FFloat16Color> create_init_data(int N, float L, std::function<float (FVector2D)> wave_spectrum, int seed) {

  std::random_device rd{};
  std::mt19937 rng{rd()};
  std::normal_distribution<float> dist;
  rng.seed(seed);

  TArray<FFloat16Color> res;

  for (int z = 0; z < N; z++) {
    for (int x = 0; x < N; x++) {

      FVector2D wave_vector = FVector2D(
        2.0 * PI * (z - floor(N / 2.0)) / L,
        2.0 * PI * (x - floor(N / 2.0)) / L
      );

      FVector2D neg_wave_vector = FVector2D(
        -2.0 * PI * (z - floor(N / 2.0)) / L,
        -2.0 * PI * (x - floor(N / 2.0)) / L
      );

      float xi_r = dist(rng);
      float xi_i = dist(rng);
      std::complex<float> complex_rv(xi_r, xi_i);

      xi_r = dist(rng);
      xi_i = dist(rng);
      std::complex<float> complex_rv2(xi_r, xi_i);

      float delta_k = 2.0f * PI / L;

      std::complex<float> h0 = sqrt(1.0f / 2.0f) * complex_rv * sqrt(2.0f * wave_spectrum(wave_vector) * delta_k * delta_k);
      std::complex<float> h0_conj = std::conj(sqrt(1.0f / 2.0f) * complex_rv2 * sqrt(2.0f * wave_spectrum(neg_wave_vector) * delta_k * delta_k));

      res.Add(FFloat16Color(FLinearColor(h0.real(), h0.imag(), h0_conj.real(), h0_conj.imag())));
    }
  }

  return res;

}

void FourierComponentsShader::Buildh0Textures(int N, float L, std::function<float (FVector2D)> wave_spectrum, int seed) {

  this->m_N = N;

  TArray<FFloat16Color> init_data = create_init_data(N, L, wave_spectrum, seed);

  //////////////////////////////
  //////////////////////////////
  //////////////////////////////

  {
    FRHIResourceCreateInfo CreateInfo;
    FTexture2DRHIRef Texture2DRHI = RHICreateTexture2D(
      N,
      N,
      PF_FloatRGBA,
      1,
      1,
      TexCreate_RenderTargetable,
      CreateInfo);

    TArray<FFloat16Color> pixel_data;
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < N; j++) {
        // pixel_data.Add(FFloat16Color(FLinearColor(i, j, 0.0, 1.0))); // Dummy data for now
        pixel_data.Add(FFloat16Color(FLinearColor(init_data[i*N+j].R, init_data[i*N+j].G, 0.0, 1.0)));
      }
    }

    uint32 DestStride = 0;
    FFloat16Color* data = (FFloat16Color*) RHILockTexture2D(Texture2DRHI, 0, RLM_WriteOnly, DestStride, false);
    FMemory::Memcpy(data, pixel_data.GetData(), sizeof(FFloat16Color) * pixel_data.Num());
    RHIUnlockTexture2D(Texture2DRHI, 0, false);

    this->tilde_h0_k = Texture2DRHI;
  }

  //////////////////////////////
  //////////////////////////////
  //////////////////////////////

  {
    FRHIResourceCreateInfo CreateInfo;
    FTexture2DRHIRef Texture2DRHI = RHICreateTexture2D(
      N,
      N,
      PF_FloatRGBA,
      1,
      1,
      TexCreate_RenderTargetable,
      CreateInfo);

    TArray<FFloat16Color> pixel_data;
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < N; j++) {
        // pixel_data.Add(FFloat16Color(FLinearColor(i, -j, 0.0, 1.0))); // Dummy data for now
        pixel_data.Add(FFloat16Color(FLinearColor(init_data[i*N+j].B, init_data[i*N+j].A, 0.0, 1.0)));
      }
    }

    uint32 DestStride = 0;
    FFloat16Color* data = (FFloat16Color*) RHILockTexture2D(Texture2DRHI, 0, RLM_WriteOnly, DestStride, false);
    FMemory::Memcpy(data, pixel_data.GetData(), sizeof(FFloat16Color) * pixel_data.Num());
    RHIUnlockTexture2D(Texture2DRHI, 0, false);

    this->tilde_h0_neg_k = Texture2DRHI;
  }
}

void FourierComponentsShader::BuildAndExecuteGraph(
  FRHICommandListImmediate &RHI_cmd_list,
  float t,
  UTextureRenderTarget2D* tilde_hkt_dy,
  UTextureRenderTarget2D* tilde_hkt_dxz) {

	FRDGBuilder graph_builder(RHI_cmd_list);

	FParameters* PassParameters;
	PassParameters = graph_builder.AllocParameters<FourierComponentsShader::FParameters>();

	PassParameters->t = t;

  PassParameters->tilde_h0_k = register_texture(graph_builder, this->tilde_h0_k->GetTexture2D(), "tilde_h0_k");
  PassParameters->tilde_h0_neg_k = register_texture(graph_builder, this->tilde_h0_neg_k->GetTexture2D(), "tilde_h0_neg_k");

  CustomUAV uav1 = create_UAV(graph_builder, tilde_hkt_dy, TEXT("Compute_Something_Texture1"));
  CustomUAV uav2 = create_UAV(graph_builder, tilde_hkt_dxz, TEXT("Compute_Something_Texture2"));

  PassParameters->tilde_hkt_dy = uav1.uav_ref;
  PassParameters->tilde_hkt_dxz = uav2.uav_ref;

	TShaderMapRef<FourierComponentsShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

  FComputeShaderUtils::AddPass(
    graph_builder,
    RDG_EVENT_NAME("FourierComponents Pass"),
    ComputeShader,
    PassParameters,
    FIntVector(m_N / 8, m_N / 8, 1)
  );

	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget1;
	graph_builder.QueueTextureExtraction(uav1.ref, &PooledComputeTarget1);

	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget2;
	graph_builder.QueueTextureExtraction(uav2.ref, &PooledComputeTarget2);

	graph_builder.Execute();

	RHI_cmd_list.CopyToResolveTarget(
    PooledComputeTarget1.GetReference()->GetRenderTargetItem().TargetableTexture,
    tilde_hkt_dy->GetRenderTargetResource()->TextureRHI,
    FResolveParams()
  );

	RHI_cmd_list.CopyToResolveTarget(
    PooledComputeTarget2.GetReference()->GetRenderTargetItem().TargetableTexture,
    tilde_hkt_dxz->GetRenderTargetResource()->TextureRHI,
    FResolveParams()
  );

  // DEBUG READ-BACK
  // {
  //   FRHIResourceCreateInfo CreateInfo;
  //   FTexture2DRHIRef readback_tex = RHICreateTexture2D(
  //     m_N,
  //     m_N,
  //     PF_FloatRGBA,
  //     1,
  //     1,
  //     TexCreate_RenderTargetable,
  //     CreateInfo);

  //   RHI_cmd_list.CopyToResolveTarget(
  //     tilde_hkt_dy->GetRenderTargetResource()->TextureRHI,
  //     readback_tex->GetTexture2D(),
  //     FResolveParams()
  //   );

  //   // UE_LOG(LogTemp, Warning, TEXT("READBACK START"));

  //   FReadSurfaceDataFlags read_flags(RCM_MinMax);
  //   read_flags.SetLinearToGamma(false);

  //   TArray<FFloat16Color> rdata;
  //   RHI_cmd_list.ReadSurfaceFloatData(
  //     readback_tex->GetTexture2D(),
  //     FIntRect(0, 0, m_N, m_N),
  //     rdata,
  //     read_flags
  //   );

  //   // UE_LOG(LogTemp, Warning, TEXT("Amount of pixels: %i"), rdata.Num());
  //   // for (int i = 0; i < rdata.Num(); i++) {
  //   //   UE_LOG(LogTemp, Warning, TEXT("%i: (%f, %f, %f, %f)"), i, rdata[i].R.GetFloat(), rdata[i].G.GetFloat(), rdata[i].B.GetFloat(), rdata[i].A.GetFloat());
  //   // }
  //   // UE_LOG(LogTemp, Warning, TEXT("READBACK END"));
  // }

}