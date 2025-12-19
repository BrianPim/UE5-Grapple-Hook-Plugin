# UE5 Grapple Hook Plugin
## Version 1.0

A basic Grapple Hook feature with blueprint support

**Disclaimer:** This is an ongoing project, and future additions and functionality are planned.

## C++ Classes

### UGrappleHookController
Extends **UActorComponent**. Simply add as a component to your **PlayerCharacter** blueprint to get started!

Functions (**public**)
- **bool** HasValidGrappleTarget()
  - **BlueprintPure**  
- **bool** IsGrappling()
  - **BlueprintPure**
- **AActor*** GetGrappleEndPointActor()
  - **BlueprintPure**

Functions (**protected**)
- **void** SetupGrappleHook()
- **void** UseGrappleHook()
- **void** CancelGrappleHook()
  - **BlueprintCallable**
- **void** SetupGrapplePointActor(**FVector** ImpactPoint, **USceneComponent*** HitComponent)
- **TOptional< FHitResult >** GrappleHookLineTrace() **const**
- **bool** CheckGrappleBlocked(**FVector** Direction) **const**

Delegates (**public**)
- **FGrappleEvent** OnGrappleStart
- **FGrappleEvent** OnGrappleEnd

Variables (**public**)
- **TObjectPtr< UInputAction >** ActionGrappleHook
- **TObjectPtr< UInputMappingContext >** InputMappingContext
    
Variables (**private**)
- **UClass*** GrappleEndPointActor
  - Class of the Actor BP used to represent the end of the Grapple Hook
  - Modifiable from the blueprint Details panel.
- **float** MaxGrappleRange
  - How far away a Grapple can be initiated.
  - Modifiable from the blueprint Details panel.
- **float** MaxSpeed
  - Maximum Velocity that the Player can reach while Grappling.
  - Modifiable from the blueprint Details panel.
- **float** InitialSpeed
  -Velocity that the Player starts at when Grappling.
  - Modifiable from the blueprint Details panel.
- **float** SpeedLerpDuration
  - The time it takes to go from InitialSpeed to MaxSpeed.
  - Modifiable from the blueprint Details panel.
- **float** ReleaseRange
  - The distance from the end of the Grapple path where the Grapple action is cancelled.
  - Modifiable from the blueprint Details panel.
- **float** ReleaseVelocityMultiplier
  - What we multiply the CurrentSpeed by to apply an impulse after the grapple ends (can be set to 0 to instantly kill velocity)
  - Modifiable from the blueprint Details panel.
- **bool** CancelIfBlocked
  - Whether or not the Grapple will auto-cancel if there's an object blocking the Grapple path.
  - Modifiable from the blueprint Details panel.
- **FVector** CancelIfBlockedTraceDimensions
  - Dimensions of the Box Trace used to check if the Grapple path is blocked.
  - Modifiable from the blueprint Details panel.
- **float** CancelIfBlockedTraceOffset
  - Offset of the Box Trace used to check if the Grapple path is blocked.
  - Modifiable from the blueprint Details panel.
- **TObjectPtr< UEnhancedInputComponent >** EnhancedInputComponent
- **TObjectPtr< APlayerController >** PlayerController
- **TObjectPtr< ACharacter >** PlayerCharacter
- **TObjectPtr< UCharacterMovementComponent >** MovementComponent
- **TObjectPtr< AActor >** GrapplePoint
- **FCollisionQueryParams** GrappleCollisionQueryParams
- **float** CurrentSpeed
- **float** SpeedLerpElapsed
- **float** PreviousGravityScale
- **bool** PreviousYawBool
  
## Included Content & Blueprints of Note

### ThirdPerson Folder
Contains a modified Player Character taken from Unreal's default third person project. Grapple Hook component has already been added, and code has been added to the **Event Graph** to implement features such as custom animations, a CableActor, and the included UI widget.

### UI Folder
A folder containing a basic UI reticle widget + functionality. Using this is entirely optional, though it may provide a solid starting point for your own applications.

### Input Folder
A folder containing potentially helpful **Input Actions** and **Input Mapping Contexts**.
