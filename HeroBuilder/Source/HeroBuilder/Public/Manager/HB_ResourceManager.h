// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Resource/HB_Resource_Base.h"
#include "Config/ResourceData.h"
#include "HB_ResourceManager.generated.h"

class AHB_ResourceManager;

/**
 * 资源数量条目（用于网络复制）
 */
USTRUCT()
struct FResourceAmountEntry:public FFastArraySerializerItem
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TEnumAsByte<EResourceType> ResourceType = EResourceType::RT_None;

	UPROPERTY()
	int32 Amount = 0;
	UPROPERTY()
	int32 LastAmount = 0;
    void PostReplicatedAdd(const FFastArraySerializer& ArraySerializer);
	void PostReplicatedChange(const FFastArraySerializer& ArraySerializer);
};
USTRUCT()
struct FResourceWarehouse:public FFastArraySerializer
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<FResourceAmountEntry> ResourceAmountList;
	//反向指针：仅本地使用，不参与复制；供FastArrayItem回调反查Manager→World→Subsystem
	TWeakObjectPtr<AHB_ResourceManager> OwnerManager;
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaInfo) 
	{ 
		return FFastArraySerializer::FastArrayDeltaSerialize<FResourceAmountEntry,FResourceWarehouse>(ResourceAmountList, DeltaInfo, *this);
	}
};
//关键：告诉 UE 这个结构体走 NetDeltaSerializer
template<>
struct TStructOpsTypeTraits<FResourceWarehouse> : public TStructOpsTypeTraitsBase2<FResourceWarehouse>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
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

	//每种资源类型对应的累计数量（FastArray 差量复制）
	UPROPERTY(Replicated)
	FResourceWarehouse ResourceWarehouse;

public:
	TArray<TObjectPtr<AHB_Resource_Base>> GetAllResources() const;
	void AddResource(AHB_Resource_Base* Resource);
	void RemoveResource(AHB_Resource_Base* Resource);

	//累加获取的资源数量
	void SetResourceAmount(EResourceType InType, int32 InAmount);
	//扣除资源数量(返回是否成功扣除)
	bool ConsumeResourceAmount(EResourceType InType, int32 InAmount);
	//获取指定类型的资源数量
	int32 GetResourceAmount(EResourceType InType) const;
	//获取所有资源数量
	const TArray<FResourceAmountEntry>& GetAllResourceAmount() const { return ResourceWarehouse.ResourceAmountList; }

public:
	AHB_ResourceManager();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
};
