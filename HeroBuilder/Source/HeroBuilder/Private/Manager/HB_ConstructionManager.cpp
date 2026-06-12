// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_ConstructionManager.h"
#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

//—— FPreBuildingInfo 的 FastArray 回调（客户端在收到 增/删/改 时被调用） ——
//说明：Item 回调内只反查 Manager，由 Manager 统一派发委托（与 ResourceManager 一致）
void FPreBuildingInfo::PreReplicatedRemove(const FFastArraySerializer& ArraySerializer)
{
	const FPreBuildingContainer& Container = static_cast<const FPreBuildingContainer&>(ArraySerializer);
	AHB_ConstructionManager* Mgr = Container.OwnerManager.Get();
	if (!Mgr)
	{
		return;
	}
	//被移除时：如果还拿到有效预览 Actor / Class，视为 Old=当前值, New=空
	if (PreBuildingMeshActor)
	{
		Mgr->BroadcastPreBuildingActorChanged(Character.Get(), PreBuildingMeshActor, nullptr);
	}
	if (BuildingClass)
	{
		Mgr->BroadcastPreBuildingClassChanged(Character.Get(), BuildingClass, nullptr);
	}
}

void FPreBuildingInfo::PostReplicatedAdd(const FFastArraySerializer& ArraySerializer)
{
	//首次到达：把当前值作为本地基线缓存，避免后续 Change 被误判
	PreviousBuildingClass        = BuildingClass;
	PreviousPreBuildingMeshActor = PreBuildingMeshActor;
	bPreviousIsActive            = bIsActive;

	const FPreBuildingContainer& Container = static_cast<const FPreBuildingContainer&>(ArraySerializer);
	AHB_ConstructionManager* Mgr = Container.OwnerManager.Get();
	if (!Mgr)
	{
		return;
	}
	//首次到达：以 Old=默认 / Old=nullptr 形式派发当前值
	if (BuildingClass)
	{
		Mgr->BroadcastPreBuildingClassChanged(Character.Get(), nullptr, BuildingClass);
	}
	if (PreBuildingMeshActor)
	{
		Mgr->BroadcastPreBuildingActorChanged(Character.Get(), nullptr, PreBuildingMeshActor);
	}
	Mgr->BroadcastPreBuildingActiveChanged(Character.Get(), bIsActive);
}

void FPreBuildingInfo::PostReplicatedChange(const FFastArraySerializer& ArraySerializer)
{
	//Item 内任意字段变化都会回调到此，需自行做差量比较
	const bool bClassChanged    = (PreviousBuildingClass != BuildingClass);
	const bool bMeshActorChanged= (PreviousPreBuildingMeshActor.Get() != PreBuildingMeshActor);
	const bool bActiveChanged   = (bPreviousIsActive != bIsActive);

	const TSubclassOf<AHB_Building_Base> OldClass    = PreviousBuildingClass;
	APreBuilding* const                  OldActor    = PreviousPreBuildingMeshActor.Get();

	//刷新本地基线
	PreviousBuildingClass        = BuildingClass;
	PreviousPreBuildingMeshActor = PreBuildingMeshActor;
	bPreviousIsActive            = bIsActive;

	if (!bClassChanged && !bMeshActorChanged && !bActiveChanged)
	{
		return;
	}

	const FPreBuildingContainer& Container = static_cast<const FPreBuildingContainer&>(ArraySerializer);
	AHB_ConstructionManager* Mgr = Container.OwnerManager.Get();
	if (!Mgr)
	{
		return;
	}
	if (bClassChanged)
	{
		Mgr->BroadcastPreBuildingClassChanged(Character.Get(), OldClass, BuildingClass);
	}
	if (bMeshActorChanged)
	{
		Mgr->BroadcastPreBuildingActorChanged(Character.Get(), OldActor, PreBuildingMeshActor);
	}
	if (bActiveChanged)
	{
		Mgr->BroadcastPreBuildingActiveChanged(Character.Get(), bIsActive);
	}
}

AHB_ConstructionManager::AHB_ConstructionManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true; //单例 Manager：所有客户端都需要拿到这张表
}

void AHB_ConstructionManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_ConstructionManager, PreBuildingContainer);
}

void AHB_ConstructionManager::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	//为容器填入反向指针，供 FastArrayItem 回调反查 Manager→World→Subsystem
	PreBuildingContainer.OwnerManager = this;
}

//—— 查表辅助 ——
const FPreBuildingInfo* AHB_ConstructionManager::FindEntry(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return nullptr;
	}
	for (const FPreBuildingInfo& Entry : PreBuildingContainer.PreBuildingEntries)
	{
		if (Entry.Character == InCharacter)
		{
			return &Entry;
		}
	}
	return nullptr;
}

