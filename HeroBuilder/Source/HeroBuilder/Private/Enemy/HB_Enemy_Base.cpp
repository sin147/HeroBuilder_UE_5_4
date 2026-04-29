// Fill out your copyright notice in the Description page of Project Settings.
#include "Enemy/HB_Enemy_Base.h"
#include "Net/Core/PropertyConditions/PropertyConditions.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/HB_WaveSubsystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "../../HeroBuilderCharacter.h"
#include "Kismet/GameplayStatics.h"

bool AHB_Enemy_Base::SwitchState(EEnemyState NewState)
{
	if (CurrentState == EEnemyState::Death)
	{
		return false;  // 死亡锁定，静默失败

	}
	if (CurrentState == NewState)
	{
		UE_LOG(LogTemp, Verbose, TEXT("SwitchState: Already in state %d"), static_cast<uint8>(NewState));
		return false;  // 状态未变，静默失败
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
	return DamageComponent->bIsDeath;
}

// Called when the game starts or when spawned
void AHB_Enemy_Base::BeginPlay()
{
	Super::BeginPlay();
	ENetMode NetMode=GetWorld()->GetNetMode();
	AIController = GetController<AAIController>();

    OnEnemyDeath.BindUObject(GetWorld()->GetSubsystem<UHB_WaveSubsystem>(), &UHB_WaveSubsystem::OnEnemyDeath);
	if (IsValid(DamageComponent))
	{
		DamageComponent->OnDeath_Server.BindUObject(this, &AHB_Enemy_Base::Death);
		if (NetMode == NM_Client || NetMode == NM_Standalone || NetMode == NM_ListenServer)
		{
			DamageComponent->OnApplyDamage_Client.BindUObject(this, &AHB_Enemy_Base::OnClientApplyDamage);
		}
	}
}

void AHB_Enemy_Base::OnClientApplyDamage(AActor* Attacker, float Damage)
{
	//客户端只做表现TODO:血量 UI 表现等
	UE_LOG(LogTemp, Log, TEXT("Client:Damage %lf"),Damage);
}
void AHB_Enemy_Base::Death()
{
	if (!HasAuthority())
	{
		return;
	}
	if (CurrentState==EEnemyState::Death)
	{
		return;
	}
	CurrentState = EEnemyState::Death;
	if (!IsValid(AIController))
	{
		AIController = GetController<AAIController>();
	}
	if (IsValid(AIController) && AIController->GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		AIController->PauseMove(AIController->GetCurrentMoveRequestID());
	}
	OnEnemyDeath.ExecuteIfBound(this);
	FTimerDelegate DeathDelegate;
	TWeakObjectPtr<AHB_Enemy_Base> WeakThis(this);
	DeathDelegate.BindLambda([WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->Destroy();
			}

		});
    GetWorld()->GetTimerManager().SetTimer(DeathTimer,DeathDelegate, DeathTime,false);
	
}

