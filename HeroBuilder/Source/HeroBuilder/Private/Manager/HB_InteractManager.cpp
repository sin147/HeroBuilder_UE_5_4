// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_InteractManager.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AHB_InteractManager::AHB_InteractManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true; //单例 Manager：所有客户端都需要拿到这张表
}

void AHB_InteractManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_InteractManager, CharacterInteractArray);
}

EInteractType AHB_InteractManager::GetCurrentInteractType(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return IT_None;
	}
	for (const FInteractEntry& Entry : CharacterInteractArray)
	{
		if (Entry.Character == InCharacter)
		{
			return Entry.InteractType;
		}
	}
	return IT_Normal;
}

void AHB_InteractManager::SetCurrentInteractType(ACharacter* InCharacter, EInteractType NewType)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	for (FInteractEntry& Entry : CharacterInteractArray)
	{
		if (Entry.Character == InCharacter)
		{
			Entry.InteractType = NewType;
			return;
		}
	}
	//未找到则新增一项
	FInteractEntry NewEntry;
	NewEntry.Character = InCharacter;
	NewEntry.InteractType = NewType;
	CharacterInteractArray.Add(NewEntry);

}

EInteractMode AHB_InteractManager::GetCurrentInteractMode(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return IM_Normal;
	}
	for (const FInteractEntry& Entry : CharacterInteractArray)
	{
		if (Entry.Character == InCharacter)
		{
			return Entry.InteractMode;
		}
	}
	return IM_Normal;
}

void AHB_InteractManager::SetCurrentInteractMode(ACharacter* InCharacter, EInteractMode NewMode)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	for (FInteractEntry& Entry : CharacterInteractArray)
	{
		if (Entry.Character == InCharacter)
		{
			Entry.InteractMode = NewMode;
			return;
		}
	}
	//未找到则新增一项
	FInteractEntry NewEntry;
	NewEntry.Character = InCharacter;
	NewEntry.InteractMode = NewMode;
	CharacterInteractArray.Add(NewEntry);
}

AActor* AHB_InteractManager::GetInteractTarget(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return nullptr;
	}
	for (const FInteractEntry& Entry : CharacterInteractArray)
	{
		if (Entry.Character == InCharacter)
		{
			return Entry.InteractTarget;
		}
	}
	return nullptr;
}

void AHB_InteractManager::SetInteractTarget(ACharacter* InCharacter, AActor* Target)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	for (FInteractEntry& Entry : CharacterInteractArray)
	{
		if (Entry.Character == InCharacter)
		{
			if (Entry.InteractTarget == Target)
			{
				return;
			}
			Entry.InteractTarget = Target;
			return;
		}
	}
	//未找到则新增一项
	FInteractEntry NewEntry;
	NewEntry.Character = InCharacter;
	NewEntry.InteractTarget = Target;
	CharacterInteractArray.Add(NewEntry);
}

void AHB_InteractManager::RemoveEntry(ACharacter* InCharacter)
{
	if (!InCharacter)
	{
		return;
	}
	CharacterInteractArray.RemoveAll([InCharacter](const FInteractEntry& Entry)
	{
		return Entry.Character == InCharacter;
	});
}

ACharacter* AHB_InteractManager::FindLocalCharacter() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->IsLocalController())
		{
			return Cast<ACharacter>(PC->GetPawn());
		}
	}
	return nullptr;
}

void AHB_InteractManager::OnRep_CharacterInteractArray()
{
	//整张数组的复制只能触发一次回调，无法精确知道"哪一项"变了。
	//此处对比上一次缓存的"自己那一项"，仅在自己的Entry发生变化时触发本地响应（如刷新UI/光标）。
	ACharacter* LocalChar = FindLocalCharacter();
	if (!LocalChar)
	{
		return;
	}
	const FInteractEntry* CurEntry = nullptr;
	for (const FInteractEntry& Entry : CharacterInteractArray)
	{
		if (Entry.Character == LocalChar)
		{
			CurEntry = &Entry;
			break;
		}
	}
	if (!CurEntry)
	{
		bHasLastLocalEntry = false;
		return;
	}

	const bool bChanged =
		!bHasLastLocalEntry ||
		LastLocalEntry.InteractType != CurEntry->InteractType ||
		LastLocalEntry.InteractMode != CurEntry->InteractMode;

	if (bChanged)
	{
		LastLocalEntry = *CurEntry;
		bHasLastLocalEntry = true;
		//占位：客户端在此响应自己交互类型/模式变更（如刷新UI/光标提示）
	}
}