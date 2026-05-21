// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HB_WorldSubsystem_Base.h"
#include "../Config/ConstructionData.h"
#include "Engine/StaticMeshActor.h"
#include "HB_ConstructionSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConstructionSubsystem,Log,All)

class AHB_Building_Base;

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_ConstructionSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
private:
	float GridWidth=400;
	float GridHeight=100;
	TObjectPtr<UConstructionData> ConstructionData;
	TMap<TObjectPtr<ACharacter>,TObjectPtr<APreviewBuildingActor>> PreBuildingMeshActorMap;
	TMap<TObjectPtr<ACharacter>, TSubclassOf<AHB_Building_Base>> BuildingClassMap;
	ENetMode NetMode;
	void TickPreviewBuildingPos();
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
	void Client_SwitchBuilding(ACharacter* InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass);
    void Server_SwitchBuilding(ACharacter* InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass);
	void ConstructionBegin(ACharacter* InCharacter);
	void Client_ActiveConstructionMode(TObjectPtr<ACharacter>InCharacter);
	void Server_ActiveConstructionMode(TObjectPtr<ACharacter>InCharacter);
	void Client_CancelConstructionMode(TObjectPtr<ACharacter>InCharacter);
	void Server_CancelConstructionMode(TObjectPtr<ACharacter>InCharacter);
};
UCLASS()
class APreviewBuildingActor : public AStaticMeshActor
{
	GENERATED_BODY()
private:
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Mesh)
	UStaticMesh* ReplicatedMesh;
	UFUNCTION()
	void OnRep_Mesh();
public:
	APreviewBuildingActor();
	void SetMesh(UStaticMesh* InMesh);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
