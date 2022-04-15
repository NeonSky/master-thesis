#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

#include "SubmergedTriangles.h"

class SHADERMODELS_API GPUBoatShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(GPUBoatShader)
	SHADER_USE_PARAMETER_STRUCT(GPUBoatShader, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER(float, SpeedInput)
		SHADER_PARAMETER(FVector2D, VelocityInput)

		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<FVector4>, ElevationTexture)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<GPUSumbergedTriangle>, SubmergedTrianglesBuffer)

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, BoatTexture)

		SHADER_PARAMETER_RDG_TEXTURE_ARRAY(Texture2D<FVector4>, OtherBoatTextures, [1])

		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4>, ReadbackTexture)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("N_TRIANGLES"), 140);
	}

	void ResetBoatTexture(FRHICommandListImmediate &RHI_cmd_list, UTextureRenderTarget2D* input_output);

	void BuildAndExecuteGraph(
        FRHICommandListImmediate &RHI_cmd_list,
        float speed_input,
        FVector2D velocity_input,
        UTextureRenderTarget2D* elevation_texture,
		TRefCountPtr<FRDGPooledBuffer> submerged_triangles_buffer,
        UTextureRenderTarget2D* boat_texture,
        TArray<UTextureRenderTarget2D*> other_boat_textures,
        UTextureRenderTarget2D* readback_texture,
        TArray<FFloat16Color>* readback_target);

};