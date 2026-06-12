#include "SkillSystem/GameplayCueNotify/Components/GroundIndicatorComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UGroundIndicatorComponent::UGroundIndicatorComponent()
{
	// 30FPS 수준으로 실시간 바닥 추적을 수행하도록 Tick 간격 설정 (퍼포먼스 최적화)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.033f;

	// 스프링암 회전 억제 및 본 추적 최적화 설정
	TargetArmLength = 0.0f;
	bDoCollisionTest = false;
	bInheritPitch = false;
	bInheritRoll = false;
	bInheritYaw = true;
}

void UGroundIndicatorComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 지연 생성을 보장하여 설정값 유실 방지
	EnsureIndicatorMeshCompExists();

	// 렌더링 씬에 하위 컴포넌트 강제 등록
	if (IndicatorMeshComp && !IndicatorMeshComp->IsRegistered())
	{
		IndicatorMeshComp->RegisterComponent();
	}

	// 소켓 부착 여부에 관계없이 생성 시 1회는 무조건 지면을 찾아갑니다.
	UpdateGroundPosition();

	// 부모의 틱이 끝난 후 본 컴포넌트의 틱이 돌도록 설정하여, Absolute Rotation 사용 시 발생하는 1프레임 회전 지터링을 방지합니다.
	if (USceneComponent* ParentComp = GetAttachParent())
	{
		PrimaryComponentTick.AddPrerequisite(ParentComp, ParentComp->PrimaryComponentTick);
	}
}

void UGroundIndicatorComponent::UpdateGroundPosition()
{
	USceneComponent* ParentComp = GetAttachParent();
	if (!ParentComp)
	{
		return;
	}

	const FVector ParentLoc = ParentComp->GetComponentLocation();
	
	// 시작점을 공중 10m로 띄워, 지형에 파묻힌 상태에서도 무조건 위에서 아래로 레이를 쏘도록 합니다.
	const FVector TraceStart = ParentLoc + FVector(0.f, 0.f, 1000.f);
	const FVector EndLoc = TraceStart - FVector(0.f, 0.f, 3000.f); 
	FHitResult HitResult;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신이 속한 액터 무시

	float TargetZ = ParentLoc.Z; // 트레이스 실패 시 기본값은 부모 높이 유지
	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, EndLoc, ECC_GameTraceChannel9, QueryParams))
	{
		TargetZ = HitResult.Location.Z + ZOffsetFromFloor;
	}

	// X, Y는 부모 위치를 그대로 따라가고, Z 위치만 찾아낸 지면(TargetZ)으로 강제 덮어씌웁니다.
	FVector NewLocation = ParentLoc;
	NewLocation.Z = TargetZ;
	SetWorldLocation(NewLocation);
}

void UGroundIndicatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USceneComponent* ParentComp = GetAttachParent();
	if (!ParentComp)
	{
		return;
	}

	if (bIsTrackingDynamicGround)
	{
		// 실시간으로 Z축 높이를 지형에 맞게 추적합니다.
		UpdateGroundPosition();
	}
}

void UGroundIndicatorComponent::EnsureIndicatorMeshCompExists()
{
	if (!IndicatorMeshComp)
	{
		IndicatorMeshComp = NewObject<UStaticMeshComponent>(this, TEXT("IndicatorMeshComp"));
		if (IndicatorMeshComp)
		{
			IndicatorMeshComp->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			
			// 기본 데칼용 메쉬 로드 및 설정 (생성자 외부이므로 LoadObject 사용)
			static UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
			if (PlaneMesh)
			{
				IndicatorMeshComp->SetStaticMesh(PlaneMesh);
			}

			// 렌더링 전용 컴포넌트이므로 콜리전 관련 최적화
			IndicatorMeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
			IndicatorMeshComp->SetGenerateOverlapEvents(false);
			IndicatorMeshComp->CanCharacterStepUpOn = ECB_No;

			// 만약 이 컴포넌트(스프링암) 자체가 이미 등록(Registered)된 상태라면, 자식 메쉬도 즉시 씬에 등록해 주어야 렌더링됩니다.
			if (IsRegistered())
			{
				IndicatorMeshComp->RegisterComponent();
			}
		}
	}
}

void UGroundIndicatorComponent::SetIndicatorMaterial(int32 ElementIndex, UMaterialInterface* Material)
{
	EnsureIndicatorMeshCompExists();
	if (IndicatorMeshComp)
	{
		IndicatorMeshComp->SetMaterial(ElementIndex, Material);
	}
}

void UGroundIndicatorComponent::SetIndicatorScale(const FVector& NewScale)
{
	EnsureIndicatorMeshCompExists();
	if (IndicatorMeshComp)
	{
		IndicatorMeshComp->SetWorldScale3D(NewScale);
	}
}
