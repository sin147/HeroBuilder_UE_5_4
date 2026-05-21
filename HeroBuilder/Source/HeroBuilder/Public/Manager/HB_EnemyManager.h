// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "Enemy/HB_Enemy_Base.h"
#include "HB_EnemyManager.generated.h"


/**
 * 
 */
UCLASS()
class HEROBUILDER_API AHB_EnemyManager : public AHB_Base_Manager
{
	GENERATED_BODY()
private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AHB_Enemy_Base>> Enemies;
public:

    TArray<TObjectPtr<AHB_Enemy_Base>> GetAllEnemies() const;
	void AddEnemy(AHB_Enemy_Base* Enemy);
	void RemoveEnemy(AHB_Enemy_Base* Enemy);
public:
	AHB_EnemyManager();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
