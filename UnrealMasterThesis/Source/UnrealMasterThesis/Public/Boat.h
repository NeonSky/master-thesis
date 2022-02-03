// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "OceanSurfaceSimulation.h"
#include "ShaderModels.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Boat.generated.h"

UCLASS(Blueprintable)
class UNREALMASTERTHESIS_API ABoat : public APawn {
	GENERATED_BODY()
	
public:	
	ABoat();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

  // Called to bind functionality to input
  virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

protected:

	// Called once on "Play"
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AOceanSurfaceSimulation* ocean_surface_simulation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* spectrum_y_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float slow_speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float normal_speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float fast_speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float rotation_speed;

  FVector2D m_velocity_input;
  float m_speed_input;

  bool m_has_requested_elevations;
  FRenderCommandFence m_elevation_read_fence;
  TArray<FFloat16Color> m_elevation_data; // We read from this one...
  TArray<FFloat16Color> m_elevation_data_scratch; // but we map new data to this one

  void UpdateElevations();
  void UpdateTransform(float DeltaTime);

  void UseSlowSpeed();
  void UseNormalSpeed();
  void UseFastSpeed();

  void HorizontalAxis(float input);
  void VerticalAxis(float input);
};