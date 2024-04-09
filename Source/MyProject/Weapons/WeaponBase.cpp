// Fill out your copyright notice in the Description page of Project Settings.


// Fill out your copyright notice in the Description page of Project Settings.


#include "../Weapons/WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AWeaponBase::AWeaponBase()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	BaseDamage = 20.0f;
	MuzzleSocket = "MuzzleFlash";
	FireRate = 600;
	MaxMagCapacity = 30;
	bCanFire = true;

	SetReplicates(true);
}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	Capacity = MaxMagCapacity;
	TimeBetweenShots = 60 / FireRate;
}

void AWeaponBase::Fire()
{
	if(GetLocalRole() < ROLE_Authority)
	{
		ServerFire();
	}
	
	if(AActor* MyOwner = GetOwner())
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();
		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, ECC_Visibility, QueryParams))
		{
			AActor* HitActor = Hit.GetActor();

			UGameplayStatics::ApplyPointDamage(HitActor, BaseDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);

			if(MuzzleEffect)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
			}
		}
		//DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false,1.0f, 0, 1.0f);

		if(MuzzleEffect)
		{
			UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, WeaponMesh, MuzzleSocket);
		}
		LastTimeFired = GetWorld()->TimeSeconds;

		if(FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(),FireSound,WeaponMesh->GetSocketLocation("MuzzleFlash"));
		}
		--Capacity;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"),Capacity));
		if(Capacity <= 0)
		{
			StopFire();
			bCanFire = false;
		}
	}
}

void AWeaponBase::StartFire()
{
	if(bCanFire)
	{
		float FireDelay = FMath::Max(LastTimeFired + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
		GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &AWeaponBase::Fire,TimeBetweenShots, true, FireDelay);
	}
}

void AWeaponBase::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void AWeaponBase::Reload()
{
	WeaponMesh->PlayAnimation(ReloadingMontage,false);
	Capacity = MaxMagCapacity;
	bCanFire = true;
}


void AWeaponBase::ServerFire_Implementation()
{
	Fire();
}

bool AWeaponBase::ServerFire_Validate()
{
	return true;
}