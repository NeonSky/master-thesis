// Fill out your copyright notice in the Description page of Project Settings.

#include "Boat.h"
#include "Globals/StatelessHelpers.h"

#include "Engine/TextureRenderTarget2D.h"

DECLARE_CYCLE_STAT(TEXT("CustomDebug ~ UpdateElevations"), STAT_UpdateElevations, STATGROUP_CustomDebug);

ABoat::ABoat() {
	// Configure Tick() to be called every frame.
	PrimaryActorTick.bCanEverTick = true;
}

void ABoat::BeginPlay() {
	Super::BeginPlay();

  m_velocity_input = FVector2D(0.0f);
  m_speed_input = normal_speed;
  m_has_requested_elevations = false;
}

void ABoat::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

  UpdateElevations();
  UpdateTransform(DeltaTime);
}

void ABoat::UpdateElevations() {

  SCOPE_CYCLE_COUNTER(STAT_UpdateElevations);

	TArray<FVector2D> sample_points;
	sample_points.Push(FVector2D(GetActorLocation().X, GetActorLocation().Y));
  TArray<float> elevations = ocean_surface_simulation->sample_elevation_points(sample_points);

  SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, elevations[0]));

}

void ABoat::UpdateTransform(float DeltaTime) {
  if (m_velocity_input.IsNearlyZero(0.001f)) {
    return;
  }

  FVector forward = GetActorForwardVector();
  FVector right = GetActorRightVector();

  // The direction we want to move in
  FVector dir = (right * m_velocity_input.X + forward * m_velocity_input.Y).GetSafeNormal();

  // Apply rotation by interpolating from our previous direction towards the new one.
  FRotator cur_rot = FRotator(0.0f, GetActorRotation().Yaw, 0.0f);
  FRotator dir_rot = FRotator(0.0f, 180.0f * (atan2(dir.Y, dir.X) / PI), 0.0f);

  auto new_rot = FQuat::Slerp(cur_rot.Quaternion(), dir_rot.Quaternion(), rotation_speed);

  SetActorRotation(new_rot, ETeleportType::None);

  // The boat only accelerates back or forth.
  dir = forward * m_velocity_input.Y;

  // Apply movement step
  FVector pos = GetActorLocation();
  FVector delta_pos = METERS_TO_UNREAL_UNITS * m_speed_input * dir * DeltaTime;
  SetActorLocation(pos + delta_pos);
}

void ABoat::SetupPlayerInputComponent(class UInputComponent* InputComponent) {
  Super::SetupPlayerInputComponent(InputComponent);

  InputComponent->BindAction("Speed1", IE_Pressed, this, &ABoat::UseSlowSpeed);
  InputComponent->BindAction("Speed2", IE_Pressed, this, &ABoat::UseNormalSpeed);
  InputComponent->BindAction("Speed3", IE_Pressed, this, &ABoat::UseFastSpeed);

  InputComponent->BindAxis("HorizontalAxis", this, &ABoat::HorizontalAxis);
  InputComponent->BindAxis("VerticalAxis", this, &ABoat::VerticalAxis);
}

void ABoat::UseSlowSpeed()   { m_speed_input = slow_speed; }
void ABoat::UseNormalSpeed() { m_speed_input = normal_speed; }
void ABoat::UseFastSpeed()   { m_speed_input = fast_speed; }

void ABoat::HorizontalAxis(float input) {
  m_velocity_input.X = input;
}

void ABoat::VerticalAxis(float input) {
  m_velocity_input.Y = input;
}
