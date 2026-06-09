// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridData.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class HEROBUILDER_API UGridData : public UDataAsset
{
	GENERATED_BODY()
private:
	// 网格宽度
	UPROPERTY(EditAnywhere,SimpleDisplay="网格宽度")
	int32 GridWidth;
    UPROPERTY(EditAnywhere,SimpleDisplay="网格高度")
    int32 GridHeight;

public:
    int32 GetGridWidth() const;
    int32 GetGridHeight() const;
};
