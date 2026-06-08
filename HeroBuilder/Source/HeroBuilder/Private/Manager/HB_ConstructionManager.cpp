// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_ConstructionManager.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

//—— FPreBuildingInfo 的 FastArray 回调（客户端在收到 增/删/改 时被调用） ——
//说明：UHB_ConstructionSubsystem 当前未提供面向预览体数据变化的公开委托，
//此处保留与原 TMap 直接读取一致的占位语义；后续如需对外通知，
//可在此通过反向指针 Container.OwnerManager → World → Subsystem 进行派发。
void FPreBuildingInfo::PreReplicatedRemove(const FFastArraySerializer& /*ArraySerializer*/)
{
	//占位：本条记录被移除，可在此触发本地清理（如销毁本地预览Actor、隐藏UI）
}

void FPreBuildingInfo::PostReplicatedAdd(const FFastArraySerializer& /*ArraySerializer*/)
{
	//首次到达：把当前值作为本地基线缓存，避免后续Change被误判
	PreviousBuildingClass        = BuildingClass;
	PreviousPreBuildingMeshActor = PreBuildingMeshActor;
	bPreviousIsActive            = bIsActive;
	//占位：本条记录被新增，可在此初始化本地表现
}

void FPreBuildingInfo::PostReplicatedChange(const FFastArraySerializer& /*ArraySerializer*/)
{
	//与 FInteractEntry 一致：Item 内任意字段变化都会回调到此，需自行做差量比较
	const bool bClassChanged    = (PreviousBuildingClass != BuildingClass);
	const bool bMeshActorChanged= (PreviousPreBuildingMeshActor.Get() != PreBuildingMeshActor);
	const bool bActiveChanged   = (bPreviousIsActive != bIsActive);

	//刷新本地基线
	PreviousBuildingClass        = BuildingClass;
	PreviousPreBuildingMeshActor = PreBuildingMeshActor;
	bPreviousIsActive            = bIsActive;

	if (!bClassChanged && !bMeshActorChanged && !bActiveChanged)
	{
		return;
	}
	//占位：在此响应自己/他人建造预览数据变更（如刷新预览Mesh、显隐切换）
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
	Entry.BuildingClass = NewClass;
	PreBuildingContainer.MarkItemDirty(Entry);
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
	Entry.PreBuildingMeshActor = NewActor;
	PreBuildingContainer.MarkItemDirty(Entry);
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
