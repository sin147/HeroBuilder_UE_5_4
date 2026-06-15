// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Config/ResourceData.h"
#include "Config/InteractData.h"
#include "Components/WidgetComponent.h"
#include "Types/HB_Enums.h"
#include "HB_Resource_Base.generated.h"

class UHB_DamageComponent;
class UHB_InteractComponent;

struct FResourceConfig;
DECLARE_LOG_CATEGORY_EXTERN(LogResource, Log, All);

// 血量变化委托：旧值、新值、最大值、伤害来源
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnResourceHealthChangedDelegate, float, OldHealth, float, NewHealth, float, MaxHealth, AActor*, Attacker);

class UBoxComponent;

UCLASS(Abstract)
class HEROBUILDER_API AHB_Resource_Base : public AActor
{
	GENERATED_BODY()

public:
	// 血量变化时调用（蓝图可重写）
	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthChanged(float OldHealth, float NewHealth, float MaxHealthValue, AActor* Attacker);

private:

	//受击后多少秒进入恢复状态（脱战恢复延迟）
	UPROPERTY(EditAnywhere, Replicated, Category = "Attribute")
	float RecoverDelay = 5.f;
	float CurrentRecoverDelay = 0.f;

	//每秒恢复的生命值
	UPROPERTY(EditAnywhere, Replicated, Category = "Attribute")
	float RecoverSpeed = 10.f;

	//受击硬直时间
	UPROPERTY(EditAnywhere, Replicated, Category = "Attribute")
	float BeHitDuration = 0.3f;
	float CurrentBeHitDuration = 0.f;

	//死亡保留时间
	UPROPERTY(EditAnywhere, Replicated, Category = "Attribute")
	float DeathTime = 5.f;

	//资源类型
	UPROPERTY(EditAnywhere, Replicated, Category = "Attribute|Resource")
	TEnumAsByte<EResourceType> ResourceType = EResourceType::RT_None;

	//死亡时掉落的资源数量
	UPROPERTY(EditAnywhere, Replicated, Category = "Attribute|Resource")
	int32 ResourceAmount = 10;
private:
	UFUNCTION()
	void OnRep_CurrentlyState();

	// 记录上一次状态：服务端在 SwitchState 时先写入此字段，再写入 CurrentState；
	// 由于 RepLayout 按属性声明顺序打包，客户端 OnRep_CurrentlyState 触发时 LastState 已经收到。
	UPROPERTY(Replicated)
	TEnumAsByte<EResourceState> LastState = EResourceState::RS_Idle;

protected:

	UPROPERTY(ReplicatedUsing = OnRep_CurrentlyState, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TEnumAsByte<EResourceState> CurrentState= EResourceState::RS_Idle;
	bool SwitchState(EResourceState NewState);
	FString GetStateName(EResourceState State);

	ENetMode NetMode;

	//用于检测的碰撞盒（Profile = Resource）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	TObjectPtr<UBoxComponent> CollisionBox;

	/** 血量显示 Widget 组件，可在蓝图中指定 WidgetClass */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	/** 伤害组件：维护血量与伤害逻辑 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
	TObjectPtr<UHB_DamageComponent> DamageComponent;

	/** 交互组件：统一描述本Actor对应的玩家交互类型（吸引玩家进入哪种 EInteractType） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact")
	TObjectPtr<UHB_InteractComponent> InteractComponent;

public:
	// Sets default values for this actor's properties
	AHB_Resource_Base();
	bool IsDeath();

	//获取碰撞盒
	UFUNCTION(BlueprintCallable)
	UBoxComponent* GetCollisionBox() const { return CollisionBox; }

	//获取资源类型
	UFUNCTION(BlueprintCallable)
	EResourceType GetResourceType() const { return ResourceType; }

	//获取资源数量
	UFUNCTION(BlueprintCallable)
	int32 GetResourceAmount() const { return ResourceAmount; }

	//获取该资源关联的交互模式（转发到 InteractComponent，保留旧接口语义）
	UFUNCTION(BlueprintCallable)
	EInteractType GetInteractMode() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	//在所有组件创建/注册完成、属性首次复制之前调用：用于绑定 DamageComponent 委托，
	//避免在 BeginPlay 时机才绑定导致首包 OnRep 派发被错过；同时对蓝图子类删除/覆盖继承组件做运行时兜底。
	virtual void PostInitializeComponents() override;

	//受击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnBeHit(AActor* Attacker);

	//恢复表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnRecover();

	//死亡表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnDeath();

	void OnEnterState(EResourceState EnterState);

	void OnLeaveState(EResourceState LeaveState);

	/** 由DamageComponent委托回调：血量变化 */
	UFUNCTION()
	void HandleHealthChanged(float OldHealth, float NewHealth, float MaxHealthValue, AActor* Attacker);

	/** 由DamageComponent委托回调：死亡 */
	UFUNCTION()
	void HandleDeath();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//初始化资源数据
	void InitialResource(const FResourceConfig& InConfig);
};
