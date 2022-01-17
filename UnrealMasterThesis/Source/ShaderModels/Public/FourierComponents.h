#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

struct FourierComponentsSettings {
  float tile_size;
  float gravity;
  float amplitude;
  float wave_alignment;
  float wind_speed;
  FVector2D wind_direction;
};

class SHADERMODELS_API FourierComponentsShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(FourierComponentsShader)
	SHADER_USE_PARAMETER_STRUCT(FourierComponentsShader, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER(int, N)
		SHADER_PARAMETER(float, L)
		SHADER_PARAMETER(float, t)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<FVector4>, tilde_h0_k)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<FVector4>, tilde_h0_neg_k)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, tilde_hkt_dx)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, tilde_hkt_dy)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, tilde_hkt_dz)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("N_THREADS_X"), 1);
		OutEnvironment.SetDefine(TEXT("N_THREADS_Y"), 1);
	}

	void Buildh0Textures(int N, FourierComponentsSettings settings);

	void BuildAndExecuteGraph(
    FRHICommandListImmediate &RHI_cmd_list,
    float L,
    UTextureRenderTarget2D* tilde_hkt_dx,
    UTextureRenderTarget2D* tilde_hkt_dy,
    UTextureRenderTarget2D* tilde_hkt_dz
  );

private:

  // https://forums.unrealengine.com/t/assertion-failed-alignedoffset-typelayout-size/243865
  LAYOUT_FIELD(int, m_N)
  LAYOUT_FIELD(FTexture2DRHIRef, tilde_h0_k)
  LAYOUT_FIELD(FTexture2DRHIRef, tilde_h0_neg_k)

};
