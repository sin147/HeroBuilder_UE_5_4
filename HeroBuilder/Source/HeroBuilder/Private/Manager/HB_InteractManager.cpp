// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_InteractManager.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

//—— FInteractEntry 的 FastArray 回调（客户端在收到 增/删/改 时被调用） ——
//说明：UHB_InteractSubsystem 当前未提供面向交互类型/模式/目标的公开委托，
//此处保留与原 OnRep_CharacterInteractArray 一致的占位语义；后续如需对外通知，
//可在此通过反向指针 Container.OwnerManager → World → Subsystem 进行派发。
void FInteractEntry::PreReplicatedRemove(const FFastArraySerializer& /*ArraySerializer*/)
{
	//占位：本条记录被移除，可在此触发本地清理（如取消高亮、隐藏交互提示）
}

void FInteractEntry::PostReplicatedAdd(const FFastArraySerializer& /*ArraySerializer*/)
{
	//首次到达：把当前值作为本地基线缓存，避免后续Change被误判
	PreviousInteractType   = InteractType;
	PreviousInteractMode   = InteractMode;
	PreviousInteractTarget = InteractTarget;
	//占位：本条记录被新增，可在此初始化本地表现
}

void FInteractEntry::PostReplicatedChange(const FFastArraySerializer& /*ArraySerializer*/)
{
	//与 FCharacterStateEntry 一致：Item 内任意字段变化都会回调到此，需自行做差量比较
	const bool bTypeChanged   = (PreviousInteractType   != InteractType);
	const bool bModeChanged   = (PreviousInteractMode   != InteractMode);
	const bool bTargetChanged = (PreviousInteractTarget.Get() != InteractTarget);

	//刷新本地基线
	PreviousInteractType   = InteractType;
	PreviousInteractMode   = InteractMode;
	PreviousInteractTarget = InteractTarget;

	if (!bTypeChanged && !bModeChanged && !bTargetChanged)
	{
		return;
	}
	//占位：在此响应自己/他人交互类型/模式/目标的变更（如刷新UI/光标提示）
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
	Entry.InteractType = NewType;
	CharacterInteractContainer.MarkItemDirty(Entry);
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
	Entry.InteractMode = NewMode;
	CharacterInteractContainer.MarkItemDirty(Entry);
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
	Entry.InteractTarget = Target;
	CharacterInteractContainer.MarkItemDirty(Entry);
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