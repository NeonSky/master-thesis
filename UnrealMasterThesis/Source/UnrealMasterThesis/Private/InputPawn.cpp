#include "InputPawn.h"
#include "Globals/StatelessHelpers.h"
#include "Kismet/GameplayStatics.h"

#include <chrono>

static float average_cpu_cost;

AInputPawn::AInputPawn() {
	// Configure Tick() to be called every frame.
	PrimaryActorTick.bCanEverTick = true;
}

void AInputPawn::BeginPlay() {
	Super::BeginPlay();

  m_velocity_input = FVector2D(0.0f);
  m_speed_input = slow_speed;

  average_cpu_cost = 0.0f;
}

void AInputPawn::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

    if (playBackInputSequence && frame < preRecordedInputSequence.Num()) {
        auto& state = preRecordedInputSequence[frame];
        m_speed_input = state.speed_input;
        m_velocity_input.X = state.velocity_input.X;
        m_velocity_input.Y = state.velocity_input.Y;
        m_velocity_input2.X = state.velocity_input2.X;
        m_velocity_input2.Y = state.velocity_input2.Y;
    }

    UpdatePayload payload;
    payload.speed_input = m_speed_input;
    payload.velocity_input = m_velocity_input;
    payload.velocity_input2 = m_velocity_input2;


    if (measure_cpu_cost) {
      auto timer_start = std::chrono::high_resolution_clock::now();
      on_fixed_update.Broadcast(payload);
      auto timer_end = std::chrono::high_resolution_clock::now();

      auto timer_duration = std::chrono::duration_cast<std::chrono::microseconds>(timer_end - timer_start);
      float ms_timer_duration = ((float) timer_duration.count()) / 1000.0f;

      if (frame == 0) {
        average_cpu_cost = ms_timer_duration;
      } else {
        average_cpu_cost = (ms_timer_duration + frame * average_cpu_cost) / (frame+1);

        if (frame % 60 == 0) {
          UE_LOG(LogTemp, Warning, TEXT("CPU cost: %f ms"), average_cpu_cost);
          GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("CPU cost: %f ms"), average_cpu_cost));
        }
      }
    }
    else {
      on_fixed_update.Broadcast(payload);
    }

    APlayerController* controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    controller->SetViewTarget(camera_target, FViewTargetTransitionParams());

    frame++;
    if (take_screenshot_of_frame == frame) {
      Record();
    }
}

void AInputPawn::SetupPlayerInputComponent(class UInputComponent* inputComponent) {
  Super::SetupPlayerInputComponent(inputComponent);

  inputComponent->BindAction("Speed1", IE_Pressed, this, &AInputPawn::UseSlowSpeed);
  inputComponent->BindAction("Speed2", IE_Pressed, this, &AInputPawn::UseNormalSpeed);
  inputComponent->BindAction("Speed3", IE_Pressed, this, &AInputPawn::UseFastSpeed);

  inputComponent->BindAxis("HorizontalAxis", this, &AInputPawn::HorizontalAxis);
  inputComponent->BindAxis("VerticalAxis", this, &AInputPawn::VerticalAxis);

  inputComponent->BindAxis("HorizontalAxis2", this, &AInputPawn::HorizontalAxis2);
  inputComponent->BindAxis("VerticalAxis2", this, &AInputPawn::VerticalAxis2);

  inputComponent->BindKey(FKey("P"), IE_Pressed, this, &AInputPawn::Record);
  inputComponent->BindKey(FKey("V"), IE_Pressed, this, &AInputPawn::Viewport);
}

void AInputPawn::UseSlowSpeed()   { m_speed_input = slow_speed; }
void AInputPawn::UseNormalSpeed() { m_speed_input = normal_speed; }
void AInputPawn::UseFastSpeed()   { m_speed_input = fast_speed; }

void AInputPawn::HorizontalAxis(float input) {
  m_velocity_input.X = input;
}

void AInputPawn::VerticalAxis(float input) {
  m_velocity_input.Y = input;
}

void AInputPawn::HorizontalAxis2(float input) {
  m_velocity_input2.X = input;
}

void AInputPawn::VerticalAxis2(float input) {
  m_velocity_input2.Y = input;
}

void AInputPawn::Record() {
		FString fname = *FString(TEXT("scr.png"));
		FString AbsoluteFilePath = FPaths::ProjectDir() + fname;
		FScreenshotRequest::RequestScreenshot(AbsoluteFilePath, true, false);
}

void AInputPawn::Viewport() {
    FVector2D resolution;
    GEngine->GameViewport->GetViewportSize(resolution);
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("The viewport resolution is (%f, %f)"), resolution.X, resolution.Y));
}