// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HB_DamageSubsystem.generated.h"
DECLARE_LOG_CATEGORY_EXTERN(LogDamageSubsystem, Log, All);

struct FDamageInfo
{
	AActor* Attacker;
	float Damage;
	AActor* Target;
};

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_DamageSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

private:
    TQueue<FDamageInfo> DamageQueue;
public:
	UFUNCTION(BlueprintCallable)
    void TakeDamage(AActor* Attacker, float Damage, AActor* Target);

	virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { return TStatId(); }
};
