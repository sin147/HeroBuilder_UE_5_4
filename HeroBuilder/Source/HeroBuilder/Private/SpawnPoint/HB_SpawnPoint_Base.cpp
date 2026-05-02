// Fill out your copyright notice in the Description page of Project Settings.


#include "SpawnPoint/HB_SpawnPoint_Base.h"


// Sets default values
AHB_SpawnPoint_Base::AHB_SpawnPoint_Base()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AHB_SpawnPoint_Base::BeginPlay()
{
	Super::BeginPlay();
	RemainingSpawnBeforeDelay = SpawnBeforeDelay;
	RemainingSpawnAfterDelay = SpawnAfterDelay;
	NetMode = GetWorld()->GetNetMode();
}

void AHB_SpawnPoint_Base::OnSpawn(AActor* SpawnActor)
{

}

// Called every frame
void AHB_SpawnPoint_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	switch (SpawnState)
	{
	case SS_None:
		break;
	case SS_Before:
	{
		RemainingSpawnBeforeDelay -= DeltaTime;
		OnSpawnBefore();
		if (RemainingSpawnBeforeDelay <= 0)
		{
			SpawnState = SS_Spawing;
		}
		break;
	}
	case SS_Spawing:
	{
		
		OnSpawn(GetWorld()->SpawnActor<AActor>(SpawnActorClass, GetActorLocation(), GetActorRotation()));
		SpawnState = SS_After;
		break;
	}
	case SS_After:
	{
		RemainingSpawnAfterDelay -= DeltaTime;
		if (RemainingSpawnAfterDelay <= 0)
		{
			SpawnState = SS_None;
		}
		break;
	}
	default:
		break;
	}
}

bool AHB_SpawnPoint_Base::CanSpawn()
{

	return SpawnState==SS_None;
}

bool AHB_SpawnPoint_Base::Spawn(TSubclassOf<AActor> ActorClass, bool Force)
{
	if (NetMode != NM_DedicatedServer && NetMode != NM_ListenServer && NetMode != NM_Standalone)
	{
		return false;
	}
	if (ActorClass&& (SpawnState==SS_None or Force))
	{
		RemainingSpawnBeforeDelay = SpawnBeforeDelay;
		RemainingSpawnAfterDelay = SpawnAfterDelay;
		SpawnActorClass = ActorClass;
		SpawnState = SS_Before;
		return true;
	}
	return false;
}

