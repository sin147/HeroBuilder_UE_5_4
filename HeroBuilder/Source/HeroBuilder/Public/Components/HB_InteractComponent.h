// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Config/InteractData.h"
#include "HB_InteractComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogInteractComponent, Log, All);

/**
 * 交互组件
 * 挂载在可被玩家交互的Actor上（资源/建筑/敌人等），
 * 用以统一描述"玩家靠近本Actor时，应切换到的 EInteractType"，
 * 让 HB_InteractSubsystem 不必再针对每种类型 Cast 判断。
 *
 * 使用方式：
 *   1. 在目标Actor的构造函数里 CreateDefaultSubobject 一份；
 *   2. （可选）在配置初始化时调用 SetInteractType / SetIsInteractable 覆盖；
 *   3. 由所属Actor对外提供 IsAlive 之类的语义状态（通过 SetIsInteractable）。
 */
UCLASS(ClassGroup = (HeroBuilder), meta = (BlueprintSpawnableComponent))
class HEROBUILDER_API UHB_InteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHB_InteractComponent();

	/** 玩家靠近此Actor时应切换到的交互类型（如树木→IT_Lumber、矿石→IT_Mine、敌人→IT_Attack） */
	UFUNCTION(BlueprintCallable, Category = "Interact")
	EInteractType GetInteractType() const { return InteractType; }

	UFUNCTION(BlueprintCallable, Category = "Interact")
	void SetInteractType(EInteractType NewType) { InteractType = NewType; }

	/** 当前是否可被交互（如资源/敌人死亡后置为false，不再吸引玩家切换交互类型） */
	UFUNCTION(BlueprintCallable, Category = "Interact")
	bool IsInteractable() const { return bIsInteractable; }

	UFUNCTION(BlueprintCallable, Category = "Interact")
	void SetIsInteractable(bool bInInteractable) { bIsInteractable = bInInteractable; }

	/** 综合查询：当前是否可被交互；若可，则返回的InteractType为有效值，否则返回IT_None */
	UFUNCTION(BlueprintCallable, Category = "Interact")
	EInteractType GetEffectiveInteractType() const
	{
		return bIsInteractable ? InteractType.GetValue() : IT_None;
	}

protected:
	/** 该Actor对应的交互类型（可在蓝图CDO中设置默认值，运行时也可Set覆盖） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<EInteractType> InteractType = IT_Normal;

	/** 是否可被交互（默认true；运行时由所属Actor根据自身状态如死亡设为false） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact", meta = (AllowPrivateAccess = "true"))
	bool bIsInteractable = true;
};
