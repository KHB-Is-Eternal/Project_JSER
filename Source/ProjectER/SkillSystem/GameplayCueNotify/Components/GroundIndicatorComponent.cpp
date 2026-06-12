#include "SkillSystem/GameplayCueNotify/Components/GroundIndicatorComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UGroundIndicatorComponent::UGroundIndicatorComponent()
{
	// 30FPS 수준으로 실시간 바닥 추적을 수행하도록 Tick 간격 설정 (퍼포먼스 최적화)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.033f;
	
	// 기본 데칼용 메쉬 로드 (언리얼 내장 Plane)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane"));
	if (PlaneMeshFinder.Succeeded())
	{
		SetStaticMesh(PlaneMeshFinder.Object);
	}

	// 렌더링 전용 컴포넌트이므로 콜리전 관련 최적화
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);
	CanCharacterStepUpOn = ECB_No;
}

void UGroundIndicatorComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 소켓 부착 여부에 관계없이 생성 시 1회는 무조건 지면을 찾아갑니다.
	UpdateGroundPosition();
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

	// 1. Z축 위치 보정
	// 사용자가 설정한 본 부착 모드(bAttachToBone)일 때만 매 틱마다 트레이스를 쏴서 뼈다귀의 흔들림을 상쇄합니다.
	if (bIsTrackingDynamicGround)
	{
		UpdateGroundPosition();
	}
}
