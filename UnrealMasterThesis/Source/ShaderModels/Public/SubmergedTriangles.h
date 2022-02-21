#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Modules/ModuleManager.h"

#include "Engine/StaticMeshActor.h"
#include "StaticMeshResources.h"

struct GPUSumbergedTriangle {
	FVector4 normal_and_height;
	FVector4 center_and_area;
};

class SHADERMODELS_API SubmergedTrianglesShader : public FGlobalShader {
public:

	DECLARE_GLOBAL_SHADER(SubmergedTrianglesShader)
	SHADER_USE_PARAMETER_STRUCT(SubmergedTrianglesShader, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, ) 

		SHADER_PARAMETER_SRV(Buffer<uint32>, IndexBuffer)
		SHADER_PARAMETER_SRV(Buffer<float>, PositionBuffer)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<GPUSumbergedTriangle>, OutputBuffer)

	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("N_THREADS_X"), 1);
	}

	void BuildAndExecuteGraph(
        FRHICommandListImmediate &RHI_cmd_list,
		AStaticMeshActor* collision_mesh,
        TRefCountPtr<FRDGPooledBuffer>* output_buffer);

};