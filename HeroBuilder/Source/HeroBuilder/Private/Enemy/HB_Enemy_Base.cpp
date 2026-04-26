// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/HB_Enemy_Base.h"
#include "Net/Core/PropertyConditions/PropertyConditions.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/HB_WaveSubsystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "../../HeroBuilderCharacter.h"
#include "Kismet/GameplayStatics.h"

void AHB_Enemy_Base::UpdateHealth()
{
	if (!bIsServer)
	{
		return;
	}
	Health=FMath::Clamp(Health-PreDamage,0.0f, MaxHealth);
    PreDamage = 0.0f;
    if (Health <= 0.0f)
    {
        Death();
    }
}

bool AHB_Enemy_Base::SwitchState(EEnemyState NewState)
{
	if (CurrentState == EEnemyState::Death)
	{
		UE_LOG(LogTemp, Warning, TEXT("SwitchState: Cannot switch state from Death"));
		return false;  // 死亡锁定，静默失败

	}
	if (CurrentState == NewState)
	{
		UE_LOG(LogTemp, Verbose, TEXT("SwitchState: Already in state %d"), static_cast<uint8>(NewState));
		return false;  // 状态未变，静默失败
	}
	if(!IsValid(Target))
	{
       Target= UGameplayStatics::GetActorOfClass(this, TargetClass);
	   if(!IsValid(Target))
		{
			UE_LOG(LogTemp, Warning, TEXT("SwitchState: Cannot find target"));
			return false;
		}
	}
	CurrentState = NewState;
	return true;
}

// Sets default values
AHB_Enemy_Base::AHB_Enemy_Base()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	DamageComponent = CreateDefaultSubobject<UHB_DamageComponent>(TEXT("DamageComponent"));
	
	// 设置 AI 控制器类
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

bool AHB_Enemy_Base::IsDeath()
{
	return CurrentState == EEnemyState::Death;
}

// Called when the game starts or when spawned
void AHB_Enemy_Base::BeginPlay()
{
	Super::BeginPlay();
	ENetMode NetMode=GetWorld()->GetNetMode();
	AIController = GetController<AAIController>();
	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer || NetMode == NM_Standalone)
	{
		bIsServer = true;
	}
	Health = MaxHealth;
    if (NetMode == NM_DedicatedServer|| NetMode == NM_Standalone || NetMode == NM_ListenServer)
	{
		DamageComponent->OnApplyDamage_Server.BindUObject(this, &AHB_Enemy_Base::OnServerApplyDamage);
	}
	if (NetMode == NM_Client || NetMode == NM_Standalone || NetMode == NM_ListenServer)
	{
		DamageComponent->OnApplyDamage_Client.BindUObject(this, &AHB_Enemy_Base::OnClientApplyDamage);
	}
    OnEnemyDeath.BindUObject(GetWorld()->GetSubsystem<UHB_WaveSubsystem>(), &UHB_WaveSubsystem::OnEnemyDeath);
	if(AIController)
	{
		AIController->MoveToActor(Target,AttackDistance);
		SwitchState(EEnemyState::Move);
	}
}

void AHB_Enemy_Base::OnServerApplyDamage(AActor* Attacker, float Damage)
{
    //扣血
	PreDamage += Damage;
}

void AHB_Enemy_Base::OnClientApplyDamage(AActor* Attacker, float Damage)
{
	//客户端只做动画表现
	UE_LOG(LogTemp, Log, TEXT("Client:CurrentlyHealth %lf"), Health- Damage);
}

void AHB_Enemy_Base::Death()
{
	if (CurrentState==EEnemyState::Death)
	{
		return;
	}
	CurrentState = EEnemyState::Death;
	OnEnemyDeath.ExecuteIfBound(this);
	if (!IsValid(AIController))
	{
		AIController = GetController<AAIController>();
	}
	if (IsValid(AIController) && AIController->GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		AIController->PauseMove(AIController->GetCurrentMoveRequestID());
	}
	FTimerHandle DeathTimer;
	FTimerDelegate DeathDelegate;
	DeathDelegate.BindLambda([this]()
		{
			Destroy();
		});
    GetWorld()->GetTimerManager().SetTimer(DeathTimer,DeathDelegate, DeathTime,false);
}

bool AHB_Enemy_Base::CanAttack(AActor* TargetActor)
{
	
	return false;
}

