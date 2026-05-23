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

	for (FResourceAmountEntry& Entry : ResourceAmountList)
	{
		if (Entry.ResourceType == InType)
		{
			Entry.Amount += InAmount;
			return;
		}
	}

	FResourceAmountEntry NewEntry;
	NewEntry.ResourceType = InType;
	NewEntry.Amount = InAmount;
	ResourceAmountList.Add(NewEntry);
}

bool AHB_ResourceManager::ConsumeResourceAmount(EResourceType InType, int32 InAmount)
{
	if (InType == EResourceType::RT_None || InAmount <= 0)
	{
		return false;
	}

	for (FResourceAmountEntry& Entry : ResourceAmountList)
	{
		if (Entry.ResourceType == InType)
		{
			if (Entry.Amount < InAmount)
			{
				return false;
			}
			Entry.Amount -= InAmount;
			return true;
		}
	}
	return false;
}

int32 AHB_ResourceManager::GetResourceAmount(EResourceType InType) const
{
	for (const FResourceAmountEntry& Entry : ResourceAmountList)
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
	DOREPLIFETIME(AHB_ResourceManager, ResourceAmountList);
}
