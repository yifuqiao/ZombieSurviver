// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombieSurviver.h"
#include "WoodWall.h"
#include "WoodPlank.h"

// Sets default values
AWoodWall::AWoodWall()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	//m_woodPrefab = CreateDefaultSubobject<AWoodPlank>(TEXT("wood"));
	m_hitPoint = 100;
}

// Called when the game starts or when spawned
void AWoodWall::BeginPlay()
{
	Super::BeginPlay();

	if (m_woodPrefab == nullptr)
		return;

	for (int32 i = 0; i < m_leftPoints.Num(); ++i)
	{
		FActorSpawnParameters Parameters;
		Parameters.Template = m_woodPrefab;
		
		AWoodPlank* plank = GetWorld()->SpawnActor<AWoodPlank>(m_woodPrefab->GetClass(), Parameters);
		// this order is important
		plank->AttachRootComponentToActor(this);
		plank->Init(m_leftPoints[i]->GetActorLocation(), m_rightPoints[i]->GetActorLocation(), this->GetActorRotation());
		plank->SetOwner(GetOwner());
		m_allWoodPlanks.Add(plank);
	}
}

void AWoodWall::TakeDamage(int32 deltaDamange)
{
	m_hitPoint -= deltaDamange;
	if (m_hitPoint < 30)
	{
		// Hides visible components
		m_allWoodPlanks[0]->SetActorHiddenInGame(true);

		// Disables collision components
		m_allWoodPlanks[0]->SetActorEnableCollision(false);

		// Stops the Actor from ticking
		m_allWoodPlanks[0]->SetActorTickEnabled(false);
	}
	else if (m_hitPoint < 60)
	{
		// Hides visible components
		m_allWoodPlanks[1]->SetActorHiddenInGame(true);

		// Disables collision components
		m_allWoodPlanks[1]->SetActorEnableCollision(false);

		// Stops the Actor from ticking
		m_allWoodPlanks[1]->SetActorTickEnabled(false);
	}
	else if(m_hitPoint<90)
	{
		// Hides visible components
		m_allWoodPlanks[2]->SetActorHiddenInGame(true);

		// Disables collision components
		m_allWoodPlanks[2]->SetActorEnableCollision(false);

		// Stops the Actor from ticking
		m_allWoodPlanks[2]->SetActorTickEnabled(false);
	}
}

