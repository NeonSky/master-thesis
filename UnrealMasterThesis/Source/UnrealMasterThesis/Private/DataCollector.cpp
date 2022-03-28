// Fill out your copyright notice in the Description page of Project Settings.
#include "DataCollector.h"
#include "ImageUtils.h"
#include "Serialization/BufferArchive.h"
#include "Engine/TextureRenderTarget2D.h"




// Sets default values for this component's properties
UDataCollector::UDataCollector()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UDataCollector::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UDataCollector::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UDataCollector::collectBoatData(FVector boatPos) {
	//UE_LOG(LogTemp, Warning, TEXT("Logging a position"));
	boatPositions.Add(boatPos);
}

void UDataCollector::saveDataToFile(TArray<float>& data) {
	FString fname = *FString(TEXT("TempTestData/TestData") + FString::FromInt(frameNumber++) + TEXT(".txt"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;

	FString textToSave = TEXT("Test\n");
	int i = 0;
	for (float f : data) {
		FString floatString = FString::SanitizeFloat(f);
		textToSave.Append(TEXT("r: ") + floatString + TEXT(", "));
		if (i >= 255) { 
			textToSave.Append(TEXT(" \n"));
			i = 0; 
		}
		i++;
	}
	//UE_LOG(LogTemp, Error, TEXT("\n\nTEST: %f\n\n"), data[256]);
	//UE_LOG(LogTemp, Error, TEXT("Saving to file: %s"), AbsoluteFilePath);
	FFileHelper::SaveStringToFile(textToSave, *AbsoluteFilePath);
}

void UDataCollector::saveTextureToFile(UTextureRenderTarget2D* rtt) {
	FString AbsoluteFilePath = FPaths::ProjectDir() + "test.exr";
	FArchive* Ar = IFileManager::Get().CreateFileWriter(*AbsoluteFilePath);

	if (!Ar) {
		UE_LOG(LogTemp, Error, TEXT("\n\nNO AR\n\n"));
		return;
	}
	auto test = rtt->GetFormat();
	FBufferArchive Buffer;
	const bool success = FImageUtils::ExportRenderTarget2DAsEXR(rtt, Buffer);
	if (success) {
		//Ar->Serialize(const_cast<uint8*>(Buffer.GetData()), Buffer.Num());
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("\n\nFAIL\n\n"));
	}

	delete Ar;
}
