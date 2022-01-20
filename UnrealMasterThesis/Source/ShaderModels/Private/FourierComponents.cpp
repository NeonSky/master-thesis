#include "FourierComponents.h"

#include "RenderGraph.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/GameState.h"

#include <complex>
#include <random>

IMPLEMENT_GLOBAL_SHADER(FourierComponentsShader, "/Project/UnrealMasterThesis/FourierComponents.usf", "MainCompute", SF_Compute);

float PhillipsSpectrum(FVector2D k_vec, FourierComponentsSettings settings) {
  if (k_vec.IsZero()) {
    return 0.0f;
  }

  float phillips_L = pow(settings.wind_speed, 2.0) / settings.gravity;

  float wave_number = k_vec.Size(); // vector magnitude

  FVector2D k_hat = k_vec.GetSafeNormal();

  float k_hat_dot_omega_hat = FVector2D::DotProduct(k_hat, settings.wind_direction);

  float res = settings.amplitude;
  res *= exp(-1.0 / pow(wave_number * phillips_L, 2.0));
  res /= pow(wave_number, 4.0);
  res *= pow(k_hat_dot_omega_hat, settings.wave_alignment);

  float damping   = 0.001f;
  float wave_length = wave_number * damping; // Used by Tessendorf in the first set of examples
  res *= exp(-pow(wave_number, 2.0) * pow(wave_length, 2.0));

  return res;
}

float Jonswap(float omega, const FourierComponentsSettings& settings) {
  double U_10 = settings.wind_speed;
  double F = 100000.0; // fetch
  double gamma = 3.3f;
  double g = settings.gravity;
  double alpha = 0.076f * pow(g * F / pow(U_10, 2.0f), -0.22f);
  double omega_p = 22.0f * pow(U_10 * F / pow(g, 2.0f), -0.33f);
  double sigma = omega <= omega_p ? 0.07f : 0.09f;
  double r = exp(-pow(omega - omega_p, 2.0) / (2.0 * sigma * sigma * omega_p * omega_p));

  return alpha * g * g * exp(-(5.0/4.0) * pow(omega_p / omega, 4.0)) * pow(gamma, r) / pow(omega, 5.0);
}

// Donelan-Banner Directional Spreading. See Horvath paper
float DirectionalSpectrum(float theta, float omega, const FourierComponentsSettings& settings) {

  float U_10 = settings.wind_speed;
  float F = 100000.0; // fetch
  float g = settings.gravity;

  float omega_p = 22.0f * pow(U_10 * F / pow(g, 2.0f), -0.33f);
  float omega_over_omega_p = omega / omega_p;

  float beta = 0.0f;
  if (omega_over_omega_p < 0.95f) {
    beta = 2.61f * pow(omega_over_omega_p, 1.3f);
  }
  else if (omega_over_omega_p < 1.6f) {
    beta = 2.28f * pow(omega_over_omega_p, -1.3f);
  }
  else {
    float e = -0.4 + 0.8393 * exp(-0.567 * log(pow(omega_over_omega_p, 2.0)));
    beta = pow(10, e);
  }

  float sech_term = 1.0f / cosh(beta * theta); // https://mathworld.wolfram.com/HyperbolicSecant.html

  return beta * pow(sech_term, 2.0) / (2 * tanh(beta * PI));
}

float JonswapSpectrum(FVector2D k_vec, const FourierComponentsSettings& settings) {
  if (k_vec.IsZero()) {
    return 0.0f;
  }

  float k = k_vec.Size();
  float g = settings.gravity;

  float omega = sqrt(k * g);
  float theta = atan2(k_vec.Y, k_vec.X);
  float theta_p = atan2(settings.wind_direction.Y, settings.wind_direction.X); // The peak is equal to the wind direction
  float theta_diff = theta - theta_p; // Horvath equation 41

  return Jonswap(omega, settings) * DirectionalSpectrum(theta_diff, omega, settings) * sqrt(g) / (2 * k * sqrt(k));
}

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

TArray<FFloat16Color> create_init_data(int N, FourierComponentsSettings settings) {

  settings.wind_direction.Normalize();

  auto wave_spectrum = [=](FVector2D k_vec, FourierComponentsSettings settings) {
    if (settings.amplitude > 0.0) {
      return PhillipsSpectrum(k_vec, settings);
    } else {
      return JonswapSpectrum(k_vec, settings);
    }
  };

  std::random_device rd{};
  std::mt19937 rng{rd()};
  std::normal_distribution<float> dist;

  TArray<FFloat16Color> res;

  for (int z = 0; z < N; z++) {
    for (int x = 0; x < N; x++) {

      FVector2D wave_vector = FVector2D(
        2.0 * PI * (z - floor(N / 2.0)) / settings.tile_size,
        2.0 * PI * (x - floor(N / 2.0)) / settings.tile_size
      );

      FVector2D neg_wave_vector = FVector2D(
        2.0 * PI * ((-z) - floor(N / 2.0)) / settings.tile_size,
        2.0 * PI * ((-x) - floor(N / 2.0)) / settings.tile_size
      );

      float xi_r = dist(rng);
      float xi_i = dist(rng);
      std::complex<float> complex_rv(xi_r, xi_i);

      float delta_k = 2.0f * PI / settings.tile_size;

      std::complex<float> h0 = sqrt(1.0f / 2.0f) * complex_rv * sqrt(2.0f * wave_spectrum(wave_vector, settings) * delta_k * delta_k);
      std::complex<float> h0_conj = std::conj(sqrt(1.0f / 2.0f) * complex_rv * sqrt(2.0f * wave_spectrum(neg_wave_vector, settings) * delta_k * delta_k));

      res.Add(FFloat16Color(FLinearColor(h0.real(), h0.imag(), h0_conj.real(), h0_conj.imag())));
    }
  }

  return res;

}

