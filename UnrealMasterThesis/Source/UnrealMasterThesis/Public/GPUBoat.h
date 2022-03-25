// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IBoat.h"
#include "ShaderModels.h"

#include "GPUBoat.generated.h"

UCLASS(Blueprintable)
class UNREALMASTERTHESIS_API AGPUBoat : public AActor, public IBoatInterface {
	GENERATED_BODY()
	
public:	
	AGPUBoat();

	virtual void Update(UpdatePayload update_payload) override;
    virtual UTextureRenderTarget2D* GetBoatRTT() override;
    virtual FeWaveRTTs GeteWaveRTTs() override;
    virtual TRefCountPtr<FRDGPooledBuffer> GetSubmergedTriangles() override;

protected:

	// Called once on "Play"
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AActor* camera_target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool camera_follow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AStaticMeshActor* collision_mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* elevation_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FeWaveRTTs ewave_rtts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* boat_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* readback_rtt;

	ShaderModelsModule m_shader_models_module; // Reference to the ShaderModels module

	TRefCountPtr<FRDGPooledBuffer> m_submerged_triangles;
};