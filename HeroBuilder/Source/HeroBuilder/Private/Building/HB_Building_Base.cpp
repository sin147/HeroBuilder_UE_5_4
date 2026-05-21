// Fill out your copyright notice in the Description page of Project Settings.


#include "Building/HB_Building_Base.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Subsystems/HB_EnemySubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"

DEFINE_LOG_CATEGORY(LogBuilding);

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
	bIsServer = GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_Standalone || GetNetMode() == NM_ListenServer;
}

void AHB_Building_Base::InitialBuilding(FBuildingConfig InConfig)
{
	CombatRange = InConfig.CombatRange;
	Attack=InConfig.Attack;
	RotateSpeed = InConfig.RotateSpeed;
	PreAttackDelay = InConfig.PreAttackDelay;
	PostAttackDelay = InConfig.PostAttackDelay;
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
}

// Called every frame
void AHB_Building_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority())
	{
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
			if(DeathTime>0)
			{
				DeathTime -= DeltaTime;
			}
			else
			{
				SetActorTickEnabled(false);
				GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->DestroyBuilding(this);
			}
			break;
		default:
			break;
		}
	}
}

void AHB_Building_Base::OnClientApplyDamage(AActor* Attacker, float Damage)
{
	//客户端只做动画表现
    UE_LOG(LogTemp, Log, TEXT("Client:CurrentlyHealth %lf"),Damage);
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

void AHB_Building_Base::ApplyDamage(AActor* Attacker, float Damage)
{
    CurrentHealth=FMath::Max(0, CurrentHealth - Damage);
    if (CurrentHealth <= 0)
    {
		SwitchState(EBuildingState::BS_Death);
    }
}

void AHB_Building_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_Building_Base, CurrentState);
	DOREPLIFETIME(AHB_Building_Base, Target);
	DOREPLIFETIME(AHB_Building_Base, CurrentHealth);
}