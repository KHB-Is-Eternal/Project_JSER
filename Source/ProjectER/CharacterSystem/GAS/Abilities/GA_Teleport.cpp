#include "GA_Teleport.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "GameFramework/PlayerState.h"
#include "NavigationSystem.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/Subsystem/Respawn/ER_RespawnSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Engine/World.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"


UGA_Teleport::UGA_Teleport()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_Teleport::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Log, TEXT("[GA_Teleport] ActivateAbility"));

	if (TriggerEventData == nullptr || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 1. 아군 텔레포트 식별: UI가 넘겨준 GameplayEventData->Target에 액터 객체가 있다면 아군 텔레포트로 간주
	if (TriggerEventData->Target != nullptr)
	{
		TargetAllyActor = TriggerEventData->Target;
	}
	// 2. 구역 텔레포트 식별: 기존처럼 Magnitude를 통해 지역 인덱스를 받아옴
	else
	{
		TargetRegionIndex = static_cast<int32>(TriggerEventData->EventMagnitude);
	}

	// 지정된 시간 대기 태스크 생성 및 실행
	UAbilityTask_WaitDelay* WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, TeleportDelayTime);
	if (WaitDelayTask != nullptr)
	{
		WaitDelayTask->OnFinish.AddDynamic(this, &UGA_Teleport::OnDelayFinish);
		WaitDelayTask->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Teleport::OnDelayFinish()
{
	// Early Return: 액티브 상태가 아니면 즉시 탈출
	if (!IsActive())
	{
		return;
	}

	ABaseCharacter* Char = Cast<ABaseCharacter>(GetAvatarActorFromActorInfo());
	if (Char == nullptr)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	FVector DestLocation = FVector::ZeroVector;
	bool bIsAllyTeleport = false;

	// 캐스팅 시간 종료 후 실시간으로 타겟의 최신 위치 추적
	if (TargetAllyActor.IsValid())
	{
		const AActor* TargetActor = TargetAllyActor.Get();
		FVector OriginLocation = FVector::ZeroVector;
		
		// 대상을 PlayerState로 넘겼을 때의 위치 추출 처리
		if (const APlayerState* TargetPS = Cast<APlayerState>(TargetActor))
		{
			if (const APawn* TargetPawn = TargetPS->GetPawn())
			{
				OriginLocation = TargetPawn->GetActorLocation();
				bIsAllyTeleport = true;
			}
		}
		// 대상을 Character 폰 그대로 넘겼을 때의 위치 추출 처리
		else if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
		{
			OriginLocation = TargetPawn->GetActorLocation();
			bIsAllyTeleport = true;
		}

		if (bIsAllyTeleport)
		{
			// 1. 반경 100~200 유닛 사이의 랜덤 오프셋 생성
			float RandomRadius = FMath::RandRange(100.f, 200.f);
			float RandomAngle = FMath::RandRange(0.f, 360.f);

			DestLocation = OriginLocation;
			DestLocation.X += RandomRadius * FMath::Cos(FMath::DegreesToRadians(RandomAngle));
			DestLocation.Y += RandomRadius * FMath::Sin(FMath::DegreesToRadians(RandomAngle));

			// 2. 내비메시(이동 가능 구역) 보정 (벽 파고듬 및 맵 밖 추락 방지)
			if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation ProjectedLocation;
				FVector Extent(500.f, 500.f, 500.f); // 벽이나 장애물에 걸렸을 때 근처를 탐색할 범위 크기
				
				if (NavSys->ProjectPointToNavigation(DestLocation, ProjectedLocation, Extent))
				{
					// 정상적으로 보정 성공: 벽 바깥쪽/절벽 안쪽 등 안전한(Walkable) 지표면 적용
					DestLocation = ProjectedLocation.Location;
				}
				else
				{
					// 탐색 범위 내에도 발 디딜 곳이 없다면 강제로 아군 본체와 정확히 같은 위치로 폴백
					DestLocation = OriginLocation;
				}
			}
		}
	}

	// 사망 시의 동작 분기
	if (IsCharacterDead())
	{
		UER_RespawnSubsystem* RespawnSS = GetWorld()->GetSubsystem<UER_RespawnSubsystem>();
		if (RespawnSS != nullptr)
		{
			if (bIsAllyTeleport)
			{
				// 사망한 상태에서 아군 캐릭터 쪽으로 실시간 부활 로직!
				Char->Server_Revive(DestLocation);
			}
			else
			{
				// 지역으로 부활 로직
				const FTransform DestTransform = RespawnSS->GetRespawnPointLocation(TargetRegionIndex);
				Char->Server_Revive(DestTransform.GetLocation());
			}
		}
	}
	// 생존 시의 동작 분기
	else
	{
		if (bIsAllyTeleport)
		{
			// 아군의 최신 위치로 일반 텔레포트
			Char->TeleportTo(DestLocation, Char->GetActorRotation());
		}
		else
		{
			// 기존 지역 지정 방식 텔레포트
			if (AER_InGameMode* GM = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode()))
			{
				GM->RequestTeleportToRegion(Char, TargetRegionIndex);
			}
		}
	}

	// 구역 정보 업데이트
	if (ULevelAreaTrackerComponent* Tracker = Char->FindComponentByClass<ULevelAreaTrackerComponent>()) 
	{
		Tracker->UpdateArea();
	}
	else 
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_Teleport] ULevelAreaTrackerComponent missing on Character"));
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UGA_Teleport::IsCharacterDead() const
{
	if (ABaseCharacter* Char = Cast<ABaseCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UAbilitySystemComponent* ASC = Char->GetAbilitySystemComponent())
		{
			return ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Life.Death")));
		}
	}
	return false;
}
