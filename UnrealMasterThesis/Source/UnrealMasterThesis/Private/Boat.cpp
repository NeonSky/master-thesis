// Fill out your copyright notice in the Description page of Project Settings.

#include "Boat.h"
#include "Globals/StatelessHelpers.h"

#include "Engine/TextureRenderTarget2D.h"
#include "DrawDebugHelpers.h"

ABoat::ABoat() {}

void ABoat::BeginPlay() {
	Super::BeginPlay();

  input_pawn->on_fixed_update.AddUObject<ABoat>(this, &ABoat::Update);

  m_rigidbody = Rigidbody(mass); // kg (wet weight, i.e. including fuel). Our boat should be similar to a cabin cruiser: https://www.godownsize.com/how-much-do-boats-weigh/

  m_requested_elevations_on_frame = 0;
  m_cur_frame = 0;

  // Sync the rigidbody transform with the UE transform
  m_rigidbody.position = GetActorLocation() / METERS_TO_UNREAL_UNITS;
  m_rigidbody.orientation = GetActorQuat();

  FetchCollisionMeshData();
}

void ABoat::FetchCollisionMeshData() {

  FStaticMeshLODResources& mesh_res = collision_mesh->GetStaticMeshComponent()->GetStaticMesh()->RenderData->LODResources[0];

  // Index data
  mesh_res.IndexBuffer.GetCopy(m_collision_mesh_indices);

  // Vertex data
  FPositionVertexBuffer& pos_buffer = mesh_res.VertexBuffers.PositionVertexBuffer;
  uint32 n_vertices = pos_buffer.GetNumVertices();

  m_collision_mesh_vertices.SetNum(n_vertices);

  FVector* data = (FVector*) pos_buffer.GetVertexData();
  FMemory::Memcpy(m_collision_mesh_vertices.GetData(), data, sizeof(FVector) * n_vertices);

  FTransform transform = collision_mesh->GetActorTransform();
  for (auto& v : m_collision_mesh_vertices) {
    v = transform.TransformPosition(v) / METERS_TO_UNREAL_UNITS;
  }

  // Total surface area
  m_collision_mesh_surface_area = 0.0f;

  uint32 n_triangles = m_collision_mesh_indices.Num() / 3;
  for (int i = 0; i < n_triangles; i++) {

    uint32 i0 = m_collision_mesh_indices[3*i+0];
    uint32 i1 = m_collision_mesh_indices[3*i+1];
    uint32 i2 = m_collision_mesh_indices[3*i+2];

    FVector v0 = m_collision_mesh_vertices[i0];
    FVector v1 = m_collision_mesh_vertices[i1];
    FVector v2 = m_collision_mesh_vertices[i2];

    float area = FVector::CrossProduct(v2 - v0, v1 - v0).Size() / 2.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("v0: %i, %f, %f, %f"), i0, v0.X, v0.Y, v0.Z);
    UE_LOG(LogTemp, Warning, TEXT("v1: %i, %f, %f, %f"), i1, v1.X, v1.Y, v1.Z);
    UE_LOG(LogTemp, Warning, TEXT("v2: %i, %f, %f, %f"), i2, v2.X, v2.Y, v2.Z);
    UE_LOG(LogTemp, Warning, TEXT("i = %i has area: %f"), i, area);

    m_collision_mesh_surface_area += area;

  }
}

void ABoat::Update(UpdatePayload update_payload) {

  m_speed_input = update_payload.speed_input;
  m_velocity_input = update_payload.velocity_input;

  FlushPersistentDebugLines(this->GetWorld());

  UpdateReadbackQueue();
  UpdateSubmergedTriangles();

  float submerged_area = 0.0f;
  for (auto& t : m_submerged_triangles) {
    submerged_area += t.area;
  }
  float r_s = submerged_area / m_collision_mesh_surface_area;
  UE_LOG(LogTemp, Warning, TEXT("CPU Debug output: %f, %f, %f"), m_rigidbody.position.X, m_rigidbody.position.Y, m_rigidbody.position.Z);
  UE_LOG(LogTemp, Warning, TEXT("CPU Debug output: %f, %f, %f, %f"), m_rigidbody.orientation.X, m_rigidbody.orientation.Y, m_rigidbody.orientation.Z, m_rigidbody.orientation.W);
  UE_LOG(LogTemp, Warning, TEXT("CPU Debug output: %f, %f, %f"), r_s, submerged_area, m_collision_mesh_surface_area);

  ApplyGravity();
  ApplyBuoyancy();
  ApplyResistanceForces(r_s);
  ApplyUserInput(r_s);

  m_rigidbody.Update(0.02f); // We use a fixed delta time for physics

  // DebugDrawVelocities();

  SetActorLocation(METERS_TO_UNREAL_UNITS * m_rigidbody.position);
  SetActorRotation(m_rigidbody.orientation, ETeleportType::None);

  m_cur_frame++;
}

