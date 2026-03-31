// Fill out your copyright notice in the Description page of Project Settings.


#include "Systems/EnemySubsystem.h"
#include "NetworkProxy/EnemyProxy.h"

void UEnemySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnemyConfigData = LoadObject<UEnemyConfigData>(nullptr, TEXT("/Game/Data/EnemyConfig.EnemyConfig"));
	NetworkProxyClass = AEnemyProxy::StaticClass();
}

AEnemyBase* UEnemySubsystem::SpawnEnemyFromTransform(TSubclassOf<AEnemyBase> EnemyClass, const FTransform& Transform)
{
	//获取敌人属性
	if (EnemyConfigData)
	{
        FEnemyConfig EnemyConfig = EnemyConfigData->GetEnemyConfig(EnemyClass);
		//创建敌人
		AEnemyBase* Enemy = GetWorld()->SpawnActorDeferred<AEnemyBase>(EnemyClass,Transform);
		if (Enemy)
		{
            Enemy->SetEnemyConfig(EnemyConfig);
			Enemy->FinishSpawning(Transform);
		}
		return Enemy;
	}
	return nullptr;
}

AEnemyBase* UEnemySubsystem::SpawnEnemyByEnemyBirthPoint(TSubclassOf<AEnemyBase> EnemyClass)
{
	if (EnemyBirthPoints.IsEmpty())
	{
		UKismetSystemLibrary::PrintString(GetWorld(), "未放置地方敌方出生点");
		return nullptr;
	}
	//随机获取生成位置点
	FTransform EnemyBirthTransform= EnemyBirthPoints[FMath::RandRange(0, EnemyBirthPoints.Num()-1)]->GetActorTransform();
	return SpawnEnemyFromTransform(EnemyClass,EnemyBirthTransform);
}

AEnemyBirthPoint::AEnemyBirthPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>("ArrowComponent");
}
