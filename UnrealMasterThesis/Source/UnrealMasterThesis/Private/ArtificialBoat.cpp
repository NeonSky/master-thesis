#include "ArtificialBoat.h"

#include "Globals/StatelessHelpers.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"

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

    ENQUEUE_RENDER_COMMAND(void)(
        [this](FRHICommandListImmediate& RHI_cmd_list) {

            FRDGBuilder graph_builder(RHI_cmd_list);

            TArray<TRefCountPtr<FRDGPooledBuffer>> buffers;
            buffers.SetNum(artificial_frame_delay+1);

            for (int i = 0; i < buffers.Num(); i++) {

                TArray<FVector4> dummy_data;
                int N = 70; // triangle count
                dummy_data.SetNum(3*N);
                for (int j = 0; j < dummy_data.Num(); j++) {
                    dummy_data[j] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
                }

                FRDGBufferRef rdg_buffer_ref = CreateVertexBuffer(
                    graph_builder,
                    TEXT("LatencyElevationsBuffer"),
                    FRDGBufferDesc::CreateBufferDesc(sizeof(float), dummy_data.Num()),
                    dummy_data.GetData(),
                    sizeof(float) * dummy_data.Num(),
                    ERDGInitialDataFlags::None
                );

                graph_builder.QueueBufferExtraction(rdg_buffer_ref, &buffers[i]);
            }

            graph_builder.Execute();


            for (int i = 0; i < buffers.Num(); i++) {
                m_readback_queue.push(buffers[i]);
            }
        });

    FRenderCommandFence fence;
    fence.BeginFence();
    fence.Wait();

    m_requested_elevations_on_frame = 0;
    m_cur_frame = 0;
    
}

void AArtificialBoat::UpdateReadbackQueue(TArray<UTextureRenderTarget2D*> other_boat_textures, TArray<UTextureRenderTarget2D*> other_wake_textures) {

    if (m_cur_frame - m_requested_elevations_on_frame >= artificial_frame_skip) {

        if (true) { // TODO: make boolean flag
            static std::uniform_int_distribution<> uniform_int_dist(0, delay_distribution.Num() - 1);
            float sample = delay_distribution[uniform_int_dist(rng)];
            int next_artificial_frame_skip = 0;
            if (sample > 20 && sample <= 40) {
                next_artificial_frame_skip = 1;
            }
            else if (sample > 40 && sample <= 60) {
                next_artificial_frame_skip = 2;
            }
            else if (sample > 60) {
                next_artificial_frame_skip = 3;
            }
            // UE_LOG(LogTemp, Error, TEXT("Next artificial frame skip and delay (skip, delay): (%d, %f)"), next_artificial_frame_skip, sample);
            artificial_frame_skip = next_artificial_frame_skip;
        }


        m_requested_elevations_on_frame = m_cur_frame;

        TRefCountPtr<FRDGPooledBuffer> latency_elevations = m_readback_queue.front();
        m_readback_queue.pop();

        m_shader_models_module.UpdateArtificialBoat1(
            collision_mesh,
            elevation_rtt,
            ewave_rtts.eWaveHV,
            other_wake_textures,
            ewave_rtts.obstruction,
            boat_rtt,
            other_boat_textures,
            &latency_elevations);

        FRenderCommandFence fence;
        fence.BeginFence();
        fence.Wait();

        // Requeue latest fetch
        m_readback_queue.push(latency_elevations);
    }

}

void AArtificialBoat::Update(UpdatePayload update_payload, std::function<void(TRefCountPtr<FRDGPooledBuffer>)> callback) {

    if (IsHidden()) {
        SetActorHiddenInGame(false);
        shared_state.boat_rtts.Push(this->boat_rtt);
        shared_state.ewave_rtts.Push(this->ewave_rtts.eWaveHV);
    }

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


    UpdateReadbackQueue(other_boat_textures, other_wake_textures);

    if (m_readback_queue.front() == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("Shouldn't happen."));
        return;
    }

    FVector2D velocity_input = use_p2_inputs ? update_payload.velocity_input2 : update_payload.velocity_input;

    m_shader_models_module.UpdateArtificialBoat2(
        update_payload.speed_input,
        velocity_input,
        collision_mesh,
        elevation_rtt,
        ewave_rtts.eWaveHV,
        other_wake_textures,
        ewave_rtts.obstruction,
        boat_rtt,
        other_boat_textures,
        readback_rtt,
        this,
        callback,
        &m_readback_queue.front());

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

void AArtificialBoat::setDist(TArray<float> dist, int seed, bool organic)
{
    delay_distribution = dist;
    rng.seed(seed);
    organicDelay = organic;
}
