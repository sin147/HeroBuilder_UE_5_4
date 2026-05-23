// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Config/ResourceData.h"
#include "Interface/HB_DamageInterface.h"
#include "HB_Resource_Base.generated.h"

struct FResourceConfig;
DECLARE_LOG_CATEGORY_EXTERN(LogResource, Log, All);

UENUM(BlueprintType)
enum EResourceState : uint8
{
	RS_Idle UMETA(DisplayName = "Idle"),
	RS_BeHit UMETA(DisplayName = "BeHit"),
	RS_Recover UMETA(DisplayName = "Recover"),
	RS_Death UMETA(DisplayName = "Death"),
};

UCLASS()
class HEROBUILDER_API AHB_Resource_Base : public AActor, public IHB_DamageInterface
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float MaxHealth = 100.f;
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float CurrentHealth = 100.f;

	//受击后多少秒进入恢复状态（脱战恢复延迟）
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float RecoverDelay = 5.f;
	float CurrentRecoverDelay = 0.f;

	//每秒恢复的生命值
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float RecoverSpeed = 10.f;

	//受击硬直时间
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float BeHitDuration = 0.3f;
	float CurrentBeHitDuration = 0.f;

	//死亡保留时间
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float DeathTime = 5.f;

	//资源类型
	UPROPERTY(EditAnywhere, Category = "Attribute|Resource")
	EResourceType ResourceType = EResourceType::RT_None;

	//死亡时掉落的资源数量
	UPROPERTY(EditAnywhere, Category = "Attribute|Resource")
	int32 ResourceAmount = 10;

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TEnumAsByte<EResourceState> CurrentState;
	bool SwitchState(EResourceState NewState);
	FString GetStateName(EResourceState State);

	ENetMode NetMode;

public:
	// Sets default values for this actor's properties
	AHB_Resource_Base();
	bool IsDeath();

	//获取资源类型
	UFUNCTION(BlueprintCallable)
	EResourceType GetResourceType() const { return ResourceType; }

	//获取资源数量
	UFUNCTION(BlueprintCallable)
	int32 GetResourceAmount() const { return ResourceAmount; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//受击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnBeHit(AActor* Attacker);

	//恢复表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnRecover();

	//死亡表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnDeath();

	virtual void ApplyDamage(AActor* Attacker, float Damage) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//初始化资源数据
	void InitialResource(const FResourceConfig& InConfig);
};
