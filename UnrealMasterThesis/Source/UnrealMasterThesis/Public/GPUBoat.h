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
	~AGPUBoat();

	virtual void Update(UpdatePayload update_payload, std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback) override;
    virtual UTextureRenderTarget2D* GetBoatRTT() override;
    virtual FeWaveRTTs GeteWaveRTTs() override;
	virtual FVector WorldPosition3D() override;
	virtual void setDist(TArray<float> dist, int seed, bool organic) override;

protected:

	// Called once on "Play"
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AActor* camera_target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool use_p2_inputs;

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

};