// Called every frame
void AHB_Enemy_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateHealth();
	//TODO
	if (bIsServer)
	{
		switch (CurrentState)
		{
		case EEnemyState::Move:
		{
			if (!IsValid(AIController))
			{
				AIController = GetController<AAIController>();
			}
			if (!IsValid(AIController)) break;

			const EPathFollowingStatus::Type Status = AIController->GetMoveStatus();

			if (Status == EPathFollowingStatus::Paused)
			{
				// 有暂停的路径，恢复
				AIController->ResumeMove(AIController->GetCurrentMoveRequestID());
			}
			else if (Status == EPathFollowingStatus::Idle)
			{
				// 没有路径，重新发起
				if (IsValid(Target))
				{
					AIController->MoveToActor(Target, AttackDistance);
				}
				else
				{
					// Target 没了，不该在 Move 状态
					SwitchState(EEnemyState::Idle);
				}
			}
			// Moving 什么都不做
            break;
		}
		case EEnemyState::PreAttack:
		{
			if (!IsValid(AIController))
			{
				AIController = GetController<AAIController>();
			}
			if (IsValid(AIController) && AIController->GetMoveStatus() == EPathFollowingStatus::Moving)
			{
				AIController->PauseMove(AIController->GetCurrentMoveRequestID());
			}
			if (FMath::IsNearlyEqual(CurrentAttackDelay, AttackPreDelay, KINDA_SMALL_NUMBER))
			{
				OnPreAttack();
			}
			CurrentAttackDelay -= DeltaTime;
			if (CurrentAttackDelay <= 0)
			{
				CurrentAttackDelay = AttackPostDelay;
				SwitchState(EEnemyState::Attack);
			}
			break;
		}
		case EEnemyState::Attack:
		{
			if (!IsValid(AIController))
			{
				AIController = GetController<AAIController>();
			}
			if (IsValid(AIController) && AIController->GetMoveStatus() == EPathFollowingStatus::Moving)
			{
				AIController->PauseMove(AIController->GetCurrentMoveRequestID());
			}
			OnAttack();
			if (FMath::IsNearlyEqual(AttackPostDelay, 0.0f, KINDA_SMALL_NUMBER) && FMath::IsNearlyEqual(AttackPreDelay, 0.0f, KINDA_SMALL_NUMBER))
			{
				// 双零延迟：保持 Attack 状态，Tick 每帧持续调用 OnAttack
			}
			else if (FMath::IsNearlyEqual(AttackPostDelay, 0.0f, KINDA_SMALL_NUMBER))
			{
				CurrentAttackDelay = AttackPreDelay;
				SwitchState(EEnemyState::PreAttack);
			}
			else
			{
				SwitchState(EEnemyState::PostAttack);
				CurrentAttackDelay = AttackPostDelay;
			}

			break;
		}
		case EEnemyState::PostAttack:
		{
			if (!IsValid(AIController))
			{
				AIController = GetController<AAIController>();
			}
			if (IsValid(AIController) && AIController->GetMoveStatus() == EPathFollowingStatus::Moving)
			{
				AIController->PauseMove(AIController->GetCurrentMoveRequestID());
			}
			if (FMath::IsNearlyEqual(CurrentAttackDelay, AttackPostDelay, KINDA_SMALL_NUMBER))
			{
				OnPostAttack();
			}
			CurrentAttackDelay -= DeltaTime;
			if (CurrentAttackDelay <= 0.0f)
			{
				CurrentAttackDelay = AttackPreDelay;
				SwitchState(EEnemyState::PreAttack);
			}
			break;
		}
		case EEnemyState::Death:
		{
			break;
		}
		case EEnemyState::Idle:
		{
			if (!IsValid(AIController))
			{
				AIController = GetController<AAIController>();
			}
			if (IsValid(AIController) && AIController->GetMoveStatus() == EPathFollowingStatus::Moving)
			{
				AIController->PauseMove(AIController->GetCurrentMoveRequestID());
			}
			break;
		}
		default:
			break;
		}
	}
	
}

// Called to bind functionality to input
void AHB_Enemy_Base::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AHB_Enemy_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AHB_Enemy_Base, Health);
    DOREPLIFETIME(AHB_Enemy_Base, CurrentState);
	DOREPLIFETIME(AHB_Enemy_Base, Attack);
	DOREPLIFETIME(AHB_Enemy_Base, MaxHealth);
}

void AHB_Enemy_Base::StartMove()
{
	if (bIsServer && CurrentState != EEnemyState::Move)
	{
		if (!IsValid(Target)) //后续需要添加是否死亡
		{
			return;
		}
		if (!IsValid(AIController))
		{
			AIController = GetController<AAIController>();
		}
		if (!IsValid(AIController))
		{
			return;
		}
		if (AIController->GetMoveStatus() != EPathFollowingStatus::Paused)
		{
			AIController->MoveToActor(Target, AttackDistance);
		}
        SwitchState(EEnemyState::Move);
	}
	else
	{
        UE_LOG(LogTemp, Error, TEXT("StartMove is not supported in client"));
	}
}

void AHB_Enemy_Base::StopMove()
{

	if (bIsServer && CurrentState == EEnemyState::Move)
	{
        SwitchState(EEnemyState::Idle);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StopMove is not supported in client"));
	}
}

void AHB_Enemy_Base::StartAttack()
{
	if (bIsServer && CurrentState != EEnemyState::Attack && CurrentState!=EEnemyState::PreAttack && CurrentState!=EEnemyState::PostAttack)
	{
		if (FMath::IsNearlyZero(AttackPreDelay))
		{
			SwitchState(EEnemyState::Attack);
		}
		else
		{
			CurrentAttackDelay = AttackPreDelay;
			SwitchState(EEnemyState::PreAttack);
		}

        
	}
}

void AHB_Enemy_Base::StopAttack()
{
	if (bIsServer && CurrentState == EEnemyState::Attack)
	{
        SwitchState(EEnemyState::Idle);
	}
}