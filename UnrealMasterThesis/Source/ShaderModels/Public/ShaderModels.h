#pragma once

#include "FourierComponents.h"

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

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
		int offsetSign_x,
		int offsetSign_y,
		UTextureRenderTarget2D* eWave_h,
		UTextureRenderTarget2D* eWave_hPrev,
		UTextureRenderTarget2D* eWave_v,
		UTextureRenderTarget2D* eWave_vPrev
	);

	void ComputeAdd(
		UTextureRenderTarget2D* term1,
		UTexture2D* term2,
		// UTextureRenderTarget2D* term2,
		UTextureRenderTarget2D* result
	);

	void ComputeScale(UTextureRenderTarget2D* input_output_rtt, float scale);
	void ComputeObstruction(
		TArray<FVector4> SubmergedTriangles,
		int L,
		UTextureRenderTarget2D* obstructionMap_rtt,
		UTextureRenderTarget2D* h_rtt,
		UTextureRenderTarget2D* v_rtt,
		float xPos, 
		float yPos,
		int offsetSign_x,
		int offsetSign_y);

};
