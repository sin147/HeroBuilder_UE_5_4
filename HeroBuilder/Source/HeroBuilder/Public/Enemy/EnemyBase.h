// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/DamageInterface.h"
#include "EnemyBase.generated.h"

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Normal UMETA(DisplayName = "普通怪物"),
	Fast UMETA(DisplayName = "快速怪物"),
	Elite UMETA(DisplayName = "精英怪物"),
	Boss UMETA(DisplayName = "BOSS怪物")
};

UCLASS(Blueprintable)
class HEROBUILDER_API AEnemyBase : public ACharacter,public IDamageInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float AttackDamage;

	// Enemy functions
    virtual void ApplyDamage(float DamageAmount, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Die();
};
