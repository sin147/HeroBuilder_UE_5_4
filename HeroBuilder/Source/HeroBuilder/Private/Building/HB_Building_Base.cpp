// Fill out your copyright notice in the Description page of Project Settings.


#include "Building/HB_Building_Base.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/HB_BuildingSubsystem.h"

void AHB_Building_Base::FindAnyValidTarget()
{
	if(IsValidTarget(*Target))
	{
		return;
	}
	GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->FindAnyVaildTarget(this);
}

// Sets default values
AHB_Building_Base::AHB_Building_Base()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	DamageComponent = CreateDefaultSubobject<UHB_DamageComponent>(TEXT("DamageComponent"));
	RotateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RotateMesh"));
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RotateMesh->SetupAttachment(RootComponent);
	BaseMesh->SetupAttachment(RootComponent);
	bIsServer = GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_Standalone || GetNetMode() == NM_ListenServer;
}

void AHB_Building_Base::SetTarget(AActor* InTarget)
{
    Target = InTarget;
}

bool AHB_Building_Base::SwitchState(EBuildingState NewState)
{
    if(CurrentState == NewState|| CurrentState == EBuildingState::Death)
	{
		return false;
	}
	CurrentState = NewState;
	return true;
}



// Called when the game starts or when spawned
void AHB_Building_Base::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_Client || GetNetMode() == NM_Standalone || GetNetMode() == NM_ListenServer)
    {

        DamageComponent->OnApplyDamage_Client.BindUObject(this, &AHB_Building_Base::OnClientApplyDamage);

    }
}

// Called every frame
void AHB_Building_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority())
	{
			FindAnyValidTarget();
			switch (CurrentState)
			{
			case EBuildingState::Idle:
				break;
			case EBuildingState::Rotate:
			{
				FVector TargetLocation = Target->GetActorLocation();
				FVector CurrentLocation = GetActorLocation();
				FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(CurrentLocation, TargetLocation);
				LookAtRotation.Pitch = 0;
				LookAtRotation.Roll = 0;
				RotateMesh->SetWorldRotation(FMath::RInterpTo(RotateMesh->GetComponentRotation(), LookAtRotation, DeltaTime, RotateSpeed));
				break;
			}
			case EBuildingState::PreAttack:
			{
				if (CurrentAttackDelay > 0)
				{
					CurrentAttackDelay -= DeltaTime;
				}
				else
				{
					OnPreAttack(Target);
					SwitchState(EBuildingState::Attack);
				}
				break;
			}
			case EBuildingState::Attack:
			{
				OnAttack(Target);
				CurrentAttackDelay = PostAttackDelay;
				SwitchState(EBuildingState::PostAttack);
				break;
			}
			case EBuildingState::PostAttack:
			{
				if (CurrentAttackDelay > 0)
				{
					CurrentAttackDelay -= DeltaTime;
				}
				else
				{
					OnPostAttack(Target);
					SwitchState(EBuildingState::Idle);
				}
				break;
			}
			case EBuildingState::Death:
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

void AHB_Building_Base::Server_Death()
{
	CurrentState = EBuildingState::Death;

}

void AHB_Building_Base::StartRotate()
{
	SwitchState(EBuildingState::Rotate);

}

void AHB_Building_Base::StopRotate()
{
	SwitchState(EBuildingState::Idle);
}

void AHB_Building_Base::StartAttack()
{
	if (SwitchState(EBuildingState::PreAttack))
	{
		CurrentAttackDelay = PreAttackDelay;
	}
}

void AHB_Building_Base::StopAttack()
{
	if (SwitchState(EBuildingState::Idle))
	{
		CurrentAttackDelay = 0;
	}
}

bool AHB_Building_Base::IsValidTarget(const AActor& InTarget) const
{
    return InTarget.GetComponentByClass<UHB_DamageComponent>() && !InTarget.GetComponentByClass<UHB_DamageComponent>()->bIsDeath && UKismetMathLibrary::Vector_Distance(GetActorLocation(), InTarget.GetActorLocation()) <= CombatRange;
}

void AHB_Building_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_Building_Base, CurrentState);
	DOREPLIFETIME(AHB_Building_Base, Target);
	DOREPLIFETIME(AHB_Building_Base, CombatRange);
}

