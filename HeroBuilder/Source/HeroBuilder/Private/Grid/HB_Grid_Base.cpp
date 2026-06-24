// Fill out your copyright notice in the Description page of Project Settings.


#include "Grid/HB_Grid_Base.h"
#include "Subsystems/HB_GridSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"

// Sets default values
AHB_Grid_Base::AHB_Grid_Base()
{
 	// 不需要每帧 Tick，关闭以节省 CPU 开销
	PrimaryActorTick.bCanEverTick = false;

	// 网格仅作为视觉/静态布局存在，由两端各自基于 GridData 在 BeginPlay 中本地生成相同结果，
	// 不需要走网络复制；同时避免 SubObject 触发
	// LogNetPackageMap "FNetGUIDCache::SupportsObject ... NOT Supported" 警告。
	bReplicates = false;
	bNetLoadOnClient = true;

	// 使用 ISM 一次性绘制所有格子，一次 DrawCall 完成全部实例渲染
	GridISMComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GridISMComponent"));
	GridISMComponent->SetMobility(EComponentMobility::Static);

	// 碰撞配置：让 Character Capsule 能正常踩在格子上
	GridISMComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GridISMComponent->SetCollisionObjectType(ECC_WorldStatic);
	GridISMComponent->SetCollisionResponseToAllChannels(ECR_Block);
	GridISMComponent->SetGenerateOverlapEvents(false);
	GridISMComponent->SetCanEverAffectNavigation(true);
	GridISMComponent->CanCharacterStepUpOn = ECB_Yes;
	GridISMComponent->SetIsReplicated(false);

	RootComponent = GridISMComponent;
}

// Called when the game starts or when spawned
void AHB_Grid_Base::BeginPlay()
{
	Super::BeginPlay();
}

// 在构造时生成所有Grid实例：编辑器中放置/调整属性、以及运行时Spawn都会触发
void AHB_Grid_Base::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UHB_GridSubsystem* GridSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UHB_GridSubsystem>() : nullptr;
	if (!GridSubsystem || !GridISMComponent)
	{
		return;
	}

	// 重新构造前先清空旧实例，避免编辑器中重复构造时累计
	GridISMComponent->ClearInstances();

	const int32 GridWidthFragment = GridSubsystem->GetGridWidthFragment();
	const int32 GridLengthFragment = GridSubsystem->GetGridLengthFragment();

	// 预留实例容量，避免多次扩容
	const int32 InstanceCount = GridWidthFragment * GridLengthFragment;
	TArray<FTransform> InstanceTransforms;
	InstanceTransforms.Reserve(InstanceCount);

	// 从左上开始生成（每个实例相对 RootComponent 的局部偏移）
	for (int32 x = 0; x < GridWidthFragment; ++x)
	{
		for (int32 y = 0; y < GridLengthFragment; ++y)
		{
            InstanceTransforms.Emplace(FTransform(FVector(GridWidthFragment * 50, GridLengthFragment * 50, 0) - FVector(x * 100, y * 100, 0)));
		}
	}

	// bWorldSpace = false：使用相对（局部）空间坐标
	GridISMComponent->AddInstances(InstanceTransforms, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/false);

	// 强制重建物理状态，确保所有实例的碰撞 Body 都已生成
	GridISMComponent->RecreatePhysicsState();
}

// Called every frame
void AHB_Grid_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

