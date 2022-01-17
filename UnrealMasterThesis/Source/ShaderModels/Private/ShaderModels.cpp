#include "ShaderModels.h"

#include "GlobalShader.h"
#include "ShaderCore.h" 
#include "Engine/TextureRenderTarget2D.h"

void ShaderModelsModule::StartupModule() {
	UE_LOG(LogTemp, Warning, TEXT("ShaderModelsModule::StartupModule()"));

  	FString shader_dir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
  	AddShaderSourceDirectoryMapping(TEXT("/Project/UnrealMasterThesis"), shader_dir);
}

void ShaderModelsModule::ShutdownModule() {
	UE_LOG(LogTemp, Warning, TEXT("ShaderModelsModule::ShutdownModule()"));
}

void ShaderModelsModule::Test(float f) {
	UE_LOG(LogTemp, Warning, TEXT("Test: %.9g"), f);
}