// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Subsystems/HB_GridSubsystem.h"
#include "Subsystems/HB_CharacterSubsystem.h"
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

	FPreBuildingInfo* PreBuildingInfo = BuildingClassMap.Find(InOwnerCharacter);
	if (!PreBuildingInfo || !IsValid(PreBuildingInfo->PreBuildingMeshActor))
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("SwitchBuilding: character has not entered ConstructionMode"));
		return;
	}

	//更新预览模型和建筑类
	UStaticMesh* PreBuildingMesh = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->GetBuildingPreviewMesh(InBuildingClass);
	PreBuildingInfo->PreBuildingMeshActor->GetStaticMeshComponent()->SetStaticMesh(PreBuildingMesh);
	PreBuildingInfo->BuildingClass = InBuildingClass;

	GridWidth = GetWorld()->GetSubsystem<UHB_GridSubsystem>()->GetGridWidth();
	GridHeight = GetWorld()->GetSubsystem<UHB_GridSubsystem>()->GetGridHeight();
}

void UHB_ConstructionSubsystem::ConstructionBegin(ACharacter* InCharacter)
{
	// 建造逻辑仅在权威端执行
	if (NetMode == NM_Client)
	{
		return;
	}
	if (CheckCanConstruction(InCharacter))
	{
		FPreBuildingInfo* PreBuildingInfo = BuildingClassMap.Find(InCharacter);
		AStaticMeshActor* TargetPreStaticMeshActor = PreBuildingInfo->PreBuildingMeshActor;
		GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->SpawnBuilding(
			PreBuildingInfo->BuildingClass,
			FTransform(TargetPreStaticMeshActor->GetActorRotation(), TargetPreStaticMeshActor->GetActorLocation(), TargetPreStaticMeshActor->GetActorScale()));
		UE_LOG(LogConstructionSubsystem, Log, TEXT("ConstructionBegin: spawn building"));
	}
}

void UHB_ConstructionSubsystem::ActiveConstructionMode(TObjectPtr<ACharacter>InCharacter)
{
	if (!IsValid(InCharacter) || !IsValid(ConstructionData))
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("ActiveConstructionMode: invalid parameters or ConstructionData"));
		return;
	}

	// 同一个 Character 重复进入直接忽略，避免泄漏
	if (BuildingClassMap.Contains(InCharacter))
	{
		return;
	}

	TSubclassOf<AHB_Building_Base> DefaultBuildingClass = ConstructionData->GetDefaultBuildingClass();

	// 生成预览模型
	AStaticMeshActor* NewPreStaticMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>();
	if (!NewPreStaticMeshActor)
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("Failed to spawn PreStaticMeshActor"));
		return;
	}
	NewPreStaticMeshActor->SetMobility(EComponentMobility::Movable);

	UStaticMesh* PreBuildingMesh = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->GetBuildingPreviewMesh(DefaultBuildingClass);
	NewPreStaticMeshActor->GetStaticMeshComponent()->SetStaticMesh(PreBuildingMesh);

	FPreBuildingInfo NewInfo;
	NewInfo.BuildingClass = DefaultBuildingClass;
	NewInfo.PreBuildingMeshActor = NewPreStaticMeshActor;
	BuildingClassMap.Add(InCharacter, NewInfo);
}

void UHB_ConstructionSubsystem::CancelConstructionMode(TObjectPtr<ACharacter>InCharacter)
{
	FPreBuildingInfo* PreBuildingInfo = BuildingClassMap.Find(InCharacter);
	if (!PreBuildingInfo)
	{
		return;
	}
	if (IsValid(PreBuildingInfo->PreBuildingMeshActor))
	{
		PreBuildingInfo->PreBuildingMeshActor->Destroy();
	}
	BuildingClassMap.Remove(InCharacter);
}

void UHB_ConstructionSubsystem::TickPreviewBuildingPos()
{
	if (BuildingClassMap.IsEmpty())
	{
		return;
	}

	UHB_GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UHB_GridSubsystem>();
	if (!GridSubsystem)
	{
		return;
	}

	for (const TPair<TObjectPtr<ACharacter>, FPreBuildingInfo>& Pair : BuildingClassMap)
	{
		AHeroBuilderCharacter* PlayerCharacter = Cast<AHeroBuilderCharacter>(Pair.Key);
		if (!PlayerCharacter)
		{
			continue;
		}
		AStaticMeshActor* BuildingMeshActor = Pair.Value.PreBuildingMeshActor;
		if (!IsValid(BuildingMeshActor))
		{
			continue;
		}
		UCameraComponent* FollowCamera = GetWorld()->GetSubsystem<UHB_CharacterSubsystem>()->GetCharacterFollowCamera(PlayerCharacter);
		if (!FollowCamera)
		{
			continue;
		}
		FVector PreviewLocation = PlayerCharacter->GetActorLocation() + FollowCamera->GetForwardVector() * 200;
		FVector2D GridIndex = GridSubsystem->CalulateGridIndexByLocation(PreviewLocation);
		BuildingMeshActor->SetActorLocation(FVector(GridWidth * GridIndex.X + GridWidth / 2, GridWidth * GridIndex.Y + GridWidth / 2, 0));
	}
}

bool UHB_ConstructionSubsystem::CheckCanConstruction(ACharacter* InCharacter)
{
	AHeroBuilderCharacter* PlayerCharacter = Cast<AHeroBuilderCharacter>(InCharacter);
	FPreBuildingInfo* PreBuildingInfo = BuildingClassMap.Find(InCharacter);
	if (!PreBuildingInfo || !IsValid(PreBuildingInfo->PreBuildingMeshActor))
	{
		return false;
	}
	AStaticMeshActor* TargetPreStaticMeshActor = PreBuildingInfo->PreBuildingMeshActor;
	FVector CheckLocation = TargetPreStaticMeshActor->GetActorLocation();
	FVector2D GridIndex = GetWorld()->GetSubsystem<UHB_GridSubsystem>()->CalulateGridIndexByLocation(CheckLocation);

	FVector StartLocation((GridIndex.X + 0.5) * GridWidth, (GridIndex.Y + 0.5) * GridWidth, GridHeight * 0.5);

	// 计算盒形范围
	FVector HalfSize(GridWidth * 0.5f, GridWidth * 0.5f, GridHeight * 0.5f);

	// 使用BoxTraceMultiByProfile检测盒形范围内的所有碰撞
	FHitResult OutHit;
	TArray<AActor*> ActorsToIgnore;

	FName TraceProfile = FName("Construction");

	if (UKismetSystemLibrary::BoxTraceSingleByProfile(
		this,
		StartLocation,
		StartLocation,
		HalfSize,
		FRotator::ZeroRotator,
		TraceProfile, // 使用指定碰撞profile
		false, // bTraceComplex
		ActorsToIgnore,
		EDrawDebugTrace::None, // 不绘制调试线
		OutHit,
		true, // bIgnoreSelf
		FLinearColor::Red,
		FLinearColor::Green,
		5.0f))
	{
		return false;
	}

	return true;
}