void ABoat::DebugDrawTriangle(FVector v0, FVector v1, FVector v2, FColor color) {

  v0 *= METERS_TO_UNREAL_UNITS;
  v1 *= METERS_TO_UNREAL_UNITS;
  v2 *= METERS_TO_UNREAL_UNITS;

  DrawDebugLine(this->GetWorld(), v0, v1, color, true, 0.0f, 200, 1.0f);
  DrawDebugLine(this->GetWorld(), v1, v2, color, true, 0.0f, 200, 1.0f);
  DrawDebugLine(this->GetWorld(), v2, v0, color, true, 0.0f, 200, 1.0f);

}

void ABoat::DebugDrawForce(FVector c, FVector f, FColor color) {

  c *= METERS_TO_UNREAL_UNITS;
  f *= METERS_TO_UNREAL_UNITS;

  DrawDebugDirectionalArrow(
    this->GetWorld(),
    c,
    c + f,
    1.0f,
    color,
    true,
    0.0f,
    0,
    1.0f
  );

}

void ABoat::DebugDrawVelocities() {

  FTransform transform = m_rigidbody.Transform();

  uint32 n_triangles = m_collision_mesh_indices.Num() / 3;
  for (int i = 0; i < n_triangles; i++) {

    SubmergedTriangle t;
    t.velocity = 0.0f;

    // Triangle vertex indices 
    uint32 i0 = m_collision_mesh_indices[3*i+0];
    uint32 i1 = m_collision_mesh_indices[3*i+1];
    uint32 i2 = m_collision_mesh_indices[3*i+2];

    // Triangle vertices 
    FVector v0 = m_collision_mesh_vertices[i0];
    FVector v1 = m_collision_mesh_vertices[i1];
    FVector v2 = m_collision_mesh_vertices[i2];

    // Convert vertices to world space
    v0 = transform.TransformPosition(v0);
    v1 = transform.TransformPosition(v1);
    v2 = transform.TransformPosition(v2);

    FVector centroid = (v0 + v1 + v2) / 3.0f;

    DebugDrawForce(centroid, m_rigidbody.VelocityAt(centroid), FColor::Orange);
  }
}

void ABoat::UpdateReadbackQueue() {

  // Request elevations
  if (m_cur_frame - m_requested_elevations_on_frame >= artificial_frame_skip) {
    m_requested_elevations_on_frame = m_cur_frame;

    // Fetch ocean surface elevations for all vertices
    FTransform transform = m_rigidbody.Transform();
    TArray<FVector2D> sample_points;
    for (auto &v : m_collision_mesh_vertices) {
      FVector v_ws = transform.TransformPosition(v) * METERS_TO_UNREAL_UNITS;
      sample_points.Push(FVector2D(v_ws.X, v_ws.Y));
    }
    TArray<float> elevations = ocean_surface_simulation->sample_elevation_points(sample_points);

    m_readback_queue.push(elevations);
  }

  // Update latest response
  if (m_readback_queue.size() > artificial_frame_delay) {
    m_latest_elevations = m_readback_queue.front();
    m_readback_queue.pop();
  }

}

