#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ER_PointActor.generated.h"

UENUM(BlueprintType)
enum class EPointActorType : uint8
{
	None,
	SpawnPoint,
	RespawnPoint,
	ObjectPoint,
	SafePoint,
};

//16개
UENUM(BlueprintType)
enum class ERegionType : uint8
{
	None = 99,
	Arrow = 0,			//양궁	->	골목길
	BackStreet = 1,		//골목	->	달동네
	Beach = 2,			//모사	->	해변
	Church = 3,			//성당	->	병원
	Factory = 4,		//공장	->	공장
	Forest = 5,			//숲		->	산책로
	Hospital = 6,		//병원	->	오피스
	Hotel = 7,			//호텔	->	리조트
	HousingDistrict = 8,//고주	->	상업구역
	Lab = 9,			//연구소	->	광장
	MainDistrict = 10,	//번화가	->	구시가지
	Pond = 11,			//연못	->	연못
	Port = 12,			//항구	->	창고
	School = 13,		//학교	->	학교
	Sematary = 14,		//묘지	->	공원
	Temple = 15,		//절		->	수변 공원

};


UCLASS()
class PROJECTER_API AER_PointActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AER_PointActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ✅ Rep 되면 클라에서 데칼 토글
	UFUNCTION()
	void OnRep_Selected();

public:
	UFUNCTION(BlueprintCallable)
	void SetSelectedVisual(bool bOn);

public:	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPointActorType PointType = EPointActorType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERegionType RegionType = ERegionType::None;


private:
	// 루트
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	// ✅ 선택 표시 데칼
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDecalComponent> SelectedDecal;

	// ✅ 선택 상태(Rep)
	UPROPERTY(ReplicatedUsing = OnRep_Selected)
	bool bSelected = false;
};
