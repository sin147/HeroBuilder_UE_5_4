// Fill out your copyright notice in the Description page of Project Settings.


#include "Building/HB_Building_Base.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Subsystems/HB_EnemySubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Components/HB_DamageComponent.h"
#include "Components/HB_InteractComponent.h"

DEFINE_LOG_CATEGORY(LogBuilding);

void AHB_Building_Base::OnRep_Rotation()
{
	RotateMesh->SetWorldRotation(RotateMeshRotation);
}

// Sets default values
AHB_Building_Base::AHB_Building_Base()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;
	RotateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RotateMesh"));
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RotateMesh->SetupAttachment(RootComponent);
	BaseMesh->SetupAttachment(RootComponent);

	// 创建血量 Widget 组件
	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidget->SetupAttachment(RootComponent);
	HealthBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetDrawSize(FVector2D(150.f, 20.f));

	// 创建伤害组件
	DamageComponent = CreateDefaultSubobject<UHB_DamageComponent>(TEXT("DamageComponent"));

	// 创建交互组件：建筑默认作为可被攻击的目标，玩家靠近时切为 IT_Attack
	InteractComponent = CreateDefaultSubobject<UHB_InteractComponent>(TEXT("InteractComponent"));
	if (InteractComponent)
	{
		InteractComponent->SetInteractType(IT_Attack);
	}
}

void AHB_Building_Base::TickBuildingState(float DeltaTime)
{
    if (HasAuthority())
	{
		if (CurrentState == EBuildingState::BS_Death)
		{
			// 待修建：不进行状态机推进（不取消 Tick，避免复活后需要重新开启）
			return;
		}
		if(!IsValidTarget(Target))
		{
			SwitchState(EBuildingState::BS_Idle);
		}

		switch (CurrentState)
		{
		case EBuildingState::BS_Idle:
		{
			if (!IsValidTarget(Target))
			{
				if (WasFindTarget == false)
				{
					WasFindTarget = true;
					GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->FindAnyVaildTarget(this);
				}
			}
			else
			{
				WasFindTarget = false;
				SwitchState(EBuildingState::BS_Rotate);
			}
			break;
		}
		case EBuildingState::BS_Rotate:
		{
			if (!IsValidTarget(Target))
			{
				SwitchState(EBuildingState::BS_Idle);
				break;
			}
				
			// 距离检查，避免对过远目标进行旋转计算
			float Distance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());
			if (Distance > CombatRange) 
			{
				break;
			}
			FVector TargetLocation = Target->GetActorLocation();
			FVector CurrentLocation = GetActorLocation();
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(CurrentLocation, TargetLocation);
			LookAtRotation.Pitch = 0;
			LookAtRotation.Roll = 0;
				
			// 计算当前旋转与目标旋转的角度差（处理360度环绕）
			FRotator CurrentRotation = RotateMesh->GetComponentRotation();
			float AngleDifference = FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, LookAtRotation.Yaw);
			AngleDifference = FMath::Abs(AngleDifference);
				
			// 动态调整旋转速度：角度差越大速度越快，越小速度越慢
			float DynamicRotateSpeed = RotateSpeed;
			if (AngleDifference < 30.0f) // 当角度差小于30度时减速
			{
				DynamicRotateSpeed = RotateSpeed * FMath::Clamp(AngleDifference / 30.0f, 0.1f, 1.0f);
			}
				
			RotateMesh->SetWorldRotation(FMath::RInterpTo(CurrentRotation, LookAtRotation, DeltaTime, DynamicRotateSpeed));
			RotateMeshRotation = RotateMesh->GetComponentRotation();
			// 计算朝向相似度，大于MIN_ATTACK_ANGLE时开始攻击
			FVector CurrentForward = RotateMesh->GetForwardVector();
			CurrentForward.Z = 0; // 忽略Z轴分量
			CurrentForward.Normalize();
				
			FVector TargetDirection = (TargetLocation - CurrentLocation).GetSafeNormal();
			TargetDirection.Z = 0; // 同样忽略Z轴分量
			TargetDirection.Normalize();
				
			float DotProduct = FVector::DotProduct(CurrentForward, TargetDirection);
				
			if (DotProduct > MIN_ATTACK_ANGLE)
			{
				StartAttack();
			}
			break;
		}
		case EBuildingState::BS_PreAttack:
		{
			if (!IsValidTarget(Target))
			{
				SwitchState(EBuildingState::BS_Idle);
				break;
			}
			if (CurrentAttackDelay > 0)
			{
				CurrentAttackDelay -= DeltaTime;
			}
			else
			{
				OnPreAttack(Target);
				SwitchState(EBuildingState::BS_Attack);
			}
			break;
		}
		case EBuildingState::BS_Attack:
		{
			if (!IsValidTarget(Target))
			{
				SwitchState(EBuildingState::BS_Idle);
				break;
			}
			OnAttack(Target);
			CurrentAttackDelay = PostAttackDelay;
			SwitchState(EBuildingState::BS_PostAttack);
			break;
		}
		case EBuildingState::BS_PostAttack:
		{
			if (CurrentAttackDelay > 0)
			{
				CurrentAttackDelay -= DeltaTime;
			}
			else
			{
				OnPostAttack(Target);
				// 攻击完成后直接重新开始攻击循环
				if (IsValidTarget(Target))
				{
					SwitchState(EBuildingState::BS_Rotate);
				}
				else
				{
					SwitchState(EBuildingState::BS_Idle);
				}
			}
			break;
		}
		case EBuildingState::BS_Death:
		{
			// 这里不再 SetActorTickEnabled(false)：保留 Tick 以支持“被修建后复活”交互。
			// 实际状态机在函数顶部已提前 return，这里不会被达到。
			break;
		}
		default:
			break;
		}
	}
}