void ABoat::UpdateSubmergedTriangles() {

  // Skip if no there has not been any response yet
  if (m_latest_elevations.Num() == 0) {
    return;
  }

  m_submerged_triangles.SetNum(0);

  FTransform transform = m_rigidbody.Transform();

  uint32 n_triangles = m_collision_mesh_indices.Num() / 3;
  for (int i = 0; i < n_triangles; i++) {

    SubmergedTriangle t;
    t.velocity = 0.0f;

    // Triangle vertex indices 
    uint32 i0 = m_collision_mesh_indices[3*i+0];
    uint32 i1 = m_collision_mesh_indices[3*i+1];
    uint32 i2 = m_collision_mesh_indices[3*i+2];

    // Triangle vertices 
    FVector v0 = m_collision_mesh_vertices[i0];
    FVector v1 = m_collision_mesh_vertices[i1];
    FVector v2 = m_collision_mesh_vertices[i2];

    // Convert vertices to world space
    v0 = transform.TransformPosition(v0);
    v1 = transform.TransformPosition(v1);
    v2 = transform.TransformPosition(v2);

    // Compute normal (which is identical regardless of cut)
    t.normal = FVector::CrossProduct(v2 - v0, v1 - v0).GetSafeNormal();

    // Debug draw the entire collision mesh
    // DebugDrawTriangle(v0, v1, v2, FColor::Red);

    // The relevant ocean elevation samples
    float e0 = m_latest_elevations[i0] / METERS_TO_UNREAL_UNITS;
    float e1 = m_latest_elevations[i1] / METERS_TO_UNREAL_UNITS;
    float e2 = m_latest_elevations[i2] / METERS_TO_UNREAL_UNITS;

    // Heights relative to the ocean surface for each vertex
    float h0 = v0.Z - e0;
    float h1 = v1.Z - e1;
    float h2 = v2.Z - e2;

    // Debug draw the relative heights
    // DrawDebugLine(this->GetWorld(), v0 * METERS_TO_UNREAL_UNITS, (v0 - FVector(0.0, 0.0, h0)) * METERS_TO_UNREAL_UNITS, FColor::White, true, 0.0f, 0, 1.0f);
    // DrawDebugLine(this->GetWorld(), v1 * METERS_TO_UNREAL_UNITS, (v1 - FVector(0.0, 0.0, h1)) * METERS_TO_UNREAL_UNITS, FColor::White, true, 0.0f, 0, 1.0f);
    // DrawDebugLine(this->GetWorld(), v2 * METERS_TO_UNREAL_UNITS, (v2 - FVector(0.0, 0.0, h2)) * METERS_TO_UNREAL_UNITS, FColor::White, true, 0.0f, 0, 1.0f);

    // Sort vertices by height
    TArray<std::pair<float, FVector>> ps = {
      std::make_pair(h0, v0),
      std::make_pair(h1, v1),
      std::make_pair(h2, v2)
    };
    ps.Sort([](std::pair<float, FVector> p1, std::pair<float, FVector> p2) {
      return p1.first < p2.first;
    });

    // Extract back into variables with short names
    float h_L = ps[0].first;
    float h_M = ps[1].first;
    float h_H = ps[2].first;

    FVector v_L = ps[0].second; // The vertex whose relative height above the ocean surface is the smallest.
    FVector v_M = ps[1].second;
    FVector v_H = ps[2].second; // The vertex whose relative height above the ocean surface is the greatest.

    ////////////////////////////////////////////
    /* Cut into submerged triangles (3 cases) */
    ////////////////////////////////////////////

    // No vertex above water (assume fully submerged)
    if (h_H < 0.0f) {

      t.v_L = v_L;
      t.v_M = v_M;
      t.v_H = v_H;

      // Debug draw fully submerged triangles
      // DebugDrawTriangle(v_L, v_M, v_H, FColor::Green);

      t.centroid = (v_L + v_M + v_H) / 3.0;
      t.height   = abs(h_L + h_M + h_H) / 3.0f; // TODO: compute properly. We should sample the elevation at the centroid
      t.area     = FVector::CrossProduct(v_H - v_L, v_M - v_L).Size() / 2.0f;

      m_submerged_triangles.Push(t);
    }

    // Only one vertex above water (the H vertex)
    else if (h_M < 0.0f) {

      // Approximate the intermediate points where the triangle should be cut.
      float t_M = -h_M / (h_H - h_M);
      float t_L = -h_L / (h_H - h_L);

      FVector I_M = v_M + t_M * (v_H - v_M);
      FVector I_L = v_L + t_L * (v_H - v_L);

      // Debug draw partially submerged triangles
      // DebugDrawTriangle(v_L, I_L, I_M, FColor::Yellow);
      // DebugDrawTriangle(v_L, v_M, I_M, FColor::Yellow);

      // We will end up with a quad in this case. We will treat it as two triangles.

      // Triangle #1
      t.centroid = (v_L + I_L + v_M) / 3.0;
      t.height   = abs(h_L + h_M + 0.0f) / 3.0f; // TODO: compute properly. We should sample the elevation at the centroid
      t.area     = FVector::CrossProduct(I_L - v_L, v_M - v_L).Size() / 2.0f;
      m_submerged_triangles.Push(t);

      // Triangle #2
      t.centroid = (v_M + I_M + I_L) / 3.0;
      t.height   = abs(h_M + 0.0f + 0.0f) / 3.0f; // TODO: compute properly. We should sample the elevation at the centroid
      t.area     = FVector::CrossProduct(I_M - v_M, I_L - v_M).Size() / 2.0f;
      m_submerged_triangles.Push(t);
    }

    // Only one vertex below water (the L vertex)
    else if (h_L < 0.0f) {

      // Approximate the intermediate points where the triangle should be cut.
      float t_M = -h_L / (h_M - h_L);
      float t_H = -h_L / (h_H - h_L);

      FVector J_M = v_L + t_M * (v_M - v_L);
      FVector J_H = v_L + t_H * (v_H - v_L);

      // Debug draw partially submerged triangles
      // DebugDrawTriangle(v_L, J_M, J_H, FColor::Yellow);

      t.centroid = (v_L + J_M + J_H) / 3.0;
      t.height   = abs(h_L + 0.0f + 0.0f) / 3.0f; // TODO: compute properly. We should sample the elevation at the centroid
      t.area     = FVector::CrossProduct(J_M - v_L, J_H - v_L).Size() / 2.0f;
      m_submerged_triangles.Push(t);
    }

  }

}

