// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Component/HB_DamageComponent.h"
#include "HB_Building_Base.generated.h"

UENUM(BlueprintType)
enum class EBuildingState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Rotate UMETA(DisplayName = "Rotate"),
	PreAttack UMETA(DisplayName = "PreAttack"),
	Attack UMETA(DisplayName = "Attack"),
	PostAttack UMETA(DisplayName = "PostAttack"),
	Death UMETA(DisplayName = "Death")
};

UCLASS()
class HEROBUILDER_API AHB_Building_Base : public AActor
{
	GENERATED_BODY()

private:
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float Health;
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float MaxHealth;
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float Attack;
	float PreDamage;
	AActor* Target;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	TEnumAsByte<EBuildingState> CurrentState;
public:	
	// Sets default values for this actor's properties
	AHB_Building_Base();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	TObjectPtr<UHB_DamageComponent> DamageComponent;

	void OnServerApplyDamage(AActor* Attacker, float Damage);
	void OnClientApplyDamage(AActor* Attacker, float Damage);
	void RotateToTarget();

	bool bIsServer;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};