// Called every frame
void AHB_Enemy_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//TODO
	if (HasAuthority())
	{
		// 确保 AIController 有效
		if (!IsValid(AIController))
		{
			AIController = GetController<AAIController>();

			if (!IsValid(AIController))
			{
				UE_LOG(LogTemp, Warning, TEXT("Tick: No valid AIController found"));
				return;
			}
		}
		const EPathFollowingStatus::Type Status = AIController->GetMoveStatus();

		switch (CurrentState)
		{
		case EEnemyState::Move:
		{
			if (Status == EPathFollowingStatus::Paused)
			{
				// 有暂停的路径，恢复
				AIController->ResumeMove(AIController->GetCurrentMoveRequestID());
				UE_LOG(LogTemp, Log, TEXT("Resuming paused move"));
			}
			else if (Status == EPathFollowingStatus::Idle)
			{
				if(IsValid(Target))
				{
					AIController->MoveToActor(Target, CombatRange);
				}
				else
				{
					//尝试寻找新的目标TODO
                    Target = UGameplayStatics::GetActorOfClass(this, TargetClass);
                    if (IsValid(Target))
                    {
                        AIController->MoveToActor(Target, CombatRange);
                    }
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Tick Move: No valid target found"));
						// 没有目标，保持 Idle 状态，等待下一次 Tick 继续寻找
						SwitchState(EEnemyState::Idle);
					}
				}
			}
			// Moving 什么都不做
            break;
		}
		case EEnemyState::PreAttack:
		{
			if (Status == EPathFollowingStatus::Moving)
			{
				AIController->PauseMove(AIController->GetCurrentMoveRequestID());
			}
			if (FMath::IsNearlyEqual(CurrentAttackDelay, AttackPreDelay, KINDA_SMALL_NUMBER))
			{
				OnPreAttack();
				UE_LOG(LogTemp, Log, TEXT("PreAttack"));
			}
			CurrentAttackDelay -= DeltaTime;
			if (CurrentAttackDelay <= 0)
			{
				CurrentAttackDelay = AttackPostDelay;
				SwitchState(EEnemyState::Idle);
			}
			break;
		}
		case EEnemyState::Attack:
		{
            if (Status == EPathFollowingStatus::Moving)
			{
				AIController->PauseMove(AIController->GetCurrentMoveRequestID());
			}
			OnAttack();
			UE_LOG(LogTemp, Log, TEXT("Attack"));
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
			if (Status == EPathFollowingStatus::Moving)
			{
				AIController->PauseMove(AIController->GetCurrentMoveRequestID());
			}
			if (FMath::IsNearlyEqual(CurrentAttackDelay, AttackPostDelay, KINDA_SMALL_NUMBER))
			{
				OnPostAttack();
				UE_LOG(LogTemp, Log, TEXT("PostAttack"));
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
			if (Status == EPathFollowingStatus::Moving)
			{
				AIController->PauseMove(AIController->GetCurrentMoveRequestID());
				UE_LOG(LogTemp, Log, TEXT("Idle"));
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
    DOREPLIFETIME(AHB_Enemy_Base, CurrentState);
}

void AHB_Enemy_Base::StartMove()
{
	if (HasAuthority())
	{
		if (CurrentState != EEnemyState::Move)
		{

			SwitchState(EEnemyState::Move);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Currently Is Move"));
		}
	}
	else
	{
        UE_LOG(LogTemp, Error, TEXT("StartMove is not supported in client"));
	}
}

void AHB_Enemy_Base::StopMove()
{
	if (HasAuthority())
	{

		if (CurrentState == EEnemyState::Move)
		{
			SwitchState(EEnemyState::Idle);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Currently Is Not Move"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StopMove is not supported in client"));
	}
}

void AHB_Enemy_Base::StartAttack()
{
	if (HasAuthority())
	{
		//判断目标是否在攻击范围内TODO
        if (FVector::Distance(GetActorLocation(), Target->GetActorLocation()) > CombatRange)
        {
            UE_LOG(LogTemp, Error, TEXT("StartAttack: Target is out of combat range"));
			//查找范围内的目标TODO
			FVector Center = GetActorLocation();
			FVector EndLocation = Center; // SphereTraceMultiByProfile 不需要实际的结束位置，因为它会使用半径来定义范围
            FName TraceProfile = "DamageEnemy";
			TArray<FHitResult> OutHits;
			UKismetSystemLibrary::SphereTraceMultiByProfile(
				this,
				Center,
				EndLocation,
				CombatRange,
				TraceProfile, // 使用指定碰撞profile
				false, // bTraceComplex
				TArray<AActor*>(),
				EDrawDebugTrace::None, // 不绘制调试线
				OutHits,
				true, // bIgnoreSelf
				FLinearColor::Red,
				FLinearColor::Green,
                5.0f);
			if (OutHits.Num() > 0)
			{
				Target = OutHits[0].GetActor();
			}
            return;
        }

		if (CurrentState != EEnemyState::Attack && CurrentState != EEnemyState::PreAttack && CurrentState != EEnemyState::PostAttack)
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
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Currently Is Attak"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StartAttack is not supported in client"));
	}
}

void AHB_Enemy_Base::StopAttack()
{
	if(HasAuthority())
	{
		if (CurrentState == EEnemyState::Attack || CurrentState == EEnemyState::PreAttack || CurrentState == EEnemyState::PostAttack)
		{
			SwitchState(EEnemyState::Idle);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Currently Is Not Attack"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StopAttack is not supported in client"));
	}
}