#include "UI/UI_AMiniMapCapture.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameModeBase/State/ER_GameState.h"
#include "Components/LineBatchComponent.h"

// Sets default values
AUI_AMiniMapCapture::AUI_AMiniMapCapture()
{
	PrimaryActorTick.bCanEverTick = false;

    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootScene;

    CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
    CaptureComponent->SetupAttachment(RootComponent);

    // 기본 설정 (블루프린트에서 수정 가능하도록 최소화)
    CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
    CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;


    CaptureComponent->SetAbsolute(false, true, false);
    CaptureComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
  

    CaptureComponent->ShowFlags.SetDynamicShadows(false); // 동적 그림자
    CaptureComponent->ShowFlags.SetGlobalIllumination(false); // 루멘
    CaptureComponent->ShowFlags.SetDecals(false); // 데칼 전체 제외 (데칼은 HiddenActors로 숨길 수 없는 엔진 구조라 ShowFlags로 차단)

    // 생성자나 BeginPlay에서 설정
    CaptureComponent->bCaptureEveryFrame = false; // 매 프레임 캡처 중지
    CaptureComponent->bCaptureOnMovement = false; // 움직일 때마다 캡처 중지

    //CaptureComponent->ShowFlags.SetMotionBlur(false); // 잔상 제거용
    //CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_BaseColor; // 포스트 프로세싱 무효화
}

void AUI_AMiniMapCapture::UpdateMiniMap()
{
    CaptureComponent->CaptureScene();
}

FVector AUI_AMiniMapCapture::GetMapCenter() const
{
    return CaptureComponent ? CaptureComponent->GetComponentLocation() : GetActorLocation();
}

float AUI_AMiniMapCapture::GetMapOrthoWidth() const
{
    return CaptureComponent ? CaptureComponent->OrthoWidth : 0.f;
}

// Called when the game starts or when spawned
void AUI_AMiniMapCapture::BeginPlay()
{
	Super::BeginPlay();

    CaptureComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 10000.0f));
    CaptureComponent->OrthoWidth = 25000.0f; // 맵 전체가 다 들어올 정도의 너비

    // 디버그 라인(DrawDebugBox/Sphere/Capsule 등)이 미니맵에 찍히지 않도록 전역 제외
    if (ULineBatchComponent* WorldLineBatcher = GetWorld()->GetLineBatcher(UWorld::ELineBatcherType::World))
    {
        WorldLineBatcher->SetHiddenInSceneCapture(true);
    }
    if (ULineBatchComponent* PersistentLineBatcher = GetWorld()->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent))
    {
        PersistentLineBatcher->SetHiddenInSceneCapture(true);
    }


    // 레벨 로드 시 1회만 전체맵 캡처 (정적 배경 텍스처용) — 씬 안정화를 위해 다음 틱에 실행
    GetWorldTimerManager().SetTimerForNextTick(this, &AUI_AMiniMapCapture::UpdateMiniMap);

    // 게임 시작 / 페이즈 변경 / 금지구역 색 전환 완료 시 재캡처
    if (AER_GameState* GS = GetWorld()->GetGameState<AER_GameState>())
    {
        GS->OnGameStarted.AddDynamic(this, &AUI_AMiniMapCapture::UpdateMiniMap);
        GS->OnPhaseChanged.AddDynamic(this, &AUI_AMiniMapCapture::OnPhaseChanged_Recapture);
        GS->OnHazardVisualsFinished.AddDynamic(this, &AUI_AMiniMapCapture::UpdateMiniMap);
    }
}

void AUI_AMiniMapCapture::OnPhaseChanged_Recapture(int32 NewPhase)
{
    UpdateMiniMap();
}

