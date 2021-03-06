// Fill out your copyright notice in the Description page of Project Settings.
#include "DataCollector.h"
#include "InputPawn.h"
#include "ImageUtils.h"
#include "ShaderModels.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/BufferArchive.h"
#include "Engine/TextureRenderTarget2D.h"




// Sets default values for this component's properties
UDataCollector::UDataCollector()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false; // does not update independently. The update method is called by OceanSurfaceSimulation after each ocean update
}


// Called when the game starts
void UDataCollector::BeginPlay()
{
	Super::BeginPlay();
}


void UDataCollector::update(UpdatePayload update_payload) {
	if (frameNumber == data_collection_settings.framesToRecord) {
		UE_LOG(LogTemp, Warning, TEXT("Recording finished."));
		if (data_collection_settings.shouldCollectInputStates) {
			saveInputToFile();
		}
		if (data_collection_settings.shouldCollectBoatData) {
			saveBoatDataToFile();
		}
		data_collection_settings.shouldCollectBoatData = false;
		data_collection_settings.shouldCollecteWaveTextures = false;
		data_collection_settings.shouldCollectInputStates = false;
	}
	frameNumber++;

	if (data_collection_settings.shouldCollectInputStates) {
		inputStates.Add(update_payload);
	}
	
	if (data_collection_settings.shouldCollecteWaveTextures && frameNumber % data_collection_settings.screenshotInterval == 0) {
		TArray<float> hv_rtt_r_channel_data;
		shaderModule->ComputeSerialization(eWave_hv_rtt, serialization_rtt, hv_rtt_r_channel_data);
		saveeWaveDataToFile(hv_rtt_r_channel_data); // TODO save v texture
	}

	if (data_collection_settings.shouldCollectBoatData) {
		for (int i = 0; i < boats.Num(); i++) {
			auto boat = boats[i];
			if (boat) {
				boatPositions[i].Add(boat->WorldPosition3D());
			}
		}
	}
	
	static int screenShotNr = 0;
	// screenshotInterval <= 0 means no screenshots
	if (data_collection_settings.screenshotInterval > 0 && frameNumber % data_collection_settings.screenshotInterval == 0) {
		FString fname = *FString(TEXT("SavedBoatData/Screenshots/screenshot_") + FString::FromInt(screenShotNr++) + TEXT(".png"));
		FString AbsoluteFilePath = FPaths::ProjectDir() + fname;
		FScreenshotRequest::RequestScreenshot(AbsoluteFilePath, false, false);
	}
}

