// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_InteractSubsystem.h"
#include "Subsystems/HB_ConstructionSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HeroBuilder/HeroBuilderCharacter.h"
#include "Engine/OverlapResult.h"
#include "Component/HB_InteractComponent.h"

DEFINE_LOG_CATEGORY(LogInteractSubsystem);

void UHB_InteractSubsystem::TickUpdateInteractTarget(float DeltaTime)
{
	for (APlayerController* PlayerController : PlayerControllers)
	{
		if (!IsValid(PlayerController))
		{
			continue;
		}
		ACharacter* Character = Cast<ACharacter>(PlayerController->GetPawn());
		if (!IsValid(Character))
		{
			continue;
		}
		UHB_InteractComponent* InteractComp = Character->FindComponentByClass<UHB_InteractComponent>();
		if (!InteractComp)
		{
			continue;
		}

		TArray<FOverlapResult> OutResults;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);

		// 球形检测（引擎物理系统加速）
		bool bHit = GetWorld()->OverlapMultiByProfile(
			OutResults,
			Character->GetActorLocation(),
			FQuat::Identity,
			TEXT("InteractTrace"),
			FCollisionShape::MakeSphere(InteractTraceDistance)
		);

		// 找到范围内最近的物品
		AActor* NewNearest = nullptr;
		float MinDist = InteractTraceDistance;

		if (bHit)
		{
			for (auto& Res : OutResults)
			{
				AActor* Actor = Res.GetActor();
				if (!IsValid(Actor) || Actor == Character)
				{
					continue;
				}
				float Dist = FVector::Dist(Character->GetActorLocation(), Actor->GetActorLocation());
				if (Dist < MinDist)
				{
					MinDist = Dist;
					NewNearest = Actor;
				}
			}
		}

		// 更新最近交互目标
		InteractComp->SetInteractTarget(NewNearest);
	}
}

void UHB_InteractSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (NetMode != ENetMode::NM_Client)
	{
		TickUpdateInteractTarget(DeltaTime);
	}
}

void UHB_InteractSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
	InteractData = LoadObject<UInteractData>(this, TEXT("/Game/Config/DA_InteractConfig"));

	if (!InteractData)
	{
		UE_LOG(LogInteractSubsystem, Error, TEXT("Failed to load InteractData!"));
	}
}

void UHB_InteractSubsystem::OnPlayerLogin(AGameModeBase* GameMode, APlayerController* PlayerController)
{
	Super::OnPlayerLogin(GameMode, PlayerController);
	PlayerControllers.AddUnique(PlayerController);
}

void UHB_InteractSubsystem::OnPlayerLogout(AGameModeBase* GameMode, AController* Exiting)
{
	Super::OnPlayerLogout(GameMode, Exiting);
	APlayerController* PC = Cast<APlayerController>(Exiting);
	PlayerControllers.Remove(PC);
}

void UHB_InteractSubsystem::SwitchInteractMode(ACharacter* InCharacter, EPlayerCharacterInteractMode NewMode)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	UHB_InteractComponent* InteractComp = InCharacter->FindComponentByClass<UHB_InteractComponent>();
	if (!InteractComp)
	{
		return;
	}
	const EPlayerCharacterInteractMode CurrentMode = InteractComp->GetInteractMode();
	if (CurrentMode == NewMode)
	{
		return;
	}
	LeaveInteractMode(InCharacter, CurrentMode);
	EnterInteractMode(InCharacter, NewMode);
}

