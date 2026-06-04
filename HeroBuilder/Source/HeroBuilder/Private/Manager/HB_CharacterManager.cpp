// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_CharacterManager.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

//—— FCharacterStateEntry 的 FastArray 回调（客户端在收到 增/删/改 时被调用） ——
//目前不需要在客户端做额外处理，给空实现即可；后续如果需要 UI 刷新等本地反馈再在这里补
void FCharacterStateEntry::PreReplicatedRemove(const FFastArraySerializer& /*ArraySerializer*/)
{
}

void FCharacterStateEntry::PostReplicatedAdd(const FFastArraySerializer& /*ArraySerializer*/)
{
}

void FCharacterStateEntry::PostReplicatedChange(const FFastArraySerializer& /*ArraySerializer*/)
{
}

AHB_CharacterManager::AHB_CharacterManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true; //单例 Manager：所有客户端都需要拿到这张表
}

void AHB_CharacterManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_CharacterManager, CharacterStateContainer);
}

const FCharacterStateEntry* AHB_CharacterManager::FindEntry(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return nullptr;
	}
	for (const FCharacterStateEntry& Entry : CharacterStateContainer.CharacterStateEntries)
	{
		if (Entry.Character == InCharacter)
		{
			return &Entry;
		}
	}
	return nullptr;
}

FCharacterStateEntry* AHB_CharacterManager::FindEntryMutable(ACharacter* InCharacter)
{
	if (!IsValid(InCharacter))
	{
		return nullptr;
	}
	for (FCharacterStateEntry& Entry : CharacterStateContainer.CharacterStateEntries)
	{
		if (Entry.Character == InCharacter)
		{
			return &Entry;
		}
	}
	return nullptr;
}

FCharacterStateEntry& AHB_CharacterManager::FindOrAddEntry(ACharacter* InCharacter)
{
	for (FCharacterStateEntry& Entry : CharacterStateContainer.CharacterStateEntries)
	{
		if (Entry.Character == InCharacter)
		{
			return Entry;
		}
	}
	FCharacterStateEntry NewEntry;
	NewEntry.Character = InCharacter;
    const int32 Idx = CharacterStateContainer.CharacterStateEntries.Add(NewEntry);
	FCharacterStateEntry& AddedRef = CharacterStateContainer.CharacterStateEntries[Idx];
	//FastArray新增必须手动标脏，否则OldMap不会同步、下次序列化会告警或丢包
	CharacterStateContainer.MarkItemDirty(AddedRef);
	return AddedRef;
}

void AHB_CharacterManager::RegisterCharacter(ACharacter* InCharacter)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FindOrAddEntry(InCharacter);
}

void AHB_CharacterManager::RemoveEntry(ACharacter* InCharacter)
{
	if (!InCharacter)
	{
		return;
	}
	const int32 Removed = CharacterStateContainer.CharacterStateEntries.RemoveAll([InCharacter](const FCharacterStateEntry& Entry)
	{
		return Entry.Character == InCharacter;
	});
	if (Removed > 0)
	{
		//FastArray删除必须调用MarkArrayDirty，重建内部ItemMap
		CharacterStateContainer.MarkArrayDirty();
	}
}

//—— 同步属性访问 ——
EPlayerCharacterState AHB_CharacterManager::GetCurrentlyState(ACharacter* InCharacter) const
{
	if (const FCharacterStateEntry* Entry = FindEntry(InCharacter))
	{
		return Entry->CurrentlyState;
	}
	return EPCS_Idle;
}

void AHB_CharacterManager::SetCurrentlyState(ACharacter* InCharacter, EPlayerCharacterState NewState)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FCharacterStateEntry& Entry = FindOrAddEntry(InCharacter);
	if (Entry.CurrentlyState == NewState)
	{
		return;
	}
	Entry.CurrentlyState = NewState;
	CharacterStateContainer.MarkItemDirty(Entry);
}

AActor* AHB_CharacterManager::GetInteractTarget(ACharacter* InCharacter) const
{
	if (const FCharacterStateEntry* Entry = FindEntry(InCharacter))
	{
		return Entry->InteractTarget;
	}
	return nullptr;
}

void AHB_CharacterManager::SetInteractTarget(ACharacter* InCharacter, AActor* Target)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FCharacterStateEntry& Entry = FindOrAddEntry(InCharacter);
	if (Entry.InteractTarget == Target)
	{
		return;
	}
	Entry.InteractTarget = Target;
	CharacterStateContainer.MarkItemDirty(Entry);
}

float AHB_CharacterManager::GetAttack(ACharacter* InCharacter) const
{
	if (const FCharacterStateEntry* Entry = FindEntry(InCharacter))
	{
		return Entry->Attack;
	}
	return 0.f;
}

void AHB_CharacterManager::SetAttack(ACharacter* InCharacter, float NewAttack)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FCharacterStateEntry& Entry = FindOrAddEntry(InCharacter);
	if (Entry.Attack == NewAttack)
	{
		return;
	}
	Entry.Attack = NewAttack;
	CharacterStateContainer.MarkItemDirty(Entry);
}

float AHB_CharacterManager::GetInteractRange(ACharacter* InCharacter) const
{
	if (const FCharacterStateEntry* Entry = FindEntry(InCharacter))
	{
		return Entry->InteractRange;
	}
	return 100.f;
}

void AHB_CharacterManager::SetInteractRange(ACharacter* InCharacter, float NewRange)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FCharacterStateEntry& Entry = FindOrAddEntry(InCharacter);
	if (Entry.InteractRange == NewRange)
	{
		return;
	}
	Entry.InteractRange = NewRange;
	CharacterStateContainer.MarkItemDirty(Entry);
}