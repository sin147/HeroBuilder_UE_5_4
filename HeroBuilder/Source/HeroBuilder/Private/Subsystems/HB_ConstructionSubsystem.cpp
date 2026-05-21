// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Subsystems/HB_GridSubsystem.h"
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

void UHB_ConstructionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (NetMode != NM_Client)
	{
		TickPreviewBuildingPos();
	}
}

void UHB_ConstructionSubsystem::Client_SwitchBuilding(ACharacter* InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass)
{


}

void UHB_ConstructionSubsystem::Server_SwitchBuilding(ACharacter* InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass)
{
	if (IsValid(InOwnerCharacter) && IsValid(InBuildingClass))
	{
		//客户端更新玩家预览模型和建筑类
		UStaticMesh* PreBuildingMesh = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->GetBuildingPreviewMesh(InBuildingClass);
		PreBuildingMeshActorMap[InOwnerCharacter]->GetStaticMeshComponent()->SetStaticMesh(PreBuildingMesh);
		BuildingClassMap[InOwnerCharacter] = InBuildingClass;
		GridWidth = GetWorld()->GetSubsystem<UHB_GridSubsystem>()->GetGridWidth();
		GridHeight = GetWorld()->GetSubsystem<UHB_GridSubsystem>()->GetGridHeight();
	}
	else
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("ConstructionData is not valid"));
	}

}

void UHB_ConstructionSubsystem::ConstructionBegin(ACharacter* InCharacter)
{
	if (NetMode != NM_Client)
	{
		AStaticMeshActor* TargetPreStaticMeshActor = PreBuildingMeshActorMap[InCharacter];
		GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->SpawnBuilding(BuildingClassMap[InCharacter], FTransform(TargetPreStaticMeshActor->GetActorRotation(), TargetPreStaticMeshActor->GetActorLocation(), TargetPreStaticMeshActor->GetActorScale()));
	}
}

void UHB_ConstructionSubsystem::Client_ActiveConstructionMode(TObjectPtr<ACharacter>InCharacter)
{

}

void UHB_ConstructionSubsystem::Server_ActiveConstructionMode(TObjectPtr<ACharacter>InCharacter)
{
	if (IsValid(ConstructionData))
	{
		// 服务器生成 Actor 时，必须提前设置复制相关参数
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = InCharacter;
		SpawnParams.Instigator = InCharacter;
		SpawnParams.bNoFail = true;
		SpawnParams.bDeferConstruction = false;

		// 1. 先生成 Actor
        APreviewBuildingActor* NewPreStaticMeshActor = GetWorld()->SpawnActor<APreviewBuildingActor>(SpawnParams);
		UStaticMesh* PreBuildingMesh = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->GetBuildingPreviewMesh(ConstructionData->GetDefaultBuildingClass());
		if (!NewPreStaticMeshActor)
		{
			UE_LOG(LogConstructionSubsystem, Warning, TEXT("Failed to spawn PreStaticMeshActor"));
			return;

		}
		NewPreStaticMeshActor->SetMesh(PreBuildingMesh);
		PreBuildingMeshActorMap.Add(InCharacter, NewPreStaticMeshActor);
		BuildingClassMap.Add(InCharacter, ConstructionData->GetDefaultBuildingClass());
		GridWidth = GetWorld()->GetSubsystem<UHB_GridSubsystem>()->GetGridWidth();
		GridHeight = GetWorld()->GetSubsystem<UHB_GridSubsystem>()->GetGridHeight();
	}
	else
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("ConstructionData is not valid"));
    }
}

void UHB_ConstructionSubsystem::Client_CancelConstructionMode(TObjectPtr<ACharacter>InCharacter)
{

}

void UHB_ConstructionSubsystem::Server_CancelConstructionMode(TObjectPtr<ACharacter>InCharacter)
{
	if (!PreBuildingMeshActorMap.Contains(InCharacter))
	{
		return;
	}
	PreBuildingMeshActorMap[InCharacter]->Destroy();
	PreBuildingMeshActorMap.Remove(InCharacter);

}

void UHB_ConstructionSubsystem::TickPreviewBuildingPos()
{
	if (!PreBuildingMeshActorMap.IsEmpty())
	{
		for (TPair<TObjectPtr<ACharacter>, TObjectPtr<AStaticMeshActor>> PreBuildingMeshActorPair : PreBuildingMeshActorMap)
		{
			AHeroBuilderCharacter* PlayerCharacter = Cast<AHeroBuilderCharacter>(PreBuildingMeshActorPair.Key);
			AStaticMeshActor* BuildingMeshActor = PreBuildingMeshActorPair.Value;
			FVector PreviewLocation = PlayerCharacter->GetActorLocation() + PlayerCharacter->GetFollowCameraForward() * 200;
			int32 Y = FMath::Floor(PreviewLocation.Y / GridWidth);
			int32 X = FMath::Floor(PreviewLocation.X / GridWidth);
			BuildingMeshActor->SetActorLocation(FVector(GridWidth * X + GridWidth / 2, GridWidth * Y + GridWidth / 2, GridHeight));
		}
	}
}

void APreviewBuildingActor::OnRep_Mesh()
{
    ENetMode NetMode = GetWorld()->GetNetMode();
    if (NetMode == NM_Client)
    {
        GetStaticMeshComponent()->SetStaticMesh(ReplicatedMesh);
    }
}

APreviewBuildingActor::APreviewBuildingActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true); // 如果要同步位置，必须开这个
	bAlwaysRelevant = true; // 强制让所有客户端都收到这个Actor的复制
	SetActorEnableCollision(false);
	SetMobility(EComponentMobility::Movable);
}

void APreviewBuildingActor::SetMesh(UStaticMesh* InMesh)
{
	ReplicatedMesh = InMesh;
	GetStaticMeshComponent()->SetStaticMesh(ReplicatedMesh);
}

void APreviewBuildingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APreviewBuildingActor, ReplicatedMesh);
}
