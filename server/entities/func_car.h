#pragma once

#include "cbase.h"

class CBasePlayer;

class CFuncCar : public CBaseAnimating
{
	DECLARE_CLASS(CFuncCar, CBaseAnimating);

public:
	enum { WHEEL_FL, WHEEL_FR, WHEEL_RL, WHEEL_RR, WHEEL_COUNT };
	enum { DRIVE_FWD, DRIVE_RWD, DRIVE_AWD };
	enum { EXIT_IGNORE_SLOTS = 4 };

	void Spawn() override;
	void Precache() override;
	void Activate() override;
	void KeyValue(KeyValueData *pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	void Touch(CBaseEntity *pOther) override;
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) override;
	void OnRemove() override;
	void ReloadConfig();
	void ResetForBombRound();
	bool ShouldIgnoreExitCollision(CBaseEntity *other);
	int ObjectCaps() override { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_CONTINUOUS_USE | FCAP_ONLYDIRECT_USE; }
	CBaseEntity *GetVehicleDriver() override { return m_hDriver; }
	CBaseEntity *GetVehicleViewEntity() override;
	bool HandleVehicleImpulse(int impulse) override;
	void ToggleParkingBrake();
	void WakeFromVehicleImpact();
	CBaseEntity *GetBodyVisualEntity() { return m_hBodyVisual; }
	const Vector &GetBodyCenterOfMass() const { return m_vecBodyCenterOfMass; }
	float GetBodyLinearDamping() const { return m_flBodyLinearDamping; }
	float GetBodyAngularDamping() const { return m_flBodyAngularDamping; }
	float GetCarSpeed() const { return m_flSpeed; }
	int GetVehicleHudFlags() const;
	float GetCarSteering() const { return m_flSteering; }
	float GetCarMaxSpeed() const { return m_flMaxSpeed; }
	float GetCarReverseSpeed() const { return m_flReverseSpeed; }
	float GetCarAcceleration() const { return m_flAcceleration; }
	float GetCarBrakeForce() const { return m_flBrakeForce; }
	float GetCarDrag() const { return m_flDrag; }
	float GetCarSteerAngle() const { return m_flSteerAngle; }
	float GetCarSteerSpeed() const { return m_flSteerSpeed; }
	float GetCarWheelbase() const { return Q_max( 16.0f, fabs(
		(m_vecWheelPos[WHEEL_FL].x + m_vecWheelPos[WHEEL_FR].x -
		 m_vecWheelPos[WHEEL_RL].x - m_vecWheelPos[WHEEL_RR].x) * 0.5f )); }

	DECLARE_DATADESC();

private:

	void CarThink();
	void EnsureChildren();
	void RemoveChildren();
	void UpdateInput(float dt);
	void UpdateWheels(float dt);
	void UpdateMotion(float dt);
	void UpdateVisuals(float dt);
	bool SweepBody(const Vector &oldOrigin, const Vector &oldAngles,
		const Vector &newOrigin, const Vector &newAngles, float &fraction,
		Vector &planeNormal, bool &startSolid) const;
	bool BodyPositionClear(const Vector &origin, const Vector &angles) const;
	void DebugDraw();
	void EnterDriver(CBasePlayer *pPlayer);
	void ExitDriver(CBasePlayer *pPlayer, bool force = false);
	bool FindExitPosition(CBasePlayer *pPlayer, Vector &position) const;
	Vector LocalToWorld(const Vector &local) const;
	bool LoadConfig();
	bool ApplyConfigValue(const char *key, const char *value, bool editorOverride);
	void ApplyDefaults();
	void CreatePhysicsBody();
	void IgnoreExitCollision(CBasePlayer *player);
	void UpdateUseAction();
	void CancelUseAction();
	void SendActionBar(CBasePlayer *player, int action, float duration) const;
	void UpdateEngine(float dt);
	void StartEngine();
	void StopEngine(bool playSound);
	void StopEngineLoops();
	void UpdateHorn();
	void StopHorn();
	void CreateHeadlights();
	void RemoveHeadlights();
	void SetHeadlights(bool enabled);
	void SendVehicleHud(bool visible);
	float EvaluateDriveForce(float speedFraction) const;
	float EvaluateLongitudinalGrip(float slipRatio) const;
	bool IsDrivenWheel(int wheel) const;
	void ResetWheelDynamics();
	void UpdateDriverVisual(float dt);
	void UpdateImpactAndLanding(const Vector &velocityBefore, int groundedBefore);
	void PlayImpact(float impactSpeed);
	bool CanDrive() const;
	void WakeCarPhysics();
	void UpdateSleepState();
	bool DriverRequestsMovement();

