// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_CharacterManager.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

AHB_CharacterManager::AHB_CharacterManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true; //单例 Manager：所有客户端都需要拿到这张表
}

void AHB_CharacterManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_CharacterManager, CharacterStateArray);
}

const FCharacterStateEntry* AHB_CharacterManager::FindEntry(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return nullptr;
	}
	for (const FCharacterStateEntry& Entry : CharacterStateArray)
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
	for (FCharacterStateEntry& Entry : CharacterStateArray)
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
	for (FCharacterStateEntry& Entry : CharacterStateArray)
	{
		if (Entry.Character == InCharacter)
		{
			return Entry;
		}
	}
	FCharacterStateEntry NewEntry;
	NewEntry.Character = InCharacter;
	const int32 Idx = CharacterStateArray.Add(NewEntry);
	return CharacterStateArray[Idx];
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
	CharacterStateArray.RemoveAll([InCharacter](const FCharacterStateEntry& Entry)
	{
		return Entry.Character == InCharacter;
	});
	InternalDrivenMoveMap.Remove(InCharacter);
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
	FindOrAddEntry(InCharacter).CurrentlyState = NewState;
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
	FindOrAddEntry(InCharacter).InteractTarget = Target;
}

float AHB_CharacterManager::GetPreInteractDelay(ACharacter* InCharacter) const
{
	if (const FCharacterStateEntry* Entry = FindEntry(InCharacter))
	{
		return Entry->PreInteractDelay;
	}
	return 0.3f;
}

void AHB_CharacterManager::SetPreInteractDelay(ACharacter* InCharacter, float Delay)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FindOrAddEntry(InCharacter).PreInteractDelay = Delay;
}

float AHB_CharacterManager::GetPostInteractDelay(ACharacter* InCharacter) const
{
	if (const FCharacterStateEntry* Entry = FindEntry(InCharacter))
	{
		return Entry->PostInteractDelay;
	}
	return 0.3f;
}

void AHB_CharacterManager::SetPostInteractDelay(ACharacter* InCharacter, float Delay)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FindOrAddEntry(InCharacter).PostInteractDelay = Delay;
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
	FindOrAddEntry(InCharacter).Attack = NewAttack;
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
	FindOrAddEntry(InCharacter).InteractRange = NewRange;
}

//—— 非同步运行期数据 ——
float AHB_CharacterManager::GetCurrentInteractDelay(ACharacter* InCharacter) const
{
	if (const FCharacterStateEntry* Entry = FindEntry(InCharacter))
	{
		return Entry->CurrentInteractDelay;
	}
	return 0.f;
}

void AHB_CharacterManager::SetCurrentInteractDelay(ACharacter* InCharacter, float NewDelay)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FindOrAddEntry(InCharacter).CurrentInteractDelay = NewDelay;
}

bool AHB_CharacterManager::GetInternalDrivenMove(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return false;
	}
	if (const bool* Found = InternalDrivenMoveMap.Find(InCharacter))
	{
		return *Found;
	}
	return false;
}

void AHB_CharacterManager::SetInternalDrivenMove(ACharacter* InCharacter, bool bDriven)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	InternalDrivenMoveMap.Add(InCharacter, bDriven);
}