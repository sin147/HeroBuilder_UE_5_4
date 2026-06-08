// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_CharacterSubsystem.h"
#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_InteractSubsystem.h"
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

	//Manager可能尚未就绪：服务端StartPlay未注册完，或客户端GameState尚未复制到ReplicatedManagers
	AHB_CharacterManager* Mgr = GetManager<AHB_CharacterManager>();
	if (!Mgr)
	{
		return;
	}
	//容器已稳定，安全使用range-based for
	TArray<TObjectPtr<ACharacter>> Characters = Mgr->GetAllCharacters();
    for (ACharacter* Character : Characters)
	{
		TickCharacter(Character, DeltaTime);
	}
}

void UHB_CharacterSubsystem::OnPlayerLogin(AGameModeBase* GameMode, APlayerController* PlayerController)
{
	Super::OnPlayerLogin(GameMode, PlayerController);
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
		if (AHB_CharacterManager* Mgr = GetManager<AHB_CharacterManager>())
		{
			Mgr->RemoveEntry(ExitingChar);
		}
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
		Mgr->SetCurrentlyState(InCharacter, NewState);
		OnEnterState(InCharacter, NewState);
		//服务端权威路径：在 OnLeaveState/OnEnterState 内部已分别广播 Leave/Enter；这里补发 Changed
		OnCharacterStateChanged.Broadcast(InCharacter, NewState, CurrentState);
	}
	else
	{
		AbortInteract(InCharacter);
	}
}

bool UHB_CharacterSubsystem::CanSwitchState(ACharacter* InCharacter, EPlayerCharacterState NewState, EPlayerCharacterState OldState)
{
	//获取玩家当前交互模式
    const EInteractMode InteractMode = GetWorld()->GetSubsystem<UHB_InteractSubsystem>()->GetInteractMode(InCharacter);
	//打印当前和目标状态
	const FString InteractModeName = StaticEnum<EInteractMode>()->GetNameStringByValue((int64)InteractMode);
	const FString CurrentStateName = StaticEnum<EPlayerCharacterState>()->GetNameStringByValue((int64)OldState);
    const FString NewStateName = StaticEnum<EPlayerCharacterState>()->GetNameStringByValue((int64)NewState);
    UE_LOG(LogCharacterSubsystem, Log, TEXT("CanSwitchState:%s: %s -> %s"), *InteractModeName, *CurrentStateName, *NewStateName);
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
			if (InteractSys)
			{
				if (!GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>()->CheckCanConstruction(InCharacter))
				{
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

bool UHB_CharacterSubsystem::IsInteracting(AHeroBuilderCharacter* InCharacter)
{
	EPlayerCharacterState CurrentlyState = GetCurrentState(InCharacter);
	return CurrentlyState==EPCS_Interact||CurrentlyState==EPCS_PostInteract||CurrentlyState==EPCS_PreInteract;
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
		Cast<AHeroBuilderCharacter>(InCharacter)->OnStartInteract();
		CharacterInteractTimer.FindOrAdd(InCharacter) = 0.f;
		break;
	}
	case EPCS_Interact:
	{
		CharacterInteractTimer.FindOrAdd(InCharacter) = 0.f;
		break;
	}
	case EPCS_PostInteract:
	{
		CharacterInteractTimer.FindOrAdd(InCharacter) = 0.f;
        break;
	}
	default:
		break;
	}
	//服务端权威路径：直接广播 Enter（Leave 由 OnLeaveState 负责，Changed 由 SwitchState 负责）
	if (EnterState != EPCS_None)
	{
		OnCharacterEnterState.Broadcast(InCharacter, EnterState);
	}
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
		Cast<AHeroBuilderCharacter>(InCharacter)->OnEndInteract();
		break;
	case EPCS_Interact:
		Cast<AHeroBuilderCharacter>(InCharacter)->OnEndInteract();
		break;
	case EPCS_PostInteract:
		Cast<AHeroBuilderCharacter>(InCharacter)->OnEndInteract();
		break;
	default:
		break;
	}
	const FString StateName = StaticEnum<EPlayerCharacterState>()->GetNameStringByValue((int64)LeaveState);
	UE_LOG(LogCharacterSubsystem, Log, TEXT("'%s' Leave State '%s'!"), *GetNameSafe(InCharacter), *StateName);
	//服务端权威路径：直接广播 Leave（Enter 由 OnEnterState 负责，Changed 由 SwitchState 负责）
	if (LeaveState != EPCS_None)
	{
		OnCharacterLeaveState.Broadcast(InCharacter, LeaveState);
	}
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
	SwitchState(InCharacter, EPCS_Idle);
	UE_LOG(LogCharacterSubsystem, Log, TEXT("'%s' AbortInteract: interact flow aborted."), *GetNameSafe(InCharacter));
}

void UHB_CharacterSubsystem::BeginInteractFlow(ACharacter* InCharacter)
{
	AHB_CharacterManager* Mgr = GetCharacterManager();
	if (!Mgr)
	{
		return;
	}

	//通过 InteractSubsystem 封装接口获取交互目标与交互模式，不直接处理 Manager
	UHB_InteractSubsystem* InteractSys = GetWorld()->GetSubsystem<UHB_InteractSubsystem>();
	if (!InteractSys)
	{
		return;
	}

	AActor* Target = InteractSys->GetInteractTarget(InCharacter);
	const float Range = Mgr->GetInteractRange(InCharacter);

	//ConstructionMode下禁用追击，直接进入前摇
	if (InteractSys->GetInteractMode(InCharacter) == IM_Construction)
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
    UHB_InteractSubsystem* InteractSub = GetWorld()->GetSubsystem<UHB_InteractSubsystem>();
	if (!InteractSub)
	{
		return;
	}
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
		//更新交互计时器：FindOrAdd 兜底，避免 SimulatedProxy 等未走过 OnEnterState 的端直接 operator[] 崩溃
		float& Timer = CharacterInteractTimer.FindOrAdd(InCharacter);
		Timer += DeltaTime;
        if (Timer >= InteractSub->GetPreInteractDelay(InCharacter))
        {
            SwitchState(InCharacter, EPCS_Interact);
        }
        break;
    }
	case EPCS_Interact:
    {
		//交互瞬时执行：触发交互后立即进入后摇
		InteractSub->TryInteract(InCharacter);
		SwitchState(InCharacter, EPCS_PostInteract);
        break;
    }
	case EPCS_PostInteract:
    {
		//更新交互计时器：FindOrAdd 兜底
		float& Timer = CharacterInteractTimer.FindOrAdd(InCharacter);
		Timer += DeltaTime;
		if (Timer >= InteractSub->GetPostInteractDelay(InCharacter))
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

void UHB_CharacterSubsystem::TickMoveToTarget(ACharacter* InCharacter, float DeltaTime)
{
	//仅自治代理客户端调用：只产生AddMovementInput，走CharacterMovement标准预测管线
	AHB_CharacterManager* Mgr = GetCharacterManager();
	if (!Mgr)
	{
		return;
	}
	UHB_InteractSubsystem* InteractSys = GetWorld()->GetSubsystem<UHB_InteractSubsystem>();
	if (!InteractSys)
	{
		return;
	}
	AActor* Target = InteractSys->GetInteractTarget(InCharacter);
	const float Range = Mgr->GetInteractRange(InCharacter);
	if (!IsValid(Target))
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
		SwitchState(InCharacter, EPCS_PreInteract);
		return;
	}

	//仅自治代理（即本地玩家控制的角色）才驱动客户端追击
	if (InCharacter->GetLocalRole() != ROLE_AutonomousProxy && InCharacter->GetLocalRole() != ROLE_Authority)
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