// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Engine/StaticMeshActor.h"
#include "Building/HB_Building_Base.h"

DEFINE_LOG_CATEGORY(LogConstructionSubsystem);

void UHB_ConstructionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
    BuildingData = LoadObject<UBuildingData>(this, TEXT("/Game/Config/DA_BuildConfig"));

    if (!IsValid(BuildingData))
	{
		UE_LOG(LogConstructionSubsystem, Error, TEXT("Failed to load WaveData asset from /Game/Config/DA_WaveConfig"));
		UE_LOG(LogConstructionSubsystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
	}
	else
	{
		UE_LOG(LogConstructionSubsystem, Log, TEXT("Successfully loaded WaveData asset"));
	}
}

void UHB_ConstructionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickSpawnBuilding();
	switch (ConstructionState)
	{
	case EConstructionState_None:
		break;
	case EConstructionState_Preview:
		break;
	case EConstructionState_Spawn:
		break;
	default:
		break;
	}
}

void UHB_ConstructionSubsystem::SpawnBuilding(TSubclassOf<AHB_Building_Base> BuildingClass, FVector SpawnLocation, FRotator SpawnRotation)
{
	if (BuildingClass)
	{
		PreSpawnQueue.Enqueue(FBuildingSpawnInfo(BuildingClass, SpawnLocation, SpawnRotation));
	}
}

void UHB_ConstructionSubsystem::StartPreviewBuilding(ACharacter* InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass)
{
	if (InOwnerCharacter && InBuildingClass)
	{
		ConstructionState = EConstructionState_Preview;
		CreatePreviewBuilding(InOwnerCharacter, InBuildingClass);
	}
}

void UHB_ConstructionSubsystem::StopPreviewBuilding(ACharacter* InOwnerCharacter)
{
	ConstructionState = EConstructionState_None;
	for (FBuildingPreviewInfo TempBuildingPreviewInfo : PreviewBuildings)
	{
		if (TempBuildingPreviewInfo.OwnerCharacter == InOwnerCharacter)
		{
			TempBuildingPreviewInfo.PreivewMesh->Destroy();
			PreviewBuildings.Remove(TempBuildingPreviewInfo);
			break;
		}
	}
}

void UHB_ConstructionSubsystem::CreatePreviewBuilding(TObjectPtr<ACharacter> InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass)
{
	TObjectPtr<AStaticMeshActor> PreviewMesh =GetWorld()->SpawnActor<AStaticMeshActor>();
	PreviewMesh->GetStaticMeshComponent()->SetStaticMesh(BuildingData->GetPreviewMeshByBuildingClass(InBuildingClass));
	PreviewBuildings.Add(FBuildingPreviewInfo(InOwnerCharacter, PreviewMesh, InBuildingClass));
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

FBuildingPreviewInfo::FBuildingPreviewInfo(TObjectPtr<ACharacter> InOwnerCharacter, TObjectPtr<AStaticMeshActor> InPreivewMesh, TSubclassOf<AHB_Building_Base> InBuildingClass)
	: BuildingClass(InBuildingClass), OwnerCharacter(InOwnerCharacter), PreivewMesh(InPreivewMesh)
{

}
