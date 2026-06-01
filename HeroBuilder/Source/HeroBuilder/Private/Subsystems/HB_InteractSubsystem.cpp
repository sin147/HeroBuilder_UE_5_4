// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_InteractSubsystem.h"
#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_DamageSubsystem.h"
#include "Subsystems/HB_CharacterSubsystem.h"
#include "Manager/HB_InteractManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HeroBuilder/HeroBuilderCharacter.h"
#include "Resource/HB_Resource_Base.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"

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
		// 保留玩家主动选择的模式（如 IT_ConstructionMode）不被覆盖。
		AHB_InteractManager* InteractMgr = GetInteractManager();
		if (!InteractMgr)
		{
			continue;
		}
		const EInteractType CurrentMode = InteractMgr->GetCurrentInteractType(HBCharacter);
		const bool bIsPassiveMode =
			(CurrentMode == IT_None) ||
			(CurrentMode == IT_Normal) ||
			(CurrentMode == IT_Lumber) ||
			(CurrentMode == IT_Gather) ||
			(CurrentMode == IT_Mine) ||
			(CurrentMode == IT_Attack);

		if (bIsPassiveMode)
		{
			EInteractType DesiredMode = IT_Normal;
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
					DesiredMode = IT_Attack;
				}
			}

			if (DesiredMode != CurrentMode)
			{
				SwitchInteractType(HBCharacter, DesiredMode);
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

	//服务端：保证单例InteractManager已存在。首个玩家登录时按需Spawn。
	GetInteractManager();
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

AHB_InteractManager* UHB_InteractSubsystem::GetInteractManager() const
{
	//缓存命中且有效：直接返回
	if (IsValid(CachedInteractManager))
	{
		return CachedInteractManager;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	//遍历世界查找已存在的单例（服务端/客户端均适用；客户端仅能这里拿到复制下来的实例）
	for (TActorIterator<AHB_InteractManager> It(World); It; ++It)
	{
		AHB_InteractManager* Mgr = *It;
		if (IsValid(Mgr))
		{
			CachedInteractManager = Mgr;
			return Mgr;
		}
	}

	//未找到：仅服务端有权限Spawn
	if (NetMode != ENetMode::NM_Client)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AHB_InteractManager* NewMgr = World->SpawnActor<AHB_InteractManager>(AHB_InteractManager::StaticClass(), SpawnParams);
		if (NewMgr)
		{
			CachedInteractManager = NewMgr;
			return NewMgr;
		}
		UE_LOG(LogInteractSubsystem, Error, TEXT("Failed to spawn singleton AHB_InteractManager"));
	}
	return nullptr;
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
		const EInteractManagerInteractMode CurMode = GetInteractMode(HeroBuilderCharacter);
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
	if (AHB_InteractManager* InteractMgr = GetInteractManager())
	{
		return InteractMgr->GetCurrentInteractType(InCharacter);
	}
	return IT_None;
}

EInteractManagerInteractMode UHB_InteractSubsystem::GetInteractMode(ACharacter* InCharacter) const
{
	if (!IsValid(InCharacter))
	{
		return IMIM_Normal;
	}
	if (AHB_InteractManager* InteractMgr = GetInteractManager())
	{
		return InteractMgr->GetCurrentInteractMode(InCharacter);
	}
	return IMIM_Normal;
}

void UHB_InteractSubsystem::SetCurrentInteractMode(ACharacter* InCharacter, EInteractManagerInteractMode NewMode)
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

void UHB_InteractSubsystem::SwitchInteractMode(ACharacter* InCharacter, EInteractManagerInteractMode NewMode)
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
	const EInteractManagerInteractMode CurrentMode = InteractMgr->GetCurrentInteractMode(HBCharacter);
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

void UHB_InteractSubsystem::EnterInteractMode(ACharacter* InCharacter, EInteractManagerInteractMode EnterMode)
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
	case IMIM_Normal:
		break;
	case IMIM_Construction:
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
	const FString ModeName = StaticEnum<EInteractManagerInteractMode>()->GetNameStringByValue((int64)EnterMode);
	UE_LOG(LogInteractSubsystem, Log, TEXT("'%s' Enter InteractMode '%s'!"), *GetNameSafe(InCharacter), *ModeName);
}

void UHB_InteractSubsystem::LeaveInteractMode(ACharacter* InCharacter, EInteractManagerInteractMode LeaveMode)
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
	case IMIM_Normal:
		break;
	case IMIM_Construction:
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
		InteractMgr->SetCurrentInteractMode(HBCharacter, IMIM_Normal);
	}
	const FString ModeName = StaticEnum<EInteractManagerInteractMode>()->GetNameStringByValue((int64)LeaveMode);
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
	const EInteractManagerInteractMode Mode = GetInteractMode(InCharacter);
	switch (Mode)
	{
	case IMIM_Normal:
	{
		switch (InteractType)
		{
		case IT_None:
			break;
		case IT_Normal:
			break;
		case IT_Construction:
			break;
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
	case IMIM_Construction:
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

UAnimSequence* UHB_InteractSubsystem::GetInteractAnim(EInteractManagerInteractMode InteractMode, EInteractType InteractType) const
{
	if (!InteractData)
	{
		return nullptr;
	}
    return InteractData->GetInteractAnimation(InteractMode, InteractType);
}
