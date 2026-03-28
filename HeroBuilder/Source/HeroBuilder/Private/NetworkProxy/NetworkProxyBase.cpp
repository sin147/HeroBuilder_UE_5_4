// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkProxy/NetworkProxyBase.h"
#include "Misc/MessageDialog.h"

// Sets default values
ANetworkProxyBase::ANetworkProxyBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

}

// Called when the game starts or when spawned
void ANetworkProxyBase::BeginPlay()
{
	Super::BeginPlay();
	if (Owner == GetWorld()->GetFirstPlayerController())
	{
		MountToSubsystem();
	}
	
}

// Called every frame
void ANetworkProxyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ANetworkProxyBase::MountToSubsystem()
{
	FMessageDialog::Open(
		EAppMsgType::Ok,
		FText::FromString(GetFName().ToString()+"Not Implementation Mount To Subsystem"),
		nullptr
	);
}

