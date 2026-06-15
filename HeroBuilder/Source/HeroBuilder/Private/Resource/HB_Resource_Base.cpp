// Fill out your copyright notice in the Description page of Project Settings.


#include "Resource/HB_Resource_Base.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/HB_ResourceSubsystem.h"
#include "Config/ResourceData.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/HB_DamageComponent.h"
#include "Components/HB_InteractComponent.h"

DEFINE_LOG_CATEGORY(LogResource)

// Sets default values
AHB_Resource_Base::AHB_Resource_Base()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	// 创建用于检测的Box碰撞盒，并设置Profile为Resource
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetCollisionProfileName(TEXT("Resource"));
	CollisionBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	RootComponent = CollisionBox;

	// 创建血量 Widget 组件
	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidget->SetupAttachment(RootComponent);
	HealthBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetDrawSize(FVector2D(120.f, 15.f));

	// 创建伤害组件
	DamageComponent = CreateDefaultSubobject<UHB_DamageComponent>(TEXT("DamageComponent"));

	// 创建交互组件：描述本Actor对应的玩家交互类型（默认 IT_Normal，子类或配置初始化时覆盖）
	InteractComponent = CreateDefaultSubobject<UHB_InteractComponent>(TEXT("InteractComponent"));
}

FString AHB_Resource_Base::GetStateName(EResourceState State)
{
	switch (State)
	{
	case EResourceState::RS_Idle: return TEXT("Idle");
	case EResourceState::RS_BeHit: return TEXT("BeHit");
	case EResourceState::RS_Recover: return TEXT("Recover");
	case EResourceState::RS_Death: return TEXT("Death");
	default: return TEXT("Unknown");
	}
}

void AHB_Resource_Base::OnRep_CurrentlyState()
{
}

bool AHB_Resource_Base::SwitchState(EResourceState NewState)
{
	if (CurrentState == RS_Death)
	{
		return false; // 死亡锁定
	}
	if (CurrentState == NewState)
	{
		return false;
	}
	UE_LOG(LogResource, Log, TEXT("SwitchState %s -> %s"), *GetStateName(CurrentState), *GetStateName(NewState));

	const EResourceState OldState = CurrentState;
	// 先记录旧状态再切换，供上层随时查询
	LastState = OldState;
	// 落表为新状态（服务端会触发复制 → 客户端 OnRep_CurrentlyState）
	CurrentState = NewState;

	// 服务端等价 OnRep 派发（服务端不会触发自身的 ReplicatedUsing 回调）
	//if (HasAuthority())
	//{
		OnLeaveState(OldState);
		OnEnterState(NewState);
//	}
	return true;
}

bool AHB_Resource_Base::IsDeath()
{
	return CurrentState == RS_Death;
}

// Called when the game starts or when spawned
void AHB_Resource_Base::BeginPlay()
{
	Super::BeginPlay();
	NetMode = GetWorld()->GetNetMode();

	//委托绑定已迁移至 PostInitializeComponents（更早、且不受蓝图 BeginPlay 是否调 Parent 影响）
}

void AHB_Resource_Base::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	//运行时兜底：蓝图子类若把继承的 DamageComponent / InteractComponent 删除或覆盖，
	//C++ 指针在 CDO 后会被写回 nullptr，但实例仍可能以匿名子对象形式存在，按类型查找拿回。
	if (!DamageComponent)
	{
		DamageComponent = FindComponentByClass<UHB_DamageComponent>();
	}
	if (!InteractComponent)
	{
		InteractComponent = FindComponentByClass<UHB_InteractComponent>();
	}

	if (DamageComponent)
	{
		DamageComponent->OnHealthChanged.AddDynamic(this, &AHB_Resource_Base::HandleHealthChanged);
		DamageComponent->OnDeath.AddDynamic(this, &AHB_Resource_Base::HandleDeath);
	}
	else
	{
		UE_LOG(LogResource, Error, TEXT("[%s] DamageComponent missing in both CDO and runtime, health callbacks will NOT fire"), *GetName());
	}
}

void AHB_Resource_Base::OnEnterState(EResourceState EnterState)
{

	UE_LOG(LogResource, Log, TEXT("OnEnterState %s"), *GetStateName(EnterState));
	switch (EnterState)
	{
	case RS_Idle:
		break;
	case RS_BeHit:
		break;
	case RS_Recover:
		break;
	case RS_Death:
	{

		break;
	}
	default:
		break;
	}
}

void AHB_Resource_Base::OnLeaveState(EResourceState LeaveState)
{
	switch (LeaveState)
	{
	case RS_Idle:
		break;
	case RS_BeHit:
		break;
	case RS_Recover:
		break;
	case RS_Death:
		break;
	default:
		break;
	}
}

