#pragma once

#include "WaveSpectrums.generated.h"

/* Ocean Wave Spectrums */

USTRUCT(BlueprintType)
struct FPhillipsSpectrumSettings {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float amplitude; // in meters per second

	UPROPERTY(EditAnywhere)
	float wave_length; // in meters

	UPROPERTY(EditAnywhere)
	float wave_alignment; // usually between 2.0 and 8.0 (unitless)

	UPROPERTY(EditAnywhere)
	float wind_speed; // Wind speed in meters per second

	UPROPERTY(EditAnywhere)
	FVector2D wind_direction; // Ensured to be normalized in code
};

USTRUCT(BlueprintType)
struct FJonswapSpectrumSettings {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float fetch; // in meters

	UPROPERTY(EditAnywhere)
	float wind_speed; // Wind speed in meters per second

	UPROPERTY(EditAnywhere)
	FVector2D wind_direction; // Ensured to be normalized in code
};

float PhillipsWaveSpectrum(FVector2D k_vec, const FPhillipsSpectrumSettings& settings);
float JonswapWaveSpectrum(FVector2D k_vec, const FJonswapSpectrumSettings& settings);

/* Directional Spectrums (theta is angle relative to wind direction) */

USTRUCT(BlueprintType)
struct FDonelanBannerSettings {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float fetch; // in meters

	UPROPERTY(EditAnywhere)
	float wind_speed; // Wind speed in meters per second
};

float UniformDirectionalSpectrum();
float DonelanBannerDirectionalSpectrum(float theta, float omega, const FDonelanBannerSettings& settings);