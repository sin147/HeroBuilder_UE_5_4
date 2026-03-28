// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SubsystemBase.generated.h"

/**
 * 
 */
UCLASS()
class HEROBUILDER_API USubsystemBase : public UWorldSubsystem
{
	GENERATED_BODY()
	friend class ANetworkProxyBase;
private:
	ANetworkProxyBase* NetworkProxy;
protected:
	TSubclassOf<ANetworkProxyBase> NetworkProxyClass;

	//获取网络代理
	template<typename T>
	T* GetNetworkProxy()
	{
		if(!NetworkProxy)
		{
			return nullptr;
		}
		return Cast<T>(NetworkProxy);
	}
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual ANetworkProxyBase* CreateNetworkProxy(APlayerController* InOwner);
};