void AHB_Resource_Base::HandleHealthChanged(float OldHealth, float NewHealth, float MaxHealthValue, AActor* Attacker)
{
		// 调用蓝图可重写的 OnHealthChanged 接口
		OnHealthChanged(OldHealth, NewHealth, MaxHealthValue, Attacker);
		// 被打且未死亡 -> 进入受击状态
		if (NewHealth < OldHealth && NewHealth > 0.f)
		{
			CurrentBeHitDuration = BeHitDuration;
			CurrentRecoverDelay = RecoverDelay;
			SwitchState(RS_BeHit);
		}

}

void AHB_Resource_Base::HandleDeath()
{
	SwitchState(RS_Death);
	OnDeath();
	// 死亡后不再吸引玩家切换交互类型
	if (InteractComponent)
	{
		InteractComponent->SetIsInteractable(false);
	}

	// 通知子系统处理资源掉落与销毁
	if (UHB_ResourceSubsystem* ResourceSubsystem = GetWorld()->GetSubsystem<UHB_ResourceSubsystem>())
	{
		ResourceSubsystem->OnResourceDeath(this);
	}
}

// Called every frame
void AHB_Resource_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		return;
	}

	switch (CurrentState)
	{
	case RS_Idle:
	{
		// 满血则保持 Idle，未满血则等待脱战延迟过后切到 Recover
		if (DamageComponent && DamageComponent->GetCurrentHealth() < DamageComponent->GetMaxHealth())
		{
			CurrentRecoverDelay -= DeltaTime;
			if (CurrentRecoverDelay <= 0.f)
			{
				SwitchState(RS_Recover);
			}
		}
		break;
	}
	case RS_BeHit:
	{
		CurrentBeHitDuration -= DeltaTime;
		if (CurrentBeHitDuration <= 0.f)
		{
			SwitchState(RS_Idle);
		}
		break;
	}
	case RS_Recover:
	{
		if (DamageComponent)
		{
			DamageComponent->Heal(RecoverSpeed * DeltaTime);
			if (DamageComponent->GetCurrentHealth() >= DamageComponent->GetMaxHealth())
			{
				SwitchState(RS_Idle);
			}
		}
		break;
	}
	case RS_Death:
	{
		if (DeathTime > 0.f)
		{
			DeathTime -= DeltaTime;
		}
		else
		{
			SetActorTickEnabled(false);
			if (UHB_ResourceSubsystem* ResourceSubsystem = GetWorld()->GetSubsystem<UHB_ResourceSubsystem>())
			{
				ResourceSubsystem->DestroyResource(this);
			}
		}
		break;
	}
	default:
		break;
	}
}

void AHB_Resource_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_Resource_Base, CurrentState);
	DOREPLIFETIME(AHB_Resource_Base, LastState);
	//以下字段由服务端 SpawnResource → InitialResource 写入，客户端只在Actor首次复制时同步一次，运行期不再变化
	DOREPLIFETIME_CONDITION(AHB_Resource_Base, RecoverDelay, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AHB_Resource_Base, RecoverSpeed, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AHB_Resource_Base, BeHitDuration, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AHB_Resource_Base, DeathTime, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AHB_Resource_Base, ResourceType, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AHB_Resource_Base, ResourceAmount, COND_InitialOnly);
}

void AHB_Resource_Base::InitialResource(const FResourceConfig& InConfig)
{
	UE_LOG(LogResource, Log, TEXT("Initializing resource - Name: %s, Type: %d, Health: %.1f, Amount: %d"),
		*InConfig.ResourceName, static_cast<int32>(InConfig.ResourceType), InConfig.Health, InConfig.ResourceAmount);

	//运行时兜底：与 PostInitializeComponents 保持一致策略，避免蓝图子类删除继承组件后此处静默跳过
	if (!DamageComponent)
	{
		DamageComponent = FindComponentByClass<UHB_DamageComponent>();
	}
	if (!InteractComponent)
	{
		InteractComponent = FindComponentByClass<UHB_InteractComponent>();
	}

	if (DamageComponent)
	{
		DamageComponent->InitHealth(InConfig.Health);
	}
	else
	{
		UE_LOG(LogResource, Error, TEXT("[%s] InitialResource: DamageComponent missing, Health=%.1f not applied"), *GetName(), InConfig.Health);
	}
	RecoverDelay = InConfig.RecoverDelay;
	RecoverSpeed = InConfig.RecoverSpeed;
	BeHitDuration = InConfig.BeHitDuration;
	DeathTime = InConfig.DeathTime;
	ResourceType = InConfig.ResourceType;
	ResourceAmount = InConfig.ResourceAmount;
	if (InteractComponent)
	{
		InteractComponent->SetInteractType(InConfig.InteractMode);
	}
	else
	{
		UE_LOG(LogResource, Error, TEXT("[%s] InitialResource: InteractComponent missing, InteractMode not applied"), *GetName());
	}
}

EInteractType AHB_Resource_Base::GetInteractMode() const
{
	return InteractComponent ? InteractComponent->GetInteractType() : IT_Normal;
}

