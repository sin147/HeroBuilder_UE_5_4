// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_InteractSubsystem.h"
#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_DamageSubsystem.h"
#include "Subsystems/HB_CharacterSubsystem.h"
#include "Manager/HB_InteractManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HeroBuilder/HeroBuilderCharacter.h"
#include "Components/HB_InteractComponent.h"
#include "Building/HB_Building_Base.h"
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

		// 在所有"挂了 InteractComponent 且当前可交互（EffectiveInteractType != IT_None）"的Actor中
		// 挑选距离最近的一个，作为本帧的交互目标候选。
		AActor* NewNearest = nullptr;
		EInteractType DesiredMode = IT_None;
		float MinDist = InteractTraceDistance;

		if (bHit)
		{
			const FVector CharacterLocation = Character->GetActorLocation();
			for (auto& Res : OutResults)
			{
				AActor* Actor = Res.GetActor();
				if (!IsValid(Actor) || Actor == Character)
				{
					continue;
				}
				// 仅纳入"挂了 InteractComponent 且 InteractType 有效"的 Actor
				UHB_InteractComponent* InteractComp = Actor->FindComponentByClass<UHB_InteractComponent>();
				if (!InteractComp)
				{
					continue;
				}
				const EInteractType ActorInteractType = InteractComp->GetEffectiveInteractType();
				if (ActorInteractType == IT_None)
				{
					continue;
				}

				const float Dist = FVector::Dist(CharacterLocation, Actor->GetActorLocation());
				if (Dist < MinDist)
				{
					MinDist = Dist;
					NewNearest = Actor;
					DesiredMode = ActorInteractType;
				}
			}
		}

		// 根据目标类型同步交互模式：仅在"被动模式"下自动切换，
		// 保留玩家主动选择的模式（如 IT_ConstructionMode）不被覆盖。
		AHB_InteractManager* InteractMgr = GetInteractManager();
		if (!InteractMgr)
		{
			continue;
		}
		const EInteractType CurrentMode = InteractMgr->GetCurrentInteractType(HBCharacter);

		// 找到了有效目标 -> 设为 InteractTarget；否则清空。
		if (NewNearest)
		{
			HBCharacter->SetInteractTarget(NewNearest);
		}
		else
		{
			HBCharacter->SetInteractTarget(nullptr);
		}

		// 同步当前角色的 InteractType（无有效目标 -> 回落到 IT_Normal）
		const EInteractType ApplyMode = (DesiredMode != IT_None) ? DesiredMode : IT_Normal;
		if (ApplyMode != CurrentMode)
		{
			SwitchInteractType(HBCharacter, ApplyMode);
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

	//从单例表中移除该玩家的交互数据（避免表项泄露）
	if (PC)
	{
		if (AHB_InteractManager* InteractMgr = GetInteractManager())
		{
			if (ACharacter* ExitingChar = Cast<ACharacter>(PC->GetPawn()))
			{
				InteractMgr->RemoveEntry(ExitingChar);
			}
		}
	}
}

AHB_InteractManager* UHB_InteractSubsystem::GetInteractManager()
{
	//统一走基类通道：服务端读 GameMode 的 Manager 列表，客户端读 GameState 上已复制的 Manager 列表。
	//Manager 的创建交给 AHeroBuilderGameMode::StartPlay 统一 Spawn，子系统不再自行 Spawn。
	return GetManager<AHB_InteractManager>();
}

void UHB_InteractSubsystem::SwitchInteractType(ACharacter* InCharacter, EInteractType NewMode)
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
	AHB_InteractManager* InteractMgr = GetInteractManager();
	if (!InteractMgr)
	{
		return;
	}
	const EInteractType CurrentMode = InteractMgr->GetCurrentInteractType(HBCharacter);
	if (CurrentMode == NewMode)
	{
		return;
	}
	//模式即将切换：先中止角色当前正在进行的交互流程，避免旧动作/旧目标残留到新模式
	if (UHB_CharacterSubsystem* CharSys = GetWorld()->GetSubsystem<UHB_CharacterSubsystem>())
	{
		CharSys->AbortInteract(InCharacter);
	}
	LeaveInteractType(InCharacter, CurrentMode);
	EnterInteractType(InCharacter, NewMode);
}

