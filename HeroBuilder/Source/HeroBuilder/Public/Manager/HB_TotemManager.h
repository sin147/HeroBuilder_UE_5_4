// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "HB_TotemManager.generated.h"

class AHB_Totem_Base;

/**
 * 图腾管理器
 * 负责维护场景中所有图腾实例的列表，并参与网络复制。
 * 具体波次逻辑由 TotemSubsystem 驱动，Manager 只承担数据维护 + 遍历职责。
 */
UCLASS()
class HEROBUILDER_API AHB_TotemManager : public AHB_Base_Manager
{
	GENERATED_BODY()

private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AHB_Totem_Base>> Totems;

public:
	TArray<TObjectPtr<AHB_Totem_Base>> GetAllTotems() const;
	TArray<TObjectPtr<AHB_Totem_Base>> GetAllActiveTotems() const;
	void AddTotem(AHB_Totem_Base* InTotem);
	void RemoveTotem(AHB_Totem_Base* InTotem);

public:
	AHB_TotemManager();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
