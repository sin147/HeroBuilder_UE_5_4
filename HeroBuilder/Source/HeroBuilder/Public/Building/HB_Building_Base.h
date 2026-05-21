// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Interface/HB_DamageInterface.h"
#include "HB_Building_Base.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBuilding, Log, All);

struct FBuildingConfig;

UENUM(BlueprintType)
enum EBuildingState : uint8
{
	BS_Idle UMETA(DisplayName = "Idle"),
	BS_Rotate UMETA(DisplayName = "Rotate"),
	BS_PreAttack UMETA(DisplayName = "PreAttack"),
	BS_Attack UMETA(DisplayName = "Attack"),
	BS_PostAttack UMETA(DisplayName = "PostAttack"),
	BS_Death UMETA(DisplayName = "Death"),
};

UCLASS()
class HEROBUILDER_API AHB_Building_Base : public AActor,public IHB_DamageInterface
{
	GENERATED_BODY()

private:
	UPROPERTY(Replicated)
    TObjectPtr<AActor> Target;
	UPROPERTY(Replicated)
	TEnumAsByte<EBuildingState> CurrentState;
	bool SwitchState(EBuildingState NewState);
	float Attack=10.f;
	float DeathTime = 10.f;
	float PreAttackDelay = 1.f;
	float PostAttackDelay = 1.f;
	float CurrentAttackDelay = 0.f;
	UPROPERTY(Replicated)
	float CurrentHealth = 100.f;
	float MaxHealth = 100.f;
	float CombatRange=1000;
	bool WasFindTarget = false;
	// Sets default values for this actor's properties
	AHB_Building_Base();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	TObjectPtr<UStaticMeshComponent> RotateMesh;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(EditAnywhere, Category = "Attribute")
	TObjectPtr<USceneComponent> Root;

	//当客户端应用伤害
	void OnClientApplyDamage(AActor* Attacker, float Damage);
	//服务端死亡
	void Server_Death();
	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnPreAttack(AActor* InTarget);

	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnPostAttack(AActor* InTarget);
	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnAttack(AActor* InTarget);

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
    bool IsValidTarget(AActor* InTarget) const;
	virtual void ApplyDamage(AActor* Attacker, float Damage) override;
public:	
	void InitialBuilding(FBuildingConfig InConfig);
	void SetTarget(AActor* InTarget);
	bool IsDeath();
	float GetCombatRange() const;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 命名常量
	static constexpr float MIN_ATTACK_ANGLE = 0.95f;
	static constexpr float TARGET_CHECK_INTERVAL = 0.5f;

	// 状态名称辅助函数
	FString GetStateName(EBuildingState State);
};
