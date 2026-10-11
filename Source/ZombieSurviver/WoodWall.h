// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "WoodWall.generated.h"

UCLASS()
class ZOMBIESURVIVER_API AWoodWall : public AActor
{
	GENERATED_BODY()
	
public:	
	AWoodWall();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Zombies bite the outermost living board. Extra damage carries to the next one. */
	UFUNCTION(BlueprintCallable, Category = "Wall")
	void TakeDamage(int32 deltaDamange);

	/** One hammer blow. 0 = nothing to nail, 1 = board took a nail, 2 = a board was finished. */
	UFUNCTION(BlueprintCallable, Category = "Wall")
	int32 RepairWithHammer(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Wall")
	bool CanRepair() const;

	UFUNCTION(BlueprintPure, Category = "Wall")
	bool IsBreached() const { return bBreached; }

	UFUNCTION(BlueprintPure, Category = "Wall")
	float GetIntegrity() const;

	UFUNCTION(BlueprintPure, Category = "Wall")
	int32 GetIntactPlanks() const;

	UFUNCTION(BlueprintPure, Category = "Wall")
	int32 GetPlankCount() const { return m_allWoodPlanks.Num(); }

	/** 0 = fresh lumber, 1 = boards that have already been replaced several times. */
	UFUNCTION(BlueprintPure, Category = "Wall")
	float GetWearAlpha() const;

	UFUNCTION(BlueprintPure, Category = "Wall")
	FVector GetDoorLocation() const { return DoorLocation; }

	UFUNCTION(BlueprintPure, Category = "Wall")
	FVector GetDoorNormal() const { return DoorNormal; }

	UFUNCTION(BlueprintPure, Category = "Wall")
	int32 GetEntranceIndex() const { return EntranceIndex; }

	UFUNCTION(BlueprintPure, Category = "Wall")
	FString GetEntranceLabel() const;

	bool Separates(const FVector& A, const FVector& B) const;
	FVector GetAttackPoint(const AActor* Attacker) const;
	FVector GetInsidePoint(const FVector& InsideLocation) const;
	void SetEntranceIndex(int32 Index);

private:
	void BreakPlank(class AWoodPlank* Plank);
	void CacheDoor();
	void EnsureSeal();
	void SetSealBlocks(bool bBlock);
	void RefreshSeal();
	void UpdateTotals();
	class AWoodPlank* FindOutermostIntact() const;

	TArray<class AWoodPlank*> m_allWoodPlanks;

	int32 m_hitPoint;
	int32 m_maxHitPoint;
	float ShakeRemaining;
	float LastImpactSoundTime;
	FRotator BaseRotation;
	FVector DoorLocation;
	FVector DoorNormal;
	float DoorHalfWidth;
	float DoorHalfHeight;
	int32 EntranceIndex;
	bool bBreached;
	bool bBreachLatched;

	class UPrimitiveComponent* DoorSeal;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", Meta = (BlueprintProtected = "true"))
	TArray<AActor*> m_leftPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", Meta = (BlueprintProtected = "true"))
	TArray<AActor*> m_rightPoints;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", Meta = (BlueprintProtected = "true"))
	class AWoodPlank* m_woodPrefab;
};
