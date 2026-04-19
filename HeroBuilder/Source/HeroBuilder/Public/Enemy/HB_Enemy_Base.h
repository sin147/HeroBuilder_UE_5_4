// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Component/HB_DamageComponent.h"
#include "HB_Enemy_Base.generated.h"


UCLASS()
class HEROBUILDER_API AHB_Enemy_Base : public ACharacter
{
	GENERATED_BODY()
private:
	//血量
	UPROPERTY(Replicated)
    float Health=100;
	//攻击力
	UPROPERTY(Replicated)
	float Attack=10;
    //最大血量
	UPROPERTY(EditAnywhere, Category = "Attribute")
    float MaxHealth=100;
	//是否死亡
	UPROPERTY(Replicated)
	bool bIsDead=false;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float DeathTime=3;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float AttackDistance=100;

public:
	// Sets default values for this character's properties
	AHB_Enemy_Base();
	TObjectPtr<UHB_DamageComponent> DamageComponent;
	bool IsDeath();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	//当服务器应用伤害
	void OnServerApplyDamage(AActor* Attacker, float Damage);
    //当客户端应用伤害
    void OnClientApplyDamage(AActor* Attacker, float Damage);
	//服务端死亡
	UFUNCTION(Server,Reliable)
	void Server_Death();
	//客户端死亡
	UFUNCTION(NetMulticast,Reliable)
	void NetMulticast_Death();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//移动到目标Actor
	UFUNCTION(BlueprintCallable)
	void MoveToActor(AActor* TargetActor);

    //移动到目标位置
    UFUNCTION(BlueprintCallable)
    void MoveToLocation(FVector TargetLocation);


};
