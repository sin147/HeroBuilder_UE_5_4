// Fill out your copyright notice in the Description page of Project Settings.
#include "Enemy/HB_Enemy_Base.h"
#include "Net/UnrealNetwork.h"
#include "Navigation/PathFollowingComponent.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/HB_EnemySubsystem.h"
#include "Components/HB_DamageComponent.h"

DEFINE_LOG_CATEGORY(LogEnemy)

FString AHB_Enemy_Base::GetStateName(EEnemyState State)
{
	switch (State)
	{
	case EEnemyState::ES_Idle: return TEXT("Idle");
	case EEnemyState::ES_Move: return TEXT("Move");
	case EEnemyState::ES_PreAttack: return TEXT("PreAttack");
	case EEnemyState::ES_Attack: return TEXT("Attack");
	case EEnemyState::ES_PostAttack: return TEXT("PostAttack");
	case EEnemyState::ES_Death: return TEXT("Death");
	default: return TEXT("Unknown");
	}
}

bool AHB_Enemy_Base::SwitchState(EEnemyState NewState)
{
	if (CurrentState == ES_Death)
	{
		return false;  // 死亡锁定，静默失败

	}
	if (CurrentState == NewState)
	{
		UE_LOG(LogEnemy, Verbose, TEXT("SwitchState: Already in state %d"), static_cast<uint8>(NewState));
		return false;  // 状态未变，静默失败
	}
	UE_LOG(LogEnemy, Log, TEXT("SwitchState %s"), *GetStateName(NewState));
	CurrentState = NewState;
	return true;
}

// Sets default values
AHB_Enemy_Base::AHB_Enemy_Base()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 设置 AI 控制器类
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 创建血量 Widget 组件
	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidget->SetupAttachment(RootComponent);
	HealthBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetDrawSize(FVector2D(150.f, 20.f));

	// 创建伤害组件
	DamageComponent = CreateDefaultSubobject<UHB_DamageComponent>(TEXT("DamageComponent"));
}

bool AHB_Enemy_Base::IsDeath()
{
	return CurrentState == ES_Death;
}

// Called when the game starts or when spawned
void AHB_Enemy_Base::BeginPlay()
{
	Super::BeginPlay();
	NetMode=GetWorld()->GetNetMode();
	AIController = GetController<AAIController>();

	// 绑定伤害组件回调
	if (DamageComponent)
	{
		DamageComponent->OnHealthChanged.AddDynamic(this, &AHB_Enemy_Base::HandleHealthChanged);
		DamageComponent->OnDeath.AddDynamic(this, &AHB_Enemy_Base::HandleDeath);
		if (HasAuthority())
		{
			DamageComponent->InitHealth(MaxHealth);
		}
	}
}
bool AHB_Enemy_Base::IsValidTarget(TObjectPtr<AActor> InBuilding)
{
	if (Cast<AHB_Building_Base>(InBuilding))
	{
		return GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->IsValidBuilding(Cast<AHB_Building_Base>(InBuilding));
	}
	return false;
}
void AHB_Enemy_Base::HandleHealthChanged(float OldHealth, float NewHealth, float MaxHealthValue, AActor* Attacker)
{
}

void AHB_Enemy_Base::HandleDeath(AActor* Attacker)
{
	SwitchState(EEnemyState::ES_Death);
	if (IsValid(AIController))
	{
		AIController->StopMovement();
	}
	OnDeath();
}
// Called every frame
void AHB_Enemy_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!HasAuthority())
	{
		return;
	}

	// 确保 AIController 有效
	if (!IsValid(AIController))
	{
		AIController = GetController<AAIController>();
		if (!IsValid(AIController))
		{
			UE_LOG(LogEnemy, Warning, TEXT("Tick: No valid AIController found"));
			return;
		}
	}
	const EPathFollowingStatus::Type Status = AIController->GetMoveStatus();
	switch (CurrentState)
	{
	case ES_Idle:
	{
		if (Status == EPathFollowingStatus::Moving)
		{
			AIController->StopMovement();
		}
		if (!IsValidTarget(Target))
		{
			if (WasFindTarget == false)
			{
				WasFindTarget = true;
				GetWorld()->GetSubsystem<UHB_EnemySubsystem>()->FindAnyVaildTarget(this);
			}
		}
		else
		{
			WasFindTarget = false;
			SwitchState(EEnemyState::ES_Move);
		}
		break;
	}
	case ES_Move:
	{
		if (!IsValidTarget(Target))
		{
			SwitchState(EEnemyState::ES_Idle);
			break;
		}
		if (FVector::Distance(GetActorLocation(), Target->GetActorLocation()) > CombatRange)
		{
			if(Status != EPathFollowingStatus::Moving)
			{
				AIController->MoveToActor(Target);
			}
		}
		else
		{
			StartAttack();
		}
		break;
	}
	case ES_PreAttack:
	{
		if (!IsValidTarget(Target))
		{
			SwitchState(EEnemyState::ES_Idle);
			break;
		}
		if (Status == EPathFollowingStatus::Moving)
		{
			AIController->StopMovement();
		}
		if (FMath::IsNearlyEqual(CurrentAttackDelay, AttackPreDelay, KINDA_SMALL_NUMBER))
		{
			OnPreAttack(Target);
		}
		CurrentAttackDelay -= DeltaTime;
		if (CurrentAttackDelay <= 0)
		{
			SwitchState(ES_Attack);
		}
		break;
	}
	case ES_Attack:
	{
		if (!IsValidTarget(Target))
		{
			SwitchState(EEnemyState::ES_Idle);
			break;
		}
		if (Status == EPathFollowingStatus::Moving)
		{
			AIController->StopMovement();
		}
		OnAttack(Target);
		if (FMath::IsNearlyEqual(AttackPostDelay, 0.0f, KINDA_SMALL_NUMBER) && FMath::IsNearlyEqual(AttackPreDelay, 0.0f, KINDA_SMALL_NUMBER))
		{
			// 双零延迟：保持 Attack 状态，Tick 每帧持续调用 OnAttack
		}
		else if (FMath::IsNearlyEqual(AttackPostDelay, 0.0f, KINDA_SMALL_NUMBER))
		{
			CurrentAttackDelay = AttackPreDelay;
			SwitchState(ES_PreAttack);
		}
		else
		{
			CurrentAttackDelay = AttackPostDelay;
			SwitchState(ES_PostAttack);
		}
		break;
	}
	case ES_PostAttack:
	{
		if (!IsValidTarget(Target))
		{
			SwitchState(EEnemyState::ES_Idle);
			break;
		}
		if (Status == EPathFollowingStatus::Moving)
		{
			AIController->StopMovement();
		}
		if (FMath::IsNearlyEqual(CurrentAttackDelay, AttackPostDelay, KINDA_SMALL_NUMBER))
		{
			OnPostAttack(Target);
		}
		CurrentAttackDelay -= DeltaTime;
		if (CurrentAttackDelay <= 0.0f)
		{
			SwitchState(ES_Move);
		}
		break;
	}
	case ES_Death:
	{
		if (DeathTime > 0)
		{
			DeathTime -= DeltaTime;
		}
		else
		{
			SetActorTickEnabled(false);
			GetWorld()->GetSubsystem<UHB_EnemySubsystem>()->DestroyEnemy(this);
		}
		break;
	}
	default:
		break;
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
	DOREPLIFETIME(AHB_Enemy_Base, Target);
}

