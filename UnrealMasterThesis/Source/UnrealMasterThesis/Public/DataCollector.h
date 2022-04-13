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
	int framesToRecord;

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
	UTextureRenderTarget2D* eWave_hv_rtt;
	UTextureRenderTarget2D* serialization_rtt;
	class ShaderModelsModule* shaderModule;
	TArray<TScriptInterface<IBoatInterface>> boats;
	TArray<FVector> boatPositions;
	FDataCollectionSettings data_collection_settings;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	void update(UpdatePayload update_payload);
	void saveeWaveDataToFile(TArray<float>& data);
	void saveInputToFile();
	void saveBoatDataToFile();
	void readInputJSON(TArray<struct UpdatePayload>& inputSequence);

private:
	TArray<UpdatePayload> inputStates;
	int frameNumber = 0;
};
