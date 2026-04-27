// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_DamageSubsystem.h"
#include "Component/HB_DamageComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"	

DEFINE_LOG_CATEGORY(LogDamageSubsystem)
void UHB_DamageSubsystem::TakeDamage(AActor* Attacker, float Damage, AActor* Target)
{
	if(Target->GetComponentByClass(UHB_DamageComponent::StaticClass()))
	{
		DamageQueue.Enqueue(FDamageInfo(Attacker, Damage, Target));
	}
	else
	{
		UE_LOG(LogDamageSubsystem, Error, TEXT("Target %s has no DamageComponent"), *Target->GetName());
	}
}

void UHB_DamageSubsystem::TakeBoxRangeDamage(AActor* Attacker, float Damage, FVector StartLocation, FVector EndLocation, float Width, EDamageTargetType Target)
{
	// 计算盒形长度（StartLocation到EndLocation的距离）
	float Length = FVector::Distance(StartLocation, EndLocation);
	
	// 计算盒形范围
	FVector HalfSize(Length * 0.5f, Width * 0.5f, Width * 0.5f);
	
	// 计算从StartLocation到EndLocation的方向旋转（水平夹角）
	FRotator BoxRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, EndLocation);
	
	// 只保留Yaw旋转（水平旋转）
	BoxRotation.Pitch = 0;
	BoxRotation.Roll = 0;
	
	// 使用BoxTraceMultiByProfile检测盒形范围内的所有碰撞
	TArray<FHitResult> OutHits;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Attacker); // 忽略攻击者自身
	
	FName TraceProfile = NAME_None;

	switch (Target)
	{
	case EDamageTargetType::Player:
		TraceProfile = FName("DamagePlayer"); // 使用Player碰撞profile
		break;
	case EDamageTargetType::Enemy:
        TraceProfile = FName("DamageEnemy"); // 使用Enemy碰撞profile
		break;
	case EDamageTargetType::Environment:
        TraceProfile = FName("DamageEnvironment"); // 使用Environment碰撞profile
		break;
	default:
		break;
	}


	if (UKismetSystemLibrary::BoxTraceMultiByProfile(
		this, 
		StartLocation, 
		EndLocation, 
		HalfSize, 
		BoxRotation, 
        TraceProfile, // 使用指定碰撞profile
		false, // bTraceComplex
		ActorsToIgnore, 
		EDrawDebugTrace::None, // 不绘制调试线
		OutHits, 
		true, // bIgnoreSelf
		FLinearColor::Red, 
		FLinearColor::Green, 
		5.0f))
	{
		// 对每个命中的Actor应用伤害
		for (const FHitResult& Hit : OutHits)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor != Attacker)
			{
				TakeDamage(Attacker, Damage, HitActor);
			}
		}
	}
}

void UHB_DamageSubsystem::TakeSphereRangeDamage(AActor* Attacker, float Damage, FVector Center, float Radius, EDamageTargetType Target)
{
	// 使用SphereTraceMultiByProfile检测球形范围内的所有碰撞
	TArray<FHitResult> OutHits;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Attacker); // 忽略攻击者自身
	
	// 创建一个从中心点向上稍微偏移的终点，以确保球形检测
	FVector EndLocation = Center + FVector(0, 0, 1.0f);

	FName TraceProfile = NAME_None;

	switch (Target)
	{
	case EDamageTargetType::Player:
        TraceProfile = FName("DamagePlayer"); // 使用Player碰撞profile
		break;
	case EDamageTargetType::Enemy:
        TraceProfile = FName("DamageEnemy"); // 使用Enemy碰撞profile
		break;
	case EDamageTargetType::Environment:
        TraceProfile = FName("DamageEnvironment"); // 使用Environment碰撞profile
		break;
	default:
		break;
	}
	
	if (UKismetSystemLibrary::SphereTraceMultiByProfile(
		this, 
		Center, 
		EndLocation, 
		Radius, 
        TraceProfile, // 使用指定碰撞profile
		false, // bTraceComplex
		ActorsToIgnore, 
		EDrawDebugTrace::None, // 不绘制调试线
		OutHits, 
		true, // bIgnoreSelf
		FLinearColor::Red, 
		FLinearColor::Green, 
		5.0f))
	{
		// 对每个命中的Actor应用伤害
		for (const FHitResult& Hit : OutHits)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor != Attacker)
			{
				TakeDamage(Attacker, Damage, HitActor);
			}
		}
	}
}

void UHB_DamageSubsystem::Tick(float DeltaTime)
{
	if(!DamageQueue.IsEmpty())
	{

		FDamageInfo DamageInfo;
		if (DamageQueue.Dequeue(DamageInfo))
		{
			UHB_DamageComponent* DamageComp = Cast<UHB_DamageComponent>(DamageInfo.Target->GetComponentByClass(UHB_DamageComponent::StaticClass()));
            if (DamageComp)
            {
                DamageComp->ApplyDamage(DamageInfo.Attacker, DamageInfo.Damage);
            }
		}
		
	}
}