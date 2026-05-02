// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HB_EnemySubsystem.generated.h"

class AHB_Enemy_Base;
/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_EnemySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
private:
	TArray<TObjectPtr<AHB_Enemy_Base>> EnemyArray;
	TQueue<TObjectPtr<AHB_Enemy_Base>> NeedFindTargetEnemysQueue;
	void Find(AHB_Enemy_Base* InEnemy);
	int32 FindNumByTick = 20;
public:
	void FindAnyVaildTarget(AHB_Enemy_Base*InEnemy);
	virtual void Tick(float DeltaTime) override;
	void PaddingEnemy(AHB_Enemy_Base* NewEnemy);
	void RemoveEnemy(AHB_Enemy_Base* InEnemy);
	TArray<TObjectPtr<AHB_Enemy_Base>> GetEnemyArray();
	int32 GetEnemyNum();
	virtual TStatId GetStatId() const override { return TStatId(); }
};
