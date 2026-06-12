// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Subsystems/HB_GridSubsystem.h"
#include "Subsystems/HB_CharacterSubsystem.h"
#include "Subsystems/HB_InteractSubsystem.h"
#include "Camera/CameraComponent.h"
#include "../HeroBuilderCharacter.h"
#include "Building/HB_Building_Base.h"

DEFINE_LOG_CATEGORY(LogConstructionSubsystem);

void UHB_ConstructionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ConstructionData = LoadObject<UConstructionData>(this, TEXT("/Game/Config/DA_ConstructionConfig"));

    if (!IsValid(ConstructionData))
	{
        UE_LOG(LogConstructionSubsystem, Error, TEXT("Failed to load ConstructionData asset from /Game/Config/DA_BuildingConfig"));
		UE_LOG(LogConstructionSubsystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
	}
	else
	{
        UE_LOG(LogConstructionSubsystem, Log, TEXT("Successfully loaded ConstructionData asset"));
	}


}

void UHB_ConstructionSubsystem::PostInitialize()
{
	Super::PostInitialize();
	GridWidth = GetWorld()->GetSubsystem<UHB_GridSubsystem>()->GetGridWidth();
	GridHeight = GetWorld()->GetSubsystem<UHB_GridSubsystem>()->GetGridHeight();
	NetMode = GetWorld()->GetNetMode();
}

void UHB_ConstructionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 预览模型仅在有显示画面的端 Tick（DedicatedServer 不需要）
	TickPreviewBuildingPos();
}

void UHB_ConstructionSubsystem::SwitchBuilding(ACharacter* InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass)
{
	if (!IsValid(InOwnerCharacter) || !IsValid(InBuildingClass))
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("SwitchBuilding: invalid parameters"));
		return;
	}

	AHB_ConstructionManager* Mgr = GetConstructionManager();
	if (!Mgr)
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("SwitchBuilding: ConstructionManager not ready"));
		return;
	}
	APreBuilding* PreActor = Mgr->GetPreBuildingMeshActor(InOwnerCharacter);
	if (!IsValid(PreActor))
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("SwitchBuilding: character has not entered ConstructionMode"));
		return;
	}
	//未激活状态不允许切换建筑
	if (!Mgr->GetIsActive(InOwnerCharacter))
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("SwitchBuilding: ConstructionMode is not active for this character"));
		return;
	}

	//更新预览模型和建筑类
	UStaticMesh* PreBuildingMesh = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->GetBuildingPreviewMesh(InBuildingClass);
	PreActor->SetStaticMesh(PreBuildingMesh);
	Mgr->SetBuildingClass(InOwnerCharacter, InBuildingClass);
}

void UHB_ConstructionSubsystem::PutBuilding(ACharacter* InCharacter)
{
	//公共前置：可建造性 + 必要依赖 + Grid 索引（客户端/服务端均需要）
	if (!CheckCanConstruction(InCharacter))
	{
		return;
	}
	AHB_ConstructionManager* Mgr = GetConstructionManager();
	UHB_GridSubsystem* GridSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UHB_GridSubsystem>() : nullptr;
	if (!Mgr || !GridSubsystem)
	{
		return;
	}
	APreBuilding* PreActor = Mgr->GetPreBuildingMeshActor(InCharacter);
	if (!IsValid(PreActor))
	{
		return;
	}
	const FVector2D GridIndex = GridSubsystem->CalulateGridIndexByLocation(PreActor->GetActorLocation());
	const int32 GX = static_cast<int32>(GridIndex.X);
	const int32 GY = static_cast<int32>(GridIndex.Y);

	//客户端"乐观预占位"：本地先占位，等服务端权威 Replicate 回来时自动覆盖刷新
	if (NetMode == NM_Client)
	{
		GridSubsystem->OccupyGrid(GX, GY);
		return;
	}
	else
	{

		//服务端：走 BuildingSubsystem 的 Grid 版接口生成建筑
		TSubclassOf<AHB_Building_Base> BuildingClass = Mgr->GetBuildingClass(InCharacter);
		if (!IsValid(BuildingClass))
		{
			return;
		}
		GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->SpawnBuildingAtGrid(
			BuildingClass, GX, GY, PreActor->GetActorRotation(), PreActor->GetActorScale());
		UE_LOG(LogConstructionSubsystem, Log, TEXT("PutBuilding: spawn building"));
	}

}

