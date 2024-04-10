// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedSignature, class UHealthComponent*, HealthComp,float, Health, float, HealthDelta, const class UDamageType*, DamageType, class AController*, InstigatedBy, AActor*, DamageCauser);

UCLASS( ClassGroup=(MTPS), meta=(BlueprintSpawnableComponent) )
class MYPROJECT_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();

protected:

	UPROPERTY(Replicated, BlueprintReadOnly, Category= "HealthComponent")
	float Health;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "HealthComponent")
	float DefaultHealth;

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	// Called when the game starts
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable, Category= "HealthComponent")
	float GetHealth() {return Health;}

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChangedSignature OnHealthChanged;
};

