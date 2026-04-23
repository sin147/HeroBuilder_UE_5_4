// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/HB_Enemy_Base.h"
#include "Net/Core/PropertyConditions/PropertyConditions.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/HB_WaveSubsystem.h"
#include "AIController.h"

void AHB_Enemy_Base::UpdateHealth()
{
	if (!bIsServer)
	{
		return;
	}
	Health=FMath::Clamp(Health-PreDamage,MaxHealth,0);
    PreDamage = 0.0f;
    if (Health <= 0.0f)
    {
        Death();
    }
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
	return bIsDead;
}

// Called when the game starts or when spawned
void AHB_Enemy_Base::BeginPlay()
{
	Super::BeginPlay();
	ENetMode NetMode=GetWorld()->GetNetMode();
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
	if (bIsServer)
	{
		return;
	}
	bIsDead=true;
	OnEnemyDeath.Execute(this);
	FTimerHandle DeathTimer;
	FTimerDelegate DeathDelegate;
	DeathDelegate.BindLambda([this]()
		{
			Destroy();
		});
    GetWorld()->GetTimerManager().SetTimer(DeathTimer,DeathDelegate, DeathTime,false);
}

// Called every frame
void AHB_Enemy_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateHealth();

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
	DOREPLIFETIME(AHB_Enemy_Base, bIsDead);
	DOREPLIFETIME(AHB_Enemy_Base, Attack);
}

void AHB_Enemy_Base::MoveToActor(AActor* TargetActor)
{
	ENetMode NetMode = GetWorld()->GetNetMode();
	if (NetMode == NM_DedicatedServer || NetMode == NM_Standalone || NetMode == NM_ListenServer)
	{
		AAIController* AIController = Cast<AAIController>(GetController());
		if (AIController)
		{
			AIController->MoveToActor(TargetActor, AttackDistance,false);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AIController is not valid"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MoveToActor is not supported in client"));
	}
}

void AHB_Enemy_Base::MoveToLocation(FVector TargetLocation)
{
	ENetMode NetMode = GetWorld()->GetNetMode();
	if (NetMode == NM_DedicatedServer || NetMode == NM_Standalone || NetMode == NM_ListenServer)
	{
		AAIController* AIController = Cast<AAIController>(GetController());
		if (AIController)
		{
			AIController->MoveToLocation(TargetLocation, AttackDistance);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MoveToLocation is not supported in client"));
	}
}

