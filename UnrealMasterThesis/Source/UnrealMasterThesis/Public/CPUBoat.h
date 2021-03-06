// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IBoat.h"
#include "OceanSurfaceSimulation.h"
#include "ShaderModels.h"
#include "Rigidbody.h"
#include "HorizontalProjection.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshActor.h"
#include "StaticMeshResources.h"

#include <queue>

#include "CPUBoat.generated.h"

struct SubmergedTriangle {
	FVector v_L; // The vertex whose relative height above the ocean surface is the smallest.
	FVector v_M;
	FVector v_H; // The vertex whose relative height above the ocean surface is the greatest.

	FVector normal;   // Points out from the hull
	FVector centroid; // The center point of the triangle
	float height;     // Relative (positive) height below water water for the centroid. TODO: maybe rename to depth?
	float area;       // The surface area of the triangle
	float velocity;   // The (point) velocity at which the centroid is moving
};

UCLASS(Blueprintable)
class UNREALMASTERTHESIS_API ACPUBoat : public AActor, public IBoatInterface {
	GENERATED_BODY()
	
public:	
	ACPUBoat();

	virtual void Update(UpdatePayload update_payload, std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback) override;
    virtual UTextureRenderTarget2D* GetBoatRTT() override;
    virtual FeWaveRTTs GeteWaveRTTs() override;
	virtual FVector WorldPosition3D() override;
	virtual void setDist(TArray<float> dist, int seed) override;

protected:

	// Called once on "Play"
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AOceanSurfaceSimulation* ocean_surface_simulation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AActor* engine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AStaticMeshActor* collision_mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool use_p2_inputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float mass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float angular_drag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* boat_rtt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FeWaveRTTs ewave_rtts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool mock_async_readback;

	Rigidbody m_rigidbody;

	TArray<uint32> m_collision_mesh_indices;
	TArray<FVector> m_collision_mesh_vertices;
	float m_collision_mesh_surface_area;

	TArray<float> m_latest_elevations;

	TArray<SubmergedTriangle> m_submerged_triangles;
	float m_prev_r_s;

	FVector2D m_velocity_input;
	float m_speed_input;

	ShaderModelsModule m_shader_models_module; // Reference to the ShaderModels module

	void FetchCollisionMeshData();

	void DebugDrawTriangle(FVector v0, FVector v1, FVector v2, FColor color);
	void DebugDrawForce(FVector c, FVector f, FColor color);
	void DebugDrawVelocities();
	void UpdateElevations();
	void UpdateSubmergedTriangles();

	void ApplyGravity();
	void ApplyBuoyancy(float r_s);
	void ApplyResistanceForces(float r_s, float dt);
	void ApplyUserInput(float r_s);

	void UseSlowSpeed();
	void UseNormalSpeed();
	void UseFastSpeed();

	void HorizontalAxis(float input);
	void VerticalAxis(float input);

	void UpdateGPUState(Rigidbody prev_r, std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback);
};