// Fill out your copyright notice in the Description page of Project Settings.


#include "../Components/HealthComponent.h"

#include "Net/UnrealNetwork.h"


UHealthComponent::UHealthComponent()
{
	DefaultHealth = 100;
	Health = 100;
	SetIsReplicated(true);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	if(GetOwnerRole() == ROLE_Authority)
	{
		AActor* MyOwner = GetOwner();
		if(MyOwner)
		{
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
		}
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

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UHealthComponent, Health);
}