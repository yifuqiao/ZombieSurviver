

#include "ZombieSurviver.h"
#include "AIBrain.h"
#include "AIPathFollowingComponent.h"
#include "HealthComponent.h"



AAIBrain::AAIBrain(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAIPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
	
	UHealthComponent* hpComponent = CreateDefaultSubobject<UHealthComponent>(FName("HealthComponent"));
	GetOwner()->AddComponent(FName("HealthComponent"), false, FTransform(FVector()), hpComponent);
}

void AAIBrain::BeginPlay()
{
	Super::BeginPlay();
}

void AAIBrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (m_targetPlayer != nullptr)
	{
		UNavigationSystem* navSystem = GetWorld()->GetNavigationSystem();
		//UNavigationPath* path= navSystem->FindPathToActorSynchronously(GetWorld(), GetPawn()->GetActorLocation(), m_targetPlayer);
		navSystem->SimpleMoveToActor(this, m_targetPlayer);
	}
}
void AAIBrain::SetTarget(AActor* player)
{
	m_targetPlayer = player;
}


