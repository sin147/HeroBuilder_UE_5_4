// Fill out your copyright notice in the Description page of Project Settings.

#include "HeroBuilderGameState.h"
#include "Manager/HB_Base_Manager.h"
#include "Helper/HB_Base_Helper.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/HB_GridSubsystem.h"
#include "Subsystems/HB_EnemySubsystem.h"
#include "Subsystems/HB_ResourceSubsystem.h"
#include "Config/GridData.h"


DEFINE_LOG_CATEGORY_STATIC(LogHeroBuilderGameState, Log, All);

AHeroBuilderGameState::AHeroBuilderGameState()
{
	bReplicates = true;
}

void AHeroBuilderGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHeroBuilderGameState, ReplicatedManagers);
	// LocalHelpers 不参与网络复制（Helper 仅服务端使用）
}

void AHeroBuilderGameState::SpawnAllManagersAndHelpers()
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* CurrentClass = *It;

		// 过滤无效类：抽象类、废弃类、编辑器中间类
		if (!CurrentClass || CurrentClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}
		const FString ClassName = CurrentClass->GetName();
		if (ClassName.StartsWith(TEXT("SKEL_")) || ClassName.StartsWith(TEXT("REINST_")) || ClassName.StartsWith(TEXT("TRASHCLASS_")))
		{
			continue;
		}

		if (CurrentClass->IsChildOf(AHB_Base_Manager::StaticClass()) && CurrentClass != AHB_Base_Manager::StaticClass())
		{
			AHB_Base_Manager* Manager = World->SpawnActor<AHB_Base_Manager>(CurrentClass);
			if (IsValid(Manager))
			{
				RegisterManager(Manager);
				UE_LOG(LogHeroBuilderGameState, Log, TEXT("Create manager %s"), *Manager->GetName());
			}
		}

		if (CurrentClass->IsChildOf(AHB_Base_Helper::StaticClass()) && CurrentClass != AHB_Base_Helper::StaticClass())
		{
			AHB_Base_Helper* Helper = World->SpawnActor<AHB_Base_Helper>(CurrentClass);
			if (IsValid(Helper))
			{
				RegisterHelper(Helper);
				UE_LOG(LogHeroBuilderGameState, Log, TEXT("Create helper %s"), *Helper->GetName());
			}
		}
	}
}

void AHeroBuilderGameState::RegisterManager(AHB_Base_Manager* Manager)
{
	if (!HasAuthority() || !IsValid(Manager))
	{
		return;
	}
	ReplicatedManagers.AddUnique(Manager);
}

void AHeroBuilderGameState::RegisterHelper(AHB_Base_Helper* Helper)
{
	if (!HasAuthority() || !IsValid(Helper))
	{
		return;
	}
	LocalHelpers.AddUnique(Helper);
}

void AHeroBuilderGameState::ClearManagers()
{
	if (!HasAuthority())
	{
		return;
	}
	ReplicatedManagers.Reset();
}

void AHeroBuilderGameState::ClearHelpers()
{
	if (!HasAuthority())
	{
		return;
	}
	LocalHelpers.Reset();
}

void AHeroBuilderGameState::HandleBeginPlay()
{
	Super::HandleBeginPlay();
	
	if (!HasAuthority())
	{
		return;
	}
	
	SpawnAllManagersAndHelpers();
	
	// 根据 GridData 中配置的 Area Level 列表，依次从中心向外生成区域
	if (UWorld* World = GetWorld())
	{
		if (UHB_GridSubsystem* GridSubsystem = World->GetSubsystem<UHB_GridSubsystem>())
		{
			if (UGridData* GridData = GridSubsystem->GetGridData())
			{
				const TArray<int32> AllLevels = GridData->GetAllAreaLevels();
				for (int32 Level : AllLevels)
				{
					GridSubsystem->SpawnAreaByLevel(Level);
				}
			}
			else
			{
				UE_LOG(LogHeroBuilderGameState, Error, TEXT("HandleBeginPlay: GridData is null, skip spawning areas"));
			}
		}
		else
		{
			UE_LOG(LogHeroBuilderGameState, Error, TEXT("HandleBeginPlay: Failed to get GridSubsystem"));
		}
	}
	//生成图腾

	//生成Boss

	//生成资源
}
