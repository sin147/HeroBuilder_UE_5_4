// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeroBuilderCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "Subsystems/HB_InteractSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogPlayerCharacter);

//////////////////////////////////////////////////////////////////////////
// AHeroBuilderCharacter

void AHeroBuilderCharacter::Server_Attack_Implementation()
{

}

void AHeroBuilderCharacter::TickUpdateState(float DeltaTime)
{
	//处于交互流程中（含追击目标）：交互期间已经在Move()里禁止了移动输入，这里无需再做任何状态切换，直接保持当前状态
	if (CurrentlyState == EPCS_MoveToTarget ||
		CurrentlyState == EPCS_PreInteract  ||
		CurrentlyState == EPCS_Interact     ||
		CurrentlyState == EPCS_PostInteract)
	{
		return;
	}

	if(!GetVelocity().IsNearlyZero())
	{
		SwitchState(EPCS_Move);
	}
	else
	{
		if (CurrentlyState == EPCS_Move)
		{
			SwitchState(EPCS_Idle);
		}
	}

}

void AHeroBuilderCharacter::OnEnterState(EPlayerCharacterState EnterState)
{
	//先把状态正式设置为目标值（保证OnRep只通知一次最终值），再决定是否需要进一步跳转
	CurrentlyState = EnterState;
	const FString StateName = StaticEnum<EPlayerCharacterState>()->GetNameStringByValue((int64)EnterState);
	UE_LOG(LogPlayerCharacter, Log, TEXT("'%s' Enter State '%s'!"), *GetNameSafe(this), *StateName);

	switch (EnterState)
	{
	case EPCS_None:
		break;
	case EPCS_Idle:
		break;
	case EPCS_Move:
		break;
	case EPCS_MoveToTarget:
		//追击目标进入态：什么都不做，由Tick驱动追击
		break;
	case EPCS_PreInteract:
	{
		if (PreInteractDelay > 0.f)
		{
			CurrentInteractDelay = PreInteractDelay;
		}
		else
		{
			//无前摇：通过SwitchState正常进入Interact，保证Leave/Enter链完整
			SwitchState(EPCS_Interact);
		}
		break;
	}
	case EPCS_Interact:
	{
		//进入交互帧：只触发一次交互，随即进入后摇。
		//后续即使玩家仍按住交互键也不会重复触发，需要抬起后重新按下才会再次开启一轮流程。
		Server_TryInteract();
		SwitchState(EPCS_PostInteract);
		break;
	}
	case EPCS_PostInteract:
	{
		if (PostInteractDelay > 0.f)
		{
			CurrentInteractDelay = PostInteractDelay;
		}
		else
		{
			SwitchState(EPCS_PreInteract);
		}
		break;
	}
	default:
		break;
	}
}

void AHeroBuilderCharacter::OnLeaveState(EPlayerCharacterState LeaveState)
{
	switch (LeaveState)
	{
	case EPCS_None:
		break;
	case EPCS_Idle:
		break;
	case EPCS_Move:
	{
		GetCharacterMovement()->StopMovementImmediately();
		break;
	}
	case EPCS_MoveToTarget:
	{
		//离开追击态时也清除残留速度，避免下一状态出现惯性
		GetCharacterMovement()->StopMovementImmediately();
		break;
	}
	case EPCS_PreInteract:
    {
        break;
    }
	case EPCS_Interact:
    {
        break;
    }
	case EPCS_PostInteract:
    {
        break;
    }
	default:
		break;
	}
	//注意：不在此处把CurrentlyState置为EPCS_None，避免OnRep收到一次多余的中间值。
	//最终状态会由紧随其后的OnEnterState统一设置。
	const FString StateName = StaticEnum<EPlayerCharacterState>()->GetNameStringByValue((int64)LeaveState);
	UE_LOG(LogPlayerCharacter, Log, TEXT("'%s' Leave State '%s'!"), *GetNameSafe(this), *StateName);
}