void ABoat::ApplyGravity() {
  FVector force = FVector(0.0, 0.0, -GRAVITY * m_rigidbody.mass);
  m_rigidbody.AddForceAtPosition(force, m_rigidbody.position);
}

void ABoat::ApplyBuoyancy() {

  float height = m_rigidbody.position.Z;
  if (height < 0.0) {
      height = -height;
      float area = 27.045519;
      FVector buoyancy_force = FVector(0.0, 0.0, DENSITY_OF_WATER * GRAVITY * height * area);
      m_rigidbody.AddForceAtPosition(buoyancy_force, m_rigidbody.position);
  }

  // for (auto& t : m_submerged_triangles) {

  //   // (kg / m^3) * (m / s^2) * (m) * (m^2) = kg * (m / s^2) = N
  //   FVector buoyancy_force = -DENSITY_OF_WATER * GRAVITY * t.height * t.area * t.normal;
  //   buoyancy_force = FVector(0.0, 0.0, abs(buoyancy_force.Z));

  //   m_rigidbody.AddForceAtPosition(buoyancy_force, t.centroid);

  //   // DebugDrawForce(t.centroid, buoyancy_force / METERS_TO_UNREAL_UNITS, FColor::Purple);
  // }

}

void ABoat::ApplyResistanceForces(float r_s) {

  float c_damp = 500.0f;

  UE_LOG(LogTemp, Warning, TEXT("10:48"));

  // Linear damping of linear velocity
  m_rigidbody.AddForceAtPosition(-c_damp * r_s * m_rigidbody.linear_velocity, m_rigidbody.position);

  // Quadratic damping of linear velocity
  m_rigidbody.AddForceAtPosition(-c_damp * r_s * m_rigidbody.linear_velocity * m_rigidbody.linear_velocity.Size(), m_rigidbody.position);

  // Damping of angular velocity
  // Based on: https://forum.unity.com/threads/how-is-angular-drag-applied-to-a-rigidbody-in-unity-how-is-angular-damping-applied-in-physx.369599/#:~:text=13-,After%20many%20tests%20I%27ve%20determined%20that%20this%20highly%20advanced%20formula%20reveals%20the%20secrets,-to%20PhysX%27s%20angular
  // Multiplying by (1.0f - r_s) isn't very realistic, but the results are pretty good.
  m_rigidbody.angular_velocity -= angular_drag * m_rigidbody.angular_velocity * (1.0f - r_s);
}

void ABoat::ApplyUserInput(float r_s) {

  FVector forward = GetActorForwardVector();
  FVector right = GetActorRightVector();
  FVector up = GetActorUpVector();

  if (m_velocity_input.Y > 0.0f) {
    FVector engine_pos = (-forward) * 2.1f + (-up) * 0.3f;
    engine_pos += m_rigidbody.position;

    float engine_power = HORSEPOWER_TO_NEWTON * m_speed_input * sqrt(r_s);

    m_rigidbody.AddForceAtPosition(engine_power * m_velocity_input.Y * forward, engine_pos);
  }

  if (m_velocity_input.X != 0.0f) {
    FVector steer_pos = m_rigidbody.position + 100.0f * forward;
    float engine_power = HORSEPOWER_TO_NEWTON * sqrt(m_speed_input) * sqrt(r_s); // Nerf sideways movement
    m_rigidbody.AddForceAtPosition(engine_power * m_velocity_input.X * right, steer_pos);

  }

}