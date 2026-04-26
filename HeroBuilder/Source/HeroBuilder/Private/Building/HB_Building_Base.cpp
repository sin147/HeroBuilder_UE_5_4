// Fill out your copyright notice in the Description page of Project Settings.


#include "Building/HB_Building_Base.h"

// Sets default values
AHB_Building_Base::AHB_Building_Base()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	DamageComponent = CreateDefaultSubobject<UHB_DamageComponent>(TEXT("DamageComponent"));
	bIsServer = GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_Standalone || GetNetMode() == NM_ListenServer;
}

// Called when the game starts or when spawned
void AHB_Building_Base::BeginPlay()
{
	Super::BeginPlay();
	IsValid()
	if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_Standalone || GetNetMode() == NM_ListenServer)
	{
		DamageComponent->OnApplyDamage_Server.BindUObject(this, &AHB_Building_Base::OnServerApplyDamage);
	}
	if (GetNetMode() == NM_Client || GetNetMode() == NM_Standalone || GetNetMode() == NM_ListenServer)
    {

        DamageComponent->OnApplyDamage_Client.BindUObject(this, &AHB_Building_Base::OnClientApplyDamage);

    }
}

// Called every frame
void AHB_Building_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	switch (EBuildingState)
	{
	case EBuildingState::Idle:
		break;
	case EBuildingState::Rotate:
		break;
	case EBuildingState::PreAttack:
		break;
	case EBuildingState::Attack:
		break;
	case EBuildingState::PostAttack:
		break;
	case EBuildingState::Death:
		break;
	default:
		break;
	}

}

void AHB_Building_Base::OnServerApplyDamage(AActor* Attacker, float Damage)
{
	//扣血
	PreDamage += Damage;
}

void AHB_Building_Base::OnClientApplyDamage(AActor* Attacker, float Damage)
{
	//客户端只做动画表现
	UE_LOG(LogTemp, Log, TEXT("Client:CurrentlyHealth %lf"), Health- Damage);
}

void AHB_Building_Base::RotateToTarget()
{
	
}

void AHB_Building_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_Building_Base, Health);
	DOREPLIFETIME(AHB_Building_Base, MaxHealth);
	DOREPLIFETIME(AHB_Building_Base, Attack);
}

