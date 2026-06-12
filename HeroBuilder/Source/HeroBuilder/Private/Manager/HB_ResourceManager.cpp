// Fill out your copyright notice in the Description page of Project Settings.

#include "Manager/HB_ResourceManager.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/HB_ResourceSubsystem.h"
#include "Engine/World.h"

void FResourceAmountEntry::PostReplicatedChange(const FFastArraySerializer& ArraySerializer)
{
	//反查容器 → Manager；具体派发交给 Manager 统一处理
	const FResourceWarehouse& Container = static_cast<const FResourceWarehouse&>(ArraySerializer);
	if (AHB_ResourceManager* Mgr = Container.OwnerManager.Get())
	{
		Mgr->BroadcastResourceChange(ResourceType, Amount - LastAmount, Amount);
	}
}

void FResourceAmountEntry::PostReplicatedAdd(const FFastArraySerializer& ArraySerializer)
{
	//首次到达：Δ 视为本次到达的全量 Amount，避免被复制到来的 LastAmount 影响计算
	const FResourceWarehouse& Container = static_cast<const FResourceWarehouse&>(ArraySerializer);
	if (AHB_ResourceManager* Mgr = Container.OwnerManager.Get())
	{
		Mgr->BroadcastResourceChange(ResourceType, Amount, Amount);
	}
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

void AHB_ResourceManager::BroadcastResourceChange(EResourceType InType, int32 InDelta, int32 InNewAmount)
{
	//Manager → World → Subsystem，对外仅暴露 Subsystem 上的 BlueprintAssignable 委托
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (UHB_ResourceSubsystem* Sys = World->GetSubsystem<UHB_ResourceSubsystem>())
	{
		Sys->OnResourceChange.Broadcast(InType, InDelta, InNewAmount);
	}
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
			const int32 OldAmount = Entry.Amount;
			Entry.LastAmount = OldAmount;
			Entry.Amount = InAmount;
			ResourceWarehouse.MarkItemDirty(Entry);
			//服务端权威路径：FastArray 回调只在客户端跑，这里显式派发，保证 Listen Server 同样能收到
			BroadcastResourceChange(InType, InAmount - OldAmount, InAmount);
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
	//服务端权威路径：新增视为 Δ = InAmount
	BroadcastResourceChange(InType, InAmount, InAmount);
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
			Entry.LastAmount = Entry.Amount;
			Entry.Amount -= InAmount;
			//FastArray：扣减后标脏触发差量复制
			ResourceWarehouse.MarkItemDirty(Entry);
			//服务端权威路径：扣减 Δ 为负
			BroadcastResourceChange(InType, -InAmount, Entry.Amount);
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
