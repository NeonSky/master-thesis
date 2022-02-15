// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// NOTE: We have to use our own rigidbody implementation instead of UE's built-in system,
// as we need to be able to replicate the physics perfectly on the GPU.
struct Rigidbody {
  FVector force = FVector(0.0f);
  FVector acceleration = FVector(0.0f);
  FVector linear_velocity = FVector(0.0f);
  FVector position = FVector(0.0f);

  FVector torque = FVector(0.0f);
  FVector angular_velocity = FVector(0.0f);
  FQuat orientation;

  float mass;
  float moment_of_inertia;

  Rigidbody();
  Rigidbody(float mass);

  void AddForceAtPosition(FVector force, FVector pos);
  void Update(float dt);
  FVector VelocityAt(FVector pos);
  FTransform Transform();
};