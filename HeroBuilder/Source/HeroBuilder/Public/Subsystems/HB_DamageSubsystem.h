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

UENUM(BlueprintType)
enum ETargetType : int8
{
	Player UMETA(DisplayName = "玩家"),
    Enemy UMETA(DisplayName = "敌人"),
    Environment UMETA(DisplayName = "环境"),
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
	UFUNCTION(BlueprintCallable)
    void TakeBoxRangeDamage(AActor* Attacker, float Damage, FVector StartLocation, FVector EndLocation,float Width,ETargetType Targets);

	UFUNCTION(BlueprintCallable)
    void TakeSphereRangeDamage(AActor* Attacker, float Damage,FVector Center, float Radius,ETargetType Targets);

	virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { return TStatId(); }
};
