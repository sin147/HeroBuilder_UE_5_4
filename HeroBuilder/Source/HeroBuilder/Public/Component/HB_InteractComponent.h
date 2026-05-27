// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Config/InteractData.h"
#include "HB_InteractComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HEROBUILDER_API UHB_InteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHB_InteractComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
private:
	UPROPERTY(Replicated)
	TObjectPtr<AActor> InteractTarget;
	//当前玩家交互模式（服务端权威，复制到客户端）
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_CurrentInteractMode)
	TEnumAsByte<EPlayerCharacterInteractMode> CurrentInteractMode = IM_Normal;
	UFUNCTION()
	void OnRep_CurrentInteractMode();
	// Called to bind functionality to input
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void SetInteractTarget(AActor* Target);
	AActor* GetInteractTarget() const { return InteractTarget; }
	//设置/获取当前交互模式
	void SetInteractMode(EPlayerCharacterInteractMode NewMode);
	EPlayerCharacterInteractMode GetInteractMode() const { return CurrentInteractMode; }
};
