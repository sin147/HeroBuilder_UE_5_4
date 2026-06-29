// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystems/HB_TotemSubsystem.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Manager/HB_TotemManager.h"
#include "Helper/HB_TotemHelper.h"

DEFINE_LOG_CATEGORY(LogTotemSubsystem);

void UHB_TotemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTotemSubsystem, Log, TEXT("HB_TotemSubsystem Initialize"));

	TotemData = LoadObject<UTotemData>(this, TEXT("/Game/Config/DA_TotemConfig"));
	if (!IsValid(TotemData))
	{
		UE_LOG(LogTotemSubsystem, Error, TEXT("Failed to load TotemData asset from /Game/Config/DA_TotemConfig"));
		UE_LOG(LogTotemSubsystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
	}
	else
	{
		UE_LOG(LogTotemSubsystem, Log, TEXT("Successfully loaded TotemData asset"));
	}
}

void UHB_TotemSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 当前波次推进逻辑由每个 AHB_Totem_Base 自行 Tick 处理
	// Subsystem 层只做全局协调（如批量查找、统计等），暂无需额外逻辑
}

AHB_TotemManager* UHB_TotemSubsystem::GetTotemManager() const
{
	return GetManager<AHB_TotemManager>();
}

AHB_TotemHelper* UHB_TotemSubsystem::GetTotemHelper() const
{
	return GetHelper<AHB_TotemHelper>();
}

AHB_Totem_Base* UHB_TotemSubsystem::SpawnTotem(TSubclassOf<AHB_Totem_Base> InTotemClass, const FTransform& SpawnTransform)
{
	// 仅服务端生成
	if (NetMode == ENetMode::NM_Client)
	{
		return nullptr;
	}

	if (!InTotemClass)
	{
		UE_LOG(LogTotemSubsystem, Warning, TEXT("SpawnTotem: null class"));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// 1. 查配置
	FTotemConfig Config;
	if (!GetConfig(InTotemClass, Config))
	{
		UE_LOG(LogTotemSubsystem, Warning, TEXT("SpawnTotem: no config found for %s"), *InTotemClass->GetName());
		return nullptr;
	}

	// 2. 生成图腾 Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AHB_Totem_Base* NewTotem = World->SpawnActor<AHB_Totem_Base>(
		InTotemClass,
		SpawnTransform,
		SpawnParams
	);
	if (!IsValid(NewTotem))
	{
		UE_LOG(LogTotemSubsystem, Error, TEXT("Failed to spawn totem: %s"), *InTotemClass->GetName());
		return nullptr;
	}

	// 3. 初始化（传入查好的配置，让 Totem 自己记录最大波次等）
	NewTotem->Initialize(Config);

	// 4. 注册到 Manager
	if (AHB_TotemManager* TotemManager = GetTotemManager())
	{
		TotemManager->AddTotem(NewTotem);
	}

	// 5. 绑定波次完成回调（供全局事件/奖励用）
	NewTotem->OnAllWavesComplete.AddDynamic(this, &UHB_TotemSubsystem::OnTotemAllWavesComplete);

	UE_LOG(LogTotemSubsystem, Log, TEXT("Spawned totem %s at %s"), *NewTotem->GetName(), *SpawnTransform.GetLocation().ToString());

	OnSpawnTotem.Broadcast(NewTotem, SpawnTransform);

	return NewTotem;
}

void UHB_TotemSubsystem::ActiveTotem(AHB_Totem_Base* InTotem)
{
	if (!IsValidTotem(InTotem))
	{
		UE_LOG(LogTotemSubsystem, Warning, TEXT("ActiveTotem: invalid totem"));
		return;
	}

	UE_LOG(LogTotemSubsystem, Log, TEXT("Activating totem %s"), *InTotem->GetName());
	InTotem->Active();
}

void UHB_TotemSubsystem::SkipPreparatory(AHB_Totem_Base* InTotem)
{
	if (!IsValidTotem(InTotem))
	{
		UE_LOG(LogTotemSubsystem, Warning, TEXT("SkipPreparatory: invalid totem"));
		return;
	}

	InTotem->SkipPreparatory();
}

TArray<TObjectPtr<AHB_Totem_Base>> UHB_TotemSubsystem::GetAllTotems() const
{
	if (AHB_TotemManager* TotemManager = GetTotemManager())
	{
		return TotemManager->GetAllTotems();
	}
	return TArray<TObjectPtr<AHB_Totem_Base>>();
}

TArray<TObjectPtr<AHB_Totem_Base>> UHB_TotemSubsystem::GetAllActiveTotems() const
{
	if (AHB_TotemManager* TotemManager = GetTotemManager())
	{
		return TotemManager->GetAllActiveTotems();
	}
	return TArray<TObjectPtr<AHB_Totem_Base>>();
}

bool UHB_TotemSubsystem::IsValidTotem(TObjectPtr<AHB_Totem_Base> InTotem) const
{
	if (AHB_TotemHelper* TotemHelper = GetTotemHelper())
	{
		return TotemHelper->IsValidTotem(InTotem);
	}
	return IsValid(InTotem);
}

bool UHB_TotemSubsystem::GetConfig(TSubclassOf<AHB_Totem_Base> TotemClass, FTotemConfig& OutConfig) const
{
	if (!TotemData)
	{
		UE_LOG(LogTotemSubsystem, Error, TEXT("GetConfig: TotemData is not valid."));
		return false;
	}

	if (TotemData->GetTotemConfig(TotemClass, OutConfig))
	{
		return true;
	}

	UE_LOG(LogTotemSubsystem, Warning, TEXT("No config found for Totem class: %s"), *TotemClass->GetName());
	return false;
}

bool UHB_TotemSubsystem::GetWaveConfig(TSubclassOf<AHB_Totem_Base> TotemClass, int32 WaveIndex, FWaveConfig& OutWaveConfig) const
{
	FTotemConfig Config;
	if (GetConfig(TotemClass, Config))
	{
		if (Config.GetWaveConfig(WaveIndex, OutWaveConfig))
		{
			return true;
		}
	}
	return false;
}

void UHB_TotemSubsystem::OnTotemAllWavesComplete(AHB_Totem_Base* InTotem)
{
	if (!IsValid(InTotem))
	{
		return;
	}

	UE_LOG(LogTotemSubsystem, Log, TEXT("Totem %s all waves complete!"), *InTotem->GetName());

	// TODO: 在此处添加全局奖励逻辑（如掉落资源、解锁新区域等）
	// 示例：
	// UHB_ResourceSubsystem* ResourceSub = GetWorld()->GetSubsystem<UHB_ResourceSubsystem>();
	// if (ResourceSub) { ResourceSub->AddResourceAmount(RT_Gold, 100); }
}