

#include "ZombieSurviver.h"
#include "HealthComponent.h"


// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	m_hitPoint = 100;
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UHealthComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	// ...
	AActor* owner = this->GetOwner();
}

void UHealthComponent::TakeDamage(int32 deltaDamange)
{
	m_hitPoint -= deltaDamange;

	AActor* owner = this->GetOwner();
	if (m_hitPoint <= 0 && !owner->IsPendingKill())
	{
		
	//	owner->MarkPendingKill();

		owner->SetActorHiddenInGame(true);

		// Disables collision components
		owner->SetActorEnableCollision(false);

		// Stops the Actor from ticking
		owner->SetActorTickEnabled(false);
	
	}
}

int UHealthComponent::GetHP() const
{
	return m_hitPoint;
}

