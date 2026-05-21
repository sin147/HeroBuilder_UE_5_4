// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HeroBuilderGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHeroBuilderGameMode, Log, All);

class AHB_Base_Manager;
class AHB_Base_Helper;

UCLASS(minimalapi)
class AHeroBuilderGameMode : public AGameModeBase
{
	GENERATED_BODY()
private:
	TArray<TObjectPtr<AHB_Base_Manager>> Managers;
    TArray<TObjectPtr<AHB_Base_Helper>> Helpers;
public:
	AHeroBuilderGameMode();
	virtual void StartPlay() override;
	template<typename T>
    T* GetManager()
    {
        for (TObjectPtr<AHB_Base_Manager> Manager : Managers)
        {
            if (T* TypedManager = Cast<T>(Manager))
            {
                return TypedManager;
            }
        }
        return nullptr;
    }
    template<typename T>
	T* GetHelper()
	{
		for (TObjectPtr<AHB_Base_Helper> Helper : Helpers)
		{
			if (T* TypedHelper = Cast<T>(Helper))
			{
				return TypedHelper;
			}
		}
		return nullptr;
	}
};



