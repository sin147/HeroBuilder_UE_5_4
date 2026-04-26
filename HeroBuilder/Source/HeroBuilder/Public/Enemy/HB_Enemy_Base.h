// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Component/HB_DamageComponent.h"
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"
#include "HB_Enemy_Base.generated.h"
DECLARE_DELEGATE_OneParam(FOnEnemyDeath, AHB_Enemy_Base*/*Enemy*/);

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Move UMETA(DisplayName = "Move"),
	PreAttack UMETA(DisplayName = "PreAttack"),
	Attack UMETA(DisplayName = "Attack"),
	PostAttack UMETA(DisplayName = "PostAttack"),
	Death UMETA(DisplayName = "Death")
};


UCLASS()
class HEROBUILDER_API AHB_Enemy_Base : public ACharacter
{
	GENERATED_BODY()
private:
	//血量
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float Health=100;
	//攻击力
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float Attack=10;
    //最大血量
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float MaxHealth=100;
	//是否死亡
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float DeathTime=10;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float AttackDistance=100;
	float PreDamage = 0;
	AActor* Target;
	UPROPERTY(EditAnywhere, Category = "Attribute")
    TSubclassOf<AActor> TargetClass;

	void UpdateHealth();
	TObjectPtr<AAIController> AIController;
protected:
	bool bIsServer;
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    EEnemyState CurrentState;
	bool SwitchState(EEnemyState NewState);
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
	//攻击延迟
	UPROPERTY(EditAnywhere, Category = "Attribute|Attack")
	float AttackPreDelay = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Attribute|Attack")
	float AttackPostDelay = 1.0f;
	float CurrentAttackDelay = 0.0f;

	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
    void OnPreAttack();

	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnPostAttack();
	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnAttack();
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//移动到目标Actor
	UFUNCTION(BlueprintCallable)
	void StartMove();

	UFUNCTION(BlueprintCallable)
	void StopMove();

	UFUNCTION(BlueprintCallable)
	void StartAttack();

	UFUNCTION(BlueprintCallable)
	void StopAttack();


};
