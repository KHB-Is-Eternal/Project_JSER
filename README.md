# ProjectER: Top-Down Multiplayer Survival Action MOBA

[![Unreal Engine 5.7](https://img.shields.io/badge/Unreal%20Engine-5.7-blue?logo=unrealengine&logoColor=white)](https://www.unrealengine.com/)
[![Language](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Networking](https://img.shields.io/badge/Transport-Valve%20SteamSockets-171a21?logo=steam&logoColor=white)](https://partner.steamgames.com/)
[![Architecture](https://img.shields.io/badge/Architecture-GAS%20%7C%20StateTree%20%7C%20MVC-brightgreen)](#3-시스템-아키텍처-및-핵심-서브시스템-architectural-highlights)
[![Platform](https://img.shields.io/badge/Platform-Windows%20(DX12%20SM6)-0078D6?logo=windows&logoColor=white)](https://microsoft.com)

> **ProjectER**은 **Unreal Engine 5.7**과 최신 **ISO C++20** 표준을 기반으로 구축된 고성능 **쿼터뷰 탑다운 멀티플레이어 서바이벌 액션 배틀 아레나(Top-Down Survival Action MOBA / Battle Royale)** 프로젝트입니다.  
> 전장의 안개(Fog of War), 실시간 물리 머티리얼 기반 위상 금지구역 축소, 언리얼 엔진 5 최신 **StateTree** 및 **Significance Manager** 기반 몬스터 AI, 완전 데이터 주도형 **Gameplay Ability System(GAS)** 전투 파이프라인, 그리고 **Valve SteamSockets** 수송 계층 기반의 견고한 세션/재접속 아키텍처를 구현하고 있습니다.

---

## 목차 (Table of Contents)

1. [프로젝트 개요 및 코어 장르 (Project Overview & Core Concept)](#1-프로젝트-개요-및-코어-장르-project-overview-core-concept)
   - [1.1 장르 및 기획 의도](#11-장르-및-기획-의도)
   - [1.2 핵심 게임플레이 루프 (Core Gameplay Loop)](#12-핵심-게임플레이-루프-core-gameplay-loop)
2. [기술 스택 및 개발 환경 (Tech Stack & Environment)](#2-기술-스택-및-개발-환경-tech-stack-environment)
   - [2.1 엔진 및 컴파일러 도구 체인](#21-엔진-및-컴파일러-도구-체인)
   - [2.2 그래픽스 및 렌더링 파이프라인](#22-그래픽스-및-렌더링-파이프라인)
   - [2.3 네트워킹 및 전송 계층](#23-네트워킹-및-전송-계층)
   - [2.4 엔진 및 자체 개발 플러그인 생태계](#24-엔진-및-자체-개발-플러그인-생태계)
3. [시스템 아키텍처 및 핵심 서브시스템 (Architectural Highlights)](#3-시스템-아키텍처-및-핵심-서브시스템-architectural-highlights)
   - [3.1 Gameplay Ability System (GAS) & 전투 파이프라인](#31-gameplay-ability-system-gas-전투-파이프라인)
   - [3.2 StateTree & Significance Manager 기반 중립 몬스터 AI](#32-statetree-significance-manager-기반-중립-몬스터-ai)
   - [3.3 위상 그래프 금지구역 축소 & 모듈러 레벨 시스템](#33-위상-그래프-금지구역-축소-모듈러-레벨-시스템)
   - [3.4 아이템, 인벤토리 & 스마트 크래프팅 시스템](#34-아이템-인벤토리-스마트-크래프팅-시스템)
   - [3.5 SteamSockets 네트워킹 & 세션/재접속 아키텍처](#35-steamsockets-네트워킹-세션재접속-아키텍처)
   - [3.6 쿼터뷰 카메라, 월드 벤딩 셰이더 & MVC UI](#36-쿼터뷰-카메라-월드-벤딩-셰이더-mvc-ui)
4. [리포지토리 디렉터리 구조 및 모듈 분석 (Directory Structure & Module Breakdown)](#4-리포지토리-디렉터리-구조-및-모듈-분석-directory-structure-module-breakdown)
   - [4.1 최상위 디렉터리 구성](#41-최상위-디렉터리-구성)
   - [4.2 Source/ProjectER C++ 모듈 세부 분석](#42-sourceprojecter-c-모듈-세부-분석)
   - [4.3 Plugins/ 플러그인 모듈 세부 분석](#43-plugins-플러그인-모듈-세부-분석)
5. [프로젝트 컨트리뷰션 및 C++ 코딩 표준 (Contribution & Coding Conventions)](#5-프로젝트-컨트리뷰션-및-c-코딩-표준-contribution-coding-conventions)
   - [5.1 핵심 설계 원칙 (Early Return, PascalCase, 선언/구현 분리)](#51-핵심-설계-원칙)
   - [5.2 메모리 관리 및 에셋 참조 원칙 (TSoftObjectPtr 비동기 로딩)](#52-메모리-관리-및-에셋-참조-원칙)
   - [5.3 Const Correctness 활용 지향](#53-const-correctness-활용-지향)
   - [5.4 포인터 안전성 및 단언문(check) 활용 권장](#54-포인터-안전성-및-단언문check-활용-권장)

---

## 1. 프로젝트 개요 및 코어 장르 (Project Overview & Core Concept)

### 1.1 장르 및 기획 의도
**ProjectER**은 쿼터뷰 탑다운 시점의 MOBA 컨트롤 메카닉과 배틀로얄 서바이벌 룰셋을 융합한 **Quarter-View Multiplayer Survival Action MOBA**입니다.
* *Eternal Return*, *League of Legends* 등에서 검증된 정밀한 타겟팅/논타겟팅 조작감과 시야 제어(Line-of-Sight), 배틀로얄의 전장 축소 메카닉을 언리얼 엔진 5.7의 현대적 C++ 아키텍처로 재해석했습니다.
* **최대 3인 스쿼드 / 다수 팀 동시 대전** 환경에서 수십 종의 캐릭터 어빌리티, 중립 몬스터 사냥, 실시간 크래프팅, 전술적 와드 설치 및 전장의 안개(Fog of War) 심리전을 제공합니다.

### 1.2 핵심 게임플레이 루프 (Core Gameplay Loop)

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│                             ProjectER Core Loop Flow                             │
└──────────────────────────────────────────────────────────────────────────────────┘
  [Phase & Zone Collapse]   시드 기반 역순 BFS 금지구역 축소 및 고립 섬(Island) 방지
            │
            ▼
  [Hunting & Field Looting] 3단계 가중치 가챠(상자/몬스터 공용) 파밍 & StateTree AI 전투
            │
            ▼
  [Real-Time Smart Craft]   8칸 인벤토리 기반 우선순위 자동 탐색 & 안전한 채널링 조합
            │
            ▼
  [Vision & GAS Combat]     GPU 전장의 안개, 와드 쟁탈전, 3단계 스킬 시전 및 2단계 수명주기(Down->Death)
            │
            ▼
  [Reconnection Resilience] 연결 단절 시 폰/스탯/인벤토리 상태 보존 및 무중단 재접속 복구
```

1. **구역 전개 및 결정론적 금지구역 축소 (Phase & Area Collapse)**:
   - 무작위 최종 안전 구역(Root)으로부터 역추적하는 시드 기반 BFS 알고리즘을 통해 맵 외곽부터 중앙으로 붕괴가 전개되며, 특정 구역이 영구적으로 고립되는 현상을 위상 수학적으로 사전 차단합니다.
2. **필드 파밍 및 몬스터 사냥 (Neutral Monster Hunting & Looting)**:
   - 전장에 배치된 상자 및 중립 몬스터를 사냥하여 장비/소모품/재료를 획득합니다. 게임 페이즈에 따라 몬스터 스탯이 동적으로 스케일링되며, 30m 거리 밖에서는 Significance Manager에 의해 연산이 완벽히 컬링됩니다.
3. **실시간 스마트 제작 및 성장 (Real-Time Crafting & Leveling)**:
   - 이동을 멈추고 안전한 위치에서 `ItemRecipeTable` 기반 자동 우선순위 조합을 실행합니다. 완성된 상위 등급 장비와 스탯 물약, 순차 소비형 큐잉 음식/음료를 통해 전투력을 강화하고 레벨을 올립니다.
4. **시야 장악 및 전술적 GAS 전투 (Vision Control & Tactical GAS Combat)**:
   - GPU 레이마칭 전장의 안개(Fog of War)와 4타 분절 체력바를 가진 시야 와드를 활용하여 적의 기습을 탐지합니다. GAS의 3단계 스킬 생명주기(Casting-Active-Backswing)와 무빙 캔슬, 2단계 수명주기(빈사 다운 50% HP -> 확정 처형 사망) 메카닉을 기반으로 팀파이트를 지배합니다.
5. **무중단 재접속 회복 탄력성 (Reconnection Resilience)**:
   - 네트워크 연결이 끊긴 플레이어의 폰, 인벤토리, KDA, GAS 어트리뷰트를 서버에 안전하게 캐싱하여 매치 도중 언제든 원활하게 전장에 재진입할 수 있습니다.

---

## 2. 기술 스택 및 개발 환경 (Tech Stack & Environment)

### 2.1 엔진 및 컴파일러 도구 체인
* **Game Engine**: **Unreal Engine 5.7** (Source Build / Installed Engine at `D:\EpicGames\UE_5.7`)
  - Target Rules: `DefaultBuildSettings = BuildSettingsVersion.V6`, `IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7`
* **Programming Language**: **ISO C++20** (`/std:c++20`, C17 호환)
* **Compiler & Toolchain**: Microsoft Visual C++ (MSVC) **v143** (Toolset **14.44.35207**, Visual Studio 2022 v17.14)
* **Target Architecture**: Windows 64-bit (`Hostx64\x64`)
* **Windows SDK**: Windows 11 SDK (**10.0.22621.0**)
* **.NET Runtime**: .NET SDK **8.0.412** (`win-x64`)

### 2.2 그래픽스 및 렌더링 파이프라인
* **Graphics RHI**: **DirectX 12 (D3D12)**
* **Targeted Shader Formats**: Windows **Shader Model 6 (SM6)** (`PCD3D_SM6`), Vulkan SM6, Metal SM6
* **Substrate Materials**: `r.Substrate=True` — 차세대 모듈형 다층 머티리얼 시스템 활성화
* **Custom Depth with Stencil**: `r.CustomDepth=3` — 캐릭터 외곽선(Outline), 팀 식별 스텐실 마스크 및 전장의 안개 오클루전에 활용
* **Anti-Aliasing**: Temporal Anti-Aliasing (TAA, `r.AntiAliasingMethod=2`)
* **Virtual Textures**: Runtime Virtual Texture (RVT) 활성화 (`r.VirtualTextures=True`, `r.vt.rvt.EnableBaseColor=True`)
* **Competitive Determinism**: e스포츠 환경의 프레임 일관성 유지를 위해 Nanite 및 Lumen 동적 GI를 비활성화하고 정밀 베이킹 라이팅 및 커스텀 USH 셰이더 적용

### 2.3 네트워킹 및 전송 계층
* **NetDriver**: **Valve SteamSockets** (`SteamSocketsNetDriver`)
* **NetConnection**: `SteamSocketsNetConnection` (패킷 암호화, Valve 백본망 기반 릴레이 P2P/Listen 통신 지원)
* **Online Subsystem**: **OnlineSubsystemSteam** (AppID: `480` - Spacewar 개발용 앱 ID 연동)
* **Network Settings**: Seamless Travel 지원 (`net.AllowPIESeamlessTravel=1`), 초당 RPC 버스트 제한 최적화 (`net.MaxRPCPerNetUpdate=25`)

### 2.4 엔진 및 자체 개발 플러그인 생태계

#### Engine & Marketplace Plugins (`ProjectER.uproject`)
| 플러그인 명칭 | 로딩/범위 | 핵심 기능 및 역할 |
|---|---|---|
| `GameplayAbilities` | Runtime | Gameplay Ability System (GAS) 코어: AttributeSet, ASC, GameplayEffect, GameplayTags |
| `GameplayStateTree` | Runtime | StateTree와 GAS 간 어빌리티 트리거 및 게임플레이 이벤트 연동 계층 |
| `StateTree` | Runtime | 언리얼 엔진 5의 최신 계층형 상태 머신(HSM) 및 유틸리티 기반 의사결정 프레임워크 |
| `SteamSockets` | Runtime | Valve GameNetworkingSockets 기반 초저지연 UDP 암호화 수송 계층 |
| `OnlineSubsystemSteam` | Runtime | Steam 프렌즈, 로비 검색, 인바이트 및 매치메이킹 인터페이스 |
| `SignificanceManager` | Runtime | 뷰포트 거리 및 카메라 시야 기반 틱/연산 컬링 최적화 프레임워크 |
| `ModelingToolsEditorMode`| Editor | 에디터 레벨 내 지형 및 지오메트리 실시간 조작 툴 |

#### In-House Custom Plugins (`Plugins/`)
| 플러그인 명칭 | 로딩 페이즈 | 의존성 모듈 | 아키텍처 역할 및 세부 기능 |
|---|---|---|---|
| **`TopDownVision`** | `PostConfigInit` | `DoubleRTBufferDrawer`, `Shaders` | 전장의 안개(Fog of War) 및 시야각(Line-of-Sight) 통합 파이프라인. 커스텀 GPU 레이마칭 셰이더(`M_LOSVision_Stamp`), 동적 장애물 마스킹, 비동기 CPU 그리드 평가(`GridVisionAsyncTask`), 벽체 투명화를 위한 MID(Dynamic Material Instance) 풀링 지원 |
| **`WorldBender`** | `Default` | `RenderCore` | 버텍스 WPO(World Position Offset) 기반 구면/원통형 수평선 곡률 셰이더 효과. `CurvedWorldSubsystem`이 카메라 위치를 `CurvedWorldMPC`로 밀어넣어 렌더링 연동 |
| **`MaterialDrivenInteraction`** | `Default` | `CustomBlendModeForRTDraw` | 연속형 렌더타깃 풀링 시스템 (16개 풀링 RT, 2000cm 셀, 0.90 감쇠율). 캐릭터 이동 궤적에 따른 잔디/식생 실시간 벤딩 연동 |
| **`CustomBlendModeForRTDraw`** | `PostEngineInit` | `RenderCore` | 캔버스 `DrawTile` 실행 시 엔진의 알파 채널 억제를 우회하고 정밀한 알파 합성을 지원하는 렌더링 블렌드 모드 |
| **`DoubleRTBufferDrawer`** | `Default` | `RenderCore` | 폰 부착 캔버스의 이중 핑퐁 렌더타깃 드로어. 렌더 패스 부하를 분리하여 틱 성능 보장 |
| **`HexGridPlugin`** | `Default` | `PathFindingLibrary` | 축/입방(Axial/Cubic) 헥사고날 그리드 좌표 수학, 절차적 헥사곤 생성, 인접 인덱싱 및 반경 쿼리 |
| **`PathFindingLibrary`** | `PreLoadingScreen` | `Core`, `Engine` | `GridNodeInterface` 기반 2D/3D 다중 지오메트리 위상 A* 및 다익스트라(Dijkstra) 길찾기 알고리즘 |
| **`AdvancedSessions`** | `PreDefault` | `OnlineSubsystem` | 세션 속성 커스텀 필터링, 추가 검색 키, 음성/프레즌스 확장 블루프린트 라이브러리 |
| **`AdvancedSteamSessions`** | `PostDefault` | `OnlineSubsystemSteam` | Steam 로비 초대, 친구 목록, 클랜 태그, Steam 64비트 유저 ID 처리 전용 노드 |

---

## 3. 시스템 아키텍처 및 핵심 서브시스템 (Architectural Highlights)

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                       ProjectER Architecture Map                                       │
├────────────────────────────────┬───────────────────────────────────────┬───────────────────────────────┤
│   Combat & GAS Subsystem       │      World & Level Architecture       │      AI & Monster Subsystem   │
│ ────────────────────────────── │ ───────────────────────────────────── │ ───────────────────────────── │
│ • PlayerState ASC / AttrSet    │ • Reverse-BFS Hazard Graph            │ • UE 5.7 StateTree Tasks      │
│ • Enhanced Input Smart/Normal  │ • Physical Material NodeID Tracing    │ • Utility AI Considerations   │
│ • 3-Phase Skill State Machine  │ • On-Demand Bridge Volume Triggers    │ • Significance Manager 30m    │
│ • Sync Predicted AbilityTask   │ • LevelRootActor Serialization        │ • Dual-Sphere 800/1800 Range  │
│ • 2-Tier Life: Down -> Death   │ • Sequence Replicator with Timestamp  │ • Pack Aggro Link Broadcast   │
│ • Magnitude Formula Parser     │ • Platform Movement via GPU MPC       │ • Phase-Scaled Neutral Spawns │
├────────────────────────────────┴───────────────────────────────────────┴───────────────────────────────┤
│                                Networking, Camera & MVC Presentation Layer                             │
│ ────────────────────────────────────────────────────────────────────────────────────────────────────── │
│ • SteamSockets UDP Relay Transport Layer & GameSession Discovery ("ProjectER_Sparta")                  │
│ • Seamless Travel & FDisconnectedPlayerData (Pawn, Inventory, KDA, GAS Attributes Restoration)         │
│ • TopDownCameraComp: Follow/FreeCam Edge-Scroll & CurvedWorldMPC Vertex Horizon Deformation            │
│ • Strict MVC Pattern: AUI_HUDFactory -> UUI_HUDController (13 GAS Attr Delegates) -> UUI_MainHUD      │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

### 3.1 Gameplay Ability System (GAS) & 전투 파이프라인

#### (1) PlayerState 기반 네트워크 소유권 분리
ProjectER은 배틀로얄/MOBA 장르의 폰 사망, 리스폰 및 재접속 상황에서도 어트리뷰트와 어빌리티 스펙이 영구적으로 보존되도록 소유권을 엄격히 분리했습니다:
* **OwnerActor**: `ABasePlayerState` / `AER_PlayerState`
* **AvatarActor**: `ABaseCharacter`
* **확장 컴포넌트**: `UProjectERASC` (`UAbilitySystemComponent` 상속)
  - 소스별 GameplayCue 정밀 제거 및 2초 주기 유령(Orphan) GameplayEffect/Tag 자동 정화기 탑재.
  - GE 제거 직후 쿨다운 태그의 존재 유무를 재검증하여 UI와 네트워크 간 쿨다운 동기화 불일치를 차단하는 가드 로직 내장.

```
[Server]                                         [Client (Autonomous / Simulated)]
ABaseCharacter::PossessedBy(NewController)       ABaseCharacter::OnRep_PlayerState()
  └─ InitAbilitySystem()                           └─ InitAbilitySystem()
       ├─ ASC->InitAbilityActorInfo(PS, this)           ├─ ASC->InitAbilityActorInfo(PS, this)
       ├─ 스탯 변경 델리게이트 바인딩 (MoveSpeed 등)       ├─ 스탯 변경 델리게이트 바인딩
       ├─ HeroData->SkillDataAsset 순회 GiveAbility()    └─ InitPlayer() (로컬 카메라, UI 바인딩)
       └─ 기본 상태 이펙트 부여 (AliveState, Regen)
```

#### (2) Enhanced Input: 스마트 캐스트(Smart Cast) vs 일반 조준(Normal Cast)
`ABasePlayerController`는 키 입력 방식에 따라 조준 및 시전 모드를 동적으로 분기합니다:
* **스마트 캐스트 (`AbilityInputTagPressed`)**:
  - 커서 하향 라인트레이스(`GetHitResultUnderCursor`)로 지면 좌표를 획득.
  - `FGameplayEventData`의 `TargetData`에 즉시 패키징하여 `ASC->TriggerAbilityFromGameplayEvent()` 호출 -> 선딜레이 없이 즉시 조준 및 발동 파이프라인으로 진입.
  - `LeftCtrl` 조합 입력 감지 시 스킬 시전 대신 `OnSkillLevelUp(InputTag)`를 호출하여 스킬 포인트 투자 분기.
* **일반 조준 (`AbilityManualInputTagPressed`)**:
  - `Payload.TargetData`를 `nullptr`로 비워 전달하여 인디케이터 조준 상태(`bIsManualAiming = true`)로 진입.
  - 바닥 사거리 데칼(`ActiveRangeIndicatorComp`) 및 투사체 궤적선(`ASkillIndicatorActor`)을 렌더링.
  - 마우스 좌클릭 시 `AMouseLocationTargetActor` 또는 `ATargetActor`를 통해 최종 좌표를 확정(`ConfirmTargetingAndContinue`)하고 시전.

#### (3) 3단계 스킬 생명주기 및 무빙 캔슬 메카닉 (`USkillBase`)
모든 액티브 스킬은 몽타주 노티파이와 결합된 3단계 상태머신을 통해 구동됩니다:

```
[Start] ──> Casting (선딜레이) ──> Active (판정/효과 발동) ──> Backswing (후딜레이) ──> [End]
               │                         │                          │
        AnimNotify_Casting        AnimNotify_Active          AnimNotify_Backswing
               │                         │                          │
        • 이동 명령 차단             • ExecuteSkill() 발동      • 우클릭 이동 시 후딜 캔슬
        • 강제 취소 시 쿨타임 환불      • 판정 액터/투사체 생성       • CancelAbilities(&CancelTags)
```

* **Casting (선딜레이)**: `Skill.Animation.Casting` 태그 부여. 이동을 차단하며(`Skill.Option.AllowMovement` 제외), 하드 CC(기절, 침묵 등) 피격 시 시전이 취소되고 `AppliedCooldownHandle`을 통해 선차감된 쿨다운을 전액 환불합니다.
* **Active (발동)**: `Skill.Animation.Active` 태그 부여. 실질적인 데미지 GE 및 발사체/장판 액터를 생성합니다.
* **Backswing (후딜레이)**: `Skill.Animation.Backswing` 태그 부여. 플레이어가 우클릭 이동을 입력하는 즉시 후딜레이 애니메이션을 캔슬하고 이동을 시작하는 MOBA 특유의 **후딜 캔슬(Animation Cancel)**을 완벽히 지원합니다.
* **네트워크 폴백 타이머 (Fallback Timers)**: 패킷 유실이나 애니메이션 노티파이 누락으로 스킬이 멈추는 현상을 방지하기 위해 `SetupFallbackTimers()`를 통해 예상 시간 초과 시 강제로 상태를 전이시키는 안전장치를 내장했습니다.

#### (4) 클라이언트 예측 타임스탬프 동기화: `UAbilityTask_WaitGameplayEventSyn`
* 클라이언트가 몽타주 노티파이를 감지하면 `AGameStateBase::GetServerWorldTimeSeconds()`로 정밀 시전 시간을 산출합니다.
* `FGameplayAbilityTargetDataHandle`의 첫 번째 슬롯에는 타겟 액터 목록을, 두 번째 슬롯의 위치 정보(`LiteralTransform`)에는 클라이언트 타임스탬프를 인코딩하여 `FScopedPredictionWindow`와 함께 서버로 복제 전송합니다.
* 서버는 클라이언트 타겟 데이터 신호 도착 여부(`bClientDataPending`)를 확인하여 중복 실행을 차단하고, 랙이 발생해도 클라이언트와 서버의 판정 타이밍을 오차 없이 일치시킵니다.

#### (5) 2단계 수명주기 및 데미지 연산 파이프라인
데미지 계산은 `UBaseDamageExecutionCalc` 및 `UNormalDamageCalculation`을 통해 수행되며, 체력 고갈 시 2단계 상태머신으로 분기합니다:

$$\text{Mitigation} = \frac{100.0}{100.0 + \text{Defense}}, \quad \text{FinalDamage} = \text{RawDamage} \times \text{Mitigation}$$

```
[체력 고갈 (NewHealth <= 0)]
       │
       ├── Case 1: 현재 'Alive' 상태인 경우 ──> [빈사 다운 (Down)]
       │     ├─ SetHealth(MaxHealth * 0.5f) : 빈사 전용 50% 체력 재부여
       │     ├─ GE_DownStatus 적용 (이동속도 대폭 저하, 스킬 봉인)
       │     ├─ 8초 기여 윈도우 기반 어시스트 목록 산출 (TargetPS->GetAssists(Now, 8.0f))
       │     └─ 음식/음료 회복 큐 즉시 초기화 (ClearFoodHealEffects)
       │
       └── Case 2: 이미 'Down' 상태에서 체력이 고갈된 경우 ──> [확정 사망 (Death)]
             ├─ TargetChar->HandleDeath() 호출 (진짜 사망)
             ├─ 공격자에게 레벨 비례 경험치 지급: GrantedXP = TargetLevel * 70.0f
             └─ 인벤토리 소지품을 월드 드롭용 ULootableComponent로 변환
```

#### (6) 데이터 주도형 재귀 수식 파서: `USkillMagnitudeCalculator`
기획자가 C++ 코드 재컴파일 없이 에디터에서 복잡한 수식을 조립할 수 있도록 다형성 파서를 제공합니다:
* **연산자 (`ECalcOperator`)**: Add, Subtract, Multiply, Divide
* **피연산자 (`ECalcOperandType`)**: Constant(상수), SourceStat/TargetStat(공격력, 방어력, 스킬증폭 등 어트리뷰트 캡처), TagStack, SetByCaller, 그리고 괄호 중첩 연산을 위한 **SubFormula(재귀 하위 수식)** 지원.

#### (7) Fog of War 연동 무틱(Zero-Tick) 파티클 최적화: `UVisionParticleManagerSubsystem`
* 나이아가라 지속형 파티클이 매 틱마다 시야를 체크하며 발생하는 CPU 오버헤드를 원천 제거했습니다.
* `UVisionParticleManagerSubsystem` (WorldSubsystem)은 파티클 액터의 시야 컴포넌트 델리게이트(`OnVisionStateChanged`)를 구독하여, 로컬 플레이어의 시야에 들어올 때만 파티클을 켜고(`ApplyParticleVisibility`) 시야 밖으로 벗어나면 렌더링을 차단합니다.

---

### 3.2 StateTree & Significance Manager 기반 중립 몬스터 AI

#### (1) UE 5.7 StateTree 및 유틸리티 의사결정 (Utility Considerations)
ProjectER은 레거시 Behavior Tree를 탈피하고 언리얼 엔진 5의 최신 **StateTree** 계층형 상태 머신을 전면 도입했습니다:
* **`STCons_AttackCountUtility` (유틸리티 컨시더레이션)**:
  - 몬스터의 평타 타격 횟수(`AttackCount`)가 임계값에 도달하고 궁극기 쿨다운 태그(`Cooldown.Skill.R`)가 없으면 `Score = 1.0f`를 반환하여 강력한 특수 패턴을 동적으로 유도.
* **StateTree Tasks -> GAS 어빌리티 직접 트리거**:
  - `STT_ActivateTargetSkill`: 타겟 플레이어 데이터를 `Payload`에 담아 ASC의 `TriggerAbilityFromGameplayEvent()`로 Fast-Track 즉시 시전.
  - `STT_ActivateDirectionSkill` / `STT_ActivateGroundSkill`: 타겟 방향 벡터 및 지면 히트 결과를 계산하여 논타겟 투사체/장판 스킬 발동.
* **애니메이션 프레임 동기화**:
  - `AnimNotify_SendStateTreeEventTag`를 통해 몽타주 재생 중 특정 타격 프레임이나 모션 종료 시점에 `EventTag`를 직발송하여 타이머 없는 정밀 트랜지션을 실현.

#### (2) Significance Manager 기반 30m(3,000 유닛) 틱 원천 컬링
광대한 배틀로얄 맵에 수십 마리의 몬스터가 동시 스폰될 때 발생하는 서버 CPU 부하를 극소화하기 위해 `USignificanceManager`를 연동했습니다:
* **평가 (`EvaluateSignificance`)**: 서버 내 모든 플레이어 시점과의 거리를 측정하여 **30m(3,000 유닛, 거리 제곱 9,000,000)**를 초과하면 중요도를 `0.0f`로 판정.
* **컬링 (`PostEvaluateSignificance`)**:
  - 중요도가 `0.0f`가 되면 `CharacterMovementComponent`, `USkeletalMeshComponent`(애니메이션), `UAbilitySystemComponent`(GAS), `HPBarWidgetComp`(UI), `FoliageRTInvoker`의 **틱을 통째로 비활성화**.
  - 플레이어가 30m 이내로 접근하면 즉시 중요도 `1.0f`로 복원하여 틱을 재개.

#### (3) 이중 감지 구체(Dual-Sphere) 및 동족 군집 어그로 링크
* **이중 반경 감지 (`UMonsterRangeComponent`)**:
  - **`RangeSphere` (반경 800)**: 플레이어 진입 시 `OnPlayerCountOne` -> 경계(Alert) 상태 전환. 전원 퇴장 시 Idle 복귀.
  - **`OutSphere` (반경 1800)**: 추적 한계선(Leash Radius). 타겟이 1800 유닛 이탈 시 어그로를 해제하고 원점(`StartLocation`)으로 귀환(Return).
* **동족 팩 어그로 링크 (`MonsterGroupHitCall`)**:
  - 무리 중 한 마리가 피격되면 `AttributeSet->OnMonsterHit`를 통해 `MonsterGroup` 내 동일 ID 몬스터들에게 `SendHitEvent(Target)`를 연쇄 전파하여 무리 전체가 동시에 협공.

#### (4) 중립 몬스터 스폰 및 페이즈 레벨 스케일링 (`UER_NeutralSpawnSubsystem`)
* `UAssetManager`를 통해 몬스터 에셋을 비동기 로딩한 뒤 `SpawnActorDeferred<ABaseMonster>`로 스폰.
* `Spawned->InitMonsterData(AssetId, CurrentPhase)`를 호출하여 **현재 게임 페이즈 번호를 몬스터 레벨로 주입**.
* 몬스터의 스탯 커브 테이블(`UCurveTable`)에 따라 페이즈가 경과할수록 몬스터 체력, 공격력, 방어력이 기하급수적으로 강화.
* 금지구역에 포함된 스폰 포인트의 몬스터들은 `KillMonstersInHazards()`를 통해 자동 처형되어 서버 낭비를 방지.

---

### 3.3 위상 그래프 금지구역 축소 & 모듈러 레벨 시스템

#### (1) 결정론적 역순 BFS 붕괴 알고리즘 (Reverse-BFS Hazard Order)
배틀로얄 맵에서 안전 구역이 축소될 때 특정 구역이 잘려나가 플레이어가 갇히는 고립 섬(Disconnected Island) 현상을 위상 수학적으로 해결했습니다:
1. **결정론적 시드 (`FRandomStream(Seed)`)**: 모든 클라이언트가 일관된 붕괴 순서를 예측 가능하도록 시드 기반 난수 생성.
2. **최종 안전 구역(Root) 역추적**:
   - 전체 노드 중 무작위 노드를 최종 생존 구역(`Root`)으로 선정.
   - `Root`로부터 인접 노드들을 무작위 셔플하며 너비 우선 탐색(BFS)을 수행하여 방문 순서(`BFSOrder`)를 기록.
   - BFS 방문 순서를 역순(`Reverse`)으로 뒤집어 금지구역 순서(`HazardOrder`)를 확정 (트리의 말단 외곽 구역부터 차례로 붕괴).
3. **위상 연결성 검증 (`WouldCreateIsland`, `IsGraphConnected`)**:
   - 특정 구역을 차단했을 때 남은 안전 구역들이 2개 이상의 독립된 컴포넌트로 분리되는지 BFS로 사전 시뮬레이션하여 고립 경로 생성을 원천 차단.

#### (2) 물리 머티리얼 메타데이터 임베딩 & 온디맨드 브릿지 트레이스 (Zero-Tick)
수십 명의 플레이어가 광대한 맵을 이동할 때 매 프레임 무거운 오버랩 쿼리를 수행하지 않도록 혁신적인 최적화를 구현했습니다:
* **`ULevelAreaPhysicalMaterial`**: 피지컬 머티리얼에 구역 식별자 `NodeID`를 메타데이터로 부여하고 지형 스태틱 메시에 오버라이드.
* **`ALevelAreaBridgeVolume` & `ULevelAreaTrackerComponent`**:
  - 트래커 컴포넌트는 평상시 **Tick이 꺼져 있습니다** (`bCanEverTick = false`).
  - 구역 경계선/통로에 위치한 `ALevelAreaBridgeVolume`에 진입할 때만 활성화되어 `0.1초` 주기로 발밑 하향 레이캐스트(-150cm)를 수행.
  - 검출된 `Hit.PhysMaterial`에서 `NodeID`를 읽어 구역 전이를 감지한 뒤, 다리를 벗어나 방에 안착하면 타이머를 끄고 상태를 동결.

#### (3) 시퀀스 네트워크 동기화 및 모듈러 레벨 추출
* **`ULevelSequenceReplicatorComponent`**: 구역 붕괴 연출 시네마틱을 멀티캐스트 재생할 때 서버 시작 타임스탬프(`ServerStartTime`)를 동기화하여, 늦게 접속한 클라이언트(Late Joiner)도 재생 오차 없이 정확한 프레임 위치로 싱크 맞춤.
* **`ALevelRootActor` & `ULevelExtractionData`**: 레벨 디자이너가 배치한 방/타일의 계층 구조를 루트 역행렬(`RootInverse`)을 통해 상대 좌표계로 추출하여 에셋으로 직렬화하고, 런타임에 동적으로 로드 및 부착하는 모듈러 레벨 스트리밍 지원.
* **`APlatformUpdatorActor` & `PlatformMovement.ush`**: 이동 플랫폼의 위치/회전 쿼터니언을 매 프레임 Material Parameter Collection(MPC)으로 밀어넣어, 셰이더 WPO에서 버텍스를 직접 회전시킴으로써 스켈레탈 메시 CPU 부하를 제로화.

$$\vec{t} = 2 \cdot (\vec{q}_{xyz} \times \vec{v}), \quad \vec{v}' = \vec{v} + q_w \cdot \vec{t} + (\vec{q}_{xyz} \times \vec{t})$$

---

### 3.4 아이템, 인벤토리 & 스마트 크래프팅 시스템

#### (1) 공용 3단계 가중치 가챠 드랍 (`ULootableComponent::GenerateWeightedDrops`)
몬스터 처치 시 전리품 드랍과 월드 보물 상자(Box) 루팅이 **100% 동일한 고급 가중치 알고리즘을 공유**합니다:
1. **등급 확률 정규화**: `RarityDropRates`(Normal, Rare, Unique)의 합을 자동 정규화하여 룰렛 추첨.
2. **등급별 드랍 캡(Cap) 및 강등**: 특정 등급의 최대 드랍 수량이 소진되면 자동으로 하위 등급으로 강등 처리.
3. **등급 내 아이템 가중치 경쟁**: 선택된 등급 내 아이템들의 `FDropItemInfo::Weight`를 기반으로 최종 아이템 당첨.

#### (2) 음식/음료 순차 회복 큐잉 (`PendingFoodHealQueue`)
* 플레이어가 음식을 연속으로 섭취했을 때 회복 수치가 비정상적으로 폭증하는 것을 방지하기 위해 큐잉 시스템을 도입했습니다.
* 활성 회복 GE(`ActiveFoodGEHandle`)가 지속되는 동안 추가 섭취한 음식은 `PendingFoodHealQueue`에 대기하며, 이전 GE가 만료되는 순간 다음 음식을 꺼내어 틱당 지속 회복 GE를 동적으로 생성/적용합니다.
* 캐릭터가 빈사(Down) 또는 사망(Death) 상태에 이르면 즉시 `ClearFoodHealEffects()`를 호출하여 대기 큐와 활성 GE를 일괄 소멸시킵니다.

#### (3) 전장의 안개 연동 시야 와드 (`ABaseWardActor`)
* **지면 밀착**: `SnapToGround()` 레이캐스트를 통해 불규칙한 지형 표면에 완벽히 접지.
* **FoW 시야 및 은신**: `IVisionProviderInterface`를 통해 아군에게 시야 반경을 제공하고, 적의 시야가 닿지 않는 어둠 속에서는 적 클라이언트 렌더링에서 자동으로 컬링.
* **4타 분절형 체력바**: 일반 체력 수치가 아닌 고정 타수(4타) 기반 체력을 가지며, 피격 시 1칸씩 차감되는 분절형 HP 바를 지원.

#### (4) 실시간 스마트 제작 (Smart Crafting Flow)
* 플레이어가 제작 키를 누르면 `ItemRecipeTable`을 검색하여 현재 8칸 인벤토리에 재료 1과 재료 2를 모두 보유하고 있는 조합식 중 `Priority`가 가장 높은 최적의 레시피를 자동 선정.
* 채널링 도중 이동하거나 피격당하면 제작이 즉각 취소되며, 완료 시 `Server_CompleteCrafting` RPC를 통해 서버 권한으로 재료를 1개씩 소모하고 완성품을 안전하게 지급.

---

### 3.5 SteamSockets 네트워킹 & 세션/재접속 아키텍처

#### (1) SteamSockets 수송 계층 및 세션 서브시스템
* `SteamSocketsNetDriver`와 `SteamSocketsNetConnection`을 채택하여 Valve의 글로벌 전송 인프라 위에서 저지연 패킷 암호화 통신을 수행합니다.
* `UER_SessionSubsystem` (`UGameInstanceSubsystem`)은 세션 생성, 브라우저 검색, 참가, 파기를 총괄합니다:
  - 커스텀 검색 필터: `CUSTOM_GAME_ID = "ProjectER_Sparta"`, `CUSTOM_ROOM_NAME = "RoomName"`, `PRESENCESEARCH = true`, `LOBBYSEARCH = true`.

#### (2) 끊김 복구 및 상태 보존 (`FDisconnectedPlayerData`)
배틀로얄 매치 도중 클라이언트 연결이 일시적으로 끊겼을 때 매치가 파괴되지 않도록 복구 메커니즘을 지원합니다:
* 플레이어가 접속을 종료(`Logout`)하면 `AER_InGameMode`는 해당 플레이어의 기존 폰 참조, 인벤토리 아이템 목록(`TArray<UBaseItemData*>`), KDA 기록, 팀 ID, 그리고 **GAS 어트리뷰트 전체(`TMap<FString, float> SavedAttributes`)**를 `FDisconnectedPlayerData` 구조체에 캐싱합니다.
* 플레이어가 재접속하면 `PreLogin()`에서 고유 Net ID를 대조하고, `PostLogin()`에서 새 컨트롤러를 기존 폰에 다시 빙의(Possess)시켜 이전 상태를 무손실 복구합니다.

---

### 3.6 쿼터뷰 카메라, 월드 벤딩 셰이더 & MVC UI

#### (1) 탑다운 쿼터뷰 카메라 컴포넌트 (`UTopDownCameraComp`)
* **계층 구조**: `USpringArmComponent` (ArmLength: 2,000, Pitch: -60°, Lag Speed: 1.0) -> `UCameraComponent` + `UMainVisionRTManager` + `UMainOcclusionPainter`.
* **제어 모드**:
  - **Follow Mode**: 캐릭터 이동에 부드러운 카메라 래그(Lag)를 적용하여 추적.
  - **Free Cam Mode**: 화면 가장자리 마우스 호버 에지 스크롤(`GatherEdgeScrollInput`, 마진 20px) 및 방향키 패닝(`AddKeyPanInput`, 초당 900 유닛) 지원.
  - **Spacebar Recenter**: 스페이스바 입력 시 폰 중심으로 즉시 카메라 복귀.
* **Curved World 연동**: `UCurvedWorldSubsystem`을 통해 카메라 고도와 중심점 거리를 계산하고 `CurvedWorldMPC`를 갱신하여 지평선 메시가 화면상에서 완만하게 굽어지는 원근감을 실시간 렌더링.

#### (2) 엄격한 Model-View-Controller (MVC) 패턴 UI
ProjectER은 UI 표현 계층이 GAS 내부 구현에 직접 결합되는 것을 금지하고 엄격한 MVC 패턴을 준수합니다:

```
[Model]           GAS AttributeSet (UBaseAttributeSet)
                         │
                         ▼ (GetGameplayAttributeValueChangeDelegate)
[Controller]      UUI_HUDController (AddUObject 안전 바인딩)
                         │
                         ▼ (OnHealthChanged, OnStatChanged 등 브로드캐스트)
[View]            UUI_MainHUD & W_InventorySlot, W_LootingPopup
```

* `AUI_HUDFactory` (`AHUD` 상속): 뷰(`UUI_MainHUD`)와 컨트롤러(`UUI_HUDController`)를 생성하고 `InitOverlay()`를 통해 PlayerController, PlayerState, ASC, AttributeSet을 바인딩.
* `UUI_HUDController`: 생명력, 스태미나, 공격력, 방어력, 이동속도, 쿨타임 감소, 강인함 등 **13개 GAS 핵심 어트리뷰트 델리게이트를 `AddUObject`로 안전하게 구독**하여 댕글링 포인터 크래시를 원천 방지하고 뷰에 데이터 전달.

---

## 4. 리포지토리 디렉터리 구조 및 모듈 분석 (Directory Structure & Module Breakdown)

### 4.1 최상위 디렉터리 구성
```
d:\unreal\Project_JSER/
├── Config/                                # 엔진, 게임플레이 태그, 인풋 설정 INI
│   ├── DefaultEngine.ini                  # SteamSockets, SM6, DX12, Substrate, RHI 설정
│   ├── DefaultGameplayTags.ini            # 프로젝트 네이티브 GameplayTag 목록
│   ├── DefaultInput.ini                   # Enhanced Input 매핑 및 바인딩
│   └── DefaultGame.ini                    # AssetManager 및 AbilitySystemGlobals 설정
├── Content/                               # 언리얼 바이너리 게임 에셋 (Git LFS 관리)
│   ├── Animation/                         # 캐릭터 및 몬스터 애니메이션 시퀀스/몽타주
│   ├── BCW/                               # 몬스터 DataAsset, 스킬 데이터, AI 파라미터
│   ├── Level/                             # 게임 레벨 (Level_Title, Level_Lobby, BattleMap_Double)
│   ├── MonsterAsset/                      # 몬스터 스켈레탈 메시, 머티리얼, 텍스처
│   ├── PlayCharacter/                     # 플레이어블 캐릭터 메시, 코스메틱 에셋
│   └── VFX/                               # 나이아가라(Niagara) 파티클 시스템
├── Plugins/                               # 9개 자체 개발 및 확장 플러그인 (상세 후술)
├── Shaders/                               # 커스텀 셰이더 소스
│   └── PlatformMovement/
│       └── PlatformMovement.ush           # 쿼터니언 WPO 기반 GPU 플랫폼 변환 셰이더
├── Source/                                # C++ 소스 코드
│   ├── ProjectER.Target.cs                # 게임 타깃 룰 (BuildSettings V6, bUsesSteam=true)
│   ├── ProjectEREditor.Target.cs          # 에디터 타깃 룰 (BuildSettings V6)
│   └── ProjectER/                         # 메인 게임플레이 모듈 (상세 후술)
├── ProjectER.uproject                     # UE 5.7 프로젝트 기술서 및 플러그인 매니페스트
└── README.md                              # 본 프로젝트 종합 아키텍처 및 온보딩 문서
```

### 4.2 `Source/ProjectER/` C++ 모듈 세부 분석

| 디렉터리 경로 | 핵심 클래스 및 구조체 | 아키텍처 역할 및 책임 |
|---|---|---|
| `Camera/` | `UTopDownCameraComp` | 팔로우/자유시점 에지 스크롤 카메라, `CurvedWorldMPC` 갱신 |
| `CharacterSystem/Character/` | `ABaseCharacter`, `PathfindingBenchmarkActor`, `ABaseProjectile` | ASC 아바타 폰, 비동기 길찾기, 평타 콤보, 2단계 수명주기(Down/Death) |
| `CharacterSystem/Player/` | `ABasePlayerController`, `ABasePlayerState` | 스마트/일반 조준 입력 분기, ASC & AttributeSet 영속적 소유권 관리 |
| `CharacterSystem/GAS/` | `UProjectERASC`, `UBaseAttributeSet`, `UBaseDamageExecutionCalc` | 확장 ASC(유령 GE/태그 클리너), 13개 전투/자원 어트리뷰트, 방어력 감쇄 데미지 정산 |
| `CharacterSystem/Data/` | `UCharacterData`, `UInputConfig` | 캐릭터 데이터 에셋 (스켈레탈 메시, QWER 스킬 에셋 매핑, 스탯 커브) |
| `SkillSystem/GameAbility/` | `USkillBase`, `UMouseClickSkill`, `UMouseTargetSkill`, `UInstantSkill` | 3단계 스킬 라이프사이클(Casting/Active/Backswing), 선차감 쿨다운 환불, 무빙 캔슬 |
| `SkillSystem/AbilityTask/` | `UAbilityTask_WaitGameplayEventSyn` | 몽타주 노티파이의 클라이언트-서버 예측 및 타임스탬프 동기화 태스크 |
| `SkillSystem/Calculator/` | `USkillMagnitudeCalculator` | 데이터 주도형 사칙연산, 어트리뷰트 캡처 및 재귀 서브 수식 파서 |
| `SkillSystem/Actor/` | `ABaseRangeOverlapEffectActor`, `ABaseMissileActor` | 장판 및 발사체 논리 판정 액터 (클라이언트 비주얼 핸드셰이크 지원) |
| `SkillSystem/GameplayCueNotify/` | `AGCN_SummonedActor`, `UVisionParticleManagerSubsystem` | 판정 액터와 비주얼 분리 연동, 전장의 안개 연동 무틱(Zero-Tick) 파티클 서브시스템 |
| `Monster/` | `ABaseMonster`, `ABaseAIController`, `UMonsterRangeComponent` | StateTree 연동 몬스터 본체, 이중 반경(800/1800) 감지, 동족 팩 어그로 링크 |
| `Monster/StateTree/ & Animation/` | `STT_ActivateTargetSkill`, `STCons_AttackCountUtility`, `AnimNotify_SendStateTreeEventTag` | StateTree 커스텀 태스크/조건/유틸리티 컨시더레이션, 애니메이션 프레임 동기화 |
| `ModularLevel/` | `ALevelRootActor`, `ULevelExtractionData` | 모듈러 룸 액터 상대좌표 추출, 직렬화 및 런타임 동적 로드/부착 |
| `LevelManagement/` | `ULevelAreaGraphSubsystem`, `ULevelAreaPhysicalMaterial`, `ALevelAreaBridgeVolume`, `ULevelAreaTrackerComponent`, `ULevelSequenceReplicatorComponent` | 위상 역순 BFS 금지구역 축소, 피지컬 머티리얼 NodeID 트레이싱, 온디맨드 브릿지 감지(0.1s), 시퀀스 타임스탬프 멀티캐스트 |
| `ItemSystem/` | `UBaseInventoryComponent`, `UBaseItemData`, `UUsableItemData`, `ULootableComponent`, `ABaseWardActor`, `FItemRecipeRow` | 8칸 인벤토리, 3단계 가챠 드랍, 순차 회복 큐잉(음식/음료), FoW 시야 와드, 실시간 제작 |
| `GameModeBase/` | `AER_InGameMode`, `AER_OutGameMode`, `AER_GameState`, `AER_PlayerState` | 인게임/아웃게임 흐름, 페이즈 동기화, `FDisconnectedPlayerData` 기반 무중단 재접속 처리 |
| `GameModeBase/Subsystem/` | `UER_SessionSubsystem`, `UER_NeutralSpawnSubsystem` | SteamSockets 세션 관리(`ProjectER_Sparta`), 페이즈 레벨 몬스터 스폰 |
| `Public/UI/` & `Private/UI/` | `AUI_HUDFactory`, `UUI_HUDController`, `UUI_MainHUD` | 엄격한 MVC 패턴 HUD, 13개 GAS 어트리뷰트 `AddUObject` 바인딩 |

### 4.3 `Plugins/` 플러그인 모듈 세부 분석
* **`TopDownVision`**: MOBA 특화 2D/3D 하이브리드 시야 파이프라인. 커스텀 USH 셰이더 레이마칭을 통해 벽 뒤 그림자를 계산하고, 시야각 밖 오브젝트를 렌더링에서 마스킹하며, 벽체 투명화용 Dynamic Material Instance를 풀링 관리.
* **`WorldBender`**: 지평선 곡면 왜곡 셰이더. `CurvedWorldSubsystem`이 카메라 위치를 매 프레임 MPC로 주입하여 버텍스 WPO를 통해 맵 전체에 곡률 원근감을 부여.
* **`MaterialDrivenInteraction`**: 16개 풀링 렌더타깃을 활용하여 캐릭터의 이동 벡터를 기록하고, 잔디/수풀 머티리얼이 이를 샘플링하여 밟힌 식생의 실시간 변형 및 복원 연출.
* **`HexGridPlugin` & `PathFindingLibrary`**: 육각 그리드 수학 라이브러리와 지오메트리 독립적인 A* 길찾기 엔진.

---

## 5. 프로젝트 컨트리뷰션 및 C++ 코딩 표준 (Contribution & Coding Conventions)

Unreal Engine 공식 코딩 표준을 기본으로 하며, 코드 가독성과 안정성을 위해 아래의 가이드라인 적용을 지향합니다.

### 5.1 핵심 설계 원칙
* **조기 반환 (Early Return)**: 깊은 중첩 구조를 배제하고 유효하지 않은 조건은 함수의 최상단에서 즉시 반환하여 가독성을 높입니다.  
  *(참고: 아래 코드는 프로젝트 내 실제 구현체가 아닌 코딩 스타일 설명을 위한 예시 코드입니다)*
  ```cpp
  // [컨벤션 참고용 예시 코드]
  void ABaseCharacter::ExecuteSkill(USkillBase* InSkill)
  {
      if (InSkill == nullptr)
      {
          return;
      }
      if (!CanCastAbilities())
      {
          return;
      }

      InSkill->Activate();
  }
  ```
* **명명 규칙 (PascalCase)**: 모든 클래스, 함수, 변수, 열거형 명칭은 언리얼 엔진 표준 PascalCase를 적용합니다 (`bIsCombat`, `CurrentHealth`, `ExecuteSkill`).
* **선언부와 구현부의 분리**: 헤더 파일(`.h`)에는 클래스 정의, 함수 프로토타입, 외부 변수 등 필수 선언만 포함하여 컴파일 타임을 단축하고 순환 참조를 차단합니다. 모든 로직 본문은 소스 파일(`.cpp`)에 구현합니다.

### 5.2 메모리 관리 및 에셋 참조 원칙
* **소프트 레퍼런스 우선 (`TSoftObjectPtr`, `TSoftClassPtr`)**:
  - `UPROPERTY` 필드에서 메시, 텍스처, 데이터 에셋을 참조할 때 직접적인 하드 포인터(`UObject*`) 대신 `TSoftObjectPtr<T>` 또는 `FSoftObjectPath` 활용을 권장합니다.
  - 이를 통해 부모 액터 로드 시 연관 에셋이 불필요하게 메모리에 일괄 로드되는 것을 방지하고, `UAssetManager`를 통한 비동기 로딩을 지원합니다.

### 5.3 Const Correctness 활용 지향
* 코드 불변성을 확보하고 가독성과 컴파일러 최적화를 위해, 함수 인자, 반환형, 멤버 함수 등에 `const`를 적극적으로 활용하는 것을 지향합니다.
  ```cpp
  // [컨벤션 참고용 예시 코드]
  const FVector GetTargetAimPoint() const;
  void ProcessDamage(const float InRawDamage, const FGameplayTagContainer& InTags);
  ```

### 5.4 포인터 안전성 및 단언문(check) 활용 권장
* **런타임 널 검사 (Runtime Nullptr Check)**:
  - 동적으로 변경되거나 지연 로딩될 가능성이 있는 포인터는 역참조 전 `if (MyPointer != nullptr)` 형태의 안전 검사를 권장합니다.
* **개발 단계 단언문 (check Assertion)**:
  - 시스템 초기화 시점에 필수적인 포인터처럼 `nullptr`가 논리적 결함에 해당하는 경우, 개발 중 조기 버그 검출을 위해 `check(MyPointer);` 단언문 활용을 권장합니다.
  ```cpp
  // [컨벤션 참고용 예시 코드]
  void ABaseCharacter::PossessedBy(AController* NewController)
  {
      Super::PossessedBy(NewController);
      check(NewController);

      AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
      if (PS != nullptr)
      {
          InitAbilitySystem();
      }
  }
  ```

---

## 라이선스 및 크레딧 (License & Credits)
* 본 프로젝트는 Unreal Engine 5.7 및 C++20 기반 포트폴리오 및 기술 연구용 프로젝트입니다.
* Engine and Core Modules: © Epic Games, Inc.
* Steam Integration: © Valve Corporation.