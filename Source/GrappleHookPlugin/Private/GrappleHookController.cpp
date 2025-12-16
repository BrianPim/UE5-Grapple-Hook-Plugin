// Copyright (c) 2025 Brian Pimentel


#include "GrappleHookController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GrappleHookLog.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

class UEnhancedInputLocalPlayerSubsystem;
// Sets default values for this component's properties
UGrappleHookController::UGrappleHookController()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGrappleHookController::BeginPlay()
{
	Super::BeginPlay();

	SetupGrappleHookInput();
}


// Called every frame
void UGrappleHookController::TickComponent(float DeltaTime, ELevelTick TickType,
										   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PlayerPawn && GrapplePoint)
	{
		if (FVector::Dist(PlayerPawn->GetActorTransform().GetLocation(), GrapplePoint->GetActorTransform().GetLocation()) > GrappleReleaseRange)
		{
			FVector GrapplePointPosition = GrapplePoint->GetActorLocation();
			FVector PlayerPosition = PlayerCharacter->GetActorLocation();
			FVector Direction = (GrapplePointPosition - PlayerPosition).GetSafeNormal();
			MovementComponent->Velocity = Direction * GrappleSpeed;

			FVector DirectionXY = Direction;
			DirectionXY.Z = 0.0f;

			FRotator TargetRotation = DirectionXY.Rotation(); // Converts vector to a rotator
			PlayerCharacter->SetActorRotation(TargetRotation);

			DrawDebugSphere(GetWorld(), GrapplePointPosition, 12.f, 16, FColor::Green, false, 0.f);
			DrawDebugLine(GetWorld(), PlayerPosition, GrapplePointPosition, FColor::Green, false, 0.f);
		}
		else
		{
			CancelGrapple();
		}
	}
}


void UGrappleHookController::HandleUseGrappleHook()
{
	if (PlayerPawn && PlayerController)
	{
		if (GrapplePoint)
		{
			CancelGrapple();
			return;
		}
		
		FHitResult* HitResult = GrappleHookLineTrace();

		if (HitResult)
		{
			SetupGrapplePointActor(HitResult->ImpactPoint, HitResult->GetActor());

			if (PlayerCharacter && MovementComponent)
			{
				PreviousGravityScale = MovementComponent->GravityScale;

				PlayerController->SetIgnoreMoveInput(true);
				MovementComponent->SetMovementMode(MOVE_Flying);
				MovementComponent->GravityScale = 0.f;
			}
		}
		else
		{
			UE_LOG(GrappleHookLog, Error, TEXT("No Grapple Hit!"));
		}
	}
}


void UGrappleHookController::CancelGrapple()
{
	if (GrapplePoint)
	{
		GrapplePoint->Destroy();
		GrapplePoint = nullptr;
	}

	PlayerController->SetIgnoreMoveInput(false);

	if (MovementComponent)
	{
		MovementComponent->SetMovementMode(MOVE_Falling);
		MovementComponent->GravityScale = PreviousGravityScale ? PreviousGravityScale : 1.f;
	}
}


void UGrappleHookController::SetupGrapplePointActor(FVector ImpactPoint, AActor* HitActor)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GrapplePoint = GetWorld()->SpawnActor<AActor>(
		AActor::StaticClass(),
		ImpactPoint,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!GrapplePoint->GetRootComponent())
	{
		USceneComponent* Root = NewObject<USceneComponent>(GrapplePoint, TEXT("Root"));
		Root->RegisterComponent();
		GrapplePoint->SetRootComponent(Root);
	}

	GrapplePoint->SetActorLocation(ImpactPoint);

	FAttachmentTransformRules AttachRules(EAttachmentRule::KeepRelative, true);
	GrapplePoint->AttachToActor(HitActor, AttachRules);
	//GrapplePoint->AttachToComponent(HitActor->GetRootComponent(), AttachRules);

	if (HitActor)
	{
		UE_LOG(GrappleHookLog, Warning, TEXT("Grapple Hit Actor: %s"), *HitActor->GetName());
	}
	
	UE_LOG(GrappleHookLog, Warning, TEXT("Grapple! %s"), *ImpactPoint.ToString());
	UE_LOG(GrappleHookLog, Warning, TEXT("Grapple Point: %s"), *GrapplePoint->GetActorLocation().ToString());
}


FHitResult* UGrappleHookController::GrappleHookLineTrace()
{
	FVector CameraLocation;
	FRotator CameraRotation;

	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	FVector Normal = CameraRotation.Vector().GetSafeNormal();
		
	FHitResult HitResult;

	FVector CastStart = CameraLocation;
	FVector CastEnd = CameraLocation + Normal * MaxGrappleRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(PlayerPawn);

	bool GrappleHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CastStart,
		CastEnd,
		ECC_Visibility,
		Params
	);
	
	if (GrappleHit)
	{
		return &HitResult;
	}
	
	return nullptr;
}


void UGrappleHookController::SetupGrappleHookInput()
{
	//Store a reference to the Player's Pawn
	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	checkf(PlayerPawn, TEXT("Unable to get reference to the Local Player's Character Pawn"));

	//Store a reference to the Player's PlayerController
	PlayerController = Cast<APlayerController>(PlayerPawn->GetController());
	checkf(PlayerController, TEXT("Unable to get reference to the Local Player's PlayerController"));

	PlayerCharacter = Cast<ACharacter>(PlayerPawn);
	checkf(PlayerCharacter, TEXT("Unable to get reference to the Local Player's Character"));

	MovementComponent = PlayerCharacter->GetCharacterMovement();
	checkf(MovementComponent, TEXT("Unable to get reference to the Local Player's CharacterMovementComponent"));
	
	//Get Local Player
	TObjectPtr<ULocalPlayer> LocalPlayer = PlayerController->GetLocalPlayer();

	//Get a reference to the EnhancedInputComponent
	EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
	checkf(EnhancedInputComponent, TEXT("Unable to get reference to the EnhancedInputComponent"));

	//Adding the grapple InputMappingContext to the Player
	TObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	checkf(InputSubsystem, TEXT("Unable to get reference to the EnhancedInputLocalPlayerSubsystem"));
	
	checkf(InputMappingContext, TEXT("InputMappingContext was not specified"));
	InputSubsystem->AddMappingContext(InputMappingContext, 0);
	

	//Bind input action, only attempt to bind if valid value was provided
	if (ActionGrappleHook)
	{
		EnhancedInputComponent->BindAction(ActionGrappleHook, ETriggerEvent::Triggered, this, &UGrappleHookController::HandleUseGrappleHook);
	}
	else
	{
		UE_LOG(GrappleHookLog, Error, TEXT("Grapple Input Missing!"));
	}

	UE_LOG(GrappleHookLog, Display, TEXT("Grapple Setup Complete!"));
}





