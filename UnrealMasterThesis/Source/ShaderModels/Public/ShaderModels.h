#pragma once

#include "FourierComponents.h"

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

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

	void ComputeeWave(
		float t,
		float L,
		UTextureRenderTarget2D* eWave_h,
		UTextureRenderTarget2D* eWave_hPrev,
		UTextureRenderTarget2D* eWave_v,
		UTextureRenderTarget2D* eWave_vPrev
	);

	void ComputeAdd(
		UTextureRenderTarget2D* term1,
		UTextureRenderTarget2D* term2,
		UTextureRenderTarget2D* result
	);

};
