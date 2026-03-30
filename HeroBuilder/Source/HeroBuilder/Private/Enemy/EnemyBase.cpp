// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemyBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Sets default values
AEnemyBase::AEnemyBase()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize default values
	Health = 100.0f;
	MaxHealth = 100.0f;
	AttackDamage = 10.0f;
}

// Called when the game starts or when spawned
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyBase::ApplyDamage(float DamageAmount, AActor* DamageCauser)
{
    Health = FMath::Max(0.0f, Health - DamageAmount);
}

void AEnemyBase::Die()
{
	// TODO: Spawn death effects, drop experience and resources
	// TODO: Notify game mode about enemy death
	
	// Destroy the enemy
	Destroy();
}