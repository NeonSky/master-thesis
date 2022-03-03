#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Engine/StaticMeshActor.h"
#include "StaticMeshResources.h"
#include "RenderGraphResources.h"

#include <functional>

class SHADERMODELS_API ShaderModelsModule : public IModuleInterface {
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void GenerateButterflyTexture(UTextureRenderTarget2D* output);
	void FFT(UTextureRenderTarget2D* butterfly, UTextureRenderTarget2D* output, float scale = 1.0f);
	void FFT_Forward(UTextureRenderTarget2D* butterfly, UTextureRenderTarget2D* output);

	void Buildh0Textures(int N, float L, std::function<float (FVector2D)> wave_spectrum);

	void ComputeFourierComponents(
		float t,
		float L,
		UTextureRenderTarget2D* tilde_hkt_dx,
		UTextureRenderTarget2D* tilde_hkt_dy,
		UTextureRenderTarget2D* tilde_hkt_dz
	);

	void ComputeeWave(
		float t,
		float L,
		UTextureRenderTarget2D* eWave_h,
		UTextureRenderTarget2D* eWave_v
	);

	void Clear(UTextureRenderTarget2D* result);

	void ComputeScale(
		UTextureRenderTarget2D* input_output_rtt, 
		UTextureRenderTarget2D* copy_rtt,
		float scale);

	void ComputeObstruction(
		UTextureRenderTarget2D* boat_rtt,
		TRefCountPtr<FRDGPooledBuffer> submerged_triangles,
		UTextureRenderTarget2D* obstructionMap_rtt,
		UTextureRenderTarget2D* h_rtt,
		UTextureRenderTarget2D* v_rtt,
		UTextureRenderTarget2D* hPrev_rtt,
		UTextureRenderTarget2D* vPrev_rtt,
		int preFFT);

	void SampleElevationPoints(UTextureRenderTarget2D* elevations, TArray<FVector2D> input_sample_coordinates, TArray<float>* output);

	void ResetGPUBoat(UTextureRenderTarget2D* input_output);

	void UpdateGPUBoat(
		float speed_input,
		FVector2D velocity_input,
		AStaticMeshActor* collision_mesh,
		UTextureRenderTarget2D* elevation_texture,
		UTextureRenderTarget2D* input_output,
		UTextureRenderTarget2D* readback_texture,
		TRefCountPtr<FRDGPooledBuffer>& submerged_triangles_buffer,
		AActor* update_target);

};
