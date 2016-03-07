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

	UFUNCTION(BlueprintCallable, Category = "Wall")
	void TakeDamage(int32 deltaDamange);
	
private:
	
	TArray<class AWoodPlank*> m_allWoodPlanks;
	int32 m_hitPoint;

protected:

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", Meta = (BlueprintProtected = "true"))
	TArray<AActor*> m_leftPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", Meta = (BlueprintProtected = "true"))
	TArray<AActor*> m_rightPoints;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", Meta = (BlueprintProtected = "true"))
	class AWoodPlank* m_woodPrefab;
};
