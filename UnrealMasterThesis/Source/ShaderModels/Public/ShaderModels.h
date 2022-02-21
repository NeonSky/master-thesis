#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Engine/StaticMeshActor.h"
#include "StaticMeshResources.h"

#include <functional>

class SHADERMODELS_API ShaderModelsModule : public IModuleInterface {
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void GenerateButterflyTexture(UTextureRenderTarget2D* output);
	void FFT(UTextureRenderTarget2D* butterfly, UTextureRenderTarget2D* output);

	void Buildh0Textures(int N, float L, std::function<float (FVector2D)> wave_spectrum);

	void ComputeFourierComponents(
		float t,
		float L,
		UTextureRenderTarget2D* tilde_hkt_dx,
		UTextureRenderTarget2D* tilde_hkt_dy,
		UTextureRenderTarget2D* tilde_hkt_dz
	);

	void SampleElevationPoints(UTextureRenderTarget2D* elevations, TArray<FVector2D> input_sample_coordinates, TArray<float>* output);

	void ResetGPUBoat(UTextureRenderTarget2D* input_output);

	void UpdateGPUBoat(
		float speed_input,
		FVector2D velocity_input,
		AStaticMeshActor* collision_mesh,
		UTextureRenderTarget2D* elevation_texture,
		UTextureRenderTarget2D* input_output,
		UTextureRenderTarget2D* readback_texture,
		AActor* camera_target);

};
