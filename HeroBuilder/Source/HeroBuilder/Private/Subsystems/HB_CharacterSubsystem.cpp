// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_CharacterSubsystem.h"
#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_InteractSubsystem.h"
#include "Manager/HB_InteractManager.h"
#include "HeroBuilder/HeroBuilderCharacter.h"
#include "Components/HB_DamageComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "InputActionValue.h"
#include "EngineUtils.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogCharacterSubsystem);

//——————————————————————————————————————————————
// Tick / Player 事件
//——————————————————————————————————————————————

void UHB_CharacterSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//先批量清理无效角色，避免range-based for期间修改容器
	RegisteredCharacters.RemoveAllSwap([](const TObjectPtr<ACharacter>& Char)
	{
		return !IsValid(Char);
	});
	//容器已稳定，安全使用range-based for
	for (ACharacter* Character : RegisteredCharacters)
	{
		TickCharacter(Character, DeltaTime);
	}
}

void UHB_CharacterSubsystem::OnPlayerLogin(AGameModeBase* GameMode, APlayerController* PlayerController)
{
	Super::OnPlayerLogin(GameMode, PlayerController);
	//首个玩家登录时按需Spawn单例 CharacterManager
}

void UHB_CharacterSubsystem::OnPlayerLogout(AGameModeBase* GameMode, AController* Exiting)
{
	Super::OnPlayerLogout(GameMode, Exiting);
	APlayerController* PC = Cast<APlayerController>(Exiting);
	if (!PC)
	{
		return;
	}
	if (ACharacter* ExitingChar = Cast<ACharacter>(PC->GetPawn()))
	{
		UnregisterCharacter(ExitingChar);
	}
}

//——————————————————————————————————————————————
// Manager 访问
//——————————————————————————————————————————————

AHB_CharacterManager* UHB_CharacterSubsystem::GetCharacterManager()
{
	return GetManager<AHB_CharacterManager>();
}

//——————————————————————————————————————————————
// 角色注册
//——————————————————————————————————————————————

void UHB_CharacterSubsystem::RegisterCharacter(ACharacter* InCharacter)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	//仅服务端写表项
	if (NetMode != ENetMode::NM_Client)
	{
		if (AHB_CharacterManager* Mgr = GetCharacterManager())
		{
			Mgr->RegisterCharacter(InCharacter);
			//默认初始为Idle
			Mgr->SetCurrentlyState(InCharacter, EPCS_Idle);
		}
		RegisteredCharacters.AddUnique(InCharacter);
	}
}

void UHB_CharacterSubsystem::UnregisterCharacter(ACharacter* InCharacter)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	if (NetMode != ENetMode::NM_Client)
	{
		if (AHB_CharacterManager* Mgr = GetCharacterManager())
		{
			Mgr->RemoveEntry(InCharacter);
		}
		RegisteredCharacters.Remove(InCharacter);
	}
}

//——————————————————————————————————————————————
// 同步属性Getter / Setter（透传Manager）
//——————————————————————————————————————————————

EPlayerCharacterState UHB_CharacterSubsystem::GetCurrentState(ACharacter* InCharacter) const
{
	if (AHB_CharacterManager* Mgr = const_cast<UHB_CharacterSubsystem*>(this)->GetCharacterManager())
	{
		return Mgr->GetCurrentlyState(InCharacter);
	}
	return EPCS_Idle;
}

AActor* UHB_CharacterSubsystem::GetInteractTarget(ACharacter* InCharacter) const
{
	if (AHB_CharacterManager* Mgr = const_cast<UHB_CharacterSubsystem*>(this)->GetCharacterManager())
	{
		return Mgr->GetInteractTarget(InCharacter);
	}
	return nullptr;
}

void UHB_CharacterSubsystem::SetInteractTarget(ACharacter* InCharacter, AActor* Target)
{
	if (AHB_CharacterManager* Mgr = GetCharacterManager())
	{
		Mgr->SetInteractTarget(InCharacter, Target);
	}
}