void UHB_InteractSubsystem::EnterInteractMode(ACharacter* InCharacter, EPlayerCharacterInteractMode EnterMode)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	UHB_InteractComponent* InteractComp = InCharacter->FindComponentByClass<UHB_InteractComponent>();
	AHeroBuilderCharacter* HeroBuilderCharacter = Cast<AHeroBuilderCharacter>(InCharacter);
	if (!InteractComp)
	{
		return;
	}
	switch (EnterMode)
	{
	case IM_None:
		break;
	case IM_Normal:
		break;
	case IM_ConstructionMode:
		if (UHB_ConstructionSubsystem* ConstructionSys = GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>())
		{
			ConstructionSys->Server_ActiveConstructionMode(InCharacter);
		}
		break;
	case IM_LumberMode:
		//TODO: 进入砍伐模式
		break;
	case IM_GatherMode:
		//TODO: 进入采集模式
		break;
	case IM_MineMode:
		//TODO: 进入挖掘模式
		break;
	case IM_AttackMode:
		//TODO: 进入攻击模式
		break;
	default:
		break;
	}
	HeroBuilderCharacter->SetPreInteractDelay(InteractData->GetPreInteractDelay(EnterMode));
	HeroBuilderCharacter->SetPostInteractDelay(InteractData->GetPostInteractDelay(EnterMode));
	InteractComp->SetInteractMode(EnterMode);
	UE_LOG(LogInteractSubsystem, Log, TEXT("'%s' Enter InteractMode '%d'!"), *GetNameSafe(InCharacter), (int32)EnterMode);
}

void UHB_InteractSubsystem::LeaveInteractMode(ACharacter* InCharacter, EPlayerCharacterInteractMode LeaveMode)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	UHB_InteractComponent* InteractComp = InCharacter->FindComponentByClass<UHB_InteractComponent>();
	if (!InteractComp)
	{
		return;
	}
	switch (LeaveMode)
	{
	case IM_None:
		break;
	case IM_Normal:
		break;
	case IM_ConstructionMode:
		if (UHB_ConstructionSubsystem* ConstructionSys = GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>())
		{
			ConstructionSys->Server_CancelConstructionMode(InCharacter);
		}
		break;
	case IM_LumberMode:
		//TODO: 离开砍伐模式
		break;
	case IM_GatherMode:
		//TODO: 离开采集模式
		break;
	case IM_MineMode:
		//TODO: 离开挖掘模式
		break;
	case IM_AttackMode:
		//TODO: 离开攻击模式
		break;
	default:
		break;
	}
	InteractComp->SetInteractMode(IM_None);
	UE_LOG(LogInteractSubsystem, Log, TEXT("'%s' Leave InteractMode '%d'!"), *GetNameSafe(InCharacter), (int32)LeaveMode);
}

EPlayerCharacterInteractMode UHB_InteractSubsystem::GetInteractMode(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return IM_None;
	}
	UHB_InteractComponent* InteractComp = InCharacter->FindComponentByClass<UHB_InteractComponent>();
	if (!InteractComp)
	{
		return IM_None;
	}
	return InteractComp->GetInteractMode();
}

void UHB_InteractSubsystem::PreInteract(ACharacter* InCharacter)
{
	AHeroBuilderCharacter* HeroBuilderCharacter = Cast<AHeroBuilderCharacter>(InCharacter);
	if (!HeroBuilderCharacter)
	{

		return;
	}
}

void UHB_InteractSubsystem::PostInteract(ACharacter* InCharacter)
{

}

void UHB_InteractSubsystem::TryInteract(ACharacter* InCharacter)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	const EPlayerCharacterInteractMode Mode = GetInteractMode(InCharacter);
	switch (Mode)
	{
	case IM_None:
		break;
	case IM_Normal:
		break;
	case IM_ConstructionMode:
		if (UHB_ConstructionSubsystem* ConstructionSys = GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>())
		{
			ConstructionSys->ConstructionBegin(InCharacter);
		}
		break;
	case IM_LumberMode:
		//TODO: 触发砍伐交互
		break;
	case IM_GatherMode:
		//TODO: 触发采集交互
		break;
	case IM_MineMode:
		//TODO: 触发挖掘交互
		break;
	case IM_AttackMode:
		//TODO: 触发攻击交互
		break;
	default:
		break;
	}
}

UAnimSequence* UHB_InteractSubsystem::GetInteractAnim(EPlayerCharacterInteractMode InteractMode) const
{
	if (!InteractData)
	{
		return nullptr;
	}
    return InteractData->GetInteractAnimation(InteractMode);
}
