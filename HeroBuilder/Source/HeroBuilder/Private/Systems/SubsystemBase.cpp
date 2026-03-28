// Fill out your copyright notice in the Description page of Project Settings.


#include "Systems/SubsystemBase.h"
#include "Misc/MessageDialog.h"
#include "NetworkProxy/NetworkProxyBase.h"

void USubsystemBase::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

ANetworkProxyBase* USubsystemBase::CreateNetworkProxy(APlayerController* InOwner)
{
	if(GetWorld()->IsNetMode(ENetMode::NM_Client))
	{
		return nullptr;
	}
	if (!NetworkProxy && NetworkProxyClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = InOwner;
        UE_LOG(LogTemp, Log, TEXT("%s Create NetworkProxy: %s"), *GetFName().ToString(), *NetworkProxy->GetName());
		return GetWorld()->SpawnActor<ANetworkProxyBase>(NetworkProxyClass, SpawnParams);
    }
	else
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(GetFName().ToString()+" is not set NetworkProxyClass"),
			nullptr
		);
	}
	return nullptr;
}
