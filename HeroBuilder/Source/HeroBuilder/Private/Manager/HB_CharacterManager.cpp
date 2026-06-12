// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_CharacterManager.h"
#include "Subsystems/HB_CharacterSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

//将 PreviousState 提升为可访问的“上次快照”：仅本地使用不参与复制，由客户端 PostReplicatedAdd/Change 维护
//（由于字段位于 USTRUCT 内部且无 UPROPERTY，UE 不会复制该字段，只走进程内赋值）

//—— FCharacterStateEntry 的 FastArray 回调（客户端在收到 增/删/改 时被调用） ——
//说明：Item 回调内只反查 Manager，由 Manager 统一派发委托（与 ResourceManager 一致）
//不再在客户端调用 Subsystem::SwitchState（那是服务端权威推进函数，会再次写表/触发副作用）
void FCharacterStateEntry::PreReplicatedRemove(const FFastArraySerializer& InArraySerializer)
{
	//被移除时视为 Leave 当前态（OldState=CurrentlyState, NewState=None）
	const FCharacterStateContainer& Container = static_cast<const FCharacterStateContainer&>(InArraySerializer);
	if (AHB_CharacterManager* Mgr = Container.OwnerManager.Get())
	{
		Mgr->BroadcastCharacterStateChanged(Character.Get(), CurrentlyState, EPCS_None);
	}
}

void FCharacterStateEntry::PostReplicatedAdd(const FFastArraySerializer& InArraySerializer)
{
	//首次到达：把当前值作为本地基线缓存，避免后续 Change 被误判
	PreviousState = CurrentlyState;
	const FCharacterStateContainer& Container = static_cast<const FCharacterStateContainer&>(InArraySerializer);
	if (AHB_CharacterManager* Mgr = Container.OwnerManager.Get())
	{
		//首次到达：OldState 传 None 表示不发 Leave
		Mgr->BroadcastCharacterStateChanged(Character.Get(), EPCS_None, CurrentlyState);
	}
}

void FCharacterStateEntry::PostReplicatedChange(const FFastArraySerializer& InArraySerializer)
{
	//仅在 CurrentlyState 发生变化时派发（其他字段变化不触发）
	const EPlayerCharacterState OldState = PreviousState;
	const EPlayerCharacterState NewState = CurrentlyState;
	if (OldState == NewState)
	{
		return;
	}
	PreviousState = NewState;
	const FCharacterStateContainer& Container = static_cast<const FCharacterStateContainer&>(InArraySerializer);
	if (AHB_CharacterManager* Mgr = Container.OwnerManager.Get())
	{
		Mgr->BroadcastCharacterStateChanged(Character.Get(), OldState, NewState);
	}
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

void AHB_CharacterManager::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	//为容器填入反向指针，供 FastArrayItem 回调反查 Manager→World→Subsystem 进行事件派发
	CharacterStateContainer.OwnerManager = this;
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

TArray<TObjectPtr<ACharacter>> AHB_CharacterManager::GetAllCharacters() const
{
    TArray<TObjectPtr<ACharacter>> Characters;
    //预留避免多次realloc
    Characters.Reserve(CharacterStateContainer.CharacterStateEntries.Num());
    for (const FCharacterStateEntry& Entry : CharacterStateContainer.CharacterStateEntries)
	{
		if (IsValid(Entry.Character))
		{
			Characters.Add(Entry.Character);
		}
	}

    return Characters;
}

void AHB_CharacterManager::BroadcastCharacterStateChanged(ACharacter* InCharacter, EPlayerCharacterState OldState, EPlayerCharacterState NewState)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	//Manager → World → Subsystem，对外仅暴露 Subsystem 上的 BlueprintAssignable 委托
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UHB_CharacterSubsystem* Sys = World->GetSubsystem<UHB_CharacterSubsystem>();
	if (!Sys)
	{
		return;
	}
	//与服务端权威路径（SwitchState）保持一致的 Leave/Enter/Changed 三段式
	if (OldState != EPCS_None)
	{
		Sys->OnCharacterLeaveState.Broadcast(InCharacter, OldState);
	}
	if (NewState != EPCS_None)
	{
		Sys->OnCharacterEnterState.Broadcast(InCharacter, NewState);
	}
	Sys->OnCharacterStateChanged.Broadcast(InCharacter, NewState, OldState);
}
