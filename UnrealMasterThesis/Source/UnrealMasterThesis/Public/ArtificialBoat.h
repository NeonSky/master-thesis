// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IBoat.h"
#include "ShaderModels.h"

#include <queue>

#include "ArtificialBoat.generated.h"

UCLASS(Blueprintable)
class UNREALMASTERTHESIS_API AArtificialBoat : public AActor, public IBoatInterface {
	GENERATED_BODY()
	
public:	
	AArtificialBoat();
	~AArtificialBoat();

	virtual void Update(UpdatePayload update_payload, std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback) override;
    virtual UTextureRenderTarget2D* GetBoatRTT() override;
    virtual FeWaveRTTs GeteWaveRTTs() override;
	virtual FVector WorldPosition3D() override;

protected:

	// Called once on "Play"
	virtual void BeginPlay() override;

private:

	void UpdateReadbackQueue(TArray<UTextureRenderTarget2D*> other_boat_textures, TArray<UTextureRenderTarget2D*> other_wake_textures);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool use_p2_inputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int artificial_frame_delay; // The GPU readback latency in frames

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int artificial_frame_skip; // The number of consecutive frames we don't perform a readback

	std::queue<TRefCountPtr<FRDGPooledBuffer>> m_readback_queue;
	int m_requested_elevations_on_frame;
	int m_cur_frame;

	ShaderModelsModule m_shader_models_module; // Reference to the ShaderModels module
};