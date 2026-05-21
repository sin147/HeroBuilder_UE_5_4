// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Building/HB_Building_Base.h"
#include "ConstructionData.generated.h"

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UConstructionData : public UDataAsset
{
	GENERATED_BODY()
private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<AHB_Building_Base> DefaultBuildingClass;
public:
	TSubclassOf<AHB_Building_Base> GetDefaultBuildingClass() const { return DefaultBuildingClass; }
};
