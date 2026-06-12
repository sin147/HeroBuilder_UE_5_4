// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_InteractManager.h"
#include "Subsystems/HB_InteractSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

//—— FInteractEntry 的 FastArray 回调（客户端在收到 增/删/改 时被调用） ——
//说明：Item 回调内只反查 Manager，由 Manager 统一派发委托（与 ResourceManager 一致）
void FInteractEntry::PreReplicatedRemove(const FFastArraySerializer& ArraySerializer)
{
	//被移除时：分别广播 Type/Mode/Target 回到默认/空（按差量比较快照）
	const FInteractContainer& Container = static_cast<const FInteractContainer&>(ArraySerializer);
	AHB_InteractManager* Mgr = Container.OwnerManager.Get();
	if (!Mgr)
	{
		return;
	}
	if (InteractTarget)
	{
		Mgr->BroadcastInteractTargetChanged(Character.Get(), InteractTarget, nullptr);
	}
}

void FInteractEntry::PostReplicatedAdd(const FFastArraySerializer& ArraySerializer)
{
	//首次到达：把当前值作为本地基线缓存，避免后续 Change 被误判
	PreviousInteractType   = InteractType;
	PreviousInteractMode   = InteractMode;
	PreviousInteractTarget = InteractTarget;

	const FInteractContainer& Container = static_cast<const FInteractContainer&>(ArraySerializer);
	AHB_InteractManager* Mgr = Container.OwnerManager.Get();
	if (!Mgr)
	{
		return;
	}
	//首次到达：以 Old=默认 / Old=nullptr 形式派发当前值
	Mgr->BroadcastInteractTypeChanged(Character.Get(), IT_Normal, InteractType);
	Mgr->BroadcastInteractModeChanged(Character.Get(), IM_Normal, InteractMode);
	if (InteractTarget)
	{
		Mgr->BroadcastInteractTargetChanged(Character.Get(), nullptr, InteractTarget);
	}
}

void FInteractEntry::PostReplicatedChange(const FFastArraySerializer& ArraySerializer)
{
	//Item 内任意字段变化都会回调到此，需自行做差量比较
	const bool bTypeChanged   = (PreviousInteractType   != InteractType);
	const bool bModeChanged   = (PreviousInteractMode   != InteractMode);
	const bool bTargetChanged = (PreviousInteractTarget.Get() != InteractTarget);

	const TEnumAsByte<EInteractType> OldType   = PreviousInteractType;
	const TEnumAsByte<EInteractMode> OldMode   = PreviousInteractMode;
	AActor* const                    OldTarget = PreviousInteractTarget.Get();

	//刷新本地基线
	PreviousInteractType   = InteractType;
	PreviousInteractMode   = InteractMode;
	PreviousInteractTarget = InteractTarget;

	if (!bTypeChanged && !bModeChanged && !bTargetChanged)
	{
		return;
	}

	const FInteractContainer& Container = static_cast<const FInteractContainer&>(ArraySerializer);
	AHB_InteractManager* Mgr = Container.OwnerManager.Get();
	if (!Mgr)
	{
		return;
	}
	if (bTypeChanged)
	{
		Mgr->BroadcastInteractTypeChanged(Character.Get(), OldType, InteractType);
	}
	if (bModeChanged)
	{
		Mgr->BroadcastInteractModeChanged(Character.Get(), OldMode, InteractMode);
	}
	if (bTargetChanged)
	{
		Mgr->BroadcastInteractTargetChanged(Character.Get(), OldTarget, InteractTarget);
	}
}

AHB_InteractManager::AHB_InteractManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true; //单例 Manager：所有客户端都需要拿到这张表
}

void AHB_InteractManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_InteractManager, CharacterInteractContainer);
}

void AHB_InteractManager::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	//为容器填入反向指针，供 FastArrayItem 回调反查 Manager→World→Subsystem
	CharacterInteractContainer.OwnerManager = this;
}

//—— 查表辅助 ——
const FInteractEntry* AHB_InteractManager::FindEntry(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return nullptr;
	}
	for (const FInteractEntry& Entry : CharacterInteractContainer.InteractEntries)
	{
		if (Entry.Character == InCharacter)
		{
			return &Entry;
		}
	}
	return nullptr;
}

FInteractEntry* AHB_InteractManager::FindEntryMutable(ACharacter* InCharacter)
{
	if (!IsValid(InCharacter))
	{
		return nullptr;
	}
	for (FInteractEntry& Entry : CharacterInteractContainer.InteractEntries)
	{
		if (Entry.Character == InCharacter)
		{
			return &Entry;
		}
	}
	return nullptr;
}

