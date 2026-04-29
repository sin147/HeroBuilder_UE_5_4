// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Component/HB_DamageComponent.h"
#include "HB_Building_Base.generated.h"

UENUM(BlueprintType)
enum EBuildingState : uint8
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
	AActor* Target;
	UPROPERTY(Replicated,meta=(AllowPrivateAccess=true))
	TEnumAsByte<EBuildingState> CurrentState;
	bool SwitchState(EBuildingState NewState);
	float DeathTime = 10.f;
	float PreAttackDelay = 1.f;
	float PostAttackDelay = 1.f;
	float CurrentAttackDelay = 0.f;
public:	
	// Sets default values for this actor's properties
	AHB_Building_Base();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	TObjectPtr<UHB_DamageComponent> DamageComponent;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	TObjectPtr<UStaticMeshComponent> RotateMesh;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	//当客户端应用伤害
	void OnClientApplyDamage(AActor* Attacker, float Damage);
	//服务端死亡
	void Server_Death();
	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnPreAttack();

	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnPostAttack();
	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnAttack();

	bool bIsServer;
	UPROPERTY(Replicated,BlueprintReadOnly, Category = "Attribute", meta = (AllowPrivateAccess = true))
	float RotateSpeed = 100.0f;

	UFUNCTION(BlueprintCallable)
	void StartRotate();
	UFUNCTION(BlueprintCallable)
	void StopRotate();
	UFUNCTION(BlueprintCallable)
    void StartAttack();
    UFUNCTION(BlueprintCallable)
    void StopAttack();
	UPROPERTY(EditAnywhere, Category = "Attribute")
	TSubclassOf<AActor> TargetClass;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};
