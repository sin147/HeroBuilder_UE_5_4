// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Component/HB_DamageComponent.h"
#include "HB_Enemy_Base.generated.h"
DECLARE_DELEGATE_OneParam(FOnEnemyDeath, AHB_Enemy_Base*/*Enemy*/);

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Move UMETA(DisplayName = "Move"),
	Attack UMETA(DisplayName = "Attack"),
	Death UMETA(DisplayName = "Death")
};


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
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess=true))
	bool bIsDead=false;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float DeathTime=10;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float AttackDistance=100;
	float PreDamage = 0;
	void UpdateHealth();
	TObjectPtr<AAIController> AIController;
protected:
	bool bIsServer;
    UPROPERTY(BlueprintReadOnly, Category = "Enemy State")
    EEnemyState CurrentState;
public:
	// Sets default values for this character's properties
	AHB_Enemy_Base();
	TObjectPtr<UHB_DamageComponent> DamageComponent;
	bool IsDeath();
	FOnEnemyDeath OnEnemyDeath;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	//当服务器应用伤害
	void OnServerApplyDamage(AActor* Attacker, float Damage);
    //当客户端应用伤害
    void OnClientApplyDamage(AActor* Attacker, float Damage);
	//服务端死亡
	void Death();
	//可以攻击
	bool CanAttack(AActor* TargetActor);
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

	UFUNCTION(BlueprintCallable)
	void StopMove();

	UFUNCTION(BlueprintCallable)
	void AttackToActor(AActor* TargetActor);

	UFUNCTION(BlueprintCallable)
    void AttackToLocation(FVector TargetLocation);


};
