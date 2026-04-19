// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_DamageSubsystem.h"
#include "Component/HB_DamageComponent.h"
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
                DamageComp->Server_ApplyDamage(DamageInfo.Attacker, DamageInfo.Damage);
            }
		}
		
	}
}
