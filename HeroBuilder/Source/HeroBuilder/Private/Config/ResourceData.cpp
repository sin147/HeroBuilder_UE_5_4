// Fill out your copyright notice in the Description page of Project Settings.

#include "Config/ResourceData.h"
#include "Resource/HB_Resource_Base.h"

FResourceConfig UResourceData::GetResourceInfoByResourceClass(TSubclassOf<AHB_Resource_Base> ResourceClass)
{
	if (ResourceInfoMap.Contains(ResourceClass))
	{
		return ResourceInfoMap[ResourceClass];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ResourceData: GetResourceInfoByResourceClass: ResourceClass not found"));
	}

	// 返回默认配置
	FResourceConfig DefaultConfig;
	return DefaultConfig;
}