void UHB_InteractSubsystem::EnterInteractType(ACharacter* InCharacter, EInteractType EnterMode)
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
	case IT_None:
		break;
	case IT_Normal:
		break;
	case IT_Construction:
		break;
	case IT_Lumber:
		//TODO: 进入砍伐模式
		break;
	case IT_Gather:
		//TODO: 进入采集模式
		break;
	case IT_Mine:
		//TODO: 进入挖掘模式
		break;
	case IT_Attack:
		//TODO: 进入攻击模式
		break;
	default:
		break;
	}
	if (InteractData)
	{
		const EInteractMode CurMode = GetInteractMode(HeroBuilderCharacter);
		HeroBuilderCharacter->SetPreInteractDelay(InteractData->GetPreInteractDelay(CurMode, EnterMode));
		HeroBuilderCharacter->SetPostInteractDelay(InteractData->GetPostInteractDelay(CurMode, EnterMode));
	}
	if (AHB_InteractManager* InteractMgr = GetInteractManager())
	{
		InteractMgr->SetCurrentInteractType(HeroBuilderCharacter, EnterMode);
	}
	const FString ModeName = StaticEnum<EInteractType>()->GetNameStringByValue((int64)EnterMode);
	UE_LOG(LogInteractSubsystem, Log, TEXT("'%s' Enter InteractMode '%s'!"), *GetNameSafe(InCharacter), *ModeName);
}

void UHB_InteractSubsystem::LeaveInteractType(ACharacter* InCharacter, EInteractType LeaveMode)
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
	case IT_None:
		break;
	case IT_Normal:
		break;
	case IT_Construction:
		break;
	case IT_Lumber:
		//TODO: 离开砍伐模式
		break;
	case IT_Gather:
		//TODO: 离开采集模式
		break;
	case IT_Mine:
		//TODO: 离开挖掘模式
		break;
	case IT_Attack:
		//TODO: 离开攻击模式
		break;
	default:
		break;
	}
	if (AHB_InteractManager* InteractMgr = GetInteractManager())
	{
		InteractMgr->SetCurrentInteractType(HBCharacter, IT_None);
	}
	const FString ModeName = StaticEnum<EInteractType>()->GetNameStringByValue((int64)LeaveMode);
	UE_LOG(LogInteractSubsystem, Log, TEXT("'%s' Leave InteractMode '%s'!"), *GetNameSafe(InCharacter), *ModeName);
}

EInteractType UHB_InteractSubsystem::GetInteractType(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return IT_None;
	}
	if (AHB_InteractManager* InteractMgr = const_cast<UHB_InteractSubsystem*>(this)->GetInteractManager())
	{
		return InteractMgr->GetCurrentInteractType(InCharacter);
	}
	return IT_None;
}

EInteractMode UHB_InteractSubsystem::GetInteractMode(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return IM_Normal;
	}
	if (AHB_InteractManager* InteractMgr = const_cast<UHB_InteractSubsystem*>(this)->GetInteractManager())
	{
		return InteractMgr->GetCurrentInteractMode(InCharacter);
	}
	return IM_Normal;
}

void UHB_InteractSubsystem::SetCurrentInteractMode(ACharacter* InCharacter, EInteractMode NewMode)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	if (AHB_InteractManager* InteractMgr = GetInteractManager())
	{
		InteractMgr->SetCurrentInteractMode(InCharacter, NewMode);
	}
}

