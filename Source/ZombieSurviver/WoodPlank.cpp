// // Zombie Surviver Prototype \n Written by Yifu Qiao

#include "ZombieSurviver.h"
#include "WoodPlank.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AWoodPlank::AWoodPlank()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	m_woodPlankMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("wood"));
	RootComponent = m_woodPlankMesh;
}

void AWoodPlank::Init(FVector point1, FVector point2, FRotator rot)
{
	m_woodPlankMesh->SetWorldLocation((point1 + point2)*0.5f);
	m_woodPlankMesh->SetWorldRotation(rot);
}

