// Fill out your copyright notice in the Description page of Project Settings.

#include "Config/ResourceData.h"
#include "Resource/HB_Resource_Base.h"
#if WITH_EDITOR
#include "UObject/UObjectIterator.h"
#endif

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

#if WITH_EDITOR
UTexture2D* UResourceData::GetResourceTypeTexture(EResourceType ResourceType) const
{
	if (ResourceTypeTextureMap.Contains(ResourceType))
	{
		return ResourceTypeTextureMap[ResourceType];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ResourceData: GetResourceTypeTexture: ResourceType not found"));
	}
	return nullptr;
}
void UResourceData::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad | RF_NeedPostLoad))
	{
		RefreshResourceInfoMap();
		RefreshResourceTypeTextureMap();
	}
}

void UResourceData::PostLoad()
{
	Super::PostLoad();
	RefreshResourceInfoMap();
	RefreshResourceTypeTextureMap();
}

void UResourceData::RefreshResourceInfoMap()
{
	// 收集所有AHB_Resource_Base的子类（仅蓝图实例化用的真实子类）
	TSet<TSubclassOf<AHB_Resource_Base>> AllResourceClasses;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class || !Class->IsChildOf(AHB_Resource_Base::StaticClass()))
		{
			continue;
		}
		// 跳过基类自身（基类不直接实例化，仅作为子类继承根）
		if (Class == AHB_Resource_Base::StaticClass())
		{
			continue;
		}
		// 跳过抽象类、废弃类、被新版本替换的占位类、隐藏类、不可放置类
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_Hidden | CLASS_HideDropDown))
		{
			continue;
		}
		// 跳过SKEL_/REINST_/TRASHCLASS_/HOTRELOADED_等编译期/热重载占位类
		const FString ClassName = Class->GetName();
		if (ClassName.StartsWith(TEXT("SKEL_")) ||
			ClassName.StartsWith(TEXT("REINST_")) ||
			ClassName.StartsWith(TEXT("TRASHCLASS_")) ||
			ClassName.StartsWith(TEXT("HOTRELOADED_")) ||
			ClassName.StartsWith(TEXT("PLACEHOLDER-CLASS_")))
		{
			continue;
		}
		// 跳过瞬态/正在被GC的类
		if (Class->HasAnyFlags(RF_ClassDefaultObject | RF_NewerVersionExists | RF_BeginDestroyed))
		{
			continue;
		}
		AllResourceClasses.Add(Class);
	}

	bool bChanged = false;

	// 主动剔除无效/重复/异常Key
	// 1) nullptr Key
	// 2) 不在AllResourceClasses中的Key（包括抽象类、废弃类、REINST_占位类等）
	// 3) 同一UClass*的重复Key（TMap本身不允许重复，这里用TSet二次保险，
	//    防止因热重载/反序列化异常导致的多份相同Key残留）
	TSet<UClass*> SeenClasses;
	for (auto It = ResourceInfoMap.CreateIterator(); It; ++It)
	{
		UClass* KeyClass = It.Key();

		// 无效Key
		if (!KeyClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("ResourceData: 剔除无效Key(nullptr)"));
			It.RemoveCurrent();
			bChanged = true;
			continue;
		}

		// 异常/不再合法的Key
		if (!AllResourceClasses.Contains(KeyClass))
		{
			UE_LOG(LogTemp, Warning, TEXT("ResourceData: 剔除非法Key %s"), *KeyClass->GetName());
			It.RemoveCurrent();
			bChanged = true;
			continue;
		}

		// 重复Key（保险逻辑）
		bool bAlreadySeen = false;
		SeenClasses.Add(KeyClass, &bAlreadySeen);
		if (bAlreadySeen)
		{
			UE_LOG(LogTemp, Warning, TEXT("ResourceData: 剔除重复Key %s"), *KeyClass->GetName());
			It.RemoveCurrent();
			bChanged = true;
		}
	}

	// 添加新出现的子类（已存在的保留原配置）
	for (const TSubclassOf<AHB_Resource_Base>& ResourceClass : AllResourceClasses)
	{
		if (!ResourceInfoMap.Contains(ResourceClass))
		{
			ResourceInfoMap.Add(ResourceClass, FResourceConfig());
			bChanged = true;
		}
	}

	if (bChanged)
	{
		MarkPackageDirty();
	}
}

void UResourceData::RefreshResourceTypeTextureMap()
{
	UEnum* EnumPtr = StaticEnum<EResourceType>();
	if (!EnumPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResourceData: 无法获取EResourceType的UEnum"));
		return;
	}

	// 收集当前枚举的所有合法值（跳过 _MAX 哨兵）
	TSet<EResourceType> AllResourceTypes;
	const int32 NumEnums = EnumPtr->NumEnums();
	for (int32 i = 0; i < NumEnums; ++i)
	{
		// 跳过编译器自动生成的 _MAX 项
		if (EnumPtr->ContainsExistingMax() && i == NumEnums - 1)
		{
			continue;
		}
		const int64 Value = EnumPtr->GetValueByIndex(i);
		AllResourceTypes.Add(static_cast<EResourceType>(Value));
	}

	bool bChanged = false;

	// 主动剔除非法/重复Key（与ResourceInfoMap一致的清理逻辑）
	TSet<EResourceType> SeenTypes;
	for (auto It = ResourceTypeTextureMap.CreateIterator(); It; ++It)
	{
		const EResourceType KeyType = It.Key();

		// 异常/不再合法的Key
		if (!AllResourceTypes.Contains(KeyType))
		{
			UE_LOG(LogTemp, Warning, TEXT("ResourceData: 剔除非法ResourceType Key %d"), static_cast<int32>(KeyType));
			It.RemoveCurrent();
			bChanged = true;
			continue;
		}

		// 重复Key（保险逻辑）
		bool bAlreadySeen = false;
		SeenTypes.Add(KeyType, &bAlreadySeen);
		if (bAlreadySeen)
		{
			UE_LOG(LogTemp, Warning, TEXT("ResourceData: 剔除重复ResourceType Key %d"), static_cast<int32>(KeyType));
			It.RemoveCurrent();
			bChanged = true;
		}
	}

	// 添加新出现的枚举值（已存在的保留原纹理）
	for (const EResourceType& Type : AllResourceTypes)
	{
		if (!ResourceTypeTextureMap.Contains(Type))
		{
			ResourceTypeTextureMap.Add(Type, nullptr);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		MarkPackageDirty();
	}
}
#endif