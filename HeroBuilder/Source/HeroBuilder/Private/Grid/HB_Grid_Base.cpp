// Fill out your copyright notice in the Description page of Project Settings.
#include "Grid/HB_Grid_Base.h"
#include "Subsystems/HB_GridSubsystem.h"
#include "Config/GridData.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AHB_Grid_Base::AHB_Grid_Base()
{
 	// 不需要每帧 Tick，关闭以节省 CPU 开销
	PrimaryActorTick.bCanEverTick = false;
	// 网格需要同步到客户端，但 ISM 实例数据本身不复制，需要在客户端重新生成
	bReplicates = true;
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

void AHB_Grid_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_Grid_Base, AreaLevel);
}

// Called when the game starts or when spawned
void AHB_Grid_Base::BeginPlay()
{
	Super::BeginPlay();

	// 客户端在收到服务端复制下来的 Actor 后重新生成 ISM 实例
	if (!HasAuthority())
	{
		GenerateGridInstances();
	}
}

// 在构造时生成所有Grid实例：编辑器中放置/调整属性、以及运行时Spawn都会触发
void AHB_Grid_Base::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GenerateGridInstances();
}

void AHB_Grid_Base::GenerateGridInstances()
{
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
	FragmentInfos.Empty();
	FragmentCoordToIndexMap.Empty();
	for (int32 x = 0; x < GridWidthFragment; ++x)
	{
		for (int32 y = 0; y < GridLengthFragment; ++y)
		{
			const FVector Offset(
				(GridWidthFragment - 1) * GRID_FRAGMENT_SIZE / 2.0f - x * GRID_FRAGMENT_SIZE,
				(GridLengthFragment - 1) * GRID_FRAGMENT_SIZE / 2.0f - y * GRID_FRAGMENT_SIZE,
				0.0f);
			InstanceTransforms.Emplace(FTransform(Offset));
			// 计算该 Fragment 的全局坐标
			const FVector WorldPos = GetActorTransform().TransformPosition(Offset);
			const FGridInfo GridCoord(FMath::Floor(WorldPos.X / GRID_FRAGMENT_SIZE), FMath::Floor(WorldPos.Y / GRID_FRAGMENT_SIZE));
			const int32 FragmentIndex = x * GridLengthFragment + y;
			FragmentInfos.Add(FFragmentInfo(FragmentIndex, GridCoord));
			FragmentCoordToIndexMap.Add(GridCoord, FragmentIndex);
		}
	}
	// bWorldSpace = false：使用相对（局部）空间坐标
	GridISMComponent->AddInstances(InstanceTransforms, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/false);
	// 强制重建物理状态，确保所有实例的碰撞 Body 都已生成
	GridISMComponent->RecreatePhysicsState();
	// 向 GridSubsystem 注册本 Grid 的所有 Fragment，统一纳入 GridManager 管理
	if (HasAuthority())
	{
		GridSubsystem->RegisterGridActor(this);
	}
}

// Called every frame
void AHB_Grid_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHB_Grid_Base::SetAreaLevel(int32 InLevel)
{
	if (HasAuthority())
	{
		AreaLevel = InLevel;
		ApplyAreaMaterial();
	}
}

void AHB_Grid_Base::OnRep_AreaLevel()
{
	ApplyAreaMaterial();
}

void AHB_Grid_Base::ApplyAreaMaterial()
{
	if (AreaLevel < 0)
	{
		return;
	}

	UHB_GridSubsystem* GridSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UHB_GridSubsystem>() : nullptr;
	if (!GridSubsystem)
	{
		return;
	}

	if (UGridData* GridData = GridSubsystem->GetGridData())
	{
		FAreaConfig AreaConfig;
		if (GridData->GetAreaConfigByLevel(AreaLevel, AreaConfig))
		{
			SetGridMaterial(AreaConfig.GridMaterial);
		}
	}
}

void AHB_Grid_Base::SetGridMaterial(UMaterialInterface* Material)
{
	if (!GridISMComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] SetGridMaterial 失败：GridISMComponent 为空"), *GetName());
		return;
	}
	if (!Material)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] SetGridMaterial 失败：传入的 Material 为空"), *GetName());
		return;
	}
	if (!GridISMComponent->GetStaticMesh())
	{
		// 没有 StaticMesh 时 SetMaterial(0, ...) 不会有任何视觉效果
		UE_LOG(LogTemp, Warning, TEXT("[%s] SetGridMaterial 无效：GridISMComponent 未设置 StaticMesh，请在蓝图子类中给 GridISMComponent 配置 StaticMesh"), *GetName());
		return;
	}
	// 用 OverrideMaterials 覆盖 ISM 的所有材质槽，确保替换全部 Section
	const int32 NumMaterials = GridISMComponent->GetNumMaterials();
	for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
	{
		GridISMComponent->SetMaterial(SlotIndex, Material);
	}
	GridISMComponent->MarkRenderStateDirty();
}

bool AHB_Grid_Base::GetRandomPosition(FVector& OutPosition)
{
	UHB_GridSubsystem* GridSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UHB_GridSubsystem>() : nullptr;
	if (!GridSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] GetRandomPosition 失败：无法获取 GridSubsystem"), *GetName());
		return false;
	}
	// 以 GridManager 为权威数据源，筛选出属于本 Grid 的空闲 Fragment
	const TArray<FGridInfo> AllFreeCoords = GridSubsystem->GetFreeGridIndexs();
	TArray<FGridInfo> MyFreeCoords;
	MyFreeCoords.Reserve(AllFreeCoords.Num());
	for (const FGridInfo& Coord : AllFreeCoords)
	{
		if (FragmentCoordToIndexMap.Contains(Coord))
		{
			MyFreeCoords.Add(Coord);
		}
	}
	if (MyFreeCoords.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] GetRandomPosition 失败：没有可用的空闲 Fragment"), *GetName());
		return false;
	}
	const FGridInfo& RandomCoord = MyFreeCoords[FMath::RandRange(0, MyFreeCoords.Num() - 1)];
	const int32* IndexPtr = FragmentCoordToIndexMap.Find(RandomCoord);
	if (!IndexPtr)
	{
		return false;
	}
	const int32 FragmentIndex = *IndexPtr;
	FTransform InstanceTransform;
	if (!GridISMComponent || !GridISMComponent->GetInstanceTransform(FragmentIndex, InstanceTransform, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] GetRandomPosition 失败：无法获取实例 %d 的世界 Transform"), *GetName(), FragmentIndex);
		return false;
	}
	// 同步占用状态到 GridManager（GridActor 不再单独维护本地 bIsOccupied）
	GridSubsystem->OccupyGrid(RandomCoord.X, RandomCoord.Y);
	OutPosition = InstanceTransform.GetLocation();
	return true;
}

TArray<FGridInfo> AHB_Grid_Base::GetAllFragmentCoords() const
{
	TArray<FGridInfo> Result;
	Result.Reserve(FragmentInfos.Num());
	for (const FFragmentInfo& Info : FragmentInfos)
	{
		Result.Add(Info.GridCoord);
	}
	return Result;
}

FGridInfo AHB_Grid_Base::GetCenterFragmentCoord() const
{
	const FVector Location = GetActorLocation();
	return FGridInfo(FMath::Floor(Location.X / GRID_FRAGMENT_SIZE), FMath::Floor(Location.Y / GRID_FRAGMENT_SIZE));
}
