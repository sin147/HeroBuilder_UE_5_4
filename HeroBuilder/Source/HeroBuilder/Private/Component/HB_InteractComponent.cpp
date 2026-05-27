// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/HB_InteractComponent.h"
#include "HeroBuilder/HeroBuilderCharacter.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UHB_InteractComponent::UHB_InteractComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	// ...
}


// Called when the game starts
void UHB_InteractComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


void UHB_InteractComponent::OnRep_CurrentInteractMode()
{
}

void UHB_InteractComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHB_InteractComponent, InteractTarget);
	DOREPLIFETIME(UHB_InteractComponent, CurrentInteractMode);
}

// Called every frame
void UHB_InteractComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UHB_InteractComponent::SetInteractTarget(AActor* Target)
{
	InteractTarget = Target;
}

void UHB_InteractComponent::SetInteractMode(EPlayerCharacterInteractMode NewMode)
{
	CurrentInteractMode = NewMode;
}
