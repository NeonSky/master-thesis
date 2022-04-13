#include "ArtificialBoat.h"

#include "Globals/StatelessHelpers.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"

// Aritifical boats will only affect other artificial boats, making it possible to compare GPU and Artificial boat without interactions between them.
struct SharedState {
    TArray<UTextureRenderTarget2D*> boat_rtts;
    TArray<UTextureRenderTarget2D*> ewave_rtts;
};
static SharedState shared_state;

AArtificialBoat::AArtificialBoat() {}
AArtificialBoat::~AArtificialBoat() {
    shared_state = SharedState();
}

void AArtificialBoat::BeginPlay() {
    Super::BeginPlay();

    m_shader_models_module.ResetGPUBoat(boat_rtt);
    for (int i = 0; i < artificial_frame_delay+1; i++) {
        m_readback_queue.push(readback_bank[i]);
    }
    m_requested_elevations_on_frame = 0;
    m_cur_frame = 0;

    for (auto b : readback_bank) {
        m_shader_models_module.Clear(b, FVector4(0.0, 0.0, 0.0, 1.0)); // Clearing readback bank RTTs, for consistent data collection
    }
    
}

void AArtificialBoat::UpdateReadbackQueue() {

    if (m_cur_frame - m_requested_elevations_on_frame >= artificial_frame_skip) {
        m_requested_elevations_on_frame = m_cur_frame;

        UTextureRenderTarget2D* src = elevation_rtt;
        UTextureRenderTarget2D* rtt = m_readback_queue.front();
        m_readback_queue.pop();

        FRenderCommandFence fence;
        ENQUEUE_RENDER_COMMAND(void)(
            [src, rtt](FRHICommandListImmediate& RHI_cmd_list) {
                RHI_cmd_list.CopyToResolveTarget(
                    src->GetRenderTargetResource()->GetRenderTargetTexture(),
                    rtt->GetRenderTargetResource()->GetRenderTargetTexture(),
                    FResolveParams()
                );
            });

        // NOTE: safety measure
        fence.BeginFence();
        fence.Wait();

        // Requeue latest fetch
        m_readback_queue.push(rtt);
    }

}

void AArtificialBoat::Update(UpdatePayload update_payload, std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback) {

    if (IsHidden()) {
        SetActorHiddenInGame(false);
        shared_state.boat_rtts.Push(this->boat_rtt);
        shared_state.ewave_rtts.Push(this->ewave_rtts.eWaveHV);
    }

    UpdateReadbackQueue();

    FVector2D velocity_input = use_p2_inputs ? update_payload.velocity_input2 : update_payload.velocity_input;

    TArray<UTextureRenderTarget2D*> other_boat_textures;
    for (auto& rtt : shared_state.boat_rtts) {
        if (rtt != boat_rtt) {
            other_boat_textures.Push(rtt);
        }
    }

    TArray<UTextureRenderTarget2D*> other_wake_textures;
    for (auto& rtt : shared_state.ewave_rtts) {
        if (rtt != ewave_rtts.eWaveHV) {
            other_wake_textures.Push(rtt);
        }
    }

    m_shader_models_module.UpdateGPUBoat(
        update_payload.speed_input,
        velocity_input,
        collision_mesh,
        m_readback_queue.front(),
        ewave_rtts.eWaveHV,
        other_wake_textures,
        ewave_rtts.obstruction,
        boat_rtt,
        other_boat_textures,
        readback_rtt,
        this,
        callback);

    m_cur_frame++;
}

UTextureRenderTarget2D* AArtificialBoat::GetBoatRTT() {
    return boat_rtt;
}

FeWaveRTTs AArtificialBoat::GeteWaveRTTs() {
    return ewave_rtts;
}

FVector AArtificialBoat::WorldPosition3D() {
    return this->GetActorLocation() / METERS_TO_UNREAL_UNITS;
}
