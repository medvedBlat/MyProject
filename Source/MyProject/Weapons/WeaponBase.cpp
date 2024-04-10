// Fill out your copyright notice in the Description page of Project Settings.


// Fill out your copyright notice in the Description page of Project Settings.


#include "../Weapons/WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AWeaponBase::AWeaponBase()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	BaseDamage = 20.0f;
	MuzzleSocket = "MuzzleFlash";
	FireRate = 600;
	MaxMagCapacity = 0;
	bCanFire = true;

	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
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

		FVector TraceEndPoint = TraceEnd;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, ECC_Visibility, QueryParams))
		{
			AActor* HitActor = Hit.GetActor();

			UGameplayStatics::ApplyPointDamage(HitActor, BaseDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);

			PlayImpactEffect(Hit.ImpactPoint);

			TraceEndPoint = Hit.ImpactPoint;
		}
		//DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false,1.0f, 0, 1.0f);

		PlayFireEffect(TraceEndPoint);

		if(GetLocalRole() == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TraceEndPoint;
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

void AWeaponBase::OnRep_HitScanTrace()
{
	PlayFireEffect(HitScanTrace.TraceTo);
	PlayImpactEffect(HitScanTrace.TraceTo);
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(AWeaponBase, HitScanTrace, COND_SkipOwner);
}

void AWeaponBase::PlayFireEffect(FVector TraceEnd)
{
	if(MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, WeaponMesh, MuzzleSocket);
	}
}

void AWeaponBase::PlayImpactEffect(FVector ImpactPoint)
{
	if(ImpactEffect)
	{
		FVector MuzzleSocketLocation = WeaponMesh->GetSocketLocation(MuzzleSocket);
		FVector ShotDirection = ImpactPoint - MuzzleSocketLocation;
		ShotDirection.Normalize();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, ImpactPoint, ShotDirection.Rotation());
	}

}
