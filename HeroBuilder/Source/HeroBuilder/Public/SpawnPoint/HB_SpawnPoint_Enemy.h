// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpawnPoint/HB_SpawnPoint_Base.h"
#include "HB_SpawnPoint_Enemy.generated.h"

/**
 * 
 */
UCLASS()
class HEROBUILDER_API AHB_SpawnPoint_Enemy : public AHB_SpawnPoint_Base
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
protected:
	virtual void OnSpawn(AActor* SpawnActor) override;
};