void UHB_InteractSubsystem::SwitchInteractMode(ACharacter* InCharacter, EInteractMode NewMode)
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
	AHB_InteractManager* InteractMgr = GetInteractManager();
	if (!InteractMgr)
	{
		return;
	}
	const EInteractMode CurrentMode = InteractMgr->GetCurrentInteractMode(HBCharacter);
	if (CurrentMode == NewMode)
	{
		return;
	}
	//模式即将切换：先中止角色当前正在进行的交互流程，避免旧动作/旧目标残留到新模式
	if (UHB_CharacterSubsystem* CharSys = GetWorld()->GetSubsystem<UHB_CharacterSubsystem>())
	{
		CharSys->AbortInteract(InCharacter);
	}
	LeaveInteractMode(InCharacter, CurrentMode);
	EnterInteractMode(InCharacter, NewMode);
}

void UHB_InteractSubsystem::EnterInteractMode(ACharacter* InCharacter, EInteractMode EnterMode)
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
	switch (EnterMode)
	{
	case IM_Normal:
		break;
	case IM_Construction:
		if (UHB_ConstructionSubsystem* ConstructionSys = GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>())
		{
			ConstructionSys->Server_ActiveConstructionMode(HBCharacter);
		}
		break;
	default:
		break;
	}
	if (AHB_InteractManager* InteractMgr = GetInteractManager())
	{
		InteractMgr->SetCurrentInteractMode(HBCharacter, EnterMode);
	}
	const FString ModeName = StaticEnum<EInteractMode>()->GetNameStringByValue((int64)EnterMode);
	UE_LOG(LogInteractSubsystem, Log, TEXT("'%s' Enter InteractMode '%s'!"), *GetNameSafe(InCharacter), *ModeName);
}

void UHB_InteractSubsystem::LeaveInteractMode(ACharacter* InCharacter, EInteractMode LeaveMode)
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
	case IM_Normal:
		break;
	case IM_Construction:
		if (UHB_ConstructionSubsystem* ConstructionSys = GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>())
		{
			ConstructionSys->Server_CancelConstructionMode(HBCharacter);
		}
		break;
	default:
		break;
	}
	if (AHB_InteractManager* InteractMgr = GetInteractManager())
	{
		InteractMgr->SetCurrentInteractMode(HBCharacter, IM_Normal);
	}
	const FString ModeName = StaticEnum<EInteractMode>()->GetNameStringByValue((int64)LeaveMode);
	UE_LOG(LogInteractSubsystem, Log, TEXT("'%s' Leave InteractMode '%s'!"), *GetNameSafe(InCharacter), *ModeName);
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
	const EInteractType InteractType = GetInteractType(InCharacter);

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
	const EInteractMode Mode = GetInteractMode(InCharacter);
	switch (Mode)
	{
	case IM_Normal:
	{
		switch (InteractType)
		{
		case IT_None:
			break;
		case IT_Normal:
			break;
		case IT_Construction:
		{
			//面向"待修建建筑"：玩家普通模式下交互即为修建/治疗，治疗量复用 Attack 属性
			AHeroBuilderCharacter* HBCharacter = Cast<AHeroBuilderCharacter>(InCharacter);
			if (!HBCharacter)
			{
				break;
			}
			AHB_Building_Base* TargetBuilding = Cast<AHB_Building_Base>(HBCharacter->GetInteractTarget());
			if (TargetBuilding && TargetBuilding->IsAwaitingConstruction())
			{
				const float HealAmount = HBCharacter->GetAttack();
				TargetBuilding->HealAsConstruction(HealAmount, InCharacter);
			}
			break;
		}
		case IT_Lumber:
		case IT_Gather:
		case IT_Mine:
		case IT_Attack:
			// 砍伐/采集/挖掘/攻击：统一对当前交互目标造成伤害
			ApplyInteractDamage();
			break;
		default:
			break;
		}
		break;
	}
	case IM_Construction:
	{
		if (UHB_ConstructionSubsystem* ConstructionSys = GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>())
		{
			ConstructionSys->ConstructionBegin(InCharacter);
		}
		break;
	}
	default:
		break;
	}


}

UAnimSequence* UHB_InteractSubsystem::GetInteractAnim(EInteractMode InteractMode, EInteractType InteractType) const
{
	if (!InteractData)
	{
		return nullptr;
	}
    return InteractData->GetInteractAnimation(InteractMode, InteractType);
}
