// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Config/InteractData.h"
#include "Components/HB_DamageComponent.h"
#include "HeroBuilderCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogPlayerCharacter, Log, All);


UENUM(BlueprintType)
enum EPlayerCharacterState :uint8
{
	EPCS_None UMETA(DisplayName = "无"),
	EPCS_Idle UMETA(DisplayName = "空闲"),
	EPCS_Move UMETA(DisplayName = "移动"),
	EPCS_MoveToTarget UMETA(DisplayName = "追击目标"),
	EPCS_PreInteract UMETA(DisplayName = "交互前摇"),
	EPCS_Interact UMETA(DisplayName = "交互"),
    EPCS_PostInteract UMETA(DisplayName = "交互后摇"),
};

UCLASS(config=Game)
class AHeroBuilderCharacter : public ACharacter
{
	GENERATED_BODY()
	UPROPERTY(Replicated)
	TEnumAsByte<EPlayerCharacterState> CurrentlyState = EPCS_Idle;

	//当前交互目标（服务端权威，复制到客户端）
	UPROPERTY(Replicated)
	TObjectPtr<AActor> InteractTarget;

	//当前玩家交互模式（服务端权威，复制到客户端）
	UPROPERTY(ReplicatedUsing = OnRep_CurrentInteractMode)
	TEnumAsByte<EPlayerCharacterInteractMode> CurrentInteractMode = IM_Normal;

	UFUNCTION()
	void OnRep_CurrentInteractMode();

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ChangeConstructionModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	//交互前摇时间
	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = Interact, meta = (AllowPrivateAccess = "true"))
	float PreInteractDelay = 0.3f;
	//交互后摇时间
    UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = Interact, meta = (AllowPrivateAccess = "true"))
	float PostInteractDelay = 0.3f;
	//内部用于前/后摇倒计时
	float CurrentInteractDelay = 0.f;

	//内部标记：当前是否由系统(追击)驱动调用Move()，用于绕过Move()里的状态屏蔽
	bool bInternalDrivenMove = false;

	//角色攻击力（一次交互/攻击对目标造成的伤害量），服务端权威并复制到客户端
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float Attack = 10.f;
	//攻击范围
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float InteractRange = 100.f;

	UFUNCTION(Server,Reliable)
	void Server_Attack();
	void TickUpdateState(float DeltaTime);
	//以下两个函数仅在服务端权威环境下调用（由SwitchState统一把关），无需走Server RPC
	void OnEnterState(EPlayerCharacterState EnterState);
	void OnLeaveState(EPlayerCharacterState LeaveState);
	//根据当前是否需要追击目标，进入MoveToTarget或PreInteract（仅服务端调用）
	void BeginInteractFlow();
	//处于MoveToTarget时的每帧追击逻辑：仅在自治代理客户端调用，
	//只产生AddMovementInput的位移驱动（走CharacterMovement标准预测管线），
	//不调用SwitchState/AbortInteract等任何会改变Replicated属性的权威逻辑。
	void TickMoveToTarget(float DeltaTime);
	//服务端权威地推进MoveToTarget：到达交互范围则切PreInteract，目标失效则中止
	void TickAuthorityMoveToTarget(float DeltaTime);

#if WITH_SERVER_CODE
	//服务端权威：状态机推进（含SwitchState、Server_TryInteract等副作用）
	//仅在含服务端代码的构建里编入（DedicatedServer / Editor / Game(含Listen Server) 都会编入；纯Client-only Target不编入）
	void Tick_AuthorityState(float DeltaTime);
#endif

#if !UE_SERVER
	//客户端本地：仅做视觉/预测处理，绝不触发权威逻辑
	//仅在非DedicatedServer构建里编入
	void Tick_LocalCosmetic(float DeltaTime);
#endif
public:
	AHeroBuilderCharacter();
	void SwitchState(EPlayerCharacterState NewState);
	//中止当前正在进行的交互流程（前摇/交互/后摇），将角色拉回Idle
	void AbortInteract();
    void SetPreInteractDelay(float Delay);
    void SetPostInteractDelay(float Delay);
	UFUNCTION(BlueprintPure)
	TEnumAsByte<EPlayerCharacterState> GetCurrentState() const { return CurrentlyState; }

	//交互目标访问
	void SetInteractTarget(AActor* Target) { InteractTarget = Target; }
	AActor* GetInteractTarget() const { return InteractTarget; }
	//判断目标是否有效（非空、未销毁，且若是Resource/Building/Enemy则未死亡）
	bool IsValidTarget(AActor* Target);

	//交互模式访问
	void SetInteractMode(EPlayerCharacterInteractMode NewMode) { CurrentInteractMode = NewMode; }
	EPlayerCharacterInteractMode GetInteractMode() const { return CurrentInteractMode; }

	//攻击力访问
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetAttack() const;
	void SetAttack(float NewAttack);

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			
	void ChangeConstructionMode(const FInputActionValue& Value);

	void Interact(const FInputActionValue& Value);
	//\u4ea4\u4e92\u952e\u62ac\u8d77\u65f6\u7684\u8f93\u5165\u56de\u8c03\uff1a\u8d1f\u8d23\u8def\u7531\u5230\u670d\u52a1\u7aef\u4e2d\u6b62\u4ea4\u4e92\u6d41\u7a0b
	void OnInteractReleased(const FInputActionValue& Value);

	//请求服务端切换交互模式
	UFUNCTION(Server, Reliable)
	void Server_SwitchInteractMode(uint8 NewMode);
	//请求服务端触发交互
	UFUNCTION(Server, Reliable)
	void Server_TryInteract();
	//请求服务端开启一次交互流程（按下交互键时由客户端发起）
	UFUNCTION(Server, Reliable)
	void Server_BeginInteract();
	//请求服务端中止当前交互流程（抬起交互键时由客户端发起）
	UFUNCTION(Server, Reliable)
	void Server_AbortInteract();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FVector GetFollowCameraForward();
};

