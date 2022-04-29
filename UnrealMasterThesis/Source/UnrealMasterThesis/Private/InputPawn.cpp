#include "InputPawn.h"
#include "Globals/StatelessHelpers.h"
#include "Kismet/GameplayStatics.h"

AInputPawn::AInputPawn() {
	// Configure Tick() to be called every frame.
	PrimaryActorTick.bCanEverTick = true;
}

void AInputPawn::BeginPlay() {
	Super::BeginPlay();

    m_velocity_input = FVector2D(0.0f);
    m_speed_input = slow_speed;
}

void AInputPawn::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

    if (playBackInputSequence && frame < preRecordedInputSequence.Num()) {
        auto& state = preRecordedInputSequence[frame++];
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
    on_fixed_update.Broadcast(payload);

    APlayerController* controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    //controller->SetViewTarget(camera_target, FViewTargetTransitionParams());
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
