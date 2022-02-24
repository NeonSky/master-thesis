#include "Rigidbody.h"

Rigidbody::Rigidbody() {
  this->mass = 0.0f;
  this->moment_of_inertia = 0.0f;
}

Rigidbody::Rigidbody(float m) {
  this->mass = m;
  this->moment_of_inertia = 100.0f * m; // TODO
}

void Rigidbody::AddForceAtPosition(FVector f, FVector pos) {

  // Convert position to a relative position
  pos -= this->position;

  this->force += f;

  // There is no torque if the force is applied at the center of mass.
  // if (!pos.IsNearlyZero()) {
  //   this->torque += FVector::CrossProduct(pos, f); 
  // }
}

void Rigidbody::Update(float dt) {

  FVector linear_acceleration = this->force / this->mass;
  this->linear_velocity += linear_acceleration * dt;
  this->position += linear_velocity * dt;

  FVector angular_acceleration = this->torque / this->moment_of_inertia;
  this->angular_velocity += angular_acceleration * dt;

  // Angular velocity orientation
  auto v = 0.5f * this->angular_velocity * dt;
  FQuat avo = FQuat(v.X, v.Y, v.Z, 0.0);

  this->orientation = (this->orientation + avo * this->orientation).GetNormalized();

  // Reset forces
  this->force = FVector(0.0f);
  this->torque = FVector(0.0f);

}

FVector Rigidbody::VelocityAt(FVector pos) {
  return this->linear_velocity + FVector::CrossProduct(this->angular_velocity, pos);
}

FTransform Rigidbody::Transform() {
  return FTransform(this->orientation, this->position, FVector(1.0f, 1.0f, 1.0f));
}