void FourierComponentsShader::Buildh0Textures(int N, FourierComponentsSettings settings) {

  this->m_N = N;

  TArray<FFloat16Color> init_data = create_init_data(N, settings);

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
  float L,
  UTextureRenderTarget2D* tilde_hkt_dx,
  UTextureRenderTarget2D* tilde_hkt_dy,
  UTextureRenderTarget2D* tilde_hkt_dz) {

	FRDGBuilder graph_builder(RHI_cmd_list);

	FParameters* PassParameters;
	PassParameters = graph_builder.AllocParameters<FourierComponentsShader::FParameters>();

	PassParameters->N = m_N;
	PassParameters->L = L;
	PassParameters->t = t;

  PassParameters->tilde_h0_k = register_texture(graph_builder, this->tilde_h0_k->GetTexture2D(), "tilde_h0_k");
  PassParameters->tilde_h0_neg_k = register_texture(graph_builder, this->tilde_h0_neg_k->GetTexture2D(), "tilde_h0_neg_k");

  CustomUAV uav1 = create_UAV(graph_builder, tilde_hkt_dx, TEXT("Compute_Something_Texture1"));
  CustomUAV uav2 = create_UAV(graph_builder, tilde_hkt_dy, TEXT("Compute_Something_Texture2"));
  CustomUAV uav3 = create_UAV(graph_builder, tilde_hkt_dz, TEXT("Compute_Something_Texture3"));

  PassParameters->tilde_hkt_dx = uav1.uav_ref;
  PassParameters->tilde_hkt_dy = uav2.uav_ref;
  PassParameters->tilde_hkt_dz = uav3.uav_ref;

	TShaderMapRef<FourierComponentsShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

  FComputeShaderUtils::AddPass(
    graph_builder,
    RDG_EVENT_NAME("FourierComponents Pass"),
    ComputeShader,
    PassParameters,
    FIntVector(m_N, m_N, 1)
  );

	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget1;
	graph_builder.QueueTextureExtraction(uav1.ref, &PooledComputeTarget1);

	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget2;
	graph_builder.QueueTextureExtraction(uav2.ref, &PooledComputeTarget2);

	TRefCountPtr<IPooledRenderTarget> PooledComputeTarget3;
	graph_builder.QueueTextureExtraction(uav3.ref, &PooledComputeTarget3);

	graph_builder.Execute();

	RHI_cmd_list.CopyToResolveTarget(
    PooledComputeTarget1.GetReference()->GetRenderTargetItem().TargetableTexture,
    tilde_hkt_dx->GetRenderTargetResource()->TextureRHI,
    FResolveParams()
  );

	RHI_cmd_list.CopyToResolveTarget(
    PooledComputeTarget2.GetReference()->GetRenderTargetItem().TargetableTexture,
    tilde_hkt_dy->GetRenderTargetResource()->TextureRHI,
    FResolveParams()
  );

	RHI_cmd_list.CopyToResolveTarget(
    PooledComputeTarget3.GetReference()->GetRenderTargetItem().TargetableTexture,
    tilde_hkt_dz->GetRenderTargetResource()->TextureRHI,
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

  //   UE_LOG(LogTemp, Warning, TEXT("READBACK START"));

  //   FReadSurfaceDataFlags read_flags(RCM_MinMax);
  //   read_flags.SetLinearToGamma(false);

  //   TArray<FFloat16Color> rdata;
  //   RHI_cmd_list.ReadSurfaceFloatData(
  //     readback_tex->GetTexture2D(),
  //     FIntRect(0, 0, m_N, m_N),
  //     rdata,
  //     read_flags
  //   );

  //   UE_LOG(LogTemp, Warning, TEXT("Amount of pixels: %i"), rdata.Num());
  //   for (int i = 0; i < rdata.Num(); i++) {
  //     UE_LOG(LogTemp, Warning, TEXT("%i: (%f, %f, %f, %f)"), i, rdata[i].R.GetFloat(), rdata[i].G.GetFloat(), rdata[i].B.GetFloat(), rdata[i].A.GetFloat());
  //   }
  //   UE_LOG(LogTemp, Warning, TEXT("READBACK END"));
  // }

}