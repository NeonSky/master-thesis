#include "CPUBoat.h"

#include "Kismet/GameplayStatics.h"

ACPUBoat::ACPUBoat() {}

void ACPUBoat::BeginPlay() {
	  Super::BeginPlay();

    input_pawn->on_fixed_update.AddUObject<ACPUBoat>(this, &ACPUBoat::Update);

    m_shader_models_module.ResetGPUBoat(boat_rtt);
}

void ACPUBoat::Update(UpdatePayload update_payload) {

    m_shader_models_module.UpdateGPUBoat(
        update_payload.speed_input,
        update_payload.velocity_input,
        collision_mesh,
        elevation_rtt,
        boat_rtt,
        readback_rtt,
        this);
}