// // Zombie Surviver Prototype \n Written by Yifu Qiao

#pragma once

#include "GameFramework/Actor.h"
#include "WoodPlank.generated.h"

UCLASS()
class ZOMBIESURVIVER_API AWoodPlank : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWoodPlank();

	virtual void Tick(float DeltaSeconds) override;

	void Init(FVector point1, FVector point2, FRotator rot, class AWoodWall* Wall);

	/** A hammer blow. The first hit on a broken board starts a weaker replacement. */
	bool Nail(int32 Amount);

	/** Zombies chew the board. Returns true when this blow snaps it. */
	bool ApplyHit(int32 Amount);

	void BreakApart();

	FORCEINLINE class UStaticMeshComponent* GetMesh() const { return m_woodPlankMesh; }
	FORCEINLINE class AWoodWall* GetBarricade() const { return Barricade; }
	FORCEINLINE bool IsBroken() const { return bBroken; }
	FORCEINLINE int32 GetLife() const { return Life; }
	FORCEINLINE int32 GetMaxLife() const { return MaxLife; }
	FORCEINLINE int32 GetGeneration() const { return Generation; }

private:
	void RefreshPose();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* m_woodPlankMesh;

	UPROPERTY()
	class AWoodWall* Barricade;

	int32 Life;
	int32 MaxLife;
	int32 Generation;
	bool bBroken;
	float Shake;
	FVector BaseScale;
	FRotator BaseRotation;
};
