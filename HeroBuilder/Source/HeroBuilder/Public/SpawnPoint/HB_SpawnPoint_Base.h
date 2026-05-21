// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HB_SpawnPoint_Base.generated.h"


UCLASS()
class HEROBUILDER_API AHB_SpawnPoint_Base : public AActor
{
	GENERATED_BODY()

public:	
	// Sets default values for this actor's properties
	AHB_SpawnPoint_Base();
	// Spawn an actor
	virtual void Spawn(TSubclassOf<AActor> ActorClass);

};