void AHB_Enemy_Base::StartMove()
{
	if (HasAuthority())
	{
		if (CurrentState != ES_Move)
		{

			SwitchState(ES_Move);
		}
		else
		{
			UE_LOG(LogEnemy, Error, TEXT("Currently Is Move"));
		}
	}
	else
	{
        UE_LOG(LogEnemy, Error, TEXT("StartMove is not supported in client"));
	}
}

void AHB_Enemy_Base::StopMove()
{
	if (HasAuthority())
	{

		if (CurrentState == ES_Move)
		{
			SwitchState(ES_Idle);
		}
		else
		{
			UE_LOG(LogEnemy, Error, TEXT("Currently Is Not Move"));
		}
	}
	else
	{
		UE_LOG(LogEnemy, Error, TEXT("StopMove is not supported in client"));
	}
}

void AHB_Enemy_Base::StartAttack()
{
	if (HasAuthority())
	{
		if (CurrentState != ES_Attack && CurrentState != ES_PreAttack && CurrentState != ES_PostAttack)
		{
			if (FMath::IsNearlyZero(AttackPreDelay))
			{
				SwitchState(ES_Attack);
			}
			else
			{
				CurrentAttackDelay = AttackPreDelay;
				SwitchState(ES_PreAttack);
			}
		}
		else
		{
			UE_LOG(LogEnemy, Error, TEXT("Currently Is Attak"));
		}
	}
	else
	{
		UE_LOG(LogEnemy, Error, TEXT("StartAttack is not supported in client"));
	}
}

void AHB_Enemy_Base::StopAttack()
{
	if(HasAuthority())
	{
		if (CurrentState == ES_Attack || CurrentState == ES_PreAttack || CurrentState == ES_PostAttack)
		{
			SwitchState(ES_Idle);
		}
		else
		{
			UE_LOG(LogEnemy, Error, TEXT("Currently Is Not Attack"));
		}
	}
	else
	{
		UE_LOG(LogEnemy, Error, TEXT("StopAttack is not supported in client"));
	}
}

void AHB_Enemy_Base::SetTarget(TObjectPtr<AActor>InTarget)
{
	Target = InTarget;
	if (IsValid(AIController) && IsValid(InTarget))
	{
		AIController->MoveToActor(InTarget);
	}
}

void AHB_Enemy_Base::InitialEnemy(const FEnemyConfig& InConfig)
{
	UE_LOG(LogEnemy, Log, TEXT("Initializing enemy with config - Name: %s, CombatRange: %.1f, Attack: %.1f, MoveSpeed: %.1f, Health: %.1f"), 
		*InConfig.EnemyName, InConfig.CombatRange, InConfig.Attack, InConfig.MoveSpeed, InConfig.Health);
	
	// 设置战斗范围
	CombatRange = InConfig.CombatRange;
	
	// 设置攻击延迟
	AttackPreDelay = InConfig.PreAttackDelay;
	AttackPostDelay = InConfig.PostAttackDelay;
	
	// 设置移动速度
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = InConfig.MoveSpeed;
	}
	
	// 记录配置信息（可以根据需要添加更多属性设置）
	UE_LOG(LogEnemy, Log, TEXT("Enemy initialized successfully"));
}
