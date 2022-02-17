#include "GPUBoat.h"

#include "Kismet/GameplayStatics.h"

AGPUBoat::AGPUBoat() {

	// Configure Tick() to be called every frame.
	PrimaryActorTick.bCanEverTick = true;
}

void AGPUBoat::BeginPlay() {
	Super::BeginPlay();
}

void AGPUBoat::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

    m_shader_models_module.UpdateGPUBoat(
        input_pawn->SpeedInput(),
        input_pawn->VelocityInput(),
        elevation_rtt,
        boat_rtt,
        readback_rtt,
        camera_follow ? camera_target : nullptr);
}