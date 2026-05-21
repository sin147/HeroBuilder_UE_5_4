// Fill out your copyright notice in the Description page of Project Settings.


#include "Config/BuildingData.h"

FBuildingConfig UBuildingData::GetBuildingInfoByBuildingClass(TSubclassOf<AHB_Building_Base> BuildingClass)
{
	FBuildingConfig RetBuildingConfig;
	if (BuildingInfoMap.Contains(BuildingClass))
	{
        return BuildingInfoMap[BuildingClass];
	}
    return RetBuildingConfig;
}

UStaticMesh* UBuildingData::GetPreviewMeshByBuildingClass(TSubclassOf<AHB_Building_Base> BuildingClass)
{
	UStaticMesh* RetPreviewMesh = nullptr;
	if (BuildingInfoMap.Contains(BuildingClass))
	{
        RetPreviewMesh = BuildingInfoMap[BuildingClass].PreviewMesh;
    }
    return RetPreviewMesh;
}