void UDataCollector::saveeWaveDataToFile(TArray<float>& data) {
	FString frameNrString = FString::FromInt(frameNumber);
	if (frameNumber < 100) {
		frameNrString = TEXT("00") + frameNrString;
	}
	else if (frameNumber < 1000) {
		frameNrString = TEXT("0") + frameNrString;
	}

	FString fname = *FString(TEXT("SavedBoatData/eWaveTextures/Seed") + data_collection_settings.folderName + TEXT("/") + data_collection_settings.fileName + frameNrString + TEXT(".txt"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;
	FString textToSave = TEXT("Test\n");
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			int index = (i * 256) + j;
			FString floatString = FString::SanitizeFloat(data[index]);
			textToSave.Append(TEXT("r: ") + floatString + TEXT(", "));

		}
		textToSave.Append(TEXT(" \n"));
	}
	FFileHelper::SaveStringToFile(textToSave, *AbsoluteFilePath);
}
void UDataCollector::saveInputToFile() {
	FString fname = *FString(TEXT("SavedInputData/") + data_collection_settings.inputFileName + TEXT(".json"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;
	TSharedRef<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> states;
	for (const auto& state : inputStates) {
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		JsonObject->SetNumberField("speed", state.speed_input);
		JsonObject->SetNumberField("horizontal_1", state.velocity_input.X);
		JsonObject->SetNumberField("vertical_1", state.velocity_input.Y);
		JsonObject->SetNumberField("horizontal_2", state.velocity_input2.X);
		JsonObject->SetNumberField("vertical_2", state.velocity_input2.Y);

		FString OutputString;
		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

		states.Add(MakeShareable(new FJsonValueString(OutputString)));
	}
	JsonRootObject->SetArrayField("inputStates", states);

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonRootObject, Writer);

	FFileHelper::SaveStringToFile(OutputString, *AbsoluteFilePath);
}
static int ii = 0;
void UDataCollector::saveBoatDataToFile() {
	FString fname = *FString(TEXT("SavedBoatData/") + data_collection_settings.folderName + TEXT("/") + data_collection_settings.fileName + FString::FromInt(ii++) + TEXT(".json"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;
	TSharedRef<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
	for (int i = 0; i < boatPositions.Num(); i++) {
		TArray<TSharedPtr<FJsonValue>> boatStates;
		auto boat = boatPositions[i];
		for (const auto& pos : boat) {
			TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
			JsonObject->SetNumberField("position_x", pos.X);
			JsonObject->SetNumberField("position_y", pos.Y);
			JsonObject->SetNumberField("position_z", pos.Z);

			FString OutputString;
			TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
			FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

			boatStates.Add(MakeShareable(new FJsonValueString(OutputString)));
		}
		FString arrayName = FString::Printf(TEXT("boat_%d"), i);
		JsonRootObject->SetArrayField(arrayName, boatStates);
	}
	

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonRootObject, Writer);

	FFileHelper::SaveStringToFile(OutputString, *AbsoluteFilePath);
}

void UDataCollector::readInputJSON(TArray<UpdatePayload>& inputSequence) {
	FString fname = *FString(TEXT("SavedInputData/") + data_collection_settings.inputFileName + TEXT(".json"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;

	FString inputData;
	bool success = FFileHelper::LoadFileToString(inputData, *AbsoluteFilePath);
	if (success) {
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(inputData);
		success = FJsonSerializer::Deserialize(JsonReader, JsonParsed);
		if (success) {
			auto JsonArray = JsonParsed->GetArrayField("inputStates");
			for (const auto& v : JsonArray) {
				auto JsonStringValue = v.Get()->AsString();

				TSharedPtr<FJsonObject> JsonObjParsed;
				TSharedRef<TJsonReader<TCHAR>> JsonObjReader = TJsonReaderFactory<TCHAR>::Create(JsonStringValue);
				success = FJsonSerializer::Deserialize(JsonObjReader, JsonObjParsed);
				if (!success) {
					UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON input object"));
				}
		
				UpdatePayload state;
				state.speed_input = JsonObjParsed->GetNumberField("speed");
				state.velocity_input.X = JsonObjParsed->GetNumberField("horizontal_1");
				state.velocity_input.Y = JsonObjParsed->GetNumberField("vertical_1");
				state.velocity_input2.X = JsonObjParsed->GetNumberField("horizontal_2");
				state.velocity_input2.Y = JsonObjParsed->GetNumberField("vertical_2");
				inputSequence.Add(state);
			}
		}
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON input array object"));
	}
}

TArray<float> UDataCollector::readOrganicDistributionJSON()
{
	FString fname = *FString(TEXT("SavedInputData/Distributions/15000_emission.json"));
	FString AbsoluteFilePath = FPaths::ProjectDir() + fname;
	TArray<float> latencies;

	FString inputData;
	bool success = FFileHelper::LoadFileToString(inputData, *AbsoluteFilePath);
	if (success) {
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(inputData);
		success = FJsonSerializer::Deserialize(JsonReader, JsonParsed);
		if (success) {
			auto JsonArray_starts = JsonParsed->GetArrayField("starts");
			auto JsonArray_ends = JsonParsed->GetArrayField("ends");
			
			for (int i = 5; i < JsonArray_starts.Num() - 5; i++) {
				auto JsonStringValue_start = JsonArray_starts[i].Get()->AsString();
				auto JsonStringValue_end = JsonArray_ends[i].Get()->AsString();
				float start = FCString::Atof(*JsonStringValue_start);
				float end = FCString::Atof(*JsonStringValue_end);
				
				latencies.Add((end - start) * 1000.0f);
			}
		}
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON input array object"));
	}

	return latencies;
}
