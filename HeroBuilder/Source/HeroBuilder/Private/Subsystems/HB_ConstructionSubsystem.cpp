// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Engine/StaticMeshActor.h"
#include "../HeroBuilderCharacter.h"
#include "Building/HB_Building_Base.h"

DEFINE_LOG_CATEGORY(LogConstructionSubsystem);

void UHB_ConstructionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
    BuildingData = LoadObject<UBuildingData>(this, TEXT("/Game/Config/DA_BuildingConfig"));

    if (!IsValid(BuildingData))
	{
		UE_LOG(LogConstructionSubsystem, Error, TEXT("Failed to load WaveData asset from /Game/Config/DA_WaveConfig"));
		UE_LOG(LogConstructionSubsystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
	}
	else
	{
		UE_LOG(LogConstructionSubsystem, Log, TEXT("Successfully loaded WaveData asset"));
	}
	NetMode = GetWorld()->GetNetMode();
}

void UHB_ConstructionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (NetMode != NM_Client)
	{
		TickSpawnBuilding();
	}
	if (NetMode == NM_Client || NetMode == NM_Standalone)
	{
		TickPreviewBuilding();
	}

}

void UHB_ConstructionSubsystem::SpawnBuilding(TSubclassOf<AHB_Building_Base> BuildingClass, FVector SpawnLocation, FRotator SpawnRotation)
{
	if (NetMode!=NM_Client&&BuildingClass)
	{
		PreSpawnQueue.Enqueue(FBuildingSpawnInfo(BuildingClass, SpawnLocation, SpawnRotation));
	}
	else
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("SpawnBuilding called on client"));
	}
}

void UHB_ConstructionSubsystem::SwitchBuilding(ACharacter* InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass)
{
	if (IsValid(BuildingData))
	{
		BuildingPreviewInfo.PreivewMesh->GetStaticMeshComponent()->SetStaticMesh(BuildingData->GetPreviewMeshByBuildingClass(InBuildingClass));
	}
	else
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("BuildingData is not valid"));
	}

}

void UHB_ConstructionSubsystem::Client_ActiveConstructionMode(ACharacter* InCharacter)
{
	if (IsValid(BuildingData))
	{
		// 客户端预览模型
		BuildingPreviewInfo.OwnerCharacter = InCharacter;
		BuildingPreviewInfo.PreivewMesh = GetWorld()->SpawnActor<AStaticMeshActor>();
		BuildingPreviewInfo.PreivewMesh->SetActorEnableCollision(false);
		BuildingPreviewInfo.PreivewMesh->SetMobility(EComponentMobility::Movable);
		BuildingPreviewInfo.PreivewMesh->GetStaticMeshComponent()->SetStaticMesh(BuildingData->GetPreviewMeshByBuildingClass(AHB_Building_Base::StaticClass()));

	}
	else
	{
		UE_LOG(LogConstructionSubsystem, Warning, TEXT("BuildingData is not valid"));
	}

}

void UHB_ConstructionSubsystem::Server_ActiveConstructionMode(ACharacter* InCharacter)
{
	//仅添加数据

}

void UHB_ConstructionSubsystem::Client_CancelConstructionMode(ACharacter* InCharacter)
{
	BuildingPreviewInfo.PreivewMesh->Destroy();
	BuildingPreviewInfo.PreivewMesh = nullptr;
}

void UHB_ConstructionSubsystem::Server_CancelConstructionMode(ACharacter* InCharacter)
{
}
void UHB_ConstructionSubsystem::TickSpawnBuilding()
{
	if (!PreSpawnQueue.IsEmpty())
	{
		int32 SpawnCount = 0;
		while (SpawnCount < SpawnNumByTick && !PreSpawnQueue.IsEmpty())
		{
			FBuildingSpawnInfo SpawnInfo;
			if (PreSpawnQueue.Dequeue(SpawnInfo))
			{
				// 安全检查：确保建筑类有效
				if (SpawnInfo.BuildingClass && SpawnInfo.BuildingClass.Get())
				{
					// 生成建筑actor
					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

					AHB_Building_Base* Building = GetWorld()->SpawnActor<AHB_Building_Base>(
						SpawnInfo.BuildingClass,
						SpawnInfo.SpawnLocation,
						SpawnInfo.SpawnRotation,
						SpawnParams
					);

					if (Building)
					{
						// 添加到建筑子系统
						if (UHB_BuildingSubsystem* BuildingSubsystem = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>())
						{
							BuildingSubsystem->PaddingBuilding(Building);
							Buildings.Add(Building);
						}
					}
					else
					{
						UE_LOG(LogConstructionSubsystem, Warning, TEXT("Failed to spawn building from ConstructionSubsystem"));
					}
				}
				else
				{
					UE_LOG(LogConstructionSubsystem, Warning, TEXT("Invalid building class in ConstructionSubsystem spawn queue"));
				}

				SpawnCount++;
			}
		}
	}
}

void UHB_ConstructionSubsystem::TickPreviewBuilding()
{
	if (IsValid(BuildingPreviewInfo.PreivewMesh))
	{
		AHeroBuilderCharacter* OwnerCharacter = Cast<AHeroBuilderCharacter>(BuildingPreviewInfo.OwnerCharacter);
		FVector PreviewLocation = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetFollowCameraForward() * 200;
		if (IsValid(OwnerCharacter))
		{
			int32 Y = FMath::Floor(PreviewLocation.Y / GridSize);
			int32 X = FMath::Floor(PreviewLocation.X / GridSize);
			BuildingPreviewInfo.PreivewMesh->SetActorLocation(FVector(GridSize * X + GridSize / 2, GridSize * Y + GridSize / 2, GridHeight));
		}
	}
		
}

FBuildingPreviewInfo::FBuildingPreviewInfo(TObjectPtr<ACharacter> InOwnerCharacter, TObjectPtr<AStaticMeshActor> InPreivewMesh, TSubclassOf<AHB_Building_Base> InBuildingClass)
	: BuildingClass(InBuildingClass), OwnerCharacter(InOwnerCharacter), PreivewMesh(InPreivewMesh)
{

}