FPreBuildingInfo* AHB_ConstructionManager::FindEntryMutable(ACharacter* InCharacter)
{
	if (!IsValid(InCharacter))
	{
		return nullptr;
	}
	for (FPreBuildingInfo& Entry : PreBuildingContainer.PreBuildingEntries)
	{
		if (Entry.Character == InCharacter)
		{
			return &Entry;
		}
	}
	return nullptr;
}

FPreBuildingInfo& AHB_ConstructionManager::FindOrAddEntry(ACharacter* InCharacter)
{
	for (FPreBuildingInfo& Entry : PreBuildingContainer.PreBuildingEntries)
	{
		if (Entry.Character == InCharacter)
		{
			return Entry;
		}
	}
	FPreBuildingInfo NewEntry;
	NewEntry.Character = InCharacter;
	const int32 Idx = PreBuildingContainer.PreBuildingEntries.Add(NewEntry);
	FPreBuildingInfo& AddedRef = PreBuildingContainer.PreBuildingEntries[Idx];
	//FastArray新增必须手动标脏，否则OldMap不会同步、下次序列化会告警或丢包
	PreBuildingContainer.MarkItemDirty(AddedRef);
	return AddedRef;
}

bool AHB_ConstructionManager::HasEntry(ACharacter* InCharacter) const
{
	return FindEntry(InCharacter) != nullptr;
}

bool AHB_ConstructionManager::AddEntry(ACharacter* InCharacter,
	TSubclassOf<AHB_Building_Base> InBuildingClass,
	APreBuilding* InPreBuildingMeshActor,
	bool bInActive)
{
	if (!IsValid(InCharacter))
	{
		return false;
	}
	//已存在则不做修改：调用方应改用 SetXxx 走差量更新路径
	if (FindEntry(InCharacter) != nullptr)
	{
		return false;
	}

	//一次性构造 Entry：在 Add 进数组之前把所有字段填齐，避免插入后多次标脏导致客户端拿到中间态
	FPreBuildingInfo NewEntry;
	NewEntry.Character            = InCharacter;
	NewEntry.BuildingClass        = InBuildingClass;
	NewEntry.PreBuildingMeshActor = InPreBuildingMeshActor;
	NewEntry.bIsActive            = bInActive;

	const int32 Idx = PreBuildingContainer.PreBuildingEntries.Add(NewEntry);
	FPreBuildingInfo& AddedRef = PreBuildingContainer.PreBuildingEntries[Idx];
	//FastArray 新增必须手动标脏，否则 OldMap 不会同步、下次序列化会告警或丢包
	PreBuildingContainer.MarkItemDirty(AddedRef);

	//与 PreBuildingMeshActor 可见性保持一致（与 SetIsActive 路径一致）
	if (AddedRef.PreBuildingMeshActor)
	{
		AddedRef.PreBuildingMeshActor->SetActorHiddenInGame(!bInActive);
	}

	//服务端权威路径：FastArray 客户端回调只在客户端跑，这里显式派发，保证 Listen Server 也能收到事件
	//发送语义与 PostReplicatedAdd 客户端首达时一致：Old=空, New=当前值
	if (InBuildingClass)
	{
		BroadcastPreBuildingClassChanged(InCharacter, nullptr, InBuildingClass);
	}
	if (InPreBuildingMeshActor)
	{
		BroadcastPreBuildingActorChanged(InCharacter, nullptr, InPreBuildingMeshActor);
	}
	BroadcastPreBuildingActiveChanged(InCharacter, bInActive);

	return true;
}

//—— 建造类 ——
TSubclassOf<AHB_Building_Base> AHB_ConstructionManager::GetBuildingClass(ACharacter* InCharacter) const
{
	if (const FPreBuildingInfo* Entry = FindEntry(InCharacter))
	{
		return Entry->BuildingClass;
	}
	return nullptr;
}

void AHB_ConstructionManager::SetBuildingClass(ACharacter* InCharacter, TSubclassOf<AHB_Building_Base> NewClass)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FPreBuildingInfo& Entry = FindOrAddEntry(InCharacter);
	if (Entry.BuildingClass == NewClass)
	{
		return;
	}
	const TSubclassOf<AHB_Building_Base> OldClass = Entry.BuildingClass;
	Entry.BuildingClass = NewClass;
	PreBuildingContainer.MarkItemDirty(Entry);
	//服务端权威路径：FastArray 回调仅在客户端跑，这里显式派发，保证 Listen Server 同样能收到事件
	BroadcastPreBuildingClassChanged(InCharacter, OldClass, NewClass);
}

//—— 预览体 Actor ——
APreBuilding* AHB_ConstructionManager::GetPreBuildingMeshActor(ACharacter* InCharacter) const
{
	if (const FPreBuildingInfo* Entry = FindEntry(InCharacter))
	{
		return Entry->PreBuildingMeshActor;
	}
	return nullptr;
}

