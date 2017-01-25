// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "GrappleStudy.h"
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "CableComponent.h"
#include "GrappleStudyCharacter.h"

//////////////////////////////////////////////////////////////////////////
// AGrappleStudyCharacter

AGrappleStudyCharacter::AGrappleStudyCharacter()
{
	//We won't be ticked by default  
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	SpringArm->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	Camera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Grapple Cable
	GrappleCable = CreateDefaultSubobject<UCableComponent>(TEXT("GrappleCable"));
	//GrappleCable->SetupAttachment(RootComponent);
    GrappleCable->SetAbsolute(true, false, false);
    GrappleCable->SetVisibility(false);
    GrappleCable->CableLength = 0.0f;
    GrappleCable->NumSegments = 3;
    GrappleCable->SolverIterations = 10;
    GrappleCable->CableWidth = 8.0f;
    GrappleCable->NumSides = 8;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGrappleStudyCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AGrappleStudyCharacter::PerformJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Handle Grapple
	PlayerInputComponent->BindAction("FireGrapple", IE_Pressed, this, &AGrappleStudyCharacter::FireGrapple);
	PlayerInputComponent->BindAction("FireGrapple", IE_Released, this, &AGrappleStudyCharacter::StopGrapple);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGrappleStudyCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGrappleStudyCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AGrappleStudyCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AGrappleStudyCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AGrappleStudyCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AGrappleStudyCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AGrappleStudyCharacter::OnResetVR);
}


void AGrappleStudyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AGrappleStudyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bHooked)
	{
		if (bHookMoveFinished)
		{
			MoveGrappledPlayer();
		}
		else
		{
			bHookMoveFinished = MoveRope();
		}
	}
}

void AGrappleStudyCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AGrappleStudyCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	// jump, but only on the first touch
	if (FingerIndex == ETouchIndex::Touch1)
	{
		Jump();
	}
}

void AGrappleStudyCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch1)
	{
		StopJumping();
	}
}

bool AGrappleStudyCharacter::MoveRope()
{
    GrappleCable->SetVisibility(true);
    //GrappleParticle->SetActive(true);
    
    float HookAlpha = (GetWorld()->GetTimeSeconds() - HookGrappleTime) / HookMoveTotalTime;
    HookAlpha = UKismetMathLibrary::FClamp(HookAlpha, 0.0f, 1.0f);
    
    if (HookAlpha == 1.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveRope Endded."));
        return true;
    }
    else
    {
        FVector newLocation = UKismetMathLibrary::VLerp(HookLocation, HookGrappleLocation, HookAlpha);
        GrappleCable->SetWorldLocation(newLocation);
 
        float dist = FVector::Dist(GrappleCable->GetComponentLocation(), GetActorLocation());
        UE_LOG(LogTemp, Warning, TEXT("Cable Length %f."), dist);
        GrappleCable->CableLength = dist * 1.10f;
        
        dist = FVector::Dist(GrappleCable->GetComponentLocation(), HookGrappleLocation);
        UE_LOG(LogTemp, Warning, TEXT("Moving Rope %f."), dist);
        
        return false;
    }
}

void AGrappleStudyCharacter::MoveGrappledPlayer()
{
    FVector direction = HookGrappleLocation - GetActorLocation();
	FVector launchVelocity = GetWorld()->GetDeltaSeconds() * 100.0f * direction;
	LaunchCharacter(launchVelocity, true, true);

    float dist = FVector::Dist(GetActorLocation(), HookGrappleLocation);
//    if (dist < 250.0f)
//        StopGrapple();
    //UE_LOG(LogTemp, Warning, TEXT("Updated Cable Length %f."), dist);
    GrappleCable->CableLength = dist;
}

void AGrappleStudyCharacter::StopGrapple()
{
	bHooked = false;
	bHookMoveFinished = false;
	GrappleCable->SetVisibility(false);
    //GrappleCable->SetWorldLocation(FVector(0.0f, 40.0f, 40.0f));
}

void AGrappleStudyCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AGrappleStudyCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AGrappleStudyCharacter::MoveForward(float Value)
{
    if ((Controller != NULL) && (Value != 0.0f)) //&& !bHooked)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AGrappleStudyCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f)) // && !bHooked)
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AGrappleStudyCharacter::PerformJump()
{
	if (bHooked)
		LaunchCharacter(FVector(0.0f, 0.0f, 750.0f), true, true);
	StopGrapple();
	Jump();
}

void AGrappleStudyCharacter::FireGrapple()
{
	//SetActorRotation(FRotator(0.0f, Camera->GetComponentRotation().Yaw, 0.0f));
	FVector Start = Camera->GetComponentLocation() + (Camera->GetForwardVector() * SpringArm->TargetArmLength);
	FVector End = Start + (Camera->GetForwardVector() * 5000.0f);

	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjects;
	TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	//TraceObjects.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

	FHitResult hit;
	bool hitResult = UKismetSystemLibrary::LineTraceSingleForObjects(
		this,
		Start,
		End,
		TraceObjects,
		false,
		TArray<AActor*>(),
		EDrawDebugTrace::ForDuration,
		hit,
		true,
		FLinearColor::Green, FLinearColor::Black, 3.0f);

	if (hitResult)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit Something: %f"), hit.Distance);
        UE_LOG(LogTemp, Warning, TEXT("%f %f %f"), hit.Location.X, hit.Location.Y, hit.Location.Z);
        
        // Initialize Hook Variables
		bHooked = true;
        HookGrappleTime = GetWorld()->GetTimeSeconds();
        HookLocation = GetActorLocation();
        HookGrappleLocation = hit.Location;
        HookMoveTotalTime = FVector::Dist(HookLocation, HookGrappleLocation) / HookMoveSpeed;
        GrappleCable->SetWorldLocation(GetActorLocation());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Not Hit Something"));
		StopGrapple();
	}
}
