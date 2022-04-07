// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Globals/StatelessHelpers.h"
#include "IBoat.h"
#include "DataCollector.generated.h"

USTRUCT(BlueprintType)
struct FDataCollectionSettings {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere)
	bool shouldCollectBoatData;

	UPROPERTY(EditAnywhere)
	bool shouldCollecteWaveTextures;

	UPROPERTY(EditAnywhere)
	bool shouldCollectInputStates;

	UPROPERTY(EditAnywhere)
	bool shouldPlayBackInputSequence;

	UPROPERTY(EditAnywhere)
	int secondsToRecord;

	UPROPERTY(EditAnywhere)
	FString fileName;

	UPROPERTY(EditAnywhere)
	FString folderName;

	UPROPERTY(EditAnywhere)
	FString inputFileName;
};

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
	TArray<TScriptInterface<IBoatInterface>> boats;
	TArray<FVector> boatPositions;
	FDataCollectionSettings data_collection_settings;
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
	int framesToCollect;
};
