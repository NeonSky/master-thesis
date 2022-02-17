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
        camera_follow ? camera_target : nullptr);
}

// void AGPUBoat::SetupPlayerInputComponent(class UInputComponent* inputComponent) {
//   Super::SetupPlayerInputComponent(inputComponent);

//   inputComponent->BindAction("Speed1", IE_Pressed, this, &AGPUBoat::UseSlowSpeed);
//   inputComponent->BindAction("Speed2", IE_Pressed, this, &AGPUBoat::UseNormalSpeed);
//   inputComponent->BindAction("Speed3", IE_Pressed, this, &AGPUBoat::UseFastSpeed);

//   inputComponent->BindAxis("HorizontalAxis", this, &AGPUBoat::HorizontalAxis);
//   inputComponent->BindAxis("VerticalAxis", this, &AGPUBoat::VerticalAxis);
// }

// void AGPUBoat::UseSlowSpeed()   { m_speed_input = slow_speed; }
// void AGPUBoat::UseNormalSpeed() { m_speed_input = normal_speed; }
// void AGPUBoat::UseFastSpeed()   { m_speed_input = fast_speed; }

// void AGPUBoat::HorizontalAxis(float input) {
//   m_velocity_input.X = input;
// }

// void AGPUBoat::VerticalAxis(float input) {
//   m_velocity_input.Y = input;
// }
