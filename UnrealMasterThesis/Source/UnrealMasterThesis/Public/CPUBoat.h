// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ShaderModels.h"
#include "InputPawn.h"

#include "CPUBoat.generated.h"

UCLASS(Blueprintable)
class UNREALMASTERTHESIS_API ACPUBoat : public AActor {
	GENERATED_BODY()
	
public:	
	ACPUBoat();

protected:

	// Called once on "Play"
	virtual void BeginPlay() override;

private:

	void Update(UpdatePayload update_payload);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AInputPawn* input_pawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AStaticMeshActor* collision_mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* elevation_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* boat_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* readback_rtt;

	ShaderModelsModule m_shader_models_module; // Reference to the ShaderModels module
};