float UHB_CharacterSubsystem::GetAttack(ACharacter* InCharacter) const
{
	if (AHB_CharacterManager* Mgr = const_cast<UHB_CharacterSubsystem*>(this)->GetCharacterManager())
	{
		return Mgr->GetAttack(InCharacter);
	}
	return 0.f;
}

void UHB_CharacterSubsystem::SetAttack(ACharacter* InCharacter, float NewAttack)
{
	if (AHB_CharacterManager* Mgr = GetCharacterManager())
	{
		Mgr->SetAttack(InCharacter, NewAttack);
	}
}

float UHB_CharacterSubsystem::GetInteractRange(ACharacter* InCharacter) const
{
	if (AHB_CharacterManager* Mgr = const_cast<UHB_CharacterSubsystem*>(this)->GetCharacterManager())
	{
		return Mgr->GetInteractRange(InCharacter);
	}
	return 100.f;
}

void UHB_CharacterSubsystem::SetInteractRange(ACharacter* InCharacter, float NewRange)
{
	if (AHB_CharacterManager* Mgr = GetCharacterManager())
	{
		Mgr->SetInteractRange(InCharacter, NewRange);
	}
}

UCameraComponent* UHB_CharacterSubsystem::GetCharacterFollowCamera(AHeroBuilderCharacter* InCharacter) const
{
	if (IsValid(InCharacter))
	{
		return InCharacter->FollowCamera;
	}
	return nullptr;
}

FVector UHB_CharacterSubsystem::GetCharacterFollowCameraForward(AHeroBuilderCharacter* InCharacter) const
{
    if (UCameraComponent* Camera = GetCharacterFollowCamera(InCharacter))
    {
        return Camera->GetForwardVector();
    }
    return FVector::ForwardVector;
}

//——————————————————————————————————————————————
// 状态机：SwitchState / OnEnter / OnLeave / CanSwitch
//——————————————————————————————————————————————

void UHB_CharacterSubsystem::SwitchState(ACharacter* InCharacter, EPlayerCharacterState NewState)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	AHB_CharacterManager* Mgr = GetCharacterManager();
	if (!Mgr)
	{
		return;
	}
	const EPlayerCharacterState CurrentState = Mgr->GetCurrentlyState(InCharacter);
	if (CurrentState == NewState)
	{
		return;
	}
	if (CanSwitchState(InCharacter, NewState, CurrentState))
	{
		OnLeaveState(InCharacter, CurrentState);
		OnEnterState(InCharacter, NewState);
	}
}