void UHB_ConstructionSubsystem::SetEnablePreviewBuildingPos(ACharacter* InCharacter, bool bEnable)
{
	AHB_ConstructionManager* Mgr = GetConstructionManager();
	if (!Mgr)
	{
		return;
	}
	Mgr->SetActiveTickPos(InCharacter, bEnable);
	
}

void UHB_ConstructionSubsystem::ActiveConstructionMode(TObjectPtr<ACharacter>InCharacter)
{
	if (!IsValid(InCharacter) || !IsValid(ConstructionData))
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("ActiveConstructionMode: invalid parameters or ConstructionData"));
		return;
	}

	AHB_ConstructionManager* Mgr = GetConstructionManager();
	if (!Mgr)
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("ActiveConstructionMode: ConstructionManager not ready"));
		return;
	}
	TSubclassOf<AHB_Building_Base> DefaultBuildingClass = ConstructionData->GetDefaultBuildingClass();
	//已存在预览体：复用+重新显示，不再 Spawn
	if (Mgr->HasEntry(InCharacter))
	{
		if (Mgr->GetIsActive(InCharacter))
		{
			//重复进入，忽略
			return;
		}
	// if (!IsValid(Mgr->GetPreBuildingMeshActor(InCharacter)))
	// {
	// 	APreBuilding* NewPreStaticMeshActor = GetWorld()->SpawnActor<APreBuilding>();
	// 	if (!NewPreStaticMeshActor)
	// 	{
	// 		UE_LOG(LogConstructionSubsystem, Warning, TEXT("Failed to spawn PreStaticMeshActor"));
	// 		return;
	// 	}
	// 	UStaticMesh* PreBuildingMesh = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->GetBuildingPreviewMesh(DefaultBuildingClass);
	// 	Mgr->SetPreBuildingMeshActor(InCharacter, NewPreStaticMeshActor);
	// 	NewPreStaticMeshActor->SetStaticMesh(PreBuildingMesh);
	// }
	// //每次激活时检测当前 BuildingClass 是否有效，无效则回退到默认值并刷新预览 Mesh
	// if (!IsValid(Mgr->GetBuildingClass(InCharacter)))
	// {
	// 	Mgr->SetBuildingClass(InCharacter, DefaultBuildingClass);
	// 	UE_LOG(LogConstructionSubsystem, Log, TEXT("ActiveConstructionMode: BuildingClass invalid, fallback to default"));
	// }
		Mgr->SetIsActive(InCharacter, true);
		return;
	}

	// 首次进入：DefaultBuildingClass 必须有效，否则直接报错返回，避免向客户端复制一个 BuildingClass=nullptr 的 Entry
	if (!IsValid(DefaultBuildingClass))
	{
		UE_LOG(LogConstructionSubsystem, Error, TEXT("ActiveConstructionMode: DefaultBuildingClass is null, please check DA_ConstructionConfig"));
		return;
	}

	// 首次进入：生成预览模型
	APreBuilding* NewPreStaticMeshActor = GetWorld()->SpawnActor<APreBuilding>();
	if (!NewPreStaticMeshActor)
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("Failed to spawn PreStaticMeshActor"));
		return;
	}

	UStaticMesh* PreBuildingMesh = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->GetBuildingPreviewMesh(DefaultBuildingClass);
	NewPreStaticMeshActor->SetStaticMesh(PreBuildingMesh);

	//通过 Manager 一次性写入 Entry：原子完成 BuildingClass / PreBuildingMeshActor / bIsActive 的赋值并只标脏一次
	//避免多次 Setter 调用导致客户端先收到 BuildingClass=nullptr 的中间态
	Mgr->AddEntry(InCharacter, DefaultBuildingClass, NewPreStaticMeshActor, /*bInActive=*/true);
}

