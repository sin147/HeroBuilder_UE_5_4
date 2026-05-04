// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Config/BuildingData.h"
#include "HB_ConstructionSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConstructionSubsystem,Log,All)

class AHB_Building_Base;

UENUM(BlueprintType)
enum EConstructionState: uint8
{
	EConstructionState_None = 0 UMETA(DisplayName = "None"),
    EConstructionState_Preview = 1 UMETA(DisplayName = "Preview"),
    EConstructionState_Spawn = 2 UMETA(DisplayName = "Spawn"),
};

USTRUCT(BlueprintType)
struct FBuildingSpawnInfo
{
	GENERATED_BODY()
	
public:
	FBuildingSpawnInfo(){}
	FBuildingSpawnInfo(TSubclassOf<AHB_Building_Base> InBuildingClass, FVector InSpawnLocation, FRotator InSpawnRotation)
		: BuildingClass(InBuildingClass), SpawnLocation(InSpawnLocation), SpawnRotation(InSpawnRotation) {}
	TSubclassOf<AHB_Building_Base> BuildingClass;
	FRotator SpawnRotation;
	FVector SpawnLocation;
};

USTRUCT(BlueprintType)
struct FBuildingPreviewInfo
{
	GENERATED_BODY()
	
public:
	FBuildingPreviewInfo(){}
	FBuildingPreviewInfo(TObjectPtr<ACharacter> InOwnerCharacter, TObjectPtr<AStaticMeshActor> InPreivewMesh, TSubclassOf<AHB_Building_Base> InBuildingClass);
	TObjectPtr<ACharacter> OwnerCharacter;
	TObjectPtr<AStaticMeshActor> PreivewMesh;
	TSubclassOf<AHB_Building_Base> BuildingClass;
	bool operator==(const FBuildingPreviewInfo& Other) const { return OwnerCharacter == Other.OwnerCharacter; }
};

USTRUCT(BlueprintType)
struct FGridInfo
{
	GENERATED_BODY()
public:
	int32 X;
    int32 Y;
	TObjectPtr<AHB_Building_Base> Building;

};

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_ConstructionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
private:
	float GridSize=400;
	float GridHeight=100;
	TArray<FGridInfo> GridInfos;
	TObjectPtr<UBuildingData> BuildingData;
    TQueue<FBuildingSpawnInfo> PreSpawnQueue;
	FBuildingPreviewInfo BuildingPreviewInfo;
	TArray<TObjectPtr<AHB_Building_Base>> Buildings;
	int32 SpawnNumByTick;
	TEnumAsByte<EConstructionState> ConstructionState = EConstructionState::EConstructionState_None;
	void TickSpawnBuilding();
	void TickPreviewBuilding();
	ENetMode NetMode;
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
    void SpawnBuilding(TSubclassOf<AHB_Building_Base> BuildingClass, FVector SpawnLocation, FRotator SpawnRotation);
	void SwitchBuilding(ACharacter* InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass);

	void Client_ActiveConstructionMode(ACharacter*InCharacter);
	void Server_ActiveConstructionMode(ACharacter*InCharacter);
	void Client_CancelConstructionMode(ACharacter*InCharacter);
	void Server_CancelConstructionMode(ACharacter*InCharacter);
};
