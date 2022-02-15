// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "OceanSurfaceSimulation.h"
#include "ShaderModels.h"
#include "Rigidbody.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshActor.h"
#include "StaticMeshResources.h"

#include <queue>

#include "Boat.generated.h"

struct SubmergedTriangle {
	FVector v_L; // The vertex whose relative height above the ocean surface is the smallest.
	FVector v_M;
	FVector v_H; // The vertex whose relative height above the ocean surface is the greatest.

	FVector normal;   // Points out from the hull
	FVector centroid; // The center point of the triangle
	float height;     // Relative (positive) height below water water for the centroid
	float area;       // The surface area of the triangle
	float velocity;   // The (point) velocity at which the centroid is moving
};

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
	AActor* engine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	AStaticMeshActor* collision_mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int artificial_frame_delay; // The GPU readback latency in frames

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int artificial_frame_skip; // The number of consecutive frames we don't perform a readback

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float slow_speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float normal_speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float fast_speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float mass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float angular_drag;

	FVector2D m_velocity_input;
	float m_speed_input;

	Rigidbody m_rigidbody;

	int m_requested_elevations_on_frame;
	int m_cur_frame;

	TArray<uint32> m_collision_mesh_indices;
	TArray<FVector> m_collision_mesh_vertices;
	float m_collision_mesh_surface_area;

	std::queue<TArray<float>> m_readback_queue;
	TArray<float> m_latest_elevations;

	TArray<SubmergedTriangle> m_submerged_triangles;

	void FetchCollisionMeshData();

	void DebugDrawTriangle(FVector v0, FVector v1, FVector v2, FColor color);
	void DebugDrawForce(FVector c, FVector f, FColor color);
	void DebugDrawVelocities();
	void UpdateReadbackQueue();
	void UpdateSubmergedTriangles();

	void ApplyGravity();
	void ApplyBuoyancy();
	void ApplyResistanceForces();
	void ApplyUserInput();

	void UseSlowSpeed();
	void UseNormalSpeed();
	void UseFastSpeed();

	void HorizontalAxis(float input);
	void VerticalAxis(float input);
};