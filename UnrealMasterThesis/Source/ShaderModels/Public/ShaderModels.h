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

	void Buildh0Textures(int N, float L, std::function<float (FVector2D)> wave_spectrum, int seed);

	void ComputeFourierComponents(
		float t,
		UTextureRenderTarget2D* tilde_hkt_dy,
		UTextureRenderTarget2D* tilde_hkt_dxz
	);

	void ComputeeWave(
		float dt,
		UTextureRenderTarget2D* eWave_hv,
		UTextureRenderTarget2D* eWave_hv_copy
	);

	void Clear(UTextureRenderTarget2D* result, FVector4 clear_value);

	void ComputeScale(
		UTextureRenderTarget2D* input_output_rtt, 
		UTextureRenderTarget2D* copy_rtt,
		float scale);

	void ComputeSerialization(UTextureRenderTarget2D* input_rtt, UTextureRenderTarget2D* serialize_rtt, TArray<float>& out_param);

	void ProjectObstruction(FRHIVertexBuffer* submerged_position_buffer, UTextureRenderTarget2D* obstruction_rtt);

	void ComputeObstruction(
		UTextureRenderTarget2D* boat_rtt,
		TRefCountPtr<FRDGPooledBuffer> submerged_triangles,
		UTextureRenderTarget2D* obstructionMap_rtt,
		UTextureRenderTarget2D* hv_rtt,
		UTextureRenderTarget2D* hv_prev_rtt,
		int preFFT);

	void SampleElevationPoints(
		UTextureRenderTarget2D* elevations,
		TArray<UTextureRenderTarget2D*> wake_rtts,
		TArray<FVector2D> ws_boat_coords,
		TArray<FVector2D> input_sample_coordinates,
		TArray<float>* output);

	void ResetGPUBoat(UTextureRenderTarget2D* input_output);

	void UpdateGPUBoat(
		float speed_input,
		FVector2D velocity_input,
		AStaticMeshActor* collision_mesh,
		UTextureRenderTarget2D* elevation_texture,
		UTextureRenderTarget2D* wake_texture,
		TArray<UTextureRenderTarget2D*> other_wake_textures,
		UTextureRenderTarget2D* obstruction_texture,
		UTextureRenderTarget2D* boat_texture,
		TArray<UTextureRenderTarget2D*> other_boat_textures,
		UTextureRenderTarget2D* readback_texture,
		AActor* update_target,
		std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback);

	void UpdateArtificialBoat1(
		AStaticMeshActor* collision_mesh,
		UTextureRenderTarget2D* elevation_texture,
		UTextureRenderTarget2D* wake_texture,
		TArray<UTextureRenderTarget2D*> other_wake_textures,
		UTextureRenderTarget2D* obstruction_texture,
		UTextureRenderTarget2D* boat_texture,
		TArray<UTextureRenderTarget2D*> other_boat_textures,
		TRefCountPtr<FRDGPooledBuffer>* latency_elevations);

	void UpdateArtificialBoat2(
		float speed_input,
		FVector2D velocity_input,
		AStaticMeshActor* collision_mesh,
		UTextureRenderTarget2D* elevation_texture,
		UTextureRenderTarget2D* wake_texture,
		TArray<UTextureRenderTarget2D*> other_wake_textures,
		UTextureRenderTarget2D* obstruction_texture,
		UTextureRenderTarget2D* boat_texture,
		TArray<UTextureRenderTarget2D*> other_boat_textures,
		UTextureRenderTarget2D* readback_texture,
		AActor* update_target,
		std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback,
		TRefCountPtr<FRDGPooledBuffer>* latency_elevations);

};
