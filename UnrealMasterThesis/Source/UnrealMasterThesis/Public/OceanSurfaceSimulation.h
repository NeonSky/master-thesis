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
	float test;

	ShaderModelsModule m_shader_module;
};