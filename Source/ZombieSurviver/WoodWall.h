// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "WoodWall.generated.h"

UCLASS()
class ZOMBIESURVIVER_API AWoodWall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWoodWall();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Wall")
	void TakeDamage(int32 deltaDamange);
	
private:
	void BreakPlank(class AWoodPlank* Plank);
	
	TArray<class AWoodPlank*> m_allWoodPlanks;
	int32 m_hitPoint;
	int32 m_maxHitPoint;
	int32 m_brokenCount;
	float ShakeRemaining;
	float LastImpactSoundTime;
	FRotator BaseRotation;

protected:

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", Meta = (BlueprintProtected = "true"))
	TArray<AActor*> m_leftPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", Meta = (BlueprintProtected = "true"))
	TArray<AActor*> m_rightPoints;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", Meta = (BlueprintProtected = "true"))
	class AWoodPlank* m_woodPrefab;
};