void UHB_ConstructionSubsystem::CancelConstructionMode(TObjectPtr<ACharacter>InCharacter)
{
	AHB_ConstructionManager* Mgr = GetConstructionManager();
	if (!Mgr || !Mgr->HasEntry(InCharacter))
	{
		return;
	}
	//不再 Destroy：仅隐藏+取消激活，保留 Actor 供下次复用
	if (APreBuilding* PreActor = Mgr->GetPreBuildingMeshActor(InCharacter))
	{
		PreActor->SetActorHiddenInGame(true);
	}
	Mgr->SetIsActive(InCharacter, false);
}

void UHB_ConstructionSubsystem::TickPreviewBuildingPos()
{
	AHB_ConstructionManager* Mgr = GetConstructionManager();
	if (!Mgr)
	{
		return;
	}
	const TArray<FPreBuildingInfo>& Entries = Mgr->GetAllEntries();
	if (Entries.Num() == 0)
	{
		return;
	}

	UHB_GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UHB_GridSubsystem>();
	if (!GridSubsystem)
	{
		return;
	}

	for (const FPreBuildingInfo& Entry : Entries)
	{
		//未激活状态跳过：隐藏中的预览体不需要跟随镜头更新位置
        if (!Entry.bIsActive || !Entry.bActiveTickPos)
		{
			continue;
		}
		AHeroBuilderCharacter* PlayerCharacter = Cast<AHeroBuilderCharacter>(Entry.Character.Get());
		if (!PlayerCharacter)
		{
			continue;
		}
		AStaticMeshActor* BuildingMeshActor = Entry.PreBuildingMeshActor;
		if (!IsValid(BuildingMeshActor))
		{
			continue;
		}
		UHB_CharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UHB_CharacterSubsystem>();
        UCameraComponent* FollowCamera = CharacterSubsystem->GetCharacterFollowCamera(PlayerCharacter);
        if (!FollowCamera||CharacterSubsystem->IsInteracting(PlayerCharacter))
		{
			continue;
		}
		FVector PreviewLocation = PlayerCharacter->GetActorLocation() + FollowCamera->GetForwardVector() * 200;
		FVector2D GridIndex = GridSubsystem->CalulateGridIndexByLocation(PreviewLocation);
		BuildingMeshActor->SetActorLocation(FVector(GridWidth * GridIndex.X + GridWidth / 2, GridWidth * GridIndex.Y + GridWidth / 2, 0));
	}
}

void UHB_ConstructionSubsystem::OnCharacterEnterState(ACharacter* InCharacter, EPlayerCharacterState InState)
{
	if (InState == EPlayerCharacterState::EPCS_PreInteract)
	{
		SetEnablePreviewBuildingPos(InCharacter, false);
	}
}

void UHB_ConstructionSubsystem::OnCharacterLeaveState(ACharacter* InCharacter, EPlayerCharacterState InState)
{
    if (InState == EPlayerCharacterState::EPCS_Idle||InState == EPlayerCharacterState::EPCS_Interact)
    {
        SetEnablePreviewBuildingPos(InCharacter, true);
    }
}

bool UHB_ConstructionSubsystem::CheckCanConstruction(ACharacter* InCharacter)
{
	AHB_ConstructionManager* Mgr = GetConstructionManager();
	if (!Mgr)
	{
		return false;
	}
	APreBuilding* PreActor = Mgr->GetPreBuildingMeshActor(InCharacter);
	if (!IsValid(PreActor))
	{
		return false;
	}
	//未激活状态下不允许建造
	if (!Mgr->GetIsActive(InCharacter))
	{
		return false;
	}

	UHB_GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UHB_GridSubsystem>();
	if (!GridSubsystem)
	{
		return false;
	}

	//以预览体当前位置反算 Grid 索引，查询是否已被占用（建筑/资源占用 → 不可建造）
	const FVector CheckLocation = PreActor->GetActorLocation();
	const FVector2D GridIndex = GridSubsystem->CalulateGridIndexByLocation(CheckLocation);
	if (GridSubsystem->IsGridUsed(static_cast<int32>(GridIndex.X), static_cast<int32>(GridIndex.Y)))
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("CheckCanConstruction: false"));
		return false;
	}
	UE_LOG(LogConstructionSubsystem, Log, TEXT("CheckCanConstruction: true"));
	return true;
}

AHB_ConstructionManager* UHB_ConstructionSubsystem::GetConstructionManager()
{
	return GetManager<AHB_ConstructionManager>();
}

