// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IBoat.h"
#include "ShaderModels.h"

#include <queue>
#include <random>

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

	virtual void setDist(TArray<float> dist, int seed, bool organic) override;
	TArray<float> delay_distribution;
	bool organicDelay = false;

	std::random_device rd{};
	std::mt19937 rng{ rd() };
	

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
	int artificial_frame_delay; // The GPU readback latency in frames (0 = no latency, >0 fixed latency, <0 organic latency)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int artificial_frame_skip; // The number of consecutive frames we don't perform a readback

	std::queue<TRefCountPtr<FRDGPooledBuffer>> m_readback_queue;
	int m_requested_elevations_on_frame;
	int m_cur_frame;

	bool m_organic_delay;
	int m_cur_organic_delay;

	ShaderModelsModule m_shader_models_module; // Reference to the ShaderModels module
};