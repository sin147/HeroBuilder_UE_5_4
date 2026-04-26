// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "HB_DamageComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDamageComponent, Log, All);
DECLARE_DELEGATE_TwoParams(FOnApplyDamage_Server, AActor*/*Attacker*/, float/*Damage*/);
DECLARE_DELEGATE_TwoParams(FOnApplyDamage_Client, AActor*/*Attacker*/, float/*Damage*/);
DECLARE_DELEGATE(FOnDeath_Server);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HEROBUILDER_API UHB_DamageComponent : public UActorComponent
{
	GENERATED_BODY()
	friend class UHB_DamageSubsystem;
public:	
	// Sets default values for this component's properties
	UHB_DamageComponent();
	FOnApplyDamage_Client OnApplyDamage_Client;
	FOnDeath_Server OnDeath_Server;
	bool IsDeath();
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	//应用伤害
	UFUNCTION(BlueprintCallable)
	void ApplyDamage(AActor* Attacker, float Damage);
	UFUNCTION(BlueprintCallable)
	void TakeDamage(AActor* Target, float Damage);
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_ApplyDamage(AActor* Attacker, float Damage);
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Health = 100.f;
	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float MaxHealth = 100.f;
	float PreDamage = 0.f;
	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Attack = 10.f;
	UPROPERTY(Replicated)
	bool bIsDeath = false;
	bool bIsServer;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
