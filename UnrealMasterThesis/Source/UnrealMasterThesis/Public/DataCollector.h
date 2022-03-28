// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
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
	void saveDataToFile(TArray<float>& data);
	void saveTextureToFile(UTextureRenderTarget2D* rtt);


private:
	TArray<FVector> boatPositions;
	int frameNumber = 0;
};
