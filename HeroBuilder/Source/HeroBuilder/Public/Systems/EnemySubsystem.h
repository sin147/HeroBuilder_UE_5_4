// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Systems/SubsystemBase.h"
#include "DataConfig/EnemyConfigData.h"
#include "EnemySubsystem.generated.h"

UCLASS(MinimalAPI)
class AEnemyBirthPoint : public AActor
{
	GENERATED_BODY()
public:
	AEnemyBirthPoint();
protected:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UArrowComponent> ArrowComponent;
};
/**
 * 
 */
UCLASS()
class HEROBUILDER_API UEnemySubsystem : public USubsystemBase
{
	GENERATED_BODY()
    friend class AEnemyProxy;
private:
	TArray<TObjectPtr<AEnemyBirthPoint>> EnemyBirthPoints;
	TObjectPtr<UEnemyConfigData> EnemyConfigData;
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	AEnemyBase* SpawnEnemyFromTransform(TSubclassOf<AEnemyBase> EnemyClass, const FTransform& Transform);
	AEnemyBase* SpawnEnemyByEnemyBirthPoint(TSubclassOf<AEnemyBase> EnemyClass);
};
