// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_InteractSubsystem.h"
#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_DamageSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HeroBuilder/HeroBuilderCharacter.h"
#include "Resource/HB_Resource_Base.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Engine/OverlapResult.h"

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
		AHeroBuilderCharacter* HBCharacter = Cast<AHeroBuilderCharacter>(Character);
		if (!HBCharacter)
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
		HBCharacter->SetInteractTarget(NewNearest);

		// 根据目标类型同步交互模式：仅在“被动模式”下自动切换，
		// 保留玩家主动选择的模式（如 IM_ConstructionMode）不被覆盖。
		const EPlayerCharacterInteractMode CurrentMode = HBCharacter->GetInteractMode();
		const bool bIsPassiveMode =
			(CurrentMode == IM_None) ||
			(CurrentMode == IM_Normal) ||
			(CurrentMode == IM_LumberMode) ||
			(CurrentMode == IM_GatherMode) ||
			(CurrentMode == IM_MineMode) ||
			(CurrentMode == IM_AttackMode);

		if (bIsPassiveMode)
		{
			EPlayerCharacterInteractMode DesiredMode = IM_Normal;
			if (AHB_Resource_Base* Resource = Cast<AHB_Resource_Base>(NewNearest))
			{
				if (!Resource->IsDeath())
				{
					DesiredMode = Resource->GetInteractMode();
				}
			}
			else if (AHB_Enemy_Base* Enemy = Cast<AHB_Enemy_Base>(NewNearest))
			{
				if (!Enemy->IsDeath())
				{
					DesiredMode = IM_AttackMode;
				}
			}

			if (DesiredMode != CurrentMode)
			{
				SwitchInteractMode(HBCharacter, DesiredMode);
			}
		}
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
	AHeroBuilderCharacter* HBCharacter = Cast<AHeroBuilderCharacter>(InCharacter);
	if (!HBCharacter)
	{
		return;
	}
	const EPlayerCharacterInteractMode CurrentMode = HBCharacter->GetInteractMode();
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
	AHeroBuilderCharacter* HeroBuilderCharacter = Cast<AHeroBuilderCharacter>(InCharacter);
	if (!HeroBuilderCharacter)
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
	HeroBuilderCharacter->SetInteractMode(EnterMode);
	const FString ModeName = StaticEnum<EPlayerCharacterInteractMode>()->GetNameStringByValue((int64)EnterMode);
	UE_LOG(LogInteractSubsystem, Log, TEXT("'%s' Enter InteractMode '%s'!"), *GetNameSafe(InCharacter), *ModeName);
}

void UHB_InteractSubsystem::LeaveInteractMode(ACharacter* InCharacter, EPlayerCharacterInteractMode LeaveMode)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	AHeroBuilderCharacter* HBCharacter = Cast<AHeroBuilderCharacter>(InCharacter);
	if (!HBCharacter)
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
	HBCharacter->SetInteractMode(IM_None);
	const FString ModeName = StaticEnum<EPlayerCharacterInteractMode>()->GetNameStringByValue((int64)LeaveMode);
	UE_LOG(LogInteractSubsystem, Log, TEXT("'%s' Leave InteractMode '%s'!"), *GetNameSafe(InCharacter), *ModeName);
}

EPlayerCharacterInteractMode UHB_InteractSubsystem::GetInteractMode(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return IM_None;
	}
	AHeroBuilderCharacter* HBCharacter = Cast<AHeroBuilderCharacter>(InCharacter);
	if (!HBCharacter)
	{
		return IM_None;
	}
	return HBCharacter->GetInteractMode();
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

	// 统一的“对当前交互目标造成伤害”辅助函数
	auto ApplyInteractDamage = [this, InCharacter]()
	{
		AHeroBuilderCharacter* HBCharacter = Cast<AHeroBuilderCharacter>(InCharacter);
		if (!HBCharacter)
		{
			return;
		}
		AActor* Target = HBCharacter->GetInteractTarget();
		if (!IsValid(Target))
		{
			return;
		}
		// 伤害值来自角色自身的攻击力（Attack）属性
		const float Damage = HBCharacter->GetAttack();
		if (Damage <= 0.f)
		{
			return;
		}
		if (UHB_DamageSubsystem* DamageSys = GetWorld()->GetSubsystem<UHB_DamageSubsystem>())
		{
			DamageSys->TakeDamage(InCharacter, Damage, Target);
		}
	};

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
	case IM_GatherMode:
	case IM_MineMode:
	case IM_AttackMode:
		// 砍伐/采集/挖掘/攻击：统一对当前交互目标造成伤害
		ApplyInteractDamage();
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
