// Fill out your copyright notice in the Description page of Project Settings.

#include "OceanSurfaceSimulation.h"

#include "ImageUtils.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/TextureRenderTarget2D.h"

AOceanSurfaceSimulation::AOceanSurfaceSimulation() {
	UE_LOG(LogTemp, Warning, TEXT("AOceanSurfaceSimulation::AOceanSurfaceSimulation()"));

	// Configure Tick() to be called every frame.
	PrimaryActorTick.bCanEverTick = true;

	m_shader_module = FModuleManager::LoadModuleChecked<ShaderModelsModule>("ShaderModels");
}

void AOceanSurfaceSimulation::BeginPlay() {
	Super::BeginPlay();
}

void AOceanSurfaceSimulation::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	m_shader_module.Test(test);
}