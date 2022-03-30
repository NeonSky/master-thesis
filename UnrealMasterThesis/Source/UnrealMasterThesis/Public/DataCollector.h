// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Globals/StatelessHelpers.h"
#include "IBoat.h"
#include "DataCollector.generated.h"

/*struct BoatState {
	FVector;
	FQuat;
};*/

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREALMASTERTHESIS_API UDataCollector : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDataCollector();
	class AInputPawn* inputPawn;
	UTextureRenderTarget2D* eWave_h_rtt;
	UTextureRenderTarget2D* eWave_v_rtt;
	UTextureRenderTarget2D* serialization_rtt;
	class ShaderModelsModule* shaderModule;
	TArray<TScriptInterface<IBoatInterface>> boat_ptrs; // TODO
	TArray<FVector> boatPositions;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void saveeWaveDataToFile(TArray<float>& data);
	void saveInputToFile();
	void saveBoatDataToFile();
	void readInputJSON(TArray<InputState>& inputSequence);


private:

	TArray<InputState> inputStates;
	int frameNumber = 0;
	int framesToCollect = 60 * 10; // TODO: expose these (+ should playback) to editor so we don't have to change any code
	bool shouldCollectBoatData = true; 
	bool shouldCollecteWaveTextures = false;
	bool shouldCollectInputStates = false;
};
