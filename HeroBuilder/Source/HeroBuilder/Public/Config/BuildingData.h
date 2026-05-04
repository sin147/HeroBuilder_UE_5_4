// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Building/HB_Building_Base.h"
#include "BuildingData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FBuildingInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building")
	FString BuildingName;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building")
    TObjectPtr<UStaticMesh> PreviewMesh;

};


UCLASS()
class HEROBUILDER_API UBuildingData : public UDataAsset
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, Category = "Building")
	TMap<TSubclassOf<AHB_Building_Base>,FBuildingInfo> BuildingInfoMap;
public:
    FBuildingInfo GetBuildingInfoByBuildingClass(TSubclassOf<AHB_Building_Base> BuildingClass);
	UStaticMesh* GetPreviewMeshByBuildingClass(TSubclassOf<AHB_Building_Base> BuildingClass);
};