AHeroBuilderCharacter::AHeroBuilderCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	//——网络同步频率：默认100，移动相关Actor建议至少30以上；这里给到60可以让“服务端权威驱动”时的位置快照更密集
	NetUpdateFrequency = 60.f;
	MinNetUpdateFrequency = 30.f;

	//——开启CharacterMovement网络平滑（仿真代理/远端表现使用），缓解服务端权威移动时的“跳帧感”
	GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	GetCharacterMovement()->NetworkMaxSmoothUpdateDistance = 256.f;
	GetCharacterMovement()->NetworkNoSmoothUpdateDistance = 384.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AHeroBuilderCharacter::SwitchState(EPlayerCharacterState NewState)
{
	//SwitchState 仅在服务端权威环境下生效（OnEnter/OnLeave已不是Server RPC，需在此把关）
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentlyState == NewState)
	{
		return;
	}

	OnLeaveState(CurrentlyState);
	OnEnterState(NewState);
}

void AHeroBuilderCharacter::AbortInteract()
{
	//只有处在交互流程中（追击/前摇/交互/后摇）才需要中止
	if (CurrentlyState != EPCS_MoveToTarget &&
		CurrentlyState != EPCS_PreInteract  &&
		CurrentlyState != EPCS_Interact     &&
		CurrentlyState != EPCS_PostInteract)
	{
		return;
	}

	//清零计时，避免被Tick残留消耗
	CurrentInteractDelay = 0.f;
	//直接切回Idle，OnLeaveState/OnEnterState会负责状态转换日志
	SwitchState(EPCS_Idle);
	UE_LOG(LogPlayerCharacter, Log, TEXT("'%s' AbortInteract: interact flow aborted."), *GetNameSafe(this));
}

void AHeroBuilderCharacter::BeginInteractFlow()
{
	if (!HasAuthority())
	{
		return;
	}

	//有交互目标且尚未进入InteractRange时，先进入追击态；否则直接进入前摇
	if (IsValidTarget(InteractTarget))
	{
		const FVector Delta   = InteractTarget->GetActorLocation() - GetActorLocation();
		const FVector DeltaXY(Delta.X, Delta.Y, 0.f);
		if (DeltaXY.Size() > InteractRange)
		{
			SwitchState(EPCS_MoveToTarget);
			return;
		}
	}
	SwitchState(EPCS_PreInteract);
}

void AHeroBuilderCharacter::TickMoveToTarget(float DeltaTime)
{
	//该函数现仅由自治代理客户端调用，只产生AddMovementInput，走CharacterMovement标准预测管线。
	//目标失效 / 达到交互范围的状态推进，都由服务端的TickAuthorityMoveToTarget负责。
	if (!IsValidTarget(InteractTarget))
	{
		return;
	}

	const FVector SelfLoc   = GetActorLocation();
	const FVector TargetLoc = InteractTarget->GetActorLocation();
	const FVector Delta     = TargetLoc - SelfLoc;
	//仅按水平距离判断范围（避免Z轴差异导致误判）
	const FVector DeltaXY(Delta.X, Delta.Y, 0.f);
	const float   DistXY = DeltaXY.Size();

	//已在交互范围内：客户端不再主动驱动位移，等服务端切状态后本函数也不会再被调用
	if (DistXY <= InteractRange)
	{
		return;
	}

	//以下逻辑：把世界空间下“朝向目标的XY单位向量”反算为玩家ControlRotation局部空间的(X=前, Y=右) 2D输入，
	//交由Move()内部的旋转矩阵转回世界方向后调用AddMovementInput，走标准CharacterMovement移动管线。
	const FVector DirXY = DeltaXY.GetSafeNormal();
	FVector2D MoveInput2D(0.f, 1.f); //默认向前，会被下面重新赋值
	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		//Move()内部的计算将产生世界方向 = ForwardDirection * Y + RightDirection * X
		//所以需反解：Y = DirXY · ForwardDirection；X = DirXY · RightDirection
		MoveInput2D.Y = FVector::DotProduct(DirXY, ForwardDirection);
		MoveInput2D.X = FVector::DotProduct(DirXY, RightDirection);
	}
	else
	{
		//无Controller时退而求其次：直接在世界坐标下将DirXY当作输入方向使用
		MoveInput2D.X = DirXY.Y;
		MoveInput2D.Y = DirXY.X;
	}

	//调用Move()：设置内部驱动标记以绕过状态屏蔽
	bInternalDrivenMove = true;
	Move(FInputActionValue(MoveInput2D));
	bInternalDrivenMove = false;
}

