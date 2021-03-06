#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

#include <functional>

class SHADERMODELS_API FourierComponentsShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(FourierComponentsShader)
	SHADER_USE_PARAMETER_STRUCT(FourierComponentsShader, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER(float, t)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<FVector4>, tilde_h0_k)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<FVector4>, tilde_h0_neg_k)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, tilde_hkt_dy)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, tilde_hkt_dxz)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	void Buildh0Textures(int N, float L, std::function<float (FVector2D)> wave_spectrum, int seed);

	void BuildAndExecuteGraph(
			FRHICommandListImmediate &RHI_cmd_list,
			float t,
			UTextureRenderTarget2D *tilde_hkt_dy,
			UTextureRenderTarget2D *tilde_hkt_dxz
	);

private:

  // https://forums.unrealengine.com/t/assertion-failed-alignedoffset-typelayout-size/243865
  LAYOUT_FIELD(int, m_N)
  LAYOUT_FIELD(FTexture2DRHIRef, tilde_h0_k)
  LAYOUT_FIELD(FTexture2DRHIRef, tilde_h0_neg_k)

};
