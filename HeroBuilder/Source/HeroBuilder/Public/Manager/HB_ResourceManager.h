// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "Resource/HB_Resource_Base.h"
#include "Config/ResourceData.h"
#include "HB_ResourceManager.generated.h"

/**
 * 资源数量条目（用于网络复制）
 */
USTRUCT(BlueprintType)
struct FResourceAmountEntry
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	EResourceType ResourceType = EResourceType::RT_None;

	UPROPERTY(BlueprintReadOnly)
	int32 Amount = 0;
};

/**
 * 资源管理器：负责管理场景中的资源 Actor 列表，以及玩家所获得的资源数量
 */
UCLASS()
class HEROBUILDER_API AHB_ResourceManager : public AHB_Base_Manager
{
	GENERATED_BODY()
private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AHB_Resource_Base>> Resources;

	//每种资源类型对应的累计数量
	UPROPERTY(Replicated)
	TArray<FResourceAmountEntry> ResourceAmountList;

public:
	TArray<TObjectPtr<AHB_Resource_Base>> GetAllResources() const;
	void AddResource(AHB_Resource_Base* Resource);
	void RemoveResource(AHB_Resource_Base* Resource);

	//累加获取的资源数量
	void AddResourceAmount(EResourceType InType, int32 InAmount);
	//扣除资源数量(返回是否成功扣除)
	bool ConsumeResourceAmount(EResourceType InType, int32 InAmount);
	//获取指定类型的资源数量
	int32 GetResourceAmount(EResourceType InType) const;
	//获取所有资源数量
	const TArray<FResourceAmountEntry>& GetAllResourceAmount() const { return ResourceAmountList; }

public:
	AHB_ResourceManager();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