void AHB_Building_Base::InitialBuilding(FBuildingConfig InConfig)
{
	CombatRange = InConfig.CombatRange;
	Attack=InConfig.Attack;
	RotateSpeed = InConfig.RotateSpeed;
	PreAttackDelay = InConfig.PreAttackDelay;
	PostAttackDelay = InConfig.PostAttackDelay;

	//运行时兜底：与 PostInitializeComponents 保持一致策略，避免蓝图子类删除继承组件后此处静默跳过
	if (!DamageComponent)
	{
		DamageComponent = FindComponentByClass<UHB_DamageComponent>();
	}
	if (!InteractComponent)
	{
		InteractComponent = FindComponentByClass<UHB_InteractComponent>();
	}

	//最大血量直接落到 DamageComponent，由组件管理（仅服务端需要写入，组件内部会校验 Authority）
	if (DamageComponent)
	{
		DamageComponent->InitHealth(InConfig.MaxHealth);
	}
	else
	{
		UE_LOG(LogBuilding, Error, TEXT("[%s] InitialBuilding: DamageComponent missing, MaxHealth=%.1f not applied"), *GetName(), InConfig.MaxHealth);
	}

	//将配置中的交互类型应用到 InteractComponent，覆盖构造函数中的默认值（IT_Attack）
	if (InteractComponent)
	{
		InteractComponent->SetInteractType(InConfig.InteractType);
	}
	else
	{
		UE_LOG(LogBuilding, Error, TEXT("[%s] InitialBuilding: InteractComponent missing, InteractType not applied"), *GetName());
	}
}

void AHB_Building_Base::SetTarget(AActor* InTarget)
{
    if (IsValidTarget(InTarget) || InTarget == nullptr)
    {
        Target = InTarget;
    }
}

FString AHB_Building_Base::GetStateName(EBuildingState State)
{
	switch (State)
	{
	case EBuildingState::BS_Idle: return TEXT("Idle");
	case EBuildingState::BS_Rotate: return TEXT("Rotate");
	case EBuildingState::BS_PreAttack: return TEXT("PreAttack");
	case EBuildingState::BS_Attack: return TEXT("Attack");
	case EBuildingState::BS_PostAttack: return TEXT("PostAttack");
	case EBuildingState::BS_Death: return TEXT("Death");
	default: return TEXT("Unknown");
	}
}

bool AHB_Building_Base::SwitchState(EBuildingState NewState)
{
    if(CurrentState == NewState|| CurrentState == EBuildingState::BS_Death)
	{
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("SwitchState %s"), *GetStateName(NewState));
	CurrentState = NewState;
	return true;
}



// Called when the game starts or when spawned
void AHB_Building_Base::BeginPlay()
{
	Super::BeginPlay();
	//委托绑定已迁移至 PostInitializeComponents（更早、且不受蓝图 BeginPlay 是否调 Parent 影响）
}

