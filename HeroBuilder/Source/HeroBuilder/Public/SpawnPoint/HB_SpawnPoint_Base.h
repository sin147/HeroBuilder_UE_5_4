// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HB_SpawnPoint_Base.generated.h"


UENUM(BlueprintType)
enum ESpawnState:int8
{
	SS_None UMETA(DisplayName = "无"),
	SS_Before UMETA(DisplayName = "生成前"),
	SS_Spawing UMETA(DisplayName = "生成中"),
	SS_After UMETA(DisplayName = "生成后"),
};


UCLASS()
class HEROBUILDER_API AHB_SpawnPoint_Base : public AActor
{
	GENERATED_BODY()
	friend class UHB_WaveSubsystem;
private:
	TEnumAsByte<ESpawnState> SpawnState = SS_None;
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (AllowPrivateAccess = true))
	float SpawnBeforeDelay = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Spawn",meta = (AllowPrivateAccess = true))
	float SpawnAfterDelay = 0.0f;
	float RemainingSpawnBeforeDelay = 0.0f;
	float RemainingSpawnAfterDelay = 0.0f;
	ENetMode NetMode;
public:	
	// Sets default values for this actor's properties
	AHB_SpawnPoint_Base();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Spawn an actor
	bool Spawn(TSubclassOf<AActor> ActorClass,bool Force = false);
	// On Spawn Actor
    UFUNCTION(BlueprintImplementableEvent)
	void OnSpawnBefore();
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpawnAfter();
	virtual void OnSpawn(AActor* SpawnActor);
	TSubclassOf<AActor> SpawnActorClass;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool CanSpawn();

};
