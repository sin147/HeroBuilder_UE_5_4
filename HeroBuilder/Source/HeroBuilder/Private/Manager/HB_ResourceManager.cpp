// Fill out your copyright notice in the Description page of Project Settings.

#include "Manager/HB_ResourceManager.h"
#include "Net/UnrealNetwork.h"

AHB_ResourceManager::AHB_ResourceManager()
{
}

TArray<TObjectPtr<AHB_Resource_Base>> AHB_ResourceManager::GetAllResources() const
{
	return Resources;
}

void AHB_ResourceManager::AddResource(AHB_Resource_Base* Resource)
{
	Resources.AddUnique(Resource);
}

void AHB_ResourceManager::RemoveResource(AHB_Resource_Base* Resource)
{
	if (Resources.Contains(Resource))
	{
		Resources.RemoveSingleSwap(Resource);
	}
}

void AHB_ResourceManager::AddResourceAmount(EResourceType InType, int32 InAmount)
{
	if (InType == EResourceType::RT_None || InAmount <= 0)
	{
		return;
	}

	for (FResourceAmountEntry& Entry : ResourceWarehouse.ResourceAmountList)
	{
		if (Entry.ResourceType == InType)
		{
			Entry.Amount += InAmount;
			//FastArray：修改条目后必须标脏，否则不会触发差量复制
			ResourceWarehouse.MarkItemDirty(Entry);
			return;
		}
	}

	FResourceAmountEntry NewEntry;
	NewEntry.ResourceType = InType;
	NewEntry.Amount = InAmount;
	const int32 Idx = ResourceWarehouse.ResourceAmountList.Add(NewEntry);
	//FastArray：新增条目同样需要标脏
	ResourceWarehouse.MarkItemDirty(ResourceWarehouse.ResourceAmountList[Idx]);
}

bool AHB_ResourceManager::ConsumeResourceAmount(EResourceType InType, int32 InAmount)
{
	if (InType == EResourceType::RT_None || InAmount <= 0)
	{
		return false;
	}

	for (FResourceAmountEntry& Entry : ResourceWarehouse.ResourceAmountList)
	{
		if (Entry.ResourceType == InType)
		{
			if (Entry.Amount < InAmount)
			{
				return false;
			}
			Entry.Amount -= InAmount;
			//FastArray：扣减后标脏触发差量复制
			ResourceWarehouse.MarkItemDirty(Entry);
			return true;
		}
	}
	return false;
}

int32 AHB_ResourceManager::GetResourceAmount(EResourceType InType) const
{
	for (const FResourceAmountEntry& Entry : ResourceWarehouse.ResourceAmountList)
	{
		if (Entry.ResourceType == InType)
		{
			return Entry.Amount;
		}
	}
	return 0;
}

void AHB_ResourceManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_ResourceManager, Resources);
	DOREPLIFETIME(AHB_ResourceManager, ResourceWarehouse);
}
