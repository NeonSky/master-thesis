// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ShaderModels.h"
#include "WaveSpectrums.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"

#include "OceanSurfaceSimulation.generated.h"

UENUM()
enum WaveSpectrumType {
  Phillips UMETA(DisplayName = "Phillips"),
  Jonswap  UMETA(DisplayName = "JONSWAP"),
};

UENUM()
enum DirectionalSpreadingType {
  Uniform       UMETA(DisplayName = "Uniform"),
  DonelanBanner UMETA(DisplayName = "Donelan-Banner"),
};

UCLASS(Blueprintable)
class UNREALMASTERTHESIS_API AOceanSurfaceSimulation : public AActor {
	GENERATED_BODY()
	
public:	
	AOceanSurfaceSimulation();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	TArray<float> sample_elevation_points(TArray<FVector2D> sample_points);
	TArray<FVector4> submerged;
	float boatX = 0.0f;
	float boatY = 0.0f;
	int speed = 0;

protected:

	// Called once on "Play"
	virtual void BeginPlay() override;

private:
	bool first = true;
	float last_ran = 0.0f;
	float boatPrevX = 0.0f;
	float boatPrevY = 0.0f;
	int boatPrevXp = 0;
	int boatPrevYp = 0;
	float uvX = 0.0f;
	float uvY = 0.0f;
	float cmPerPixel;
	int xp;
	int yp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UMaterial* material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* eWave_addition_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTexture2D* eWave_addition_texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTexture2D* eWave_zero_texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* butterfly_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* spectrum_x_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* spectrum_y_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* spectrum_z_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* ewave_h_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* ewave_hPrev_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* ewave_v_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* ewave_vPrev_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<WaveSpectrumType> wave_spectrum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<DirectionalSpreadingType> directional_spreading_function;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FPhillipsSpectrumSettings phillips_settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FJonswapSpectrumSettings jonswap_settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FDonelanBannerSettings donelan_banner_settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), meta=(UIMin = "0.01", UIMax = "10.0"))
	float amplitude_scaler;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), meta=(UIMin = "1.0", UIMax = "1000.0"))
	float L; // The side length of each ocean tile in meters.

	int32 N; // Resolution in terms of vertices per horizontal unit axis.

	ShaderModelsModule m_shader_models_module; // Reference to the ShaderModels module

	TArray<UProceduralMeshComponent*> tile_meshes; // Each tile is a separate mesh but they share material



	void create_mesh();
	void update_mesh(float dt);
	void prepare_ewave();

};