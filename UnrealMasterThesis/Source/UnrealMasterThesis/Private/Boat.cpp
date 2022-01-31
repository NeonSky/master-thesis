// Fill out your copyright notice in the Description page of Project Settings.

#include "Boat.h"
#include "Globals/StatelessHelpers.h"


ABoat::ABoat() {
	UE_LOG(LogTemp, Warning, TEXT("ABoat::ABoat()"));

	// Configure Tick() to be called every frame.
	PrimaryActorTick.bCanEverTick = true;
}

void ABoat::BeginPlay() {
	Super::BeginPlay();

  m_speed_input = speed1;
}

void ABoat::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

  FVector pos = GetActorLocation();

  FVector forward = GetActorForwardVector();
  FVector right = GetActorRightVector();

  FVector dir = (right * m_velocity_input.X + forward * m_velocity_input.Y).GetSafeNormal();

  FVector delta_pos = METERS_TO_UNREAL_UNITS * m_speed_input * dir * DeltaTime;

	UE_LOG(LogTemp, Warning, TEXT("delta_pos = %f, %f, %f"), delta_pos.X, delta_pos.Y, delta_pos.Z);

  SetActorLocation(GetActorLocation() + delta_pos);
}

void ABoat::SetupPlayerInputComponent(class UInputComponent* InputComponent) {
  Super::SetupPlayerInputComponent(InputComponent);

  InputComponent->BindAction("Speed1", IE_Pressed, this, &ABoat::UseSpeed1);
  InputComponent->BindAction("Speed2", IE_Pressed, this, &ABoat::UseSpeed2);
  InputComponent->BindAction("Speed3", IE_Pressed, this, &ABoat::UseSpeed3);

  InputComponent->BindAxis("HorizontalAxis", this, &ABoat::HorizontalAxis);
  InputComponent->BindAxis("VerticalAxis", this, &ABoat::VerticalAxis);
}

void ABoat::UseSpeed1() { m_speed_input = speed1; }
void ABoat::UseSpeed2() { m_speed_input = speed2; }
void ABoat::UseSpeed3() { m_speed_input = speed3; }

void ABoat::HorizontalAxis(float input) {
  m_velocity_input.X = input;
}

void ABoat::VerticalAxis(float input) {
  m_velocity_input.Y = input;
}
