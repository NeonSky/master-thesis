// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ShaderModels.h"
#include "InputPawn.h"
#include "Runtime/Engine/Classes/Engine/CanvasRenderTarget2D.h"

#include <queue>

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

	void UpdateReadbackQueue();
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int artificial_frame_delay; // The GPU readback latency in frames

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int artificial_frame_skip; // The number of consecutive frames we don't perform a readback

	std::queue<UCanvasRenderTarget2D*> m_readback_queue;
	int m_requested_elevations_on_frame;
	int m_cur_frame;

	ShaderModelsModule m_shader_models_module; // Reference to the ShaderModels module
};