void AHB_Building_Base::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	//运行时兜底：蓝图子类若把继承的 DamageComponent / InteractComponent 删除或覆盖，
	//C++ 指针在 CDO 后会被写回 nullptr，但实例仍可能以匿名子对象形式存在，按类型查找拿回。
	if (!DamageComponent)
	{
		DamageComponent = FindComponentByClass<UHB_DamageComponent>();
	}
	if (!InteractComponent)
	{
		InteractComponent = FindComponentByClass<UHB_InteractComponent>();
	}

	if (DamageComponent)
	{
		DamageComponent->OnHealthChanged.AddDynamic(this, &AHB_Building_Base::HandleHealthChanged);
		DamageComponent->OnDeath.AddDynamic(this, &AHB_Building_Base::HandleDeath);
		DamageComponent->OnRevive.AddDynamic(this, &AHB_Building_Base::HandleRevive);
	}
	else
	{
		UE_LOG(LogBuilding, Error, TEXT("[%s] DamageComponent missing in both CDO and runtime, health callbacks will NOT fire"), *GetName());
	}
}

// Called every frame
void AHB_Building_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickBuildingState(DeltaTime);
}

void AHB_Building_Base::HandleHealthChanged(float OldHealth, float NewHealth, float MaxHealthValue, AActor* Attacker)
{
	// 调用蓝图可重写的 OnHealthChanged 接口
	OnHealthChanged(OldHealth, NewHealth, MaxHealthValue, Attacker);
}

bool AHB_Building_Base::IsAwaitingConstruction() const
{
	return CurrentState == EBuildingState::BS_Death;
}

void AHB_Building_Base::HealAsConstruction(float Amount, AActor* Healer)
{
	if (!HasAuthority() || !DamageComponent || Amount <= 0.f)
	{
		return;
	}
	// 仅“待修建”状态下生效；正常存活的建筑不接受这种交互治疗
	if (!IsAwaitingConstruction())
	{
		return;
	}
	// DamageComponent 被 ApplyDamage 击杀后 bIsDead=true，Heal 会被锁，需先 Revive 重置状态
	if (DamageComponent->IsDead())
	{
		DamageComponent->Revive();
	}
}

bool AHB_Building_Base::IsDeath()
{
    return CurrentState == EBuildingState::BS_Death;
}
float AHB_Building_Base::GetCombatRange() const
{
    return CombatRange;
}

void AHB_Building_Base::StartRotate()
{
	SwitchState(EBuildingState::BS_Rotate);

}

void AHB_Building_Base::StopRotate()
{
	SwitchState(EBuildingState::BS_Idle);
}

void AHB_Building_Base::StartAttack()
{
	if (SwitchState(EBuildingState::BS_PreAttack))
	{
		CurrentAttackDelay = PreAttackDelay;
	}
}

void AHB_Building_Base::StopAttack()
{
	if (SwitchState(EBuildingState::BS_Idle))
	{
		CurrentAttackDelay = 0;
	}
}

bool AHB_Building_Base::IsValidTarget(AActor* InTarget) const
{
	if (Cast<AHB_Enemy_Base>(InTarget))
	{
		return GetWorld()->GetSubsystem<UHB_EnemySubsystem>()->IsValidEnemy(Cast<AHB_Enemy_Base>(InTarget));
	}

	return false;
}

void AHB_Building_Base::HandleDeath()
{
	SwitchState(EBuildingState::BS_Death);
	// 建筑“死亡”在本项目中的语义 = “待修建”：仍然保持可交互，但交互类型切为 IT_Construction，
	// 让玩家在 IM_Normal 下靠近时切到建造交互、通过 TryInteract 进行治疗（HealAsConstruction）。
	if (InteractComponent)
	{
		InteractComponent->SetInteractType(IT_Construction);
		InteractComponent->SetIsInteractable(true);
	}
	OnDeath();
}
void AHB_Building_Base::HandleRevive()
{
	SwitchState(EBuildingState::BS_Idle);
	if (InteractComponent)
	{
		CurrentState = EBuildingState::BS_Idle; // 绕开 SwitchState 的“BS_Death 锁定”
		WasFindTarget = false;
		CurrentAttackDelay = 0.f;
		InteractComponent->SetInteractType(IT_Construction);
		InteractComponent->SetIsInteractable(false);
        UE_LOG(LogBuilding, Log, TEXT("%s revived from BS_Death to BS_Idle (Health=%.1f/%.1f)"), *GetName(), DamageComponent->GetCurrentHealth(), DamageComponent->GetMaxHealth());
	}
	OnRevive();
}

void AHB_Building_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_Building_Base, CurrentState);
	DOREPLIFETIME(AHB_Building_Base, Target);
    DOREPLIFETIME(AHB_Building_Base, RotateMeshRotation);
	DOREPLIFETIME(AHB_Building_Base, RotateSpeed);
}