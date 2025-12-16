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
		if (FVector::Dist(PlayerPawn->GetActorTransform().GetLocation(), GrapplePoint->GetActorTransform().GetLocation()) > ReleaseRange)
		{
			FVector GrapplePointPosition = GrapplePoint->GetActorLocation();
			FVector PlayerPosition = PlayerCharacter->GetActorLocation();
			FVector Direction = (GrapplePointPosition - PlayerPosition).GetSafeNormal();
			MovementComponent->Velocity = Direction * CurrentSpeed;

			FVector DirectionXY = Direction;
			DirectionXY.Z = 0.0f;

			FRotator TargetRotation = DirectionXY.Rotation(); // Converts vector to a rotator
			PlayerCharacter->SetActorRotation(TargetRotation);

			if (SpeedLerpElapsed < SpeedLerpDuration)
			{
				SpeedLerpElapsed = FMath::Clamp(SpeedLerpElapsed + DeltaTime, 0.0f, SpeedLerpDuration);

				CurrentSpeed = FMath::Lerp(InitialSpeed, MaxSpeed, SpeedLerpElapsed / SpeedLerpDuration);
			}

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

		if (TOptional<FHitResult> HitResult = GrappleHookLineTrace())
		{
			CurrentSpeed = InitialSpeed;
			SpeedLerpElapsed = 0.0f;
			
			PreviousGravityScale = MovementComponent->GravityScale;

			PlayerController->SetIgnoreMoveInput(true);
			MovementComponent->SetMovementMode(MOVE_Flying);
			MovementComponent->GravityScale = 0.0f;
			
			SetupGrapplePointActor(HitResult->ImpactPoint, HitResult->GetComponent());

			//Further functionality handled via Blueprint that references OnGrappleStart delegate.
			OnGrappleStart.Broadcast();
		}
		else
		{
			UE_LOG(GrappleHookLog, Error, TEXT("No Grapple Hit!"));
		}
	}
}


void UGrappleHookController::SetupGrapplePointActor(FVector ImpactPoint, USceneComponent* HitComponent)
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
	
	FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, true);
	GrapplePoint->AttachToComponent(HitComponent, AttachRules);
}


TOptional<FHitResult> UGrappleHookController::GrappleHookLineTrace()
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
		DrawDebugLine(GetWorld(), PlayerCharacter->GetActorLocation(), HitResult.ImpactPoint, FColor::Red, false, 0.f);
		return HitResult;
	}
	
	return {};
}


bool UGrappleHookController::HasValidGrappleTarget()
{
	return !GrapplePoint && GrappleHookLineTrace().IsSet();
}


void UGrappleHookController::CancelGrapple()
{
	if (GrapplePoint)
	{
		GrapplePoint->Destroy();
		GrapplePoint = nullptr;
	}

	PlayerController->SetIgnoreMoveInput(false);

	MovementComponent->SetMovementMode(MOVE_Falling);
	MovementComponent->GravityScale = PreviousGravityScale ? PreviousGravityScale : 1.0f;

	//Further functionality handled via Blueprint that references OnGrappleEnd delegate.
	OnGrappleEnd.Broadcast();
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