bool UHB_CharacterSubsystem::CanSwitchState(ACharacter* InCharacter, EPlayerCharacterState NewState, EPlayerCharacterState OldState)
{
	//获取玩家当前交互模式
    const EInteractMode InteractMode = GetWorld()->GetSubsystem<UHB_InteractSubsystem>()->GetInteractMode(InCharacter);
	switch (InteractMode)
	{
	case IM_Normal:
	{
		switch (NewState)
		{
		case EPCS_None:
			break;
		case EPCS_Idle:
			break;
		case EPCS_Move:
			break;
		case EPCS_MoveToTarget:
			break;
		case EPCS_PreInteract:
			break;
		case EPCS_Interact:
			break;
		case EPCS_PostInteract:
			break;
		default:
			break;
		}
		break;
	}
	case IM_Construction:
	{
		switch (NewState)
		{
		case EPCS_None:
			break;
		case EPCS_Idle:
			break;
		case EPCS_Move:
			break;
		case EPCS_MoveToTarget:
			break;
		case EPCS_PreInteract:
		{
			UHB_InteractSubsystem* InteractSys = GetWorld()->GetSubsystem<UHB_InteractSubsystem>();
			if (InteractSys && InteractSys->GetInteractType(InCharacter) == IT_Construction)
			{
				if (!GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>()->CheckCanConstruction(InCharacter))
				{
					SwitchState(InCharacter, EPCS_Idle);
					return false;
				}
				return true;
			}
		}
		case EPCS_Interact:
			break;
		case EPCS_PostInteract:
			break;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
	
	return true;
}

void UHB_CharacterSubsystem::OnEnterState(ACharacter* InCharacter, EPlayerCharacterState EnterState)
{
	AHB_CharacterManager* Mgr = GetCharacterManager();
	if (!Mgr)
	{
		return;
	}
	//先正式落表，保证OnRep只通知一次最终值
	Mgr->SetCurrentlyState(InCharacter, EnterState);
	const FString StateName = StaticEnum<EPlayerCharacterState>()->GetNameStringByValue((int64)EnterState);
	UE_LOG(LogCharacterSubsystem, Log, TEXT("'%s' Enter State '%s'!"), *GetNameSafe(InCharacter), *StateName);
	switch (EnterState)
	{
	case EPCS_None:
		break;
	case EPCS_Idle:
		break;
	case EPCS_Move:
		break;
	case EPCS_MoveToTarget:
		//追击目标进入态：什么都不做，由Tick驱动追击
		break;
	case EPCS_PreInteract:
	{
		if(GetWorld()->GetSubsystem<UHB_InteractSubsystem>()->GetPreInteractDelay(InCharacter)<= 0.f)
		{
			//没有前摇时，直接进入交互
			SwitchState(InCharacter, EPCS_Interact);
		}
		CharacterInteractTimer.FindOrAdd(InCharacter) = 0.f;
		break;
	}
	case EPCS_Interact:
	{
		SwitchState(InCharacter, EPCS_PostInteract);
		break;
	}
	case EPCS_PostInteract:
	{
        if (GetWorld()->GetSubsystem<UHB_InteractSubsystem>()->GetPostInteractDelay(InCharacter) <= 0.f)
		{
			SwitchState(InCharacter, EPCS_PreInteract);
        }
        break;
	}
	default:
		break;
	}
	GetManager<AHB_CharacterManager>()->SetCurrentlyState(InCharacter, EnterState);
}

void UHB_CharacterSubsystem::OnLeaveState(ACharacter* InCharacter, EPlayerCharacterState LeaveState)
{
	switch (LeaveState)
	{
	case EPCS_None:
		break;
	case EPCS_Idle:
		break;
	case EPCS_Move:
	{
		//仅本地驱动方（自治代理/服务端权威）才需要主动停止；SimulatedProxy由网络同步驱动，无需手动Stop
		const ENetRole LocalRole = InCharacter->GetLocalRole();
		if (LocalRole == ROLE_AutonomousProxy || LocalRole == ROLE_Authority)
		{
			if (UCharacterMovementComponent* Move = InCharacter->GetCharacterMovement())
			{
				Move->StopMovementImmediately();
			}
        }
		break;
	}
	case EPCS_MoveToTarget:
	{
		//仅本地驱动方（自治代理/服务端权威）才需要主动停止；与TickMoveToTarget的驱动判断保持一致
		const ENetRole LocalRole = InCharacter->GetLocalRole();
		if (LocalRole == ROLE_AutonomousProxy || LocalRole == ROLE_Authority)
		{
			if (UCharacterMovementComponent* Move = InCharacter->GetCharacterMovement())
			{
				Move->StopMovementImmediately();
			}
		}
		break;
	}
	case EPCS_PreInteract:
		break;
	case EPCS_Interact:
		break;
	case EPCS_PostInteract:
		break;
	default:
		break;
	}
	const FString StateName = StaticEnum<EPlayerCharacterState>()->GetNameStringByValue((int64)LeaveState);
	GetManager<AHB_CharacterManager>()->SetCurrentlyState(InCharacter, EPCS_None);
	UE_LOG(LogCharacterSubsystem, Log, TEXT("'%s' Leave State '%s'!"), *GetNameSafe(InCharacter), *StateName);
}

void UHB_CharacterSubsystem::AbortInteract(ACharacter* InCharacter)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	AHB_CharacterManager* Mgr = GetCharacterManager();
	if (!Mgr)
	{
		return;
	}
	const EPlayerCharacterState State = Mgr->GetCurrentlyState(InCharacter);
	//只有处在交互流程中（追击/前摇/交互/后摇）才需要中止
	if (State != EPCS_MoveToTarget &&
		State != EPCS_PreInteract  &&
		State != EPCS_Interact     &&
		State != EPCS_PostInteract)
	{
		return;
	}
	SwitchState(InCharacter, EPCS_Idle);
	UE_LOG(LogCharacterSubsystem, Log, TEXT("'%s' AbortInteract: interact flow aborted."), *GetNameSafe(InCharacter));
}

void UHB_CharacterSubsystem::BeginInteractFlow(ACharacter* InCharacter)
{
	if (!IsValid(InCharacter) || !InCharacter->HasAuthority())
	{
		return;
	}
	AHB_CharacterManager* Mgr = GetCharacterManager();
	if (!Mgr)
	{
		return;
	}
	
	//获取InteractManager来判断当前交互模式
	AHB_InteractManager* InteractMgr = GetManager<AHB_InteractManager>();
	if (!InteractMgr)
	{
		return;
	}
	
	AActor* Target = Mgr->GetInteractTarget(InCharacter);
	const float Range = Mgr->GetInteractRange(InCharacter);
	
	//ConstructionMode下禁用追击，直接进入前摇
	if (InteractMgr && InteractMgr->GetCurrentInteractMode(InCharacter) == IM_Construction)
	{
		SwitchState(InCharacter, EPCS_PreInteract);
		return;
	}
	
	//有交互目标且尚未进入InteractRange时，先进入追击态；否则直接进入前摇
	if (IsValid(Target))
	{
		const FVector Delta   = Target->GetActorLocation() - InCharacter->GetActorLocation();
		const FVector DeltaXY(Delta.X, Delta.Y, 0.f);
		if (DeltaXY.Size() > Range)
		{
			SwitchState(InCharacter, EPCS_MoveToTarget);
			return;
		}
	}
	SwitchState(InCharacter, EPCS_PreInteract);
}

//——————————————————————————————————————————————
// Tick 驱动
//——————————————————————————————————————————————

void UHB_CharacterSubsystem::TickCharacter(ACharacter* InCharacter, float DeltaTime)
{
	if (!IsValid(InCharacter))
	{
		return;
	}
	TickUpdateState(InCharacter, DeltaTime);
	TickCharacterState(InCharacter, DeltaTime);
}

void UHB_CharacterSubsystem::TickUpdateState(ACharacter* InCharacter, float DeltaTime)
{
	AHB_CharacterManager* Mgr = GetCharacterManager();
	if (!Mgr)
	{
		return;
	}
	const EPlayerCharacterState State = Mgr->GetCurrentlyState(InCharacter);
	//处于交互流程中（含追击目标）：直接保持当前状态
	if (State == EPCS_MoveToTarget ||
		State == EPCS_PreInteract  ||
		State == EPCS_Interact     ||
		State == EPCS_PostInteract)
	{
		return;
	}

	if (!InCharacter->GetVelocity().IsNearlyZero())
	{
		SwitchState(InCharacter, EPCS_Move);
	}
	else
	{
		if (State == EPCS_Move)
		{
			SwitchState(InCharacter, EPCS_Idle);
		}
	}
}

void UHB_CharacterSubsystem::TickCharacterState(ACharacter* InCharacter, float DeltaTime)
{
	AHB_CharacterManager* Mgr = GetCharacterManager();
	if (!Mgr)
	{
		return;
	}
	const EPlayerCharacterState State = Mgr->GetCurrentlyState(InCharacter);
	switch (State)
	{
	case EPCS_None:
		break;
	case EPCS_Idle:
		break;
	case EPCS_Move:
		break;
	case EPCS_MoveToTarget:
	{
		//客户端驱动追击位移：走CharacterMovement标准预测管线
		TickMoveToTarget(InCharacter, DeltaTime);
		break;
	}
	case EPCS_PreInteract:
    {
        if (CharacterInteractTimer.Contains(InCharacter) && CharacterInteractTimer[InCharacter] >= GetWorld()->GetSubsystem<UHB_InteractSubsystem>()->GetPreInteractDelay(InCharacter))
        {
            SwitchState(InCharacter, EPCS_Interact);
        }
        break;
    }
	case EPCS_Interact:
    {
        //更新交互计时器
        if (CharacterInteractTimer.Contains(InCharacter))
        {
            CharacterInteractTimer[InCharacter] += DeltaTime;
        }
        break;
    }
	case EPCS_PostInteract:
    {
        //更新交互计时器
        if (CharacterInteractTimer.Contains(InCharacter))
        {
            CharacterInteractTimer[InCharacter] += DeltaTime;
        }
		if (CharacterInteractTimer[InCharacter] >= GetWorld()->GetSubsystem<UHB_InteractSubsystem>()->GetPostInteractDelay(InCharacter))
		{
			SwitchState(InCharacter, EPCS_PreInteract);
		}
        break;
    }
	default:
		break;
	}
}

//——————————————————————————————————————————————
// 追击目标
//——————————————————————————————————————————————

void UHB_CharacterSubsystem::TickAuthorityMoveToTarget(ACharacter* InCharacter, float DeltaTime)
{
	AHB_CharacterManager* Mgr = GetCharacterManager();
	if (!Mgr)
	{
		return;
	}
	AActor* Target = Mgr->GetInteractTarget(InCharacter);
	const float Range = Mgr->GetInteractRange(InCharacter);
	if (!IsValid(Target))
	{
		AbortInteract(InCharacter);
		return;
	}
	const FVector Delta = Target->GetActorLocation() - InCharacter->GetActorLocation();
	const FVector DeltaXY(Delta.X, Delta.Y, 0.f);
	if (DeltaXY.Size() <= Range)
	{
		SwitchState(InCharacter, EPCS_PreInteract);
	}
}

void UHB_CharacterSubsystem::TickMoveToTarget(ACharacter* InCharacter, float DeltaTime)
{
	//仅自治代理客户端调用：只产生AddMovementInput，走CharacterMovement标准预测管线
	AHB_CharacterManager* Mgr = GetCharacterManager();
	if (!Mgr)
	{
		return;
	}
	AActor* Target = Mgr->GetInteractTarget(InCharacter);
	const float Range = Mgr->GetInteractRange(InCharacter);
	if (!IsValid(Target))
	{
		return;
	}

	//仅自治代理（即本地玩家控制的角色）才驱动客户端追击
	if (InCharacter->GetLocalRole() != ROLE_AutonomousProxy && InCharacter->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	AHeroBuilderCharacter* HBChar = Cast<AHeroBuilderCharacter>(InCharacter);
	if (!HBChar)
	{
		return;
	}

	const FVector SelfLoc   = InCharacter->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const FVector Delta     = TargetLoc - SelfLoc;
	const FVector DeltaXY(Delta.X, Delta.Y, 0.f);
	const float   DistXY = DeltaXY.Size();

	//已在交互范围内：客户端不再主动驱动位移
	if (DistXY <= Range)
	{
		return;
	}

	//把世界空间下"朝向目标的XY单位向量"反算为玩家ControlRotation局部空间的(X=右, Y=前) 2D输入
	const FVector DirXY = DeltaXY.GetSafeNormal();
	FVector2D MoveInput2D(0.f, 1.f);
	AController* Controller = InCharacter->GetController();
	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		MoveInput2D.Y = FVector::DotProduct(DirXY, ForwardDirection);
		MoveInput2D.X = FVector::DotProduct(DirXY, RightDirection);
	}
	else
	{
		MoveInput2D.X = DirXY.Y;
		MoveInput2D.Y = DirXY.X;
	}

	HBChar->Move(FInputActionValue(MoveInput2D));
}