void AHeroBuilderCharacter::TickAuthorityMoveToTarget(float DeltaTime)
{
	//服务端权威：只负责状态推进，不驱动实际位移。
	//位移由自治代理客户端的TickMoveToTarget()驱动，透过CharacterMovement预测管线同步到服务端。
	if (!IsValidTarget(InteractTarget))
	{
		AbortInteract();
		return;
	}

	const FVector Delta = InteractTarget->GetActorLocation() - GetActorLocation();
	const FVector DeltaXY(Delta.X, Delta.Y, 0.f);
	if (DeltaXY.Size() <= InteractRange)
	{
		SwitchState(EPCS_PreInteract);
	}
}

void AHeroBuilderCharacter::SetPreInteractDelay(float Delay)
{
	PreInteractDelay = Delay;
}

void AHeroBuilderCharacter::SetPostInteractDelay(float Delay)
{
    PostInteractDelay = Delay;
}

float AHeroBuilderCharacter::GetAttack() const
{
	return Attack;
}

void AHeroBuilderCharacter::SetAttack(float NewAttack)
{
	Attack = NewAttack;
}

void AHeroBuilderCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void AHeroBuilderCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if WITH_SERVER_CODE
	//服务端权威：跑状态更新 + 权威状态机
	if (HasAuthority())
	{
		TickUpdateState(DeltaTime);
		Tick_AuthorityState(DeltaTime);
	}
#endif
#if !UE_SERVER
	//非DedicatedServer构建：自治代理客户端跑一份只读/视觉处理
	if (!HasAuthority() && GetLocalRole() == ROLE_AutonomousProxy)
	{
		Tick_LocalCosmetic(DeltaTime);
	}
#endif
}

#if WITH_SERVER_CODE
void AHeroBuilderCharacter::Tick_AuthorityState(float DeltaTime)
{
	//服务端权威状态机：包含SwitchState、Server_TryInteract等会改变同步属性/触发权威逻辑的副作用
	switch (CurrentlyState)
	{
	case EPCS_None:
		break;
	case EPCS_Idle:
		break;
	case EPCS_Move:
		break;
	case EPCS_MoveToTarget:
	{
		//服务端不再驱动位移，仅做“到达则切PreInteract / 目标失效则中止”的状态推进
		TickAuthorityMoveToTarget(DeltaTime);
		break;
	}
	case EPCS_PreInteract:
	{
		//前摇计时，结束后进入交互帧
		if (CurrentInteractDelay > 0.f)
		{
			CurrentInteractDelay -= DeltaTime;
		}
		else
		{
			SwitchState(EPCS_Interact);
		}
		break;
	}
	case EPCS_Interact:
	{
		break;
	}
	case EPCS_PostInteract:
	{
		//后摇计时，结束后开启下一轮：根据距离判断是先追击还是直接前摇
		if (CurrentInteractDelay > 0.f)
		{
			CurrentInteractDelay -= DeltaTime;
		}
		else
		{
			BeginInteractFlow();
		}
		break;
	}
	default:
		break;
	}
}
#endif // WITH_SERVER_CODE

#if !UE_SERVER
void AHeroBuilderCharacter::Tick_LocalCosmetic(float DeltaTime)
{
	//客户端本地状态处理：CurrentlyState由服务端Replicate同步过来，这里只读使用，
	//严禁调用SwitchState、严禁修改任何Replicated属性、严禁触发Server RPC外的权威逻辑。
	switch (CurrentlyState)
	{
	case EPCS_None:
		break;
	case EPCS_Idle:
		break;
	case EPCS_Move:
		break;
	case EPCS_MoveToTarget:
	{
		//客户端驱动追击位移：走CharacterMovement标准预测管线，服务端负责状态推进
		TickMoveToTarget(DeltaTime);
		break;
	}
	case EPCS_PreInteract:
	{
		//占位：可在此处驱动本地前摇视觉反馈
		break;
	}
	case EPCS_Interact:
	{
		//占位：可在此处播放本地命中/交互瞬时反馈
		break;
	}
	case EPCS_PostInteract:
	{
		//占位：可在此处驱动本地后摇视觉反馈
		break;
	}
	default:
		break;
	}
}
#endif // !UE_SERVER

void AHeroBuilderCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHeroBuilderCharacter, CurrentlyState);
	DOREPLIFETIME(AHeroBuilderCharacter, InteractTarget);
	DOREPLIFETIME(AHeroBuilderCharacter, CurrentInteractMode);
	DOREPLIFETIME(AHeroBuilderCharacter, Attack);

}

