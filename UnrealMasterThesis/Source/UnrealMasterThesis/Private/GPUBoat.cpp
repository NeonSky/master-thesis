#include "GPUBoat.h"

#include "Kismet/GameplayStatics.h"

AGPUBoat::AGPUBoat() {}

void AGPUBoat::BeginPlay() {
	  Super::BeginPlay();

    m_shader_models_module.ResetGPUBoat(boat_rtt);
}

void AGPUBoat::Update(UpdatePayload update_payload) {

    if (IsHidden()) {
        SetActorHiddenInGame(false);
    }

    m_shader_models_module.UpdateGPUBoat(
        update_payload.speed_input,
        update_payload.velocity_input,
        collision_mesh,
        elevation_rtt,
        ewave_rtts.eWaveH,
        boat_rtt,
        readback_rtt,
        m_submerged_triangles,
        camera_follow ? camera_target : nullptr);
}

UTextureRenderTarget2D* AGPUBoat::GetBoatRTT() {
    return boat_rtt;
}

FeWaveRTTs AGPUBoat::GeteWaveRTTs() {
    return ewave_rtts;
}

TRefCountPtr<FRDGPooledBuffer> AGPUBoat::GetSubmergedTriangles() {
    return m_submerged_triangles;
}