// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ShaderModels.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"

#include "OceanSurfaceSimulation.generated.h"


UCLASS(Blueprintable)
class UNREALMASTERTHESIS_API AOceanSurfaceSimulation : public AActor {
	GENERATED_BODY()
	
public:	
	AOceanSurfaceSimulation();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	// Called once on "Play"
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UMaterial* material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* butterfly_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* spectrum_x_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* spectrum_y_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* spectrum_z_rtt;

	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float gravity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float L; // The side length of the patch in meters. Essentially animation speed. Higher -> Slower

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float amplitude; //  in meters per second.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float wave_length; // in meters

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float wave_alignment; // usually between 2.0 and 8.0

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float wind_speed; // Wind speed in meters per second.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FVector2D wind_direction; // Ensured to be normalized in code.

	int32 N; // Resolution

	ShaderModelsModule m_shader_models_module; // Reference to the ShaderModels module

	void create_mesh();
	void update_mesh();

};