FInteractEntry& AHB_InteractManager::FindOrAddEntry(ACharacter* InCharacter)
{
	for (FInteractEntry& Entry : CharacterInteractContainer.InteractEntries)
	{
		if (Entry.Character == InCharacter)
		{
			return Entry;
		}
	}
	FInteractEntry NewEntry;
	NewEntry.Character = InCharacter;
	const int32 Idx = CharacterInteractContainer.InteractEntries.Add(NewEntry);
	FInteractEntry& AddedRef = CharacterInteractContainer.InteractEntries[Idx];
	//FastArray新增必须手动标脏，否则OldMap不会同步、下次序列化会告警或丢包
	CharacterInteractContainer.MarkItemDirty(AddedRef);
	return AddedRef;
}

//—— 交互类型 ——
EInteractType AHB_InteractManager::GetCurrentInteractType(ACharacter* InCharacter) const
{
	if (const FInteractEntry* Entry = FindEntry(InCharacter))
	{
		return Entry->InteractType;
	}
	return IT_Normal;
}

void AHB_InteractManager::SetCurrentInteractType(ACharacter* InCharacter, EInteractType NewType)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FInteractEntry& Entry = FindOrAddEntry(InCharacter);
	if (Entry.InteractType == NewType)
	{
		return;
	}
	const EInteractType OldType = Entry.InteractType;
	Entry.InteractType = NewType;
	CharacterInteractContainer.MarkItemDirty(Entry);
	//服务端权威路径：FastArray 回调仅在客户端跑，这里显式派发，保证 Listen Server 同样能收到事件
	BroadcastInteractTypeChanged(InCharacter, OldType, NewType);
}

//—— 交互模式 ——
EInteractMode AHB_InteractManager::GetCurrentInteractMode(ACharacter* InCharacter) const
{
	if (const FInteractEntry* Entry = FindEntry(InCharacter))
	{
		return Entry->InteractMode;
	}
	return IM_Normal;
}

void AHB_InteractManager::SetCurrentInteractMode(ACharacter* InCharacter, EInteractMode NewMode)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FInteractEntry& Entry = FindOrAddEntry(InCharacter);
	if (Entry.InteractMode == NewMode)
	{
		return;
	}
	const EInteractMode OldMode = Entry.InteractMode;
	Entry.InteractMode = NewMode;
	CharacterInteractContainer.MarkItemDirty(Entry);
	BroadcastInteractModeChanged(InCharacter, OldMode, NewMode);
}

//—— 交互目标 ——
AActor* AHB_InteractManager::GetInteractTarget(ACharacter* InCharacter) const
{
	if (const FInteractEntry* Entry = FindEntry(InCharacter))
	{
		return Entry->InteractTarget;
	}
	return nullptr;
}

void AHB_InteractManager::SetInteractTarget(ACharacter* InCharacter, AActor* Target)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FInteractEntry& Entry = FindOrAddEntry(InCharacter);
	if (Entry.InteractTarget == Target)
	{
		return;
	}
	AActor* const OldTarget = Entry.InteractTarget;
	Entry.InteractTarget = Target;
	CharacterInteractContainer.MarkItemDirty(Entry);
	BroadcastInteractTargetChanged(InCharacter, OldTarget, Target);
}

void AHB_InteractManager::RemoveEntry(ACharacter* InCharacter)
{
	if (!InCharacter)
	{
		return;
	}
	const int32 Removed = CharacterInteractContainer.InteractEntries.RemoveAll([InCharacter](const FInteractEntry& Entry)
	{
		return Entry.Character == InCharacter;
	});
	if (Removed > 0)
	{
		//FastArray删除必须调用MarkArrayDirty，重建内部ItemMap
		CharacterInteractContainer.MarkArrayDirty();
	}
}

//—— 统一派发入口：Manager → World → Subsystem，仅对外暴露 Subsystem 上的 BlueprintAssignable 委托 ——
void AHB_InteractManager::BroadcastInteractTypeChanged(ACharacter* InCharacter, EInteractType OldType, EInteractType NewType)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (UHB_InteractSubsystem* Sys = World->GetSubsystem<UHB_InteractSubsystem>())
	{
		Sys->OnInteractTypeChanged.Broadcast(InCharacter, NewType, OldType);
	}
}

void AHB_InteractManager::BroadcastInteractModeChanged(ACharacter* InCharacter, EInteractMode OldMode, EInteractMode NewMode)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (UHB_InteractSubsystem* Sys = World->GetSubsystem<UHB_InteractSubsystem>())
	{
		Sys->OnInteractModeChanged.Broadcast(InCharacter, NewMode, OldMode);
	}
}

void AHB_InteractManager::BroadcastInteractTargetChanged(ACharacter* InCharacter, AActor* OldTarget, AActor* NewTarget)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (UHB_InteractSubsystem* Sys = World->GetSubsystem<UHB_InteractSubsystem>())
	{
		Sys->OnInteractTargetChanged.Broadcast(InCharacter, NewTarget, OldTarget);
	}
}