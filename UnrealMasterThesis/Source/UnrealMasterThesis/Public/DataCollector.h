// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Globals/StatelessHelpers.h"
#include "DataCollector.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREALMASTERTHESIS_API UDataCollector : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDataCollector();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void collectBoatData(FVector boatPos);
	
	void collectInputData(InputState state);

	void saveeWaveDataToFile(TArray<float>& data);
	void saveTextureToFile(UTextureRenderTarget2D* rtt);
	void saveInputToFile();


private:
	TArray<FVector> boatPositions;
	TArray<InputState> inputStates;
	int frameNumber = 0;
	int framesToCollect = 60 * 5;
	bool shouldCollectBoatData = false;
	bool shouldCollecteWaveTextures = false;
	bool shouldCollectInputStates = true;
};