void AHeroBuilderCharacter::OnRep_CurrentInteractMode()
{
}

FVector AHeroBuilderCharacter::GetFollowCameraForward()
{
	return FollowCamera->GetForwardVector();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHeroBuilderCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHeroBuilderCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHeroBuilderCharacter::Look);

		// Construction Mode
        EnhancedInputComponent->BindAction(ChangeConstructionModeAction, ETriggerEvent::Started, this, &AHeroBuilderCharacter::ChangeConstructionMode);

		// Interact：长按保持交互，抬起取消
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AHeroBuilderCharacter::Interact);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &AHeroBuilderCharacter::OnInteractReleased);
	}
	else
	{
		UE_LOG(LogPlayerCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AHeroBuilderCharacter::Move(const FInputActionValue& Value)
{
	//交互优先：只要交互键仍按住（包括追击/前摇/交互/后摇），就禁止一切玩家移动输入；
	//但客户端追击逻辑(TickMoveToTarget)会设置bInternalDrivenMove=true主动调用本函数驱动移动，需要放行
	if (!bInternalDrivenMove &&
		(CurrentlyState == EPCS_MoveToTarget ||
		 CurrentlyState == EPCS_PreInteract  ||
		 CurrentlyState == EPCS_Interact     ||
		 CurrentlyState == EPCS_PostInteract))
	{
		return;
	}

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AHeroBuilderCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AHeroBuilderCharacter::ChangeConstructionMode(const FInputActionValue& Value)
{
	//本地端通过Server RPC请求服务器切换交互模式
	UHB_InteractSubsystem* InteractSys = GetWorld()->GetSubsystem<UHB_InteractSubsystem>();
	if (!InteractSys)
	{
		return;
	}
	const EPlayerCharacterInteractMode CurrentMode = InteractSys->GetInteractMode(this);
	const EPlayerCharacterInteractMode NewMode = (CurrentMode == IM_ConstructionMode) ? IM_Normal : IM_ConstructionMode;
	Server_SwitchInteractMode(static_cast<uint8>(NewMode));
}

void AHeroBuilderCharacter::Interact(const FInputActionValue& Value)
{
	//仅在Idle/Move状态下可以起手交互，避免重复触发
	if (CurrentlyState != EPCS_Idle && CurrentlyState != EPCS_Move)
	{
		return;
	}
	//交互流程是服务端权威：客户端通过Server RPC通知服务端开启流程
	if (HasAuthority())
	{
		BeginInteractFlow();
	}
	else
	{
		Server_BeginInteract();
	}
}

void AHeroBuilderCharacter::Server_BeginInteract_Implementation()
{
	//服务端再次校验状态，避免客户端发送时状态在RPC到达前已变化
	if (CurrentlyState != EPCS_Idle && CurrentlyState != EPCS_Move)
	{
		return;
	}
	BeginInteractFlow();
}

void AHeroBuilderCharacter::Server_AbortInteract_Implementation()
{
	AbortInteract();
}

void AHeroBuilderCharacter::OnInteractReleased(const FInputActionValue& Value)
{
	if (HasAuthority())
	{
		AbortInteract();
	}
	else
	{
		Server_AbortInteract();
	}
}

void AHeroBuilderCharacter::Server_SwitchInteractMode_Implementation(uint8 NewMode)
{
	if (UHB_InteractSubsystem* InteractSys = GetWorld()->GetSubsystem<UHB_InteractSubsystem>())
	{
		InteractSys->SwitchInteractMode(this, static_cast<EPlayerCharacterInteractMode>(NewMode));
	}
}

void AHeroBuilderCharacter::Server_TryInteract_Implementation()
{
	if (UHB_InteractSubsystem* InteractSys = GetWorld()->GetSubsystem<UHB_InteractSubsystem>())
	{
		InteractSys->TryInteract(this);
	}
}
bool AHeroBuilderCharacter::IsValidTarget(AActor* Target)
{
	if (!IsValid(Target))
	{
		return false;
	}
	UHB_DamageComponent* DamageComp = Target->FindComponentByClass<UHB_DamageComponent>();
	if (!DamageComp)
	{
        return false;
    }
    return !DamageComp->IsDead();
}