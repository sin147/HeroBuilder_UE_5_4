// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeroBuilderCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Subsystems/HB_ConstructionSubsystem.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "Subsystems/HB_InteractSubsystem.h"
#include "Subsystems/HB_CharacterSubsystem.h"
#include "Manager/HB_InteractManager.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogPlayerCharacter);

//////////////////////////////////////////////////////////////////////////
// AHeroBuilderCharacter

void AHeroBuilderCharacter::Server_Attack_Implementation()
{

}

UHB_CharacterSubsystem* AHeroBuilderCharacter::GetCharacterSubsystem() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetSubsystem<UHB_CharacterSubsystem>();
	}
	return nullptr;
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

	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	//——网络同步频率：默认100，移动相关Actor建议至少30以上；这里给到60可以让"服务端权威驱动"时的位置快照更密集
	NetUpdateFrequency = 60.f;
	MinNetUpdateFrequency = 30.f;

	//——开启CharacterMovement网络平滑（仿真代理/远端表现使用）
	GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	GetCharacterMovement()->NetworkMaxSmoothUpdateDistance = 256.f;
	GetCharacterMovement()->NetworkNoSmoothUpdateDistance = 384.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

//——————————————————————————————————————————————
// 状态/属性访问壳：转调Subsystem
//——————————————————————————————————————————————

TEnumAsByte<EPlayerCharacterState> AHeroBuilderCharacter::GetCurrentState() const
{
	if (UHB_CharacterSubsystem* Sys = GetCharacterSubsystem())
	{
		return Sys->GetCurrentState(const_cast<AHeroBuilderCharacter*>(this));
	}
	return EPCS_Idle;
}

void AHeroBuilderCharacter::SetInteractTarget(AActor* Target)
{
	if (UHB_CharacterSubsystem* Sys = GetCharacterSubsystem())
	{
		Sys->SetInteractTarget(this, Target);
	}
}

AActor* AHeroBuilderCharacter::GetInteractTarget() const
{
	if (UHB_CharacterSubsystem* Sys = GetCharacterSubsystem())
	{
		return Sys->GetInteractTarget(const_cast<AHeroBuilderCharacter*>(this));
	}
	return nullptr;
}

float AHeroBuilderCharacter::GetAttack() const
{
	if (UHB_CharacterSubsystem* Sys = GetCharacterSubsystem())
	{
		return Sys->GetAttack(const_cast<AHeroBuilderCharacter*>(this));
	}
	return 0.f;
}

void AHeroBuilderCharacter::SetAttack(float NewAttack)
{
	if (UHB_CharacterSubsystem* Sys = GetCharacterSubsystem())
	{
		Sys->SetAttack(this, NewAttack);
	}
}

//——————————————————————————————————————————————
// 生命周期
//——————————————————————————————————————————————

void AHeroBuilderCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AHeroBuilderCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (UHB_CharacterSubsystem* Sys = GetCharacterSubsystem())
		{
			Sys->UnregisterCharacter(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AHeroBuilderCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//所有状态机Tick逻辑已迁至UHB_CharacterSubsystem::Tick；此处不再做任何处理
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHeroBuilderCharacter::Client_BeginInteract_Implementation()
{
	if (UHB_CharacterSubsystem* Sys = GetCharacterSubsystem())
	{
		Sys->BeginInteractFlow(this);
	}
}

void AHeroBuilderCharacter::Client_AbortInteract_Implementation()
{
	if (UHB_CharacterSubsystem* Sys = GetCharacterSubsystem())
	{
		Sys->AbortInteract(this);
	}
}

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
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AHeroBuilderCharacter::OnInteractPressed);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &AHeroBuilderCharacter::OnInteractReleased);
	}
	else
	{
		UE_LOG(LogPlayerCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AHeroBuilderCharacter::Move(const FInputActionValue& Value)
{
	//交互优先：只要交互流程进行中（追击/前摇/交互/后摇），就禁止一切玩家移动输入；
	//但客户端追击逻辑(Subsystem::TickMoveToTarget)会设置bInternalDrivenMove=true主动调用本函数驱动移动，需要放行
	UHB_CharacterSubsystem* Sys = GetCharacterSubsystem();
	if (Sys)
	{
		const EPlayerCharacterState State = Sys->GetCurrentState(this);
		if (State == EPCS_MoveToTarget ||
			State == EPCS_PreInteract  ||
			State == EPCS_Interact     ||
			State == EPCS_PostInteract)
		{
			return;
		}
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
	ENetMode NetMode = GetNetMode();
	if (NetMode == NM_Client || NetMode == NM_DedicatedServer)
	{
        Server_ChangeConstructionMode();
		Client_ChangeConstructionMode();
	}
	else
	{
		//单机/Listen Server宿主：本地直接走切换逻辑
		if (UWorld* World = GetWorld())
		{
			if (UHB_InteractSubsystem* InteractSys = World->GetSubsystem<UHB_InteractSubsystem>())
			{
				const EInteractMode CurMode = InteractSys->GetInteractMode(this);
				const EInteractMode NewMode = (CurMode == IM_Construction) ? IM_Normal : IM_Construction;
				InteractSys->SwitchInteractMode(this, NewMode);
			}
		}
	}
}

void AHeroBuilderCharacter::Server_ChangeConstructionMode_Implementation()
{
	//服务端权威切换：在 Normal 与 Construction 之间互切
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UHB_InteractSubsystem* InteractSys = World->GetSubsystem<UHB_InteractSubsystem>();
	if (!InteractSys)
	{
		return;
	}
	const EInteractMode CurMode = InteractSys->GetInteractMode(this);
	const EInteractMode NewMode = (CurMode == IM_Construction) ? IM_Normal : IM_Construction;
	InteractSys->SwitchInteractMode(this, NewMode);
}

void AHeroBuilderCharacter::Client_ChangeConstructionMode_Implementation()
{
	//客户端本地反馈：真正的模式切换由服务端权威驱动（Manager 的字段会通过复制下发到客户端），
	//这里只做客户端侧的就近响应（例如埋点/UI刷新hook），保持与 Client_BeginInteract 风格一致。
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UHB_InteractSubsystem* InteractSys = World->GetSubsystem<UHB_InteractSubsystem>();
	if (!InteractSys)
	{
		return;
	}
	const EInteractMode CurMode = InteractSys->GetInteractMode(this);
	const EInteractMode NewMode = (CurMode == IM_Construction) ? IM_Normal : IM_Construction;
	InteractSys->SwitchInteractMode(this, NewMode);
}

void AHeroBuilderCharacter::OnInteractPressed(const FInputActionValue& Value)
{
	UHB_CharacterSubsystem* Sys = GetCharacterSubsystem();
	if (!Sys)
	{
		return;
	}
	const EPlayerCharacterState State = Sys->GetCurrentState(this);
	//仅在Idle/Move状态下可以起手交互，避免重复触发
	if (State != EPCS_Idle && State != EPCS_Move)
	{
		return;
	}
	if (GetNetMode() == NM_Client || GetNetMode() == NM_DedicatedServer)
	{

		Client_BeginInteract();
		Server_BeginInteract();
	}
	else
	{
		Sys->BeginInteractFlow(this);
	}
}

void AHeroBuilderCharacter::Server_BeginInteract_Implementation()
{
	UHB_CharacterSubsystem* Sys = GetCharacterSubsystem();
	if (!Sys)
	{
		return;
	}
	const EPlayerCharacterState State = Sys->GetCurrentState(this);
	if (State != EPCS_Idle && State != EPCS_Move)
	{
		return;
	}
	Sys->BeginInteractFlow(this);
}

void AHeroBuilderCharacter::Server_AbortInteract_Implementation()
{
	if (UHB_CharacterSubsystem* Sys = GetCharacterSubsystem())
	{
		Sys->AbortInteract(this);
	}
}

void AHeroBuilderCharacter::OnInteractReleased(const FInputActionValue& Value)
{
	ENetMode NetMode = GetNetMode();
	if (NetMode == NM_Client || NetMode == NM_DedicatedServer)
	{
		Server_AbortInteract();
		Client_AbortInteract();
	}
	else
	{
		if (UHB_CharacterSubsystem* Sys = GetCharacterSubsystem())
		{
			Sys->AbortInteract(this);
		}
	}
}