	string_t m_iszWheelModel;
	string_t m_iszDriverModel;
	Vector m_vecWheelPos[WHEEL_COUNT];
	Vector m_vecDriverPos;
	Vector m_vecViewPos;
	Vector m_vecExitPos;
	float m_flWheelRadius;
	float m_flWheelWidth;
	float m_flMaxSpeed;
	float m_flReverseSpeed;
	float m_flAcceleration;
	float m_flBrakeForce;
	float m_flDrag;
	float m_flDirectionChangeDelay;
	float m_flThrottleRiseTime;
	float m_flAccelerationEndScale;
	float m_flDriveForceFalloff[6];
	float m_flStationaryHoldMaxSlope;
	float m_flLongitudinalGrip;
	float m_flSlipPeak;
	float m_flSlipFalloff;
	float m_flRollingResistance;
	float m_flWheelInertia;
	int m_iDriveType;
	float m_flSteerAngle;
	float m_flSteerSpeed;
	float m_flSuspensionLength;
	float m_flSpringStrength;
	float m_flSuspensionDamping;
	float m_flLateralGrip;
	float m_flHighSpeedSteerScale;
	float m_flMaxLateralAcceleration;
	float m_flHandbrakeStrength;
	float m_flHandbrakeRearGrip;
	Vector m_vecBodyCenterOfMass;
	float m_flBodyLinearDamping;
	float m_flBodyAngularDamping;
	string_t m_iszDoorSound;
	string_t m_iszEngineStartSound;
	string_t m_iszEngineIdleSound;
	string_t m_iszEngineRunSound;
	string_t m_iszEngineStopSound;
	string_t m_iszHornSound;
	Vector m_vecHeadlightPos[2];
	float m_flHeadlightDistance;
	float m_flHeadlightAngle;
	float m_flHeadlightBrightness;
	Vector m_vecHeadlightColor;
	string_t m_iszImpactSounds[4];
	string_t m_iszLandingSound;
	float m_flImpactSoundMinKph;
	float m_flImpactCooldown;
	float m_flDamageThresholdKph;
	float m_flDamageAtThreshold;
	float m_flDamageReferenceKph;
	float m_flDamageAtReference;
	float m_flDoorActionDuration;
	float m_flDoorTransitionLead;
	float m_flIgnitionHoldDuration;
	float m_flEngineStartDuration;
	float m_flEngineIdlePitch;
	float m_flEngineMaxPitch;
	float m_flEnginePitchUpSpeed;
	float m_flEnginePitchDownSpeed;
	float m_flEngineVolume;
	float m_flEngineSoundInterval;
	unsigned int m_iSoundEditorOverrides;
	unsigned int m_iEditorOverrides;
	unsigned int m_iExtraEditorOverrides;
	BOOL m_bLoadingConfig;
	Vector m_vecSpawnOrigin;
	Vector m_vecSpawnAngles;

	EHANDLE m_hDriver;
	EHANDLE m_hBodyVisual;
	EHANDLE m_hWheels[WHEEL_COUNT];
	EHANDLE m_hViewEntity;
	EHANDLE m_hDriverVisual;
	EHANDLE m_hHeadlights[2];
	Vector m_vecWheelWorld[WHEEL_COUNT];
	Vector m_vecWheelContact[WHEEL_COUNT];
	Vector m_vecWheelNormal[WHEEL_COUNT];
	BOOL m_bWheelGrounded[WHEEL_COUNT];
	float m_flCompression[WHEEL_COUNT];
	float m_flPreviousCompression[WHEEL_COUNT];
	float m_flSpeed;
	float m_flThrottle;
	int m_iDriveDirection;
	int m_iPendingDriveDirection;
	float m_flDirectionChangeUntil;
	float m_flSteering;
	float m_flWheelAngularVelocity[WHEEL_COUNT];
	float m_flWheelRotation[WHEEL_COUNT];
	float m_flWheelLongitudinalSlip[WHEEL_COUNT];
	float m_flWheelLateralSlip[WHEEL_COUNT];
	float m_flWheelLoad[WHEEL_COUNT];
	float m_flWheelLongitudinalForce[WHEEL_COUNT];
	float m_flWheelLateralForce[WHEEL_COUNT];
	float m_flWheelGripUtilization[WHEEL_COUNT];
	float m_flWheelGroundSpeed[WHEEL_COUNT];
	BOOL m_bWheelStaticLateralGrip[WHEEL_COUNT];
	float m_flWheelStaticGripBlend[WHEEL_COUNT];
	float m_flWheelRequiredStaticForce[WHEEL_COUNT];
	float m_flWheelMaxGripForce[WHEEL_COUNT];
	float m_flVerticalVelocity;
	float m_flLastThink;
	float m_flNextDebugText;
	EHANDLE m_hExitIgnorePlayers[EXIT_IGNORE_SLOTS];
	float m_flExitIgnoreUntil[EXIT_IGNORE_SLOTS];
	float m_flDriverViewYaw;
	float m_flDriverViewPitch;
	Vector m_vecLastDriverInputAngles;
	int m_iGroundedWheels;
	Vector m_vecLastSafeOrigin;
	Vector m_vecLastSafeAngles;
	BOOL m_bHasLastSafeTransform;
	EHANDLE m_hUsePlayer;
	EHANDLE m_hUseBlockedPlayer;
	int m_iUseAction;
	float m_flUseActionStart;
	int m_iEngineState;
	float m_flEngineStateUntil;
	float m_flIgnitionHoldStart;
	BOOL m_bIgnitionLatched;
	float m_flEnginePitch;
	float m_flNextEngineSound;
	BOOL m_bHornPlaying;
	float m_flNextHornRestart;
	BOOL m_bHeadlightsOn;
	BOOL m_bParkingBrakeOn;
	BOOL m_bStaticRestConstraint;
	float m_flNextVehicleHud;
	float m_flNextImpactSound;
	float m_flDriverDamageAnimUntil;
	BOOL m_bWasAirborne;
	Vector m_vecPreviousVelocity;
	BOOL m_bCarPhysicsSleeping;
	float m_flSleepCandidateSince;
};
