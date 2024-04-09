// Fill out your copyright notice in the Description page of Project Settings.


#include "../Components/HealthComponent.h"


UHealthComponent::UHealthComponent()
{
	DefaultHealth = 100;
	Health = 100;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* MyOwner = GetOwner();
	if(MyOwner)
	{
		MyOwner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if(Damage <= 0.0f)
	{
		return;
	}
	Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);
	OnHealthChanged.Broadcast(this, Health, Damage, DamageType,InstigatedBy, DamageCauser);
}

