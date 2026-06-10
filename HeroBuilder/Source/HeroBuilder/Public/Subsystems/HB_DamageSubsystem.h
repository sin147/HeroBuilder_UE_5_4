// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HB_WorldSubsystem_Base.h"
#include "Types/HB_Enums.h"
#include "HB_DamageSubsystem.generated.h"
DECLARE_LOG_CATEGORY_EXTERN(LogDamageSubsystem, Log, All);

USTRUCT()
struct FDamageInfo
{
	GENERATED_BODY()

	// 用 UPROPERTY + 反射容器(TArray)持有，GC 会将这里的引用视为强引用，
	// 保证已入队的伤害一定能消费到（不会因为 Attacker/Target 中途被 GC 而沉默丢失）。
	UPROPERTY()
	TObjectPtr<AActor> Attacker = nullptr;

	UPROPERTY()
	float Damage = 0.f;

	UPROPERTY()
	TObjectPtr<AActor> Target = nullptr;

	FDamageInfo() = default;
	FDamageInfo(AActor* InAttacker, float InDamage, AActor* InTarget)
		: Attacker(InAttacker), Damage(InDamage), Target(InTarget) {}
};

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_DamageSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TArray<FDamageInfo> DamageQueue;
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