void AHB_ConstructionManager::SetPreBuildingMeshActor(ACharacter* InCharacter, APreBuilding* NewActor)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FPreBuildingInfo& Entry = FindOrAddEntry(InCharacter);
	if (Entry.PreBuildingMeshActor == NewActor)
	{
		return;
	}
	APreBuilding* const OldActor = Entry.PreBuildingMeshActor;
	Entry.PreBuildingMeshActor = NewActor;
	PreBuildingContainer.MarkItemDirty(Entry);
	BroadcastPreBuildingActorChanged(InCharacter, OldActor, NewActor);
}

//—— 激活状态 ——
bool AHB_ConstructionManager::GetIsActive(ACharacter* InCharacter) const
{
	if (const FPreBuildingInfo* Entry = FindEntry(InCharacter))
	{
		return Entry->bIsActive;
	}
	return false;
}

void AHB_ConstructionManager::SetIsActive(ACharacter* InCharacter, bool bInActive)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FPreBuildingInfo& Entry = FindOrAddEntry(InCharacter);
	if (Entry.bIsActive == bInActive)
	{
		return;
	}
	Entry.bIsActive = bInActive;
	if (Entry.PreBuildingMeshActor)
    {
        Entry.PreBuildingMeshActor->SetActorHiddenInGame(!bInActive);
    }
	PreBuildingContainer.MarkItemDirty(Entry);
	BroadcastPreBuildingActiveChanged(InCharacter, bInActive);
}

bool AHB_ConstructionManager::GetActiveTickPos(ACharacter* InCharacter) const
{
	if (const FPreBuildingInfo* Entry = FindEntry(InCharacter))
	{
		return Entry->bActiveTickPos;
	}
	return false;
}

void AHB_ConstructionManager::SetActiveTickPos(ACharacter* InCharacter, bool bInActive)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	FPreBuildingInfo& Entry = FindOrAddEntry(InCharacter);
	if (Entry.bActiveTickPos == bInActive)
	{
		return;
	}
	Entry.bActiveTickPos = bInActive;
	PreBuildingContainer.MarkItemDirty(Entry);
}

void AHB_ConstructionManager::RemoveEntry(ACharacter* InCharacter)
{
	if (!InCharacter)
	{
		return;
	}
	const int32 Removed = PreBuildingContainer.PreBuildingEntries.RemoveAll([InCharacter](const FPreBuildingInfo& Entry)
	{
		return Entry.Character == InCharacter;
	});
	if (Removed > 0)
	{
		//FastArray删除必须调用MarkArrayDirty，重建内部ItemMap
		PreBuildingContainer.MarkArrayDirty();
	}
}

//—— 统一派发入口：Manager → World → Subsystem，仅对外暴露 Subsystem 上的 BlueprintAssignable 委托 ——
void AHB_ConstructionManager::BroadcastPreBuildingClassChanged(ACharacter* InCharacter, TSubclassOf<AHB_Building_Base> OldClass, TSubclassOf<AHB_Building_Base> NewClass)
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
	if (UHB_ConstructionSubsystem* Sys = World->GetSubsystem<UHB_ConstructionSubsystem>())
	{
		Sys->OnPreBuildingClassChanged.Broadcast(InCharacter, NewClass, OldClass);
	}
}

void AHB_ConstructionManager::BroadcastPreBuildingActorChanged(ACharacter* InCharacter, APreBuilding* OldActor, APreBuilding* NewActor)
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
	if (UHB_ConstructionSubsystem* Sys = World->GetSubsystem<UHB_ConstructionSubsystem>())
	{
		Sys->OnPreBuildingActorChanged.Broadcast(InCharacter, NewActor, OldActor);
	}
}

void AHB_ConstructionManager::BroadcastPreBuildingActiveChanged(ACharacter* InCharacter, bool bIsActive)
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
	if (UHB_ConstructionSubsystem* Sys = World->GetSubsystem<UHB_ConstructionSubsystem>())
	{
		Sys->OnPreBuildingActiveChanged.Broadcast(InCharacter, bIsActive);
	}
}

void APreBuilding::On_Rep_StateMesh()
{
	GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
}

void APreBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APreBuilding, StaticMesh);
}

APreBuilding::APreBuilding()
{
	PrimaryActorTick.bCanEverTick = true;
	SetMobility(EComponentMobility::Movable);
	SetActorEnableCollision(false);
	bReplicates = true;
	SetReplicateMovement(true);
}

void APreBuilding::SetStaticMesh(TObjectPtr<UStaticMesh> InMesh)
{
	GetStaticMeshComponent()->SetStaticMesh(InMesh);
	StaticMesh = InMesh;
}
