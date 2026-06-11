// Fill out your copyright notice in the Description page of Project Settings.

#include "Manager/HB_ResourceManager.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/HB_ResourceSubsystem.h"
#include "Engine/World.h"

void FResourceAmountEntry::PostReplicatedChange(const FFastArraySerializer& ArraySerializer)
{
	//反查容器 → Manager → World → Subsystem
	const FResourceWarehouse& Container = static_cast<const FResourceWarehouse&>(ArraySerializer);
	AHB_ResourceManager* Mgr = Container.OwnerManager.Get();
	if (!IsValid(Mgr))
	{
		return;
	}
	UWorld* World = Mgr->GetWorld();
	if (!World)
	{
		return;
	}
	UHB_ResourceSubsystem* Sys = World->GetSubsystem<UHB_ResourceSubsystem>();
	if (!Sys)
	{
		return;
	}
	//客户端 FastArray 回调路径：直接 Broadcast 公开委托，与服务端权威路径产生一致的 Leave/Enter/Changed 通知
	Sys->OnResourceChange.Broadcast(ResourceType, Amount - LastAmount, Amount);
}

void FResourceAmountEntry::PostReplicatedAdd(const FFastArraySerializer& ArraySerializer)
{
	//反查容器 → Manager → World → Subsystem
	const FResourceWarehouse& Container = static_cast<const FResourceWarehouse&>(ArraySerializer);
	AHB_ResourceManager* Mgr = Container.OwnerManager.Get();
	if (!IsValid(Mgr))
	{
		return;
	}
	UWorld* World = Mgr->GetWorld();
	if (!World)
	{
		return;
	}
	UHB_ResourceSubsystem* Sys = World->GetSubsystem<UHB_ResourceSubsystem>();
	if (!Sys)
	{
		return;
	}
	//客户端 FastArray 回调路径：直接 Broadcast 公开委托，与服务端权威路径产生一致的 Leave/Enter/Changed 通知
	Sys->OnResourceChange.Broadcast(ResourceType, Amount - LastAmount, Amount);
}


AHB_ResourceManager::AHB_ResourceManager()
{
}

void AHB_ResourceManager::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	//为容器填入反向指针，供 FastArrayItem 回调反查 Manager→World→Subsystem 进行事件派发
	ResourceWarehouse.OwnerManager = this;
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

void AHB_ResourceManager::SetResourceAmount(EResourceType InType, int32 InAmount)
{
	if (InType == EResourceType::RT_None)
	{
		return;
	}

	for (FResourceAmountEntry& Entry : ResourceWarehouse.ResourceAmountList)
	{
		if (Entry.ResourceType == InType)
		{
			Entry.LastAmount = Entry.Amount;
			Entry.Amount = InAmount;
			ResourceWarehouse.MarkItemDirty(Entry);
			return;
		}
	}

	FResourceAmountEntry NewEntry;
	NewEntry.ResourceType = InType;
    NewEntry.LastAmount = 0;
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
