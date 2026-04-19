// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HB_DamageComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDamageComponent, Log, All);
DECLARE_DELEGATE_TwoParams(FOnApplyDamage_Server, AActor*/*Attacker*/, float/*Damage*/);
DECLARE_DELEGATE_TwoParams(FOnApplyDamage_Client, AActor*/*Attacker*/, float/*Damage*/);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HEROBUILDER_API UHB_DamageComponent : public UActorComponent
{
	GENERATED_BODY()
	friend class UHB_DamageSubsystem;
public:	
	// Sets default values for this component's properties
	UHB_DamageComponent();
	//造成伤害
	void TakeDamage(AActor* Target, float Damage);
	FOnApplyDamage_Server OnApplyDamage_Server;
	FOnApplyDamage_Client OnApplyDamage_Client;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	//应用伤害
	UFUNCTION(Server, Reliable)
	void Server_ApplyDamage(AActor* Attacker, float Damage);
	UFUNCTION(Server, Reliable)
	void Server_TakeDamage(AActor* Target, float Damage);
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_ApplyDamage(AActor* Attacker, float Damage);
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
