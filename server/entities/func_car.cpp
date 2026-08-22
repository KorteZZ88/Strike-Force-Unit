#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "studio.h"
#include "func_car_shared.h"
#include "func_car.h"
#include "material.h"
#include "user_messages.h"
#include "gamerules/gamerules.h"

extern short g_sModelIndexLaser;

namespace
{
// PhysX advances at 100 Hz. Tyre and suspension impulses must be refreshed for
// every physics step; at 50 Hz every second PhysX step had gravity but no tyre
// support/static friction, causing persistent slope creep and suspension jitter.
constexpr float CAR_THINK_INTERVAL = 0.01f;
constexpr float CAR_GRAVITY = 600.0f;
constexpr float CAR_STOP_EPSILON = 2.0f;
constexpr float CAR_BODY_MASS = 1800.0f;
constexpr float CAR_LATERAL_GRIP = 4.0f;
constexpr float CAR_SLIP_REFERENCE_SPEED = 50.0f;
constexpr float CAR_MAX_WHEEL_SURFACE_SCALE = 2.0f;
constexpr float CAR_MAX_TYRE_LOAD_SCALE = 2.0f;
constexpr float CAR_STATIC_LATERAL_ENTER_SPEED = 8.0f;
constexpr float CAR_STATIC_LATERAL_EXIT_SPEED = 14.0f;
// A wheel can acquire more than the static threshold while the chassis settles
// onto a slope.  Keep a finite recapture band so kinetic friction can slow that
// small drift back into static grip instead of leaving it in the weak viscous
// branch forever.  Above this speed the normal dynamic slip model takes over.
constexpr float CAR_STATIC_LATERAL_CAPTURE_SPEED = 40.0f;
constexpr float CAR_STATIC_LATERAL_BLEND_RATE = 12.0f;
constexpr float CAR_STATIC_REST_ENTER_SPEED = 8.0f;
constexpr float CAR_STATIC_REST_EXIT_SPEED = 18.0f;
constexpr float CAR_STATIC_REST_ENTER_GRIP_FRACTION = 0.90f;
constexpr float CAR_MAX_SUSPENSION_COMPRESSION_SPEED = 120.0f;
// A held/latching rear brake must be able to recapture a slow hill roll, not only
// a chassis already inside the much narrower ordinary static-grip window.
constexpr float CAR_REAR_BRAKE_CAPTURE_SPEED = 64.0f;
constexpr float CAR_SLEEP_DELAY = 30.0f;
constexpr float CAR_SLEEP_THINK_INTERVAL = 0.1f;
constexpr float CAR_SLEEP_LINEAR_SPEED = 8.0f;
constexpr float CAR_SLEEP_ANGULAR_SPEED = 2.0f;
constexpr float CAR_VEHICLE_IMPACT_WAKE_SPEED = 40.0f;

enum CarUseAction { CAR_USE_NONE, CAR_USE_ENTER, CAR_USE_EXIT };
enum CarEngineState { CAR_ENGINE_OFF, CAR_ENGINE_STARTING, CAR_ENGINE_RUNNING };

enum CarSoundOverrideBits
{
	CAR_SND_OV_DOOR = 1u << 0, CAR_SND_OV_START = 1u << 1,
	CAR_SND_OV_IDLE = 1u << 2, CAR_SND_OV_RUN = 1u << 3,
	CAR_SND_OV_STOP = 1u << 4, CAR_SND_OV_DOOR_TIME = 1u << 5,
	CAR_SND_OV_DOOR_LEAD = 1u << 6, CAR_SND_OV_IGNITION_TIME = 1u << 7,
	CAR_SND_OV_START_TIME = 1u << 8, CAR_SND_OV_IDLE_PITCH = 1u << 9,
	CAR_SND_OV_MAX_PITCH = 1u << 10, CAR_SND_OV_PITCH_UP = 1u << 11,
	CAR_SND_OV_PITCH_DOWN = 1u << 12, CAR_SND_OV_VOLUME = 1u << 13,
	CAR_SND_OV_INTERVAL = 1u << 14, CAR_SND_OV_HORN = 1u << 15
};

enum CarOverrideBits
{
	CAR_OV_MODEL = 1u << 0, CAR_OV_WHEELMODEL = 1u << 1,
	CAR_OV_WHEEL_FL = 1u << 2, CAR_OV_WHEEL_FR = 1u << 3,
	CAR_OV_WHEEL_RL = 1u << 4, CAR_OV_WHEEL_RR = 1u << 5,
	CAR_OV_RADIUS = 1u << 6, CAR_OV_WIDTH = 1u << 7,
	CAR_OV_DRIVER = 1u << 8, CAR_OV_VIEW = 1u << 9, CAR_OV_EXIT = 1u << 10,
	CAR_OV_MAXSPEED = 1u << 11, CAR_OV_REVERSE = 1u << 12,
	CAR_OV_ACCEL = 1u << 13, CAR_OV_BRAKE = 1u << 14, CAR_OV_DRAG = 1u << 15,
	CAR_OV_STEERANGLE = 1u << 16, CAR_OV_STEERSPEED = 1u << 17,
	CAR_OV_SUSPLENGTH = 1u << 18, CAR_OV_SPRING = 1u << 19, CAR_OV_SUSPDAMP = 1u << 20,
	CAR_OV_MASS = 1u << 21, CAR_OV_COM = 1u << 22, CAR_OV_LATGRIP = 1u << 23,
	CAR_OV_HIGHSPEEDSTEER = 1u << 24, CAR_OV_MAXLAT = 1u << 25,
	CAR_OV_LINDAMP = 1u << 26, CAR_OV_ANGDAMP = 1u << 27,
	CAR_OV_HANDBRAKE = 1u << 28,
	CAR_OV_DIRECTION_DELAY = 1u << 30, CAR_OV_THROTTLE_RISE = 1u << 31
};

enum CarExtraOverrideBits
{
	CAR_XOV_LIGHT_L = 1u << 1,
	CAR_XOV_LIGHT_R = 1u << 2, CAR_XOV_LIGHT_DIST = 1u << 3,
	CAR_XOV_LIGHT_ANGLE = 1u << 4, CAR_XOV_LIGHT_BRIGHT = 1u << 5,
	CAR_XOV_LIGHT_COLOR = 1u << 6, CAR_XOV_DRIVE_FALLOFF = 1u << 7,
	CAR_XOV_DRIVER_MODEL = 1u << 8, CAR_XOV_IMPACT_MIN = 1u << 9,
	CAR_XOV_IMPACT_COOLDOWN = 1u << 10, CAR_XOV_DAMAGE_THRESHOLD = 1u << 11,
	CAR_XOV_DAMAGE_LOW = 1u << 12, CAR_XOV_DAMAGE_REFERENCE = 1u << 13,
	CAR_XOV_DAMAGE_HIGH = 1u << 14, CAR_XOV_LANDING_SOUND = 1u << 15,
	CAR_XOV_IMPACT_SOUNDS = 1u << 16, CAR_XOV_STATIONARY_SLOPE = 1u << 17,
	CAR_XOV_LONGITUDINAL_GRIP = 1u << 18, CAR_XOV_SLIP_PEAK = 1u << 19,
	CAR_XOV_SLIP_FALLOFF = 1u << 20, CAR_XOV_ROLLING_RESISTANCE = 1u << 21,
	CAR_XOV_WHEEL_INERTIA = 1u << 22, CAR_XOV_DRIVE_TYPE = 1u << 23
};

enum CarDrivetrainOverrideBits
{
	CAR_DT_OV_IDLE_RPM = 1u << 0,
	CAR_DT_OV_TORQUE_CURVE = 1u << 1,
	CAR_DT_OV_GEAR_RATIOS = 1u << 2,
	CAR_DT_OV_REVERSE_RATIO = 1u << 3,
	CAR_DT_OV_FINAL_DRIVE = 1u << 4,
	CAR_DT_OV_EFFICIENCY = 1u << 5,
	CAR_DT_OV_SHIFT_UP = 1u << 6,
	CAR_DT_OV_SHIFT_DOWN = 1u << 7,
	CAR_DT_OV_SHIFT_DURATION = 1u << 8,
	CAR_DT_OV_CONVERTER_STALL = 1u << 9,
	CAR_DT_OV_CONVERTER_RATIO = 1u << 10,
	CAR_DT_OV_CONVERTER_COUPLING = 1u << 11,
	CAR_DT_OV_CONVERTER_RESPONSE = 1u << 12
};

// Engine torque is authored in Nm while the car solver uses GoldSrc inches.
// One metre is 39.3701 game units, therefore torque scales by metres squared.
constexpr float CAR_NM_TO_GAME_TORQUE = 39.3701f * 39.3701f;
constexpr float CAR_RADIANS_TO_RPM = 60.0f / (2.0f * M_PI);

class CFuncCarChild : public CBaseAnimating
{
	DECLARE_CLASS(CFuncCarChild, CBaseAnimating);
public:
	void Spawn() override
	{
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
		pev->takedamage = DAMAGE_NO;
	}
	int ObjectCaps() override
	{
		if (pev->iuser4 == FUNC_CAR_BODY_MARKER)
			return FCAP_DONT_SAVE | FCAP_CONTINUOUS_USE | FCAP_ONLYDIRECT_USE;
		return FCAP_DONT_SAVE;
	}
	void Use(CBaseEntity *activator, CBaseEntity *caller, USE_TYPE useType, float value) override
	{
		if (pev->iuser4 == FUNC_CAR_BODY_MARKER && m_hParent != NULL)
			m_hParent->Use(activator, caller, useType, value);
	}
};

LINK_ENTITY_TO_CLASS(func_car_child, CFuncCarChild);

float CarApproach(float target, float value, float speed)
{
	if (value < target) return Q_min(value + speed, target);
	if (value > target) return Q_max(value - speed, target);
	return target;
}

float ClampFloat(float value, float low, float high)
{
	return Q_max(low, Q_min(value, high));
}
}

LINK_ENTITY_TO_CLASS(car_hummer, CFuncCar);

BEGIN_DATADESC(CFuncCar)
	DEFINE_KEYFIELD(m_iszWheelModel, FIELD_STRING, "wheelmodel"),
	DEFINE_KEYFIELD(m_iszDriverModel, FIELD_STRING, "drivermodel"),
	DEFINE_ARRAY(m_vecWheelPos, FIELD_VECTOR, CFuncCar::WHEEL_COUNT),
	DEFINE_KEYFIELD(m_vecDriverPos, FIELD_VECTOR, "driver_pos"),
	DEFINE_KEYFIELD(m_vecViewPos, FIELD_VECTOR, "view_pos"),
	DEFINE_KEYFIELD(m_vecExitPos, FIELD_VECTOR, "exit_pos"),
	DEFINE_KEYFIELD(m_flWheelRadius, FIELD_FLOAT, "wheel_radius"),
	DEFINE_KEYFIELD(m_flWheelWidth, FIELD_FLOAT, "wheel_width"),
	DEFINE_KEYFIELD(m_flMaxSpeed, FIELD_FLOAT, "maxspeed"),
	DEFINE_KEYFIELD(m_flReverseSpeed, FIELD_FLOAT, "reversespeed"),
	DEFINE_KEYFIELD(m_flAcceleration, FIELD_FLOAT, "acceleration"),
	DEFINE_KEYFIELD(m_flBrakeForce, FIELD_FLOAT, "brakeforce"),
	DEFINE_KEYFIELD(m_flDrag, FIELD_FLOAT, "drag"),
	DEFINE_KEYFIELD(m_flDirectionChangeDelay, FIELD_FLOAT, "direction_change_delay"),
	DEFINE_KEYFIELD(m_flThrottleRiseTime, FIELD_FLOAT, "throttle_rise_time"),
	DEFINE_ARRAY(m_flDriveForceFalloff, FIELD_FLOAT, 6),
	DEFINE_KEYFIELD(m_flEngineIdleRPM, FIELD_FLOAT, "engine_idle_rpm"),
	DEFINE_ARRAY(m_flEngineTorqueCurveRPM, FIELD_FLOAT, CFuncCar::ENGINE_TORQUE_POINTS),
	DEFINE_ARRAY(m_flEngineTorqueCurve, FIELD_FLOAT, CFuncCar::ENGINE_TORQUE_POINTS),
	DEFINE_ARRAY(m_flGearRatios, FIELD_FLOAT, CFuncCar::MAX_FORWARD_GEARS),
	DEFINE_FIELD(m_iForwardGearCount, FIELD_INTEGER),
	DEFINE_KEYFIELD(m_flReverseRatio, FIELD_FLOAT, "reverse_ratio"),
	DEFINE_KEYFIELD(m_flFinalDrive, FIELD_FLOAT, "final_drive"),
	DEFINE_KEYFIELD(m_flTransmissionEfficiency, FIELD_FLOAT, "transmission_efficiency"),
	DEFINE_KEYFIELD(m_flShiftUpRPM, FIELD_FLOAT, "shift_up_rpm"),
	DEFINE_KEYFIELD(m_flShiftDownRPM, FIELD_FLOAT, "shift_down_rpm"),
	DEFINE_KEYFIELD(m_flShiftDuration, FIELD_FLOAT, "shift_duration"),
	DEFINE_KEYFIELD(m_flConverterStallRPM, FIELD_FLOAT, "torque_converter_stall_rpm"),
	DEFINE_KEYFIELD(m_flConverterMaxRatio, FIELD_FLOAT, "torque_converter_max_ratio"),
	DEFINE_KEYFIELD(m_flConverterCouplingRPM, FIELD_FLOAT, "torque_converter_coupling_rpm"),
	DEFINE_KEYFIELD(m_flConverterResponseRPM, FIELD_FLOAT, "torque_converter_response_rpm"),
	DEFINE_KEYFIELD(m_flStationaryHoldMaxSlope, FIELD_FLOAT, "stationary_hold_max_slope"),
	DEFINE_KEYFIELD(m_flLongitudinalGrip, FIELD_FLOAT, "longitudinal_grip"),
	DEFINE_KEYFIELD(m_flSlipPeak, FIELD_FLOAT, "slip_peak"),
	DEFINE_KEYFIELD(m_flSlipFalloff, FIELD_FLOAT, "slip_falloff"),
	DEFINE_KEYFIELD(m_flRollingResistance, FIELD_FLOAT, "rolling_resistance"),
	DEFINE_KEYFIELD(m_flWheelInertia, FIELD_FLOAT, "wheel_inertia"),
	DEFINE_KEYFIELD(m_iDriveType, FIELD_INTEGER, "drive_type"),
	DEFINE_KEYFIELD(m_flSteerAngle, FIELD_FLOAT, "steerangle"),
	DEFINE_KEYFIELD(m_flSteerSpeed, FIELD_FLOAT, "steerspeed"),
	DEFINE_KEYFIELD(m_flSuspensionLength, FIELD_FLOAT, "suspension_length"),
	DEFINE_KEYFIELD(m_flSpringStrength, FIELD_FLOAT, "spring_strength"),
	DEFINE_KEYFIELD(m_flSuspensionDamping, FIELD_FLOAT, "suspension_damping"),
	DEFINE_KEYFIELD(m_flLateralGrip, FIELD_FLOAT, "lateral_grip"),
	DEFINE_KEYFIELD(m_flHighSpeedSteerScale, FIELD_FLOAT, "highspeed_steer_scale"),
	DEFINE_KEYFIELD(m_flMaxLateralAcceleration, FIELD_FLOAT, "max_lateral_accel"),
	DEFINE_KEYFIELD(m_flHandbrakeStrength, FIELD_FLOAT, "handbrake_strength"),
	DEFINE_KEYFIELD(m_vecBodyCenterOfMass, FIELD_VECTOR, "center_of_mass"),
	DEFINE_KEYFIELD(m_flBodyLinearDamping, FIELD_FLOAT, "linear_damping"),
	DEFINE_KEYFIELD(m_flBodyAngularDamping, FIELD_FLOAT, "angular_damping"),
	DEFINE_KEYFIELD(m_iszDoorSound, FIELD_STRING, "door_sound"),
	DEFINE_KEYFIELD(m_iszEngineStartSound, FIELD_STRING, "engine_start_sound"),
	DEFINE_KEYFIELD(m_iszEngineIdleSound, FIELD_STRING, "engine_idle_sound"),
	DEFINE_KEYFIELD(m_iszEngineRunSound, FIELD_STRING, "engine_run_sound"),
	DEFINE_KEYFIELD(m_iszEngineStopSound, FIELD_STRING, "engine_stop_sound"),
	DEFINE_KEYFIELD(m_iszHornSound, FIELD_STRING, "horn_sound"),
	DEFINE_ARRAY(m_vecHeadlightPos, FIELD_VECTOR, 2),
	DEFINE_KEYFIELD(m_flHeadlightDistance, FIELD_FLOAT, "headlight_distance"),
	DEFINE_KEYFIELD(m_flHeadlightAngle, FIELD_FLOAT, "headlight_angle"),
	DEFINE_KEYFIELD(m_flHeadlightBrightness, FIELD_FLOAT, "headlight_brightness"),
	DEFINE_KEYFIELD(m_vecHeadlightColor, FIELD_VECTOR, "headlight_color"),
	DEFINE_ARRAY(m_iszImpactSounds, FIELD_STRING, 4),
	DEFINE_KEYFIELD(m_iszLandingSound, FIELD_STRING, "landing_sound"),
	DEFINE_KEYFIELD(m_flImpactSoundMinKph, FIELD_FLOAT, "impact_sound_min_kph"),
	DEFINE_KEYFIELD(m_flImpactCooldown, FIELD_FLOAT, "impact_cooldown"),
	DEFINE_KEYFIELD(m_flDamageThresholdKph, FIELD_FLOAT, "collision_damage_threshold_kph"),
	DEFINE_KEYFIELD(m_flDamageAtThreshold, FIELD_FLOAT, "collision_damage_at_threshold"),
	DEFINE_KEYFIELD(m_flDamageReferenceKph, FIELD_FLOAT, "collision_damage_reference_kph"),
	DEFINE_KEYFIELD(m_flDamageAtReference, FIELD_FLOAT, "collision_damage_at_reference"),
	DEFINE_KEYFIELD(m_flDoorActionDuration, FIELD_FLOAT, "door_action_duration"),
	DEFINE_KEYFIELD(m_flDoorTransitionLead, FIELD_FLOAT, "door_transition_lead"),
	DEFINE_KEYFIELD(m_flIgnitionHoldDuration, FIELD_FLOAT, "ignition_hold_duration"),
	DEFINE_KEYFIELD(m_flEngineStartDuration, FIELD_FLOAT, "engine_start_duration"),
	DEFINE_KEYFIELD(m_flEngineIdlePitch, FIELD_FLOAT, "engine_idle_pitch"),
	DEFINE_KEYFIELD(m_flEngineMaxPitch, FIELD_FLOAT, "engine_max_pitch"),
	DEFINE_KEYFIELD(m_flEnginePitchUpSpeed, FIELD_FLOAT, "engine_pitch_up_speed"),
	DEFINE_KEYFIELD(m_flEnginePitchDownSpeed, FIELD_FLOAT, "engine_pitch_down_speed"),
	DEFINE_KEYFIELD(m_flEngineVolume, FIELD_FLOAT, "engine_volume"),
	DEFINE_KEYFIELD(m_flEngineSoundInterval, FIELD_FLOAT, "engine_sound_interval"),
	DEFINE_FIELD(m_iSoundEditorOverrides, FIELD_INTEGER),
	DEFINE_FIELD(m_iExtraEditorOverrides, FIELD_INTEGER),
	DEFINE_FIELD(m_iEditorOverrides, FIELD_INTEGER),
	DEFINE_FIELD(m_vecSpawnOrigin, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_vecSpawnAngles, FIELD_VECTOR),
	DEFINE_FIELD(m_hDriver, FIELD_EHANDLE),
	DEFINE_FIELD(m_hBodyVisual, FIELD_EHANDLE),
	DEFINE_ARRAY(m_vecWheelWorld, FIELD_POSITION_VECTOR, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_vecWheelContact, FIELD_POSITION_VECTOR, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_vecWheelNormal, FIELD_VECTOR, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flCompression, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flPreviousCompression, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_FIELD(m_flSpeed, FIELD_FLOAT),
	DEFINE_FIELD(m_flThrottle, FIELD_FLOAT),
	DEFINE_FIELD(m_flEngineRPM, FIELD_FLOAT),
	DEFINE_FIELD(m_flEngineTorque, FIELD_FLOAT),
	DEFINE_FIELD(m_flDrivelineRPM, FIELD_FLOAT),
	DEFINE_FIELD(m_flPerWheelDriveTorque, FIELD_FLOAT),
	DEFINE_FIELD(m_flConverterSlipRPM, FIELD_FLOAT),
	DEFINE_FIELD(m_flConverterRatio, FIELD_FLOAT),
	DEFINE_FIELD(m_flTransmittedTorque, FIELD_FLOAT),
	DEFINE_FIELD(m_iCurrentGear, FIELD_INTEGER),
	DEFINE_FIELD(m_iTargetGear, FIELD_INTEGER),
	DEFINE_FIELD(m_flShiftStartTime, FIELD_TIME),
	DEFINE_FIELD(m_flShiftEndTime, FIELD_TIME),
	DEFINE_FIELD(m_iDriveDirection, FIELD_INTEGER),
	DEFINE_FIELD(m_iPendingDriveDirection, FIELD_INTEGER),
	DEFINE_FIELD(m_flDirectionChangeUntil, FIELD_TIME),
	DEFINE_FIELD(m_flSteering, FIELD_FLOAT),
	DEFINE_ARRAY(m_flWheelAngularVelocity, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelRotation, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelLongitudinalSlip, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelLateralSlip, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelLoad, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelLongitudinalForce, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelLateralForce, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelGripUtilization, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelGroundSpeed, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_bWheelStaticLateralGrip, FIELD_BOOLEAN, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelStaticGripBlend, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelRequiredStaticForce, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flWheelMaxGripForce, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_FIELD(m_flVerticalVelocity, FIELD_FLOAT),
	DEFINE_FIELD(m_flLastThink, FIELD_TIME),
	DEFINE_FIELD(m_flNextDebugText, FIELD_TIME),
	DEFINE_ARRAY(m_hExitIgnorePlayers, FIELD_EHANDLE, CFuncCar::EXIT_IGNORE_SLOTS),
	DEFINE_ARRAY(m_flExitIgnoreUntil, FIELD_TIME, CFuncCar::EXIT_IGNORE_SLOTS),
	DEFINE_FIELD(m_flDriverViewYaw, FIELD_FLOAT),
	DEFINE_FIELD(m_flDriverViewPitch, FIELD_FLOAT),
	DEFINE_FIELD(m_vecLastDriverInputAngles, FIELD_VECTOR),
	DEFINE_FIELD(m_iGroundedWheels, FIELD_INTEGER),
	DEFINE_FIELD(m_vecLastSafeOrigin, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_vecLastSafeAngles, FIELD_VECTOR),
	DEFINE_FIELD(m_bHasLastSafeTransform, FIELD_BOOLEAN),
	DEFINE_FIELD(m_hUsePlayer, FIELD_EHANDLE),
	DEFINE_FIELD(m_hUseBlockedPlayer, FIELD_EHANDLE),
	DEFINE_FIELD(m_iUseAction, FIELD_INTEGER),
	DEFINE_FIELD(m_flUseActionStart, FIELD_TIME),
	DEFINE_FIELD(m_iEngineState, FIELD_INTEGER),
	DEFINE_FIELD(m_flEngineStateUntil, FIELD_TIME),
	DEFINE_FIELD(m_flIgnitionHoldStart, FIELD_TIME),
	DEFINE_FIELD(m_bIgnitionLatched, FIELD_BOOLEAN),
	DEFINE_FIELD(m_flEnginePitch, FIELD_FLOAT),
	DEFINE_FIELD(m_flNextEngineSound, FIELD_TIME),
	DEFINE_FIELD(m_bHornPlaying, FIELD_BOOLEAN),
	DEFINE_FIELD(m_flNextHornRestart, FIELD_TIME),
	DEFINE_FIELD(m_bHeadlightsOn, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bParkingBrakeOn, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bStaticRestConstraint, FIELD_BOOLEAN),
	DEFINE_FIELD(m_flNextImpactSound, FIELD_TIME),
	DEFINE_FIELD(m_flDriverDamageAnimUntil, FIELD_TIME),
	DEFINE_FIELD(m_bWasAirborne, FIELD_BOOLEAN),
	DEFINE_FIELD(m_vecPreviousVelocity, FIELD_VECTOR),
	DEFINE_FIELD(m_bCarPhysicsSleeping, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bRaceLocked, FIELD_BOOLEAN),
	DEFINE_FIELD(m_flSleepCandidateSince, FIELD_TIME),
	DEFINE_FIELD(m_iDrivetrainEditorOverrides, FIELD_INTEGER),
	DEFINE_FUNCTION(CarThink),
END_DATADESC()

void CFuncCar::KeyValue(KeyValueData *pkvd)
{
	if (!ApplyConfigValue(pkvd->szKeyName, pkvd->szValue, true))
	{
		BaseClass::KeyValue(pkvd);
		return;
	}
	pkvd->fHandled = TRUE;
}

bool CFuncCar::ApplyConfigValue(const char *key, const char *value, bool editorOverride)
{
	auto allowed = [&](unsigned int bit) {
		if (editorOverride) m_iEditorOverrides |= bit;
		return editorOverride || !(m_iEditorOverrides & bit);
	};
	auto number = [&](float &field, unsigned int bit) { if (allowed(bit)) field = Q_atof(value); };
	auto vector = [&](Vector &field, unsigned int bit) { if (allowed(bit)) UTIL_StringToVector(field, value); };
	auto soundAllowed = [&](unsigned int bit) {
		if (editorOverride) m_iSoundEditorOverrides |= bit;
		return editorOverride || !(m_iSoundEditorOverrides & bit);
	};
	auto sound = [&](string_t &field, unsigned int bit) { if (soundAllowed(bit)) field = ALLOC_STRING(value); };
	auto soundNumber = [&](float &field, unsigned int bit) { if (soundAllowed(bit)) field = Q_atof(value); };
	auto extraAllowed = [&](unsigned int bit) {
		if (editorOverride) m_iExtraEditorOverrides |= bit;
		return editorOverride || !(m_iExtraEditorOverrides & bit);
	};
	auto extraNumber = [&](float &field, unsigned int bit) { if (extraAllowed(bit)) field = Q_atof(value); };
	auto extraVector = [&](Vector &field, unsigned int bit) { if (extraAllowed(bit)) UTIL_StringToVector(field, value); };
	auto extraString = [&](string_t &field, unsigned int bit) { if (extraAllowed(bit)) field = ALLOC_STRING(value); };
	auto drivetrainAllowed = [&](unsigned int bit) {
		if (editorOverride) m_iDrivetrainEditorOverrides |= bit;
		return editorOverride || !(m_iDrivetrainEditorOverrides & bit);
	};
	auto drivetrainNumber = [&](float &field, unsigned int bit) {
		if (drivetrainAllowed(bit)) field = Q_atof(value);
	};

	if (FStrEq(key, "model")) { if (allowed(CAR_OV_MODEL)) pev->model = ALLOC_STRING(value); }
	else if (FStrEq(key, "wheelmodel")) { if (allowed(CAR_OV_WHEELMODEL)) m_iszWheelModel = ALLOC_STRING(value); }
	else if (FStrEq(key, "drivermodel")) extraString(m_iszDriverModel, CAR_XOV_DRIVER_MODEL);
	else if (FStrEq(key, "wheel_fl_pos")) vector(m_vecWheelPos[WHEEL_FL], CAR_OV_WHEEL_FL);
	else if (FStrEq(key, "wheel_fr_pos")) vector(m_vecWheelPos[WHEEL_FR], CAR_OV_WHEEL_FR);
	else if (FStrEq(key, "wheel_rl_pos")) vector(m_vecWheelPos[WHEEL_RL], CAR_OV_WHEEL_RL);
	else if (FStrEq(key, "wheel_rr_pos")) vector(m_vecWheelPos[WHEEL_RR], CAR_OV_WHEEL_RR);
	else if (FStrEq(key, "wheel_radius")) number(m_flWheelRadius, CAR_OV_RADIUS);
	else if (FStrEq(key, "wheel_width")) number(m_flWheelWidth, CAR_OV_WIDTH);
	else if (FStrEq(key, "driver_pos")) vector(m_vecDriverPos, CAR_OV_DRIVER);
	else if (FStrEq(key, "view_pos")) vector(m_vecViewPos, CAR_OV_VIEW);
	else if (FStrEq(key, "exit_pos")) vector(m_vecExitPos, CAR_OV_EXIT);
	else if (FStrEq(key, "maxspeed")) number(m_flMaxSpeed, CAR_OV_MAXSPEED);
	else if (FStrEq(key, "reversespeed")) number(m_flReverseSpeed, CAR_OV_REVERSE);
	else if (FStrEq(key, "acceleration")) number(m_flAcceleration, CAR_OV_ACCEL);
	else if (FStrEq(key, "brakeforce")) number(m_flBrakeForce, CAR_OV_BRAKE);
	else if (FStrEq(key, "drag")) number(m_flDrag, CAR_OV_DRAG);
	else if (FStrEq(key, "direction_change_delay")) number(m_flDirectionChangeDelay, CAR_OV_DIRECTION_DELAY);
	else if (FStrEq(key, "throttle_rise_time")) number(m_flThrottleRiseTime, CAR_OV_THROTTLE_RISE);
	else if (FStrEq(key, "stationary_hold_max_slope")) extraNumber(m_flStationaryHoldMaxSlope, CAR_XOV_STATIONARY_SLOPE);
	else if (FStrEq(key, "longitudinal_grip")) extraNumber(m_flLongitudinalGrip, CAR_XOV_LONGITUDINAL_GRIP);
	else if (FStrEq(key, "slip_peak")) extraNumber(m_flSlipPeak, CAR_XOV_SLIP_PEAK);
	else if (FStrEq(key, "slip_falloff")) extraNumber(m_flSlipFalloff, CAR_XOV_SLIP_FALLOFF);
	else if (FStrEq(key, "rolling_resistance")) extraNumber(m_flRollingResistance, CAR_XOV_ROLLING_RESISTANCE);
	else if (FStrEq(key, "wheel_inertia")) extraNumber(m_flWheelInertia, CAR_XOV_WHEEL_INERTIA);
	else if (FStrEq(key, "drive_type"))
	{
		if (extraAllowed(CAR_XOV_DRIVE_TYPE))
		{
			if (!Q_stricmp(value, "fwd")) m_iDriveType = DRIVE_FWD;
			else if (!Q_stricmp(value, "rwd")) m_iDriveType = DRIVE_RWD;
			else if (!Q_stricmp(value, "awd") || !Q_stricmp(value, "4wd")) m_iDriveType = DRIVE_AWD;
			else ALERT(at_error, "func_car: parameter 'drive_type' must be FWD, RWD or AWD / параметр должен быть FWD, RWD или AWD\n");
		}
	}
	else if (FStrEq(key, "drive_force_falloff"))
	{
		if (extraAllowed(CAR_XOV_DRIVE_FALLOFF))
		{
			float curve[6];
			if (sscanf(value, "%f %f %f %f %f %f", &curve[0], &curve[1], &curve[2],
				&curve[3], &curve[4], &curve[5]) != 6)
			{
				ALERT(at_error, "func_car: parameter 'drive_force_falloff' requires 6 values for speeds 0 25 50 75 90 100%% / параметру нужны 6 значений\n");
			}
			else memcpy(m_flDriveForceFalloff, curve, sizeof(curve));
		}
	}
	else if (FStrEq(key, "engine_idle_rpm")) drivetrainNumber(m_flEngineIdleRPM, CAR_DT_OV_IDLE_RPM);
	else if (FStrEq(key, "engine_torque_curve"))
	{
		if (drivetrainAllowed(CAR_DT_OV_TORQUE_CURVE))
		{
			float curve[ENGINE_TORQUE_POINTS * 2];
			const int parsed = sscanf(value,
				"%f %f %f %f %f %f %f %f %f %f %f %f",
				&curve[0], &curve[1], &curve[2], &curve[3], &curve[4], &curve[5],
				&curve[6], &curve[7], &curve[8], &curve[9], &curve[10], &curve[11]);
			if (parsed != ENGINE_TORQUE_POINTS * 2)
			{
				ALERT(at_error,
					"func_car: parameter 'engine_torque_curve' requires 6 RPM/torque pairs / параметру нужны 6 пар RPM/момент\n");
			}
			else for (int i = 0; i < ENGINE_TORQUE_POINTS; ++i)
			{
				m_flEngineTorqueCurveRPM[i] = curve[i * 2];
				m_flEngineTorqueCurve[i] = curve[i * 2 + 1];
			}
		}
	}
	else if (FStrEq(key, "gear_ratios"))
	{
		if (drivetrainAllowed(CAR_DT_OV_GEAR_RATIOS))
		{
			float ratios[MAX_FORWARD_GEARS] = {};
			const int count = sscanf(value, "%f %f %f %f %f %f",
				&ratios[0], &ratios[1], &ratios[2], &ratios[3], &ratios[4], &ratios[5]);
			if (count < 1)
				ALERT(at_error, "func_car: parameter 'gear_ratios' requires 1 to 6 values / параметру нужны 1-6 значений\n");
			else
			{
				memcpy(m_flGearRatios, ratios, sizeof(ratios));
				m_iForwardGearCount = count;
			}
		}
	}
	else if (FStrEq(key, "reverse_ratio")) drivetrainNumber(m_flReverseRatio, CAR_DT_OV_REVERSE_RATIO);
	else if (FStrEq(key, "final_drive")) drivetrainNumber(m_flFinalDrive, CAR_DT_OV_FINAL_DRIVE);
	else if (FStrEq(key, "transmission_efficiency")) drivetrainNumber(m_flTransmissionEfficiency, CAR_DT_OV_EFFICIENCY);
	else if (FStrEq(key, "shift_up_rpm")) drivetrainNumber(m_flShiftUpRPM, CAR_DT_OV_SHIFT_UP);
	else if (FStrEq(key, "shift_down_rpm")) drivetrainNumber(m_flShiftDownRPM, CAR_DT_OV_SHIFT_DOWN);
	else if (FStrEq(key, "shift_duration")) drivetrainNumber(m_flShiftDuration, CAR_DT_OV_SHIFT_DURATION);
	else if (FStrEq(key, "torque_converter_stall_rpm")) drivetrainNumber(m_flConverterStallRPM, CAR_DT_OV_CONVERTER_STALL);
	else if (FStrEq(key, "torque_converter_max_ratio")) drivetrainNumber(m_flConverterMaxRatio, CAR_DT_OV_CONVERTER_RATIO);
	else if (FStrEq(key, "torque_converter_coupling_rpm")) drivetrainNumber(m_flConverterCouplingRPM, CAR_DT_OV_CONVERTER_COUPLING);
	else if (FStrEq(key, "torque_converter_response_rpm")) drivetrainNumber(m_flConverterResponseRPM, CAR_DT_OV_CONVERTER_RESPONSE);
	else if (FStrEq(key, "steerangle")) number(m_flSteerAngle, CAR_OV_STEERANGLE);
	else if (FStrEq(key, "steerspeed")) number(m_flSteerSpeed, CAR_OV_STEERSPEED);
	else if (FStrEq(key, "suspension_length")) number(m_flSuspensionLength, CAR_OV_SUSPLENGTH);
	else if (FStrEq(key, "spring_strength")) number(m_flSpringStrength, CAR_OV_SPRING);
	else if (FStrEq(key, "suspension_damping")) number(m_flSuspensionDamping, CAR_OV_SUSPDAMP);
	else if (FStrEq(key, "mass")) number(m_flBodyMass, CAR_OV_MASS);
	else if (FStrEq(key, "center_of_mass")) vector(m_vecBodyCenterOfMass, CAR_OV_COM);
	else if (FStrEq(key, "lateral_grip")) number(m_flLateralGrip, CAR_OV_LATGRIP);
	else if (FStrEq(key, "highspeed_steer_scale")) number(m_flHighSpeedSteerScale, CAR_OV_HIGHSPEEDSTEER);
	else if (FStrEq(key, "max_lateral_accel")) number(m_flMaxLateralAcceleration, CAR_OV_MAXLAT);
	else if (FStrEq(key, "handbrake_strength")) number(m_flHandbrakeStrength, CAR_OV_HANDBRAKE);
	// Backward compatibility for existing user configs. Rear lateral grip is no
	// longer reduced artificially; the friction ellipse now handles the loss.
	else if (FStrEq(key, "handbrake_rear_grip")) return true;
	else if (FStrEq(key, "linear_damping")) number(m_flBodyLinearDamping, CAR_OV_LINDAMP);
	else if (FStrEq(key, "angular_damping")) number(m_flBodyAngularDamping, CAR_OV_ANGDAMP);
	else if (FStrEq(key, "door_sound")) sound(m_iszDoorSound, CAR_SND_OV_DOOR);
	else if (FStrEq(key, "engine_start_sound")) sound(m_iszEngineStartSound, CAR_SND_OV_START);
	else if (FStrEq(key, "engine_idle_sound")) sound(m_iszEngineIdleSound, CAR_SND_OV_IDLE);
	else if (FStrEq(key, "engine_run_sound")) sound(m_iszEngineRunSound, CAR_SND_OV_RUN);
	else if (FStrEq(key, "engine_stop_sound")) sound(m_iszEngineStopSound, CAR_SND_OV_STOP);
	else if (FStrEq(key, "horn_sound")) sound(m_iszHornSound, CAR_SND_OV_HORN);
	else if (FStrEq(key, "headlight_left_pos")) extraVector(m_vecHeadlightPos[0], CAR_XOV_LIGHT_L);
	else if (FStrEq(key, "headlight_right_pos")) extraVector(m_vecHeadlightPos[1], CAR_XOV_LIGHT_R);
	else if (FStrEq(key, "headlight_distance")) extraNumber(m_flHeadlightDistance, CAR_XOV_LIGHT_DIST);
	else if (FStrEq(key, "headlight_angle")) extraNumber(m_flHeadlightAngle, CAR_XOV_LIGHT_ANGLE);
	else if (FStrEq(key, "headlight_brightness")) extraNumber(m_flHeadlightBrightness, CAR_XOV_LIGHT_BRIGHT);
	else if (FStrEq(key, "headlight_color")) extraVector(m_vecHeadlightColor, CAR_XOV_LIGHT_COLOR);
	else if (FStrEq(key, "impact_sound_1")) { if (extraAllowed(CAR_XOV_IMPACT_SOUNDS)) m_iszImpactSounds[0] = ALLOC_STRING(value); }
	else if (FStrEq(key, "impact_sound_2")) { if (extraAllowed(CAR_XOV_IMPACT_SOUNDS)) m_iszImpactSounds[1] = ALLOC_STRING(value); }
	else if (FStrEq(key, "impact_sound_3")) { if (extraAllowed(CAR_XOV_IMPACT_SOUNDS)) m_iszImpactSounds[2] = ALLOC_STRING(value); }
	else if (FStrEq(key, "impact_sound_4")) { if (extraAllowed(CAR_XOV_IMPACT_SOUNDS)) m_iszImpactSounds[3] = ALLOC_STRING(value); }
	else if (FStrEq(key, "landing_sound")) extraString(m_iszLandingSound, CAR_XOV_LANDING_SOUND);
	else if (FStrEq(key, "impact_sound_min_kph")) extraNumber(m_flImpactSoundMinKph, CAR_XOV_IMPACT_MIN);
	else if (FStrEq(key, "impact_cooldown")) extraNumber(m_flImpactCooldown, CAR_XOV_IMPACT_COOLDOWN);
	else if (FStrEq(key, "collision_damage_threshold_kph")) extraNumber(m_flDamageThresholdKph, CAR_XOV_DAMAGE_THRESHOLD);
	else if (FStrEq(key, "collision_damage_at_threshold")) extraNumber(m_flDamageAtThreshold, CAR_XOV_DAMAGE_LOW);
	else if (FStrEq(key, "collision_damage_reference_kph")) extraNumber(m_flDamageReferenceKph, CAR_XOV_DAMAGE_REFERENCE);
	else if (FStrEq(key, "collision_damage_at_reference")) extraNumber(m_flDamageAtReference, CAR_XOV_DAMAGE_HIGH);
	else if (FStrEq(key, "door_action_duration")) soundNumber(m_flDoorActionDuration, CAR_SND_OV_DOOR_TIME);
	else if (FStrEq(key, "door_transition_lead")) soundNumber(m_flDoorTransitionLead, CAR_SND_OV_DOOR_LEAD);
	else if (FStrEq(key, "ignition_hold_duration")) soundNumber(m_flIgnitionHoldDuration, CAR_SND_OV_IGNITION_TIME);
	else if (FStrEq(key, "engine_start_duration")) soundNumber(m_flEngineStartDuration, CAR_SND_OV_START_TIME);
	else if (FStrEq(key, "engine_idle_pitch")) soundNumber(m_flEngineIdlePitch, CAR_SND_OV_IDLE_PITCH);
	else if (FStrEq(key, "engine_max_pitch")) soundNumber(m_flEngineMaxPitch, CAR_SND_OV_MAX_PITCH);
	else if (FStrEq(key, "engine_pitch_up_speed")) soundNumber(m_flEnginePitchUpSpeed, CAR_SND_OV_PITCH_UP);
	else if (FStrEq(key, "engine_pitch_down_speed")) soundNumber(m_flEnginePitchDownSpeed, CAR_SND_OV_PITCH_DOWN);
	else if (FStrEq(key, "engine_volume")) soundNumber(m_flEngineVolume, CAR_SND_OV_VOLUME);
	else if (FStrEq(key, "engine_sound_interval")) soundNumber(m_flEngineSoundInterval, CAR_SND_OV_INTERVAL);
	else return false;
	return true;
}

void CFuncCar::ApplyDefaults()
{
	if (!(m_iEditorOverrides & CAR_OV_RADIUS)) m_flWheelRadius = 16.0f;
	if (!(m_iEditorOverrides & CAR_OV_WIDTH)) m_flWheelWidth = 8.0f;
	if (!(m_iEditorOverrides & CAR_OV_MAXSPEED)) m_flMaxSpeed = 600.0f;
	if (!(m_iEditorOverrides & CAR_OV_REVERSE)) m_flReverseSpeed = 250.0f;
	if (!(m_iEditorOverrides & CAR_OV_ACCEL)) m_flAcceleration = 300.0f;
	if (!(m_iEditorOverrides & CAR_OV_BRAKE)) m_flBrakeForce = 500.0f;
	if (!(m_iEditorOverrides & CAR_OV_DRAG)) m_flDrag = 80.0f;
	if (!(m_iEditorOverrides & CAR_OV_DIRECTION_DELAY)) m_flDirectionChangeDelay = 0.5f;
	if (!(m_iEditorOverrides & CAR_OV_THROTTLE_RISE)) m_flThrottleRiseTime = 0.5f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_STATIONARY_SLOPE)) m_flStationaryHoldMaxSlope = 5.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_LONGITUDINAL_GRIP)) m_flLongitudinalGrip = 1.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_SLIP_PEAK)) m_flSlipPeak = 0.12f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_SLIP_FALLOFF)) m_flSlipFalloff = 0.65f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_ROLLING_RESISTANCE)) m_flRollingResistance = 12.0f;
	// Zero selects an automatic effective inertia based on body mass and radius.
	if (!(m_iExtraEditorOverrides & CAR_XOV_WHEEL_INERTIA)) m_flWheelInertia = 0.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_DRIVE_TYPE)) m_iDriveType = DRIVE_AWD;
	if (!(m_iExtraEditorOverrides & CAR_XOV_DRIVE_FALLOFF))
	{
		const float curve[6] = { 1.0f, 1.0f, 0.9f, 0.65f, 0.35f, 0.05f };
		memcpy(m_flDriveForceFalloff, curve, sizeof(curve));
	}
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_IDLE_RPM)) m_flEngineIdleRPM = 800.0f;
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_TORQUE_CURVE))
	{
		const float rpm[ENGINE_TORQUE_POINTS] = { 800.0f, 1400.0f, 2200.0f, 3200.0f, 4200.0f, 5200.0f };
		const float torque[ENGINE_TORQUE_POINTS] = { 360.0f, 500.0f, 560.0f, 530.0f, 430.0f, 0.0f };
		memcpy(m_flEngineTorqueCurveRPM, rpm, sizeof(rpm));
		memcpy(m_flEngineTorqueCurve, torque, sizeof(torque));
	}
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_GEAR_RATIOS))
	{
		const float ratios[MAX_FORWARD_GEARS] = { 3.00f, 1.70f, 1.00f, 0.72f, 0.0f, 0.0f };
		memcpy(m_flGearRatios, ratios, sizeof(ratios));
		m_iForwardGearCount = 4;
	}
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_REVERSE_RATIO)) m_flReverseRatio = -2.90f;
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_FINAL_DRIVE)) m_flFinalDrive = 4.10f;
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_EFFICIENCY)) m_flTransmissionEfficiency = 0.88f;
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_SHIFT_UP)) m_flShiftUpRPM = 4200.0f;
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_SHIFT_DOWN)) m_flShiftDownRPM = 1600.0f;
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_SHIFT_DURATION)) m_flShiftDuration = 0.25f;
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_CONVERTER_STALL)) m_flConverterStallRPM = 2400.0f;
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_CONVERTER_RATIO)) m_flConverterMaxRatio = 2.0f;
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_CONVERTER_COUPLING)) m_flConverterCouplingRPM = 1200.0f;
	if (!(m_iDrivetrainEditorOverrides & CAR_DT_OV_CONVERTER_RESPONSE)) m_flConverterResponseRPM = 3200.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_LIGHT_L)) m_vecHeadlightPos[0] = Vector(90, 28, 8);
	if (!(m_iExtraEditorOverrides & CAR_XOV_LIGHT_R)) m_vecHeadlightPos[1] = Vector(90, -28, 8);
	if (!(m_iExtraEditorOverrides & CAR_XOV_LIGHT_DIST)) m_flHeadlightDistance = 700.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_LIGHT_ANGLE)) m_flHeadlightAngle = 35.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_LIGHT_BRIGHT)) m_flHeadlightBrightness = 2.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_LIGHT_COLOR)) m_vecHeadlightColor = Vector(255, 245, 220);
	if (!(m_iExtraEditorOverrides & CAR_XOV_DRIVER_MODEL)) m_iszDriverModel = MAKE_STRING("models/cars/driver.mdl");
	if (!(m_iExtraEditorOverrides & CAR_XOV_IMPACT_SOUNDS))
	{
		m_iszImpactSounds[0] = MAKE_STRING("cars/car_impact1.wav");
		m_iszImpactSounds[1] = MAKE_STRING("cars/car_impact2.wav");
		m_iszImpactSounds[2] = MAKE_STRING("cars/car_impact3.wav");
		m_iszImpactSounds[3] = MAKE_STRING("cars/car_impact4.wav");
	}
	if (!(m_iExtraEditorOverrides & CAR_XOV_LANDING_SOUND)) m_iszLandingSound = MAKE_STRING("cars/landing.wav");
	if (!(m_iExtraEditorOverrides & CAR_XOV_IMPACT_MIN)) m_flImpactSoundMinKph = 8.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_IMPACT_COOLDOWN)) m_flImpactCooldown = 0.35f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_DAMAGE_THRESHOLD)) m_flDamageThresholdKph = 40.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_DAMAGE_LOW)) m_flDamageAtThreshold = 2.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_DAMAGE_REFERENCE)) m_flDamageReferenceKph = 100.0f;
	if (!(m_iExtraEditorOverrides & CAR_XOV_DAMAGE_HIGH)) m_flDamageAtReference = 45.0f;
	if (!(m_iEditorOverrides & CAR_OV_STEERANGLE)) m_flSteerAngle = 30.0f;
	if (!(m_iEditorOverrides & CAR_OV_STEERSPEED)) m_flSteerSpeed = 90.0f;
	if (!(m_iEditorOverrides & CAR_OV_SUSPLENGTH)) m_flSuspensionLength = 20.0f;
	if (!(m_iEditorOverrides & CAR_OV_SPRING)) m_flSpringStrength = 45.0f;
	if (!(m_iEditorOverrides & CAR_OV_SUSPDAMP)) m_flSuspensionDamping = 6.0f;
	if (!(m_iEditorOverrides & CAR_OV_MASS)) m_flBodyMass = CAR_BODY_MASS;
	if (!(m_iEditorOverrides & CAR_OV_COM)) m_vecBodyCenterOfMass = Vector(0, 0, -12);
	if (!(m_iEditorOverrides & CAR_OV_LATGRIP)) m_flLateralGrip = CAR_LATERAL_GRIP;
	if (!(m_iEditorOverrides & CAR_OV_HIGHSPEEDSTEER)) m_flHighSpeedSteerScale = 0.35f;
	if (!(m_iEditorOverrides & CAR_OV_MAXLAT)) m_flMaxLateralAcceleration = 500.0f;
	if (!(m_iEditorOverrides & CAR_OV_HANDBRAKE)) m_flHandbrakeStrength = 4.0f;
	if (!(m_iEditorOverrides & CAR_OV_LINDAMP)) m_flBodyLinearDamping = 0.08f;
	if (!(m_iEditorOverrides & CAR_OV_ANGDAMP)) m_flBodyAngularDamping = 0.45f;
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_DOOR)) m_iszDoorSound = MAKE_STRING("cars/car_door.wav");
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_START)) m_iszEngineStartSound = MAKE_STRING("cars/Hummer/eng_start.wav");
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_IDLE)) m_iszEngineIdleSound = MAKE_STRING("cars/Hummer/eng_idle.wav");
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_RUN)) m_iszEngineRunSound = MAKE_STRING("cars/Hummer/eng_run.wav");
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_STOP)) m_iszEngineStopSound = MAKE_STRING("cars/Hummer/eng_stop.wav");
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_HORN)) m_iszHornSound = MAKE_STRING("cars/hummer/hummer_horn.wav");
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_DOOR_TIME)) m_flDoorActionDuration = 1.717f;
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_DOOR_LEAD)) m_flDoorTransitionLead = 0.17f;
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_IGNITION_TIME)) m_flIgnitionHoldDuration = 0.7f;
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_START_TIME)) m_flEngineStartDuration = 0.484f;
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_IDLE_PITCH)) m_flEngineIdlePitch = 100.0f;
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_MAX_PITCH)) m_flEngineMaxPitch = 190.0f;
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_PITCH_UP)) m_flEnginePitchUpSpeed = 100.0f;
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_PITCH_DOWN)) m_flEnginePitchDownSpeed = 55.0f;
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_VOLUME)) m_flEngineVolume = 1.0f;
	if (!(m_iSoundEditorOverrides & CAR_SND_OV_INTERVAL)) m_flEngineSoundInterval = 0.08f;
}

bool CFuncCar::LoadConfig()
{
	const char *classname = GetClassname();
	char path[256];
	Q_snprintf(path, sizeof(path), "scripts/cars/%s.cfg", classname);
	int length = 0;
	char *file = reinterpret_cast<char *>(LOAD_FILE(path, &length));
	if (!file)
	{
		ALERT(at_error, "func_car: cannot load %s for entity %s / не удалось загрузить файл машины\n", path, classname);
		return false;
	}
	char key[128], value[256];
	char *cursor = file;
	while ((cursor = COM_ParseFileExt(cursor, key, sizeof(key), true)) != NULL)
	{
		if (!key[0] || FStrEq(key, "{") || FStrEq(key, "}")) continue;
		cursor = COM_ParseFileExt(cursor, value, sizeof(value), true);
		if (!cursor || !value[0])
		{
			ALERT(at_error, "func_car: %s parameter '%s' has no value / параметр не имеет значения\n", path, key);
			break;
		}
		if (!ApplyConfigValue(key, value, false))
			ALERT(at_warning, "func_car: %s unknown parameter '%s' / неизвестный параметр\n", path, key);
		else if (FStrEq(key, "mass") && Q_atof(value) < 0)
			ALERT(at_warning, "func_car: %s parameter 'mass' is negative / масса машины отрицательная\n", path);
	}
	FREE_FILE(file);
	return true;
}

void CFuncCar::Precache()
{
	if (pev->model != NULL_STRING) PRECACHE_MODEL(STRING(pev->model));
	if (m_iszWheelModel != NULL_STRING) PRECACHE_MODEL(STRING(m_iszWheelModel));
	if (m_iszDriverModel != NULL_STRING) PRECACHE_MODEL(STRING(m_iszDriverModel));
	if (m_iszDoorSound != NULL_STRING) PRECACHE_SOUND(STRING(m_iszDoorSound));
	if (m_iszEngineStartSound != NULL_STRING) PRECACHE_SOUND(STRING(m_iszEngineStartSound));
	if (m_iszEngineIdleSound != NULL_STRING) PRECACHE_SOUND(STRING(m_iszEngineIdleSound));
	if (m_iszEngineRunSound != NULL_STRING) PRECACHE_SOUND(STRING(m_iszEngineRunSound));
	if (m_iszEngineStopSound != NULL_STRING) PRECACHE_SOUND(STRING(m_iszEngineStopSound));
	if (m_iszHornSound != NULL_STRING) PRECACHE_SOUND(STRING(m_iszHornSound));
	for (int i = 0; i < 4; ++i)
		if (m_iszImpactSounds[i] != NULL_STRING) PRECACHE_SOUND(STRING(m_iszImpactSounds[i]));
	if (m_iszLandingSound != NULL_STRING) PRECACHE_SOUND(STRING(m_iszLandingSound));
}

void CFuncCar::Spawn()
{
	m_bRaceLocked = FALSE;
	ApplyDefaults();
	LoadConfig();

	Precache();
	if (pev->model == NULL_STRING)
	{
		ALERT(at_error, "func_car at %.0f %.0f %.0f has no body model\n", pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove(this);
		return;
	}
	SET_MODEL(edict(), STRING(pev->model));
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_NONE;
	// The car currently takes no structural damage, but DAMAGE_YES lets radius
	// damage deliver DMG_BLAST so a nearby explosion can wake a sleeping body.
	// TakeDamage below deliberately ignores bullets and all actual health loss.
	pev->takedamage = DAMAGE_YES;

	studiohdr_t *header = static_cast<studiohdr_t *>(GET_MODEL_PTR(edict()));
	if (header)
	{
		mstudioseqdesc_t *sequences = reinterpret_cast<mstudioseqdesc_t *>(reinterpret_cast<byte *>(header) + header->seqindex);
		UTIL_SetSize(pev, sequences[pev->sequence].bbmin, sequences[pev->sequence].bbmax);
	}
	else UTIL_SetSize(pev, Vector(-48, -32, -16), Vector(48, 32, 32));

	// Keep the mapper-authored transform independently of the moving PhysX body.
	m_vecSpawnOrigin = GetAbsOrigin();
	m_vecSpawnAngles = GetAbsAngles();
	ResetWheelDynamics();
	ResetDrivetrain();
	// Runtime ignition state must never depend on recycled edict memory. This is
	// especially visible on the first map load, before a round reset has called
	// StopEngine and cleared the hold latch for us.
	m_iEngineState = CAR_ENGINE_OFF;
	m_flEngineStateUntil = 0.0f;
	m_flIgnitionHoldStart = 0.0f;
	m_bIgnitionLatched = FALSE;
	m_flEnginePitch = m_flEngineIdlePitch;
	m_flNextEngineSound = 0.0f;
	m_bHeadlightsOn = FALSE;
	m_bParkingBrakeOn = FALSE;
	m_bStaticRestConstraint = FALSE;
	m_bCarPhysicsSleeping = FALSE;
	m_flSleepCandidateSince = 0.0f;
	m_flNextVehicleHud = 0.0f;

	CreatePhysicsBody();

	SetThink(&CFuncCar::CarThink);
	m_flLastThink = gpGlobals->time;
	m_vecLastSafeOrigin = GetAbsOrigin();
	m_vecLastSafeAngles = GetAbsAngles();
	m_bHasLastSafeTransform = TRUE;
	SetNextThink(CAR_THINK_INTERVAL);
	EnsureChildren();
}

void CFuncCar::ResetForBombRound()
{
	ResetToSpawn(false);
}

void CFuncCar::ResetForRace()
{
	ResetToSpawn(true);
	SetRaceLocked(false);
}

void CFuncCar::RemoveForRace()
{
	if (m_hDriver != NULL) return;
	CancelUseAction();
	StopHorn();
	StopEngine(false);
	SetHeadlights(false);
	RemoveChildren();
	if (m_pUserData != NULL) WorldPhysic->RemoveBody(edict());
	m_pUserData = NULL;
	m_iActorType = ACTOR_INVALID;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SetThink(NULL);
}

void CFuncCar::ResetToSpawn(bool keepDriver)
{
	CancelUseAction();
	StopHorn();
	SetHeadlights(false);
	m_bParkingBrakeOn = FALSE;
	m_bStaticRestConstraint = FALSE;
	m_bCarPhysicsSleeping = FALSE;
	m_flSleepCandidateSince = 0.0f;
	m_flNextVehicleHud = 0.0f;
	m_hUseBlockedPlayer = NULL;
	if (!keepDriver) StopEngine(false);
	if (!keepDriver && m_hDriver != NULL && m_hDriver->IsPlayer())
		ExitDriver(static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver)), true);
	if (!keepDriver) m_hDriver = NULL;
	m_flSpeed = 0.0f;
	m_flThrottle = 0.0f;
	m_iDriveDirection = 0;
	m_iPendingDriveDirection = 0;
	m_flDirectionChangeUntil = 0.0f;
	m_flSteering = 0.0f;
	ResetWheelDynamics();
	m_flVerticalVelocity = 0.0f;
	m_flDriverViewYaw = 0.0f;
	m_flDriverViewPitch = 0.0f;
	m_iGroundedWheels = 0;
	memset(m_bWheelGrounded, 0, sizeof(m_bWheelGrounded));
	memset(m_flCompression, 0, sizeof(m_flCompression));
	memset(m_flPreviousCompression, 0, sizeof(m_flPreviousCompression));

	SetAbsOrigin(m_vecSpawnOrigin);
	SetAbsAngles(m_vecSpawnAngles);
	SetAbsVelocity(g_vecZero);
	SetLocalAvelocity(g_vecZero);
	if (m_pUserData == NULL) CreatePhysicsBody();
	if (m_pUserData != NULL)
	{
		WorldPhysic->SetBodySleeping(this, false);
		WorldPhysic->SetOrigin(this, m_vecSpawnOrigin);
		WorldPhysic->SetAngles(this, m_vecSpawnAngles);
		WorldPhysic->SetVelocity(this, g_vecZero);
		WorldPhysic->SetAvelocity(this, g_vecZero);
	}

	m_vecLastSafeOrigin = m_vecSpawnOrigin;
	m_vecLastSafeAngles = m_vecSpawnAngles;
	m_bHasLastSafeTransform = TRUE;
	for (int i = 0; i < EXIT_IGNORE_SLOTS; ++i)
	{
		m_hExitIgnorePlayers[i] = NULL;
		m_flExitIgnoreUntil[i] = 0.0f;
	}
	m_flLastThink = gpGlobals->time;
	EnsureChildren();
	UpdateVisuals(0.0f);
	if (keepDriver && m_hDriver != NULL && m_hDriver->IsPlayer())
	{
		CBasePlayer *player = static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver));
		player->m_pVehicle = this;
		player->SetLocalOrigin(m_vecDriverPos);
		player->SetLocalAngles(g_vecZero);
		SET_VIEW(player->edict(), GetVehicleViewEntity()->edict());
		SendVehicleHud(true);
	}
	SetThink(&CFuncCar::CarThink);
	SetNextThink(CAR_THINK_INTERVAL);
}

void CFuncCar::SetRaceLocked(bool locked)
{
	m_bRaceLocked = locked ? TRUE : FALSE;
	if (locked)
	{
		m_bParkingBrakeOn = TRUE;
		m_flThrottle = 0.0f;
	}
	else
	{
		m_bParkingBrakeOn = FALSE;
		WakeCarPhysics();
	}
}

void CFuncCar::ForceRaceExit()
{
	if (m_hDriver != NULL && m_hDriver->IsPlayer())
		ExitDriver(static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver)), true);
}

void CFuncCar::CreatePhysicsBody()
{
	if (!WorldPhysic->Initialized()) return;
	pev->solid = SOLID_CUSTOM;
	pev->movetype = MOVETYPE_PHYSIC;
	m_pUserData = WorldPhysic->CreateBodyFromEntity(this);
	if (m_pUserData == NULL)
	{
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_NONE;
		ALERT(at_warning, "%s: PhysX body creation failed / не удалось создать физический кузов\n", GetClassname());
	}
}

void CFuncCar::ReloadConfig()
{
	m_bCarPhysicsSleeping = FALSE;
	m_flSleepCandidateSince = 0.0f;
	CancelUseAction();
	StopEngine(false);
	if (m_hDriver != NULL && m_hDriver->IsPlayer())
		ExitDriver(static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver)), true);
	RemoveChildren();
	WorldPhysic->RemoveBody(edict());
	m_pUserData = NULL;
	m_iActorType = ACTOR_INVALID;
	ApplyDefaults();
	if (!LoadConfig()) return;
	ResetWheelDynamics();
	Precache();
	if (pev->model == NULL_STRING) return;
	SET_MODEL(edict(), STRING(pev->model));
	studiohdr_t *header = static_cast<studiohdr_t *>(GET_MODEL_PTR(edict()));
	if (header)
	{
		mstudioseqdesc_t *sequences = reinterpret_cast<mstudioseqdesc_t *>(reinterpret_cast<byte *>(header) + header->seqindex);
		UTIL_SetSize(pev, sequences[pev->sequence].bbmin, sequences[pev->sequence].bbmax);
	}
	CreatePhysicsBody();
	EnsureChildren();
	ALERT(at_console, "%s: reloaded scripts/cars/%s.cfg / конфигурация перезагружена\n", GetClassname(), GetClassname());
}

void CFuncCar::ResetWheelDynamics()
{
	m_bStaticRestConstraint = FALSE;
	memset(m_flWheelAngularVelocity, 0, sizeof(m_flWheelAngularVelocity));
	memset(m_flWheelRotation, 0, sizeof(m_flWheelRotation));
	memset(m_flWheelLongitudinalSlip, 0, sizeof(m_flWheelLongitudinalSlip));
	memset(m_flWheelLateralSlip, 0, sizeof(m_flWheelLateralSlip));
	memset(m_flWheelLoad, 0, sizeof(m_flWheelLoad));
	memset(m_flWheelLongitudinalForce, 0, sizeof(m_flWheelLongitudinalForce));
	memset(m_flWheelLateralForce, 0, sizeof(m_flWheelLateralForce));
	memset(m_flWheelBrakeTorque, 0, sizeof(m_flWheelBrakeTorque));
	memset(m_bWheelLocked, 0, sizeof(m_bWheelLocked));
	memset(m_flWheelGripUtilization, 0, sizeof(m_flWheelGripUtilization));
	memset(m_flWheelGroundSpeed, 0, sizeof(m_flWheelGroundSpeed));
	memset(m_bWheelStaticLateralGrip, 0, sizeof(m_bWheelStaticLateralGrip));
	memset(m_flWheelStaticGripBlend, 0, sizeof(m_flWheelStaticGripBlend));
	memset(m_flWheelRequiredStaticForce, 0, sizeof(m_flWheelRequiredStaticForce));
	memset(m_flWheelMaxGripForce, 0, sizeof(m_flWheelMaxGripForce));
	memset(m_pWheelContactMaterial, 0, sizeof(m_pWheelContactMaterial));
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		m_flWheelMaterialLongitudinalGrip[i] = 1.0f;
		m_flWheelMaterialLateralGrip[i] = 1.0f;
		m_flWheelMaterialRollingResistance[i] = 1.0f;
	}
}

void CFuncCar::Activate()
{
	BaseClass::Activate();
	const bool restoreSleepingPose = m_bCarPhysicsSleeping != FALSE &&
		m_bHasLastSafeTransform != FALSE;
	// Save/restore may recreate a body with the engine's saved sleeping flag.
	// Cars must wake and rebuild suspension contacts before accepting input.
	if (m_iActorType == ACTOR_DYNAMIC && WorldPhysic)
	{
		m_fFreezed = FALSE;
		WorldPhysic->SetBodySleeping(this, false);
		if (restoreSleepingPose)
		{
			SetAbsOrigin(m_vecLastSafeOrigin);
			SetAbsAngles(m_vecLastSafeAngles);
		}

		// A PhysX save contains only the rigid chassis pose; raycast suspension is
		// restored separately. First escape any BSP penetration, then reconstruct
		// the saved wheel-to-ground distances so the first spring step cannot start
		// with the chassis embedded and permanently wedged in the floor.
		Vector repairedOrigin = GetAbsOrigin();
		const Vector repairedAngles = GetAbsAngles();
		const float maximumRepair = m_flSuspensionLength + m_flWheelRadius + 32.0f;
		for (float lift = 0.0f; lift <= maximumRepair &&
			!BodyPositionClear(repairedOrigin, repairedAngles); lift += 2.0f)
			repairedOrigin.z += 2.0f;
		SetAbsOrigin(repairedOrigin);

		matrix4x4 repairTransform(repairedOrigin, repairedAngles, 1.0f);
		const float traceDistance = m_flSuspensionLength + m_flWheelRadius;
		float requiredLift = 0.0f;
		int repairContacts = 0;
		for (int i = 0; i < WHEEL_COUNT; ++i)
		{
			if (!m_bWheelGrounded[i]) continue;
			const Vector mount = repairTransform.VectorTransform(m_vecWheelPos[i]);
			TraceResult trace;
			UTIL_TraceLine(mount, mount + Vector(0, 0, -traceDistance),
				ignore_monsters, edict(), &trace);
			if (trace.flFraction >= 1.0f || trace.fStartSolid) continue;
			const float savedSuspension = ClampFloat(
				m_flSuspensionLength - m_flCompression[i], 0.0f, m_flSuspensionLength);
			const float desiredMountZ = trace.vecEndPos.z + m_flWheelRadius + savedSuspension;
			requiredLift += desiredMountZ - mount.z;
			++repairContacts;
		}
		if (repairContacts > 0)
		{
			requiredLift = ClampFloat(requiredLift / repairContacts, 0.0f, maximumRepair);
			repairedOrigin.z += requiredLift;
			SetAbsOrigin(repairedOrigin);
		}

		SetAbsVelocity(g_vecZero);
		SetLocalAvelocity(g_vecZero);

		// Do not reuse the actor restored by the engine. Moving that actor out of
		// BSP does not invalidate all of its cached contact manifolds, so the first
		// simulation step can pull the chassis back into the floor and wedge it.
		// Recreate it from the repaired entity transform to start with clean broad-
		// phase and solver state.
		WorldPhysic->RemoveBody(edict());
		m_pUserData = NULL;
		m_iActorType = ACTOR_INVALID;
		CreatePhysicsBody();
		if (m_iActorType == ACTOR_DYNAMIC)
		{
			WorldPhysic->SetVelocity(this, g_vecZero);
			WorldPhysic->SetAvelocity(this, g_vecZero);
			WorldPhysic->SetBodySleeping(this, false);
		}
		for (int i = 0; i < WHEEL_COUNT; ++i)
			m_flPreviousCompression[i] = m_flCompression[i];
	}
	m_bCarPhysicsSleeping = FALSE;
	m_flSleepCandidateSince = 0.0f;
	EnsureChildren();
	if (m_hDriver != NULL && m_hDriver->IsPlayer())
	{
		CBasePlayer *player = static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver));
		player->m_pVehicle = this;
		SET_VIEW(player->edict(), GetVehicleViewEntity()->edict());
	}
	if (m_iEngineState == CAR_ENGINE_RUNNING)
	{
		m_flNextEngineSound = 0.0f;
		m_flEnginePitch = m_flEngineIdlePitch;
	}
	if (m_bHeadlightsOn) SetHeadlights(true);
}

Vector CFuncCar::LocalToWorld(const Vector &local) const
{
	return EntityToWorldTransform().VectorTransform(local);
}

void CFuncCar::EnsureChildren()
{
	if (m_hBodyVisual == NULL && pev->model != NULL_STRING)
	{
		CBaseEntity *body = CBaseEntity::Create("func_car_child", GetAbsOrigin(), GetAbsAngles(), edict());
		if (body)
		{
			SET_MODEL(body->edict(), STRING(pev->model));
			body->pev->iuser4 = FUNC_CAR_BODY_MARKER;
			// A non-blocking trace proxy makes every visible piece of the studio
			// body usable even when the hidden PhysX convex is behind a BSP corner.
			body->pev->solid = SOLID_TRIGGER;
			// func_car_child spawns point-sized. SET_MODEL alone does not populate
			// its server collision bounds, so use the authoritative chassis bounds.
			// These remain local to the identically oriented parent and therefore
			// form the correct OBB for crosshair/use tests at every car yaw.
			UTIL_SetSize(body, pev->mins, pev->maxs);
			body->SetParent(this);
			body->SetLocalOrigin(g_vecZero);
			body->SetLocalAngles(g_vecZero);
			body->pev->fuser1 = m_flMaxSpeed;
			body->pev->fuser2 = m_flReverseSpeed;
			body->pev->fuser3 = m_flAcceleration;
			body->pev->fuser4 = m_flDrag;
			body->m_flPoseParameter[0] = m_flBrakeForce;
			body->m_flPoseParameter[1] = m_flSteerAngle;
			body->m_flPoseParameter[2] = m_flSteerSpeed;
			body->m_flPoseParameter[3] = Q_max( 16.0f,
				fabs( (m_vecWheelPos[WHEEL_FL].x + m_vecWheelPos[WHEEL_FR].x -
				m_vecWheelPos[WHEEL_RL].x - m_vecWheelPos[WHEEL_RR].x) * 0.5f ));
			m_hBodyVisual = body;
			// The PhysX entity remains authoritative for collision and networking,
			// but its model is represented by the child above, on the same visual
			// path that already kept the wheels stable in multiplayer.
			pev->effects |= EF_NODRAW;
		}
	}
	if (m_iszWheelModel != NULL_STRING)
	{
		for (int i = 0; i < WHEEL_COUNT; ++i)
		{
			if (m_hWheels[i] != NULL) continue;
			CBaseEntity *wheel = CBaseEntity::Create("func_car_child", GetAbsOrigin(), GetAbsAngles(), edict());
			if (!wheel) continue;
			SET_MODEL(wheel->edict(), STRING(m_iszWheelModel));
			wheel->pev->iuser4 = FUNC_CAR_WHEEL_MARKER;
			wheel->pev->iuser2 = m_hBodyVisual != NULL ? m_hBodyVisual->entindex() : 0;
			wheel->pev->startpos = Vector(1, 1, 1);
			wheel->SetParent(this);
			wheel->SetLocalOrigin(m_vecWheelPos[i]);
			m_hWheels[i] = wheel;
		}
	}
	if (m_hViewEntity == NULL)
	{
		CBaseEntity *view = CBaseEntity::Create("func_car_child", GetAbsOrigin(), GetAbsAngles(), edict());
		if (view)
		{
			// View entities need a modelindex to pass AddToFullPack. Make it
			// transparent instead of EF_NODRAW, which the server filters out.
			SET_MODEL(view->edict(), STRING(pev->model));
			view->pev->rendermode = kRenderTransTexture;
			view->pev->renderamt = 0;
			view->pev->iuser4 = FUNC_CAR_VIEW_MARKER;
			view->pev->iuser2 = m_hBodyVisual != NULL ? m_hBodyVisual->entindex() : 0;
			view->pev->startpos = m_vecViewPos;
			// Body, wheels and camera must share the same multiplayer interpolation
			// timeline. A modelindex keeps this transparent view entity networked.
			view->pev->effects |= EF_MERGE_VISIBILITY;
			view->SetParent(this);
			view->SetLocalOrigin(m_vecViewPos);
			view->SetLocalAngles(g_vecZero);
			m_hViewEntity = view;
		}
	}
	if (m_hDriverVisual == NULL && m_iszDriverModel != NULL_STRING)
	{
		CBaseEntity *driver = CBaseEntity::Create("func_car_child", GetAbsOrigin(), GetAbsAngles(), edict());
		if (driver)
		{
			SET_MODEL(driver->edict(), STRING(m_iszDriverModel));
			driver->SetParent(this);
			driver->SetLocalOrigin(m_vecDriverPos);
			driver->SetLocalAngles(g_vecZero);
			driver->pev->effects |= EF_NODRAW;
			m_hDriverVisual = driver;
		}
	}
}

CBaseEntity *CFuncCar::GetVehicleViewEntity()
{
	return m_hViewEntity != NULL ? static_cast<CBaseEntity *>(m_hViewEntity) : this;
}

void CFuncCar::RemoveChildren()
{
	RemoveHeadlights();
	if (m_hBodyVisual != NULL) UTIL_Remove(m_hBodyVisual);
	m_hBodyVisual = NULL;
	pev->effects &= ~EF_NODRAW;
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		if (m_hWheels[i] != NULL) UTIL_Remove(m_hWheels[i]);
		m_hWheels[i] = NULL;
	}
	if (m_hViewEntity != NULL) UTIL_Remove(m_hViewEntity);
	m_hViewEntity = NULL;
	if (m_hDriverVisual != NULL) UTIL_Remove(m_hDriverVisual);
	m_hDriverVisual = NULL;
}

void CFuncCar::OnRemove()
{
	CancelUseAction();
	StopHorn();
	StopEngine(false);
	if (m_hDriver != NULL && m_hDriver->IsPlayer())
		ExitDriver(static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver)), true);
	RemoveChildren();
}

bool CFuncCar::HandleVehicleImpulse(int impulse)
{
	if (m_hDriver == NULL) return false;
	if (impulse == 100)
	{
		SetHeadlights(!m_bHeadlightsOn);
		SendVehicleHud(true);
		return true;
	}
	if (impulse == 101)
	{
		ToggleParkingBrake();
		return true;
	}
	return false;
}

void CFuncCar::ToggleParkingBrake()
{
	m_bParkingBrakeOn = !m_bParkingBrakeOn;
	m_flThrottle = 0.0f;
	m_iPendingDriveDirection = 0;
	m_flDirectionChangeUntil = 0.0f;
	for (int wheel = 0; wheel < WHEEL_COUNT; ++wheel)
		m_flWheelAngularVelocity[wheel] = 0.0f;
	SendVehicleHud(true);
}

void CFuncCar::SendVehicleHud(bool visible)
{
	int flags = visible ? 1 : 0;
	if (m_iEngineState == CAR_ENGINE_RUNNING) flags |= 2;
	if (m_bHeadlightsOn) flags |= 4;
	if (m_bParkingBrakeOn) flags |= 8;
	// The camera child is already part of every vehicle snapshot. Carry the HUD
	// state in its otherwise-unused iuser3 instead of consuming another global
	// GoldSrc user-message type.
	if (m_hViewEntity != NULL)
	{
		m_hViewEntity->pev->iuser3 = flags;
		// Reuse the already-networked camera entity; no additional user-message
		// registration is needed for the speedometer.
		m_hViewEntity->pev->fuser4 = m_flSpeed;
	}
	// SelAmmo is a legacy four-byte message that is registered but unused by
	// both this game and its client. Reusing its existing slot avoids Xash's
	// global user-message limit while providing reliable owner-only HUD data.
	if (m_hDriver != NULL && m_hDriver->IsPlayer() && gmsgSelAmmo)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgSelAmmo, NULL, m_hDriver->edict());
			WRITE_BYTE(flags);
			WRITE_SHORT((short)ClampFloat(m_flSpeed, -32767.0f, 32767.0f));
			WRITE_BYTE(0xCA);
		MESSAGE_END();
		// Keep every packet at SelAmmo's registered four-byte size. Distinct
		// signatures let the client update drivetrain values without adding a new
		// user-message type or touching entity delta fields.
		MESSAGE_BEGIN(MSG_ONE, gmsgSelAmmo, NULL, m_hDriver->edict());
			WRITE_BYTE((byte)ClampFloat((float)(m_iCurrentGear + 1), 0.0f, 255.0f));
			WRITE_SHORT((short)ClampFloat(m_flEngineRPM, 0.0f, 32767.0f));
			WRITE_BYTE(0xCB);
		MESSAGE_END();
		MESSAGE_BEGIN(MSG_ONE, gmsgSelAmmo, NULL, m_hDriver->edict());
			WRITE_BYTE(0);
			WRITE_SHORT((short)ClampFloat(m_flEngineTorque, 0.0f, 32767.0f));
			WRITE_BYTE(0xCC);
		MESSAGE_END();
		MESSAGE_BEGIN(MSG_ONE, gmsgSelAmmo, NULL, m_hDriver->edict());
			WRITE_BYTE((byte)ClampFloat(m_flConverterRatio * 100.0f, 0.0f, 255.0f));
			WRITE_SHORT((short)ClampFloat(m_flConverterSlipRPM, -32767.0f, 32767.0f));
			WRITE_BYTE(0xCD);
		MESSAGE_END();
		MESSAGE_BEGIN(MSG_ONE, gmsgSelAmmo, NULL, m_hDriver->edict());
			WRITE_BYTE(0);
			WRITE_SHORT((short)ClampFloat(m_flTransmittedTorque, 0.0f, 32767.0f));
			WRITE_BYTE(0xCE);
		MESSAGE_END();
	}
	m_flNextVehicleHud = gpGlobals->time + 0.2f;
}

int CFuncCar::GetVehicleHudFlags() const
{
	int flags = 1;
	if (m_iEngineState == CAR_ENGINE_RUNNING) flags |= 2;
	if (m_bHeadlightsOn) flags |= 4;
	if (m_bParkingBrakeOn) flags |= 8;
	return flags;
}

void CFuncCar::CreateHeadlights()
{
	for (int i = 0; i < 2; ++i)
	{
		if (m_hHeadlights[i] != NULL) continue;
		CBaseEntity *light = CreateEntityByName("env_dynlight");
		if (!light) continue;
		light->pev->rendercolor = m_vecHeadlightColor;
		light->pev->renderamt = m_flHeadlightDistance;
		light->pev->scale = m_flHeadlightAngle;
		light->pev->spawnflags = 1 | 2; // start off, no shadows
		char brightness[32];
		Q_snprintf(brightness, sizeof(brightness), "%.3f", m_flHeadlightBrightness);
		KeyValueData kvd = { "env_dynlight", "brightness", brightness, FALSE };
		DispatchKeyValue(light->edict(), &kvd);
		light->SetAbsOrigin(LocalToWorld(m_vecHeadlightPos[i]));
		light->SetAbsAngles(GetAbsAngles());
		DispatchSpawn(light->edict());
		light->SetParent(this);
		light->SetLocalOrigin(m_vecHeadlightPos[i]);
		light->SetLocalAngles(g_vecZero);
		m_hHeadlights[i] = light;
	}
}

void CFuncCar::RemoveHeadlights()
{
	for (int i = 0; i < 2; ++i)
	{
		if (m_hHeadlights[i] != NULL) UTIL_Remove(m_hHeadlights[i]);
		m_hHeadlights[i] = NULL;
	}
}

void CFuncCar::SetHeadlights(bool enabled)
{
	if (enabled) CreateHeadlights();
	m_bHeadlightsOn = enabled ? TRUE : FALSE;
	for (int i = 0; i < 2; ++i)
		if (m_hHeadlights[i] != NULL)
			m_hHeadlights[i]->Use(this, this, enabled ? USE_ON : USE_OFF, 0.0f);
}

bool CFuncCar::DriverRequestsMovement()
{
	if (m_hDriver == NULL || !m_hDriver->IsPlayer()) return false;
	const int buttons = m_hDriver->pev->button;
	return FBitSet(buttons, IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
}

void CFuncCar::WakeCarPhysics()
{
	if (!m_bCarPhysicsSleeping) return;
	m_bCarPhysicsSleeping = FALSE;
	m_flSleepCandidateSince = 0.0f;
	m_flLastThink = gpGlobals->time;
	if (m_iActorType == ACTOR_DYNAMIC && WorldPhysic)
		WorldPhysic->SetBodySleeping(this, false);
	SetNextThink(CAR_THINK_INTERVAL);
}

void CFuncCar::WakeFromVehicleImpact()
{
	WakeCarPhysics();
}

void CFuncCar::UpdateSleepState()
{
	if (m_bCarPhysicsSleeping || m_hDriver != NULL || m_iActorType != ACTOR_DYNAMIC ||
		m_iGroundedWheels < 2)
	{
		m_flSleepCandidateSince = 0.0f;
		return;
	}
	const float linearSpeed = GetAbsVelocity().Length();
	const float angularSpeed = GetAbsAvelocity().Length();
	if (linearSpeed > CAR_SLEEP_LINEAR_SPEED || angularSpeed > CAR_SLEEP_ANGULAR_SPEED)
	{
		m_flSleepCandidateSince = 0.0f;
		return;
	}
	if (m_flSleepCandidateSince <= 0.0f)
	{
		m_flSleepCandidateSince = gpGlobals->time;
		return;
	}
	if (gpGlobals->time - m_flSleepCandidateSince < CAR_SLEEP_DELAY)
		return;

	m_bCarPhysicsSleeping = TRUE;
	m_flSleepCandidateSince = 0.0f;
	// Raycast wheels do not exist as rigid contacts. Preserve the force-supported
	// ride pose explicitly so a final PhysX step cannot settle the chassis shape
	// down into BSP after suspension updates stop.
	m_vecLastSafeOrigin = GetAbsOrigin();
	m_vecLastSafeAngles = GetAbsAngles();
	m_bHasLastSafeTransform = TRUE;
	m_flSpeed = 0.0f;
	m_flThrottle = 0.0f;
	m_flSteering = 0.0f;
	for (int i = 0; i < WHEEL_COUNT; ++i)
		m_flWheelAngularVelocity[i] = 0.0f;
	if (WorldPhysic)
	{
		WorldPhysic->SetVelocity(this, g_vecZero);
		WorldPhysic->SetAvelocity(this, g_vecZero);
		WorldPhysic->SetOrigin(this, m_vecLastSafeOrigin);
		WorldPhysic->SetAngles(this, m_vecLastSafeAngles);
		WorldPhysic->SetBodySleeping(this, true);
	}
}

void CFuncCar::Touch(CBaseEntity *other)
{
	CFuncCar *otherCar = dynamic_cast<CFuncCar *>(other);
	if (otherCar)
	{
		const float relativeSpeed = (GetAbsVelocity() - otherCar->GetAbsVelocity()).Length();
		if (relativeSpeed >= CAR_VEHICLE_IMPACT_WAKE_SPEED)
		{
			WakeFromVehicleImpact();
			otherCar->WakeFromVehicleImpact();
		}
	}
	BaseClass::Touch(other);
}

int CFuncCar::TakeDamage(entvars_t *, entvars_t *, float, int damageType)
{
	if (FBitSet(damageType, DMG_BLAST))
		WakeCarPhysics();
	return 0;
}

void CFuncCar::Use(CBaseEntity *pActivator, CBaseEntity *, USE_TYPE useType, float)
{
	if (!pActivator || !pActivator->IsPlayer()) return;
	CBasePlayer *player = static_cast<CBasePlayer *>(pActivator);
	if (m_hUseBlockedPlayer == player) return;
	if (useType == USE_REMOVE)
	{
		if (m_hDriver == player) ExitDriver(player, true);
		return;
	}
	if (m_iUseAction != CAR_USE_NONE && m_hUsePlayer != player) return;
	if (m_iUseAction == CAR_USE_NONE)
	{
		if (m_hDriver != NULL && m_hDriver != player) return;
		m_hUsePlayer = player;
		m_iUseAction = m_hDriver == player ? CAR_USE_EXIT : CAR_USE_ENTER;
		m_flUseActionStart = gpGlobals->time;
		const float transitionTime = Q_max(0.01f, m_flDoorActionDuration - m_flDoorTransitionLead);
		SendActionBar(player, 3, transitionTime);
		if (m_iszDoorSound != NULL_STRING)
			EMIT_SOUND(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
				CHAN_BODY, STRING(m_iszDoorSound), 1.0f, ATTN_NORM);
	}
}

void CFuncCar::SendActionBar(CBasePlayer *player, int action, float duration) const
{
	if (!player) return;
	MESSAGE_BEGIN(MSG_ONE, gmsgActionBar, NULL, player->pev);
		WRITE_BYTE(action);
		WRITE_SHORT((int)(Q_max(0.0f, duration) * 10.0f + 0.5f));
	MESSAGE_END();
}

void CFuncCar::CancelUseAction()
{
	CBasePlayer *player = m_hUsePlayer != NULL && m_hUsePlayer->IsPlayer()
		? static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hUsePlayer)) : NULL;
	if (player) SendActionBar(player, 0, 0.0f);
	m_hUsePlayer = NULL;
	m_iUseAction = CAR_USE_NONE;
	m_flUseActionStart = 0.0f;
}

void CFuncCar::UpdateUseAction()
{
	if (m_iUseAction == CAR_USE_NONE) return;
	CBasePlayer *player = m_hUsePlayer != NULL && m_hUsePlayer->IsPlayer()
		? static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hUsePlayer)) : NULL;
	const bool entering = m_iUseAction == CAR_USE_ENTER;
	float distanceToBody = 0.0f;
	if (player)
	{
		const Vector local = EntityToWorldTransform().VectorITransform(player->GetAbsOrigin());
		Vector nearest;
		for (int axis = 0; axis < 3; ++axis)
			nearest[axis] = ClampFloat(local[axis], pev->mins[axis], pev->maxs[axis]);
		distanceToBody = (player->GetAbsOrigin() - LocalToWorld(nearest)).Length();
	}
	if (!player || !player->IsAlive() || !FBitSet(player->pev->button, IN_USE) ||
		(entering && distanceToBody > 96.0f) ||
		(entering && player->m_pVehicle != NULL) || (!entering && m_hDriver != player))
	{
		CancelUseAction();
		return;
	}
	const float transitionTime = Q_max(0.01f, m_flDoorActionDuration - m_flDoorTransitionLead);
	if (gpGlobals->time - m_flUseActionStart < transitionTime) return;
	const int completedAction = m_iUseAction;
	CancelUseAction();
	m_hUseBlockedPlayer = player;
	if (completedAction == CAR_USE_ENTER) EnterDriver(player);
	else if (!g_pGameRules || g_pGameRules->CanPlayerExitVehicle(player)) ExitDriver(player);
}

void CFuncCar::EnterDriver(CBasePlayer *player)
{
	if (!player || player->m_pVehicle != NULL || !player->EnterVehicle(this)) return;
	m_hDriver = player;
	player->pev->effects |= EF_NODRAW;
	if (m_hDriverVisual != NULL) m_hDriverVisual->pev->effects &= ~EF_NODRAW;
	player->SetLocalOrigin(m_vecDriverPos);
	player->SetLocalAngles(g_vecZero);
	player->pev->v_angle = GetAbsAngles();
	player->pev->fixangle = TRUE;
	m_flDriverViewYaw = 0.0f;
	m_flDriverViewPitch = 0.0f;
	m_vecLastDriverInputAngles = player->pev->v_angle;
	SendVehicleHud(true);
}

bool CFuncCar::ForceRaceEnter(CBasePlayer *player)
{
	if (!player || m_hDriver != NULL || player->m_pVehicle != NULL) return false;
	EnterDriver(player);
	if (m_hDriver != player) return false;

	// Race drivers spawn ready to move; no ignition hold is required.
	StopEngineLoops();
	m_iEngineState = CAR_ENGINE_RUNNING;
	m_flEngineStateUntil = 0.0f;
	m_flIgnitionHoldStart = 0.0f;
	m_bIgnitionLatched = FALSE;
	m_flEnginePitch = m_flEngineIdlePitch;
	m_flNextEngineSound = 0.0f;
	m_bParkingBrakeOn = FALSE;
	WakeCarPhysics();
	SendVehicleHud(true);
	return true;
}

bool CFuncCar::FindExitPosition(CBasePlayer *player, Vector &position) const
{
	const Vector preferred = LocalToWorld(m_vecExitPos);
	const Vector right = EntityToWorldTransform().GetRight();
	const Vector forward = EntityToWorldTransform().GetForward();
	const Vector candidates[] = {
		preferred, preferred + Vector(0, 0, 24),
		GetAbsOrigin() + right * (pev->maxs.y + 32), GetAbsOrigin() - right * (pev->maxs.y + 32),
		GetAbsOrigin() - forward * (fabs(pev->mins.x) + 32), GetAbsOrigin() + forward * (pev->maxs.x + 32)
	};
	for (const Vector &candidate : candidates)
	{
		TraceResult trace;
		UTIL_TraceEntity(player, candidate, candidate, &trace);
		if (!trace.fStartSolid && !trace.fAllSolid)
		{
			position = candidate;
			return true;
		}
	}
	return false;
}

void CFuncCar::ExitDriver(CBasePlayer *player, bool force)
{
	StopHorn();
	if (!player || m_hDriver != player) return;
	Vector exitPoint;
	if (!FindExitPosition(player, exitPoint))
	{
		if (force) exitPoint = LocalToWorld(m_vecExitPos);
		else
		{
		ALERT(at_console, "func_car: no safe exit position for %s\n", STRING(player->pev->netname));
		return;
		}
	}
	SendVehicleHud(false);
	m_hDriver = NULL;
	player->pev->effects &= ~EF_NODRAW;
	if (m_hDriverVisual != NULL) m_hDriverVisual->pev->effects |= EF_NODRAW;
	Vector angles = GetAbsAngles();
	angles.x = angles.z = 0;
	IgnoreExitCollision(player);
	player->LeaveVehicle(exitPoint, angles);
}

bool CFuncCar::CanDrive() const
{
	return !m_bRaceLocked && m_iEngineState == CAR_ENGINE_RUNNING;
}

void CFuncCar::StopEngineLoops()
{
	if (m_iszEngineIdleSound != NULL_STRING)
		STOP_SOUND(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
			CHAN_WEAPON, STRING(m_iszEngineIdleSound));
	if (m_iszEngineRunSound != NULL_STRING)
		STOP_SOUND(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
			CHAN_VOICE, STRING(m_iszEngineRunSound));
}

void CFuncCar::StopHorn()
{
	if (m_bHornPlaying && m_iszHornSound != NULL_STRING)
		STOP_SOUND(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
			CHAN_STATIC, STRING(m_iszHornSound));
	m_bHornPlaying = FALSE;
	m_flNextHornRestart = 0.0f;
}

void CFuncCar::UpdateHorn()
{
	const bool pressed = m_hDriver != NULL && FBitSet(m_hDriver->pev->button, IN_ATTACK);
	if (pressed && m_iszHornSound != NULL_STRING &&
		(!m_bHornPlaying || gpGlobals->time >= m_flNextHornRestart))
	{
		const char *sample = STRING(m_iszHornSound);
		EMIT_SOUND(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
			CHAN_STATIC, sample, 1.0f, ATTN_NORM);
		m_bHornPlaying = TRUE;
		char filepath[256];
		Q_snprintf(filepath, sizeof(filepath), "sound/%s", sample);
		const float duration = g_engfuncs.pfnGetApproxWavePlayLen
			? g_engfuncs.pfnGetApproxWavePlayLen(filepath) * 0.001f : 1.0f;
		// Restart just before the decoded WAV ends. Using the same entity/channel
		// replaces the preceding instance and produces a continuous held horn even
		// when the source WAV has no embedded GoldSrc loop markers.
		m_flNextHornRestart = gpGlobals->time + Q_max(0.05f, duration - 0.02f);
	}
	else if (!pressed) StopHorn();
}

void CFuncCar::StartEngine()
{
	if (m_iEngineState != CAR_ENGINE_OFF) return;
	StopEngineLoops();
	m_iEngineState = CAR_ENGINE_STARTING;
	// Starter begins on the key-down edge. The engine catches exactly when the
	// hold progress completes, regardless of the WAV's authored length.
	m_flEngineStateUntil = gpGlobals->time + Q_max(0.0f, m_flIgnitionHoldDuration);
	m_flEnginePitch = m_flEngineIdlePitch;
	m_flNextEngineSound = 0.0f;
	if (m_iszEngineStartSound != NULL_STRING)
		EMIT_SOUND(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
			CHAN_ITEM, STRING(m_iszEngineStartSound), m_flEngineVolume, ATTN_NORM);
}

void CFuncCar::StopEngine(bool playSound)
{
	const bool wasOn = m_iEngineState != CAR_ENGINE_OFF;
	StopEngineLoops();
	if (playSound && wasOn && m_iszEngineStopSound != NULL_STRING)
		EMIT_SOUND(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
			CHAN_ITEM, STRING(m_iszEngineStopSound), m_flEngineVolume, ATTN_NORM);
	m_iEngineState = CAR_ENGINE_OFF;
	m_flEngineStateUntil = 0.0f;
	m_flEnginePitch = m_flEngineIdlePitch;
	m_flNextEngineSound = 0.0f;
	m_iDriveDirection = 0;
	m_iPendingDriveDirection = 0;
	m_flDirectionChangeUntil = 0.0f;
	ResetDrivetrain();
}

void CFuncCar::UpdateEngine(float dt)
{
	CBasePlayer *driver = m_hDriver != NULL && m_hDriver->IsPlayer()
		? static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver)) : NULL;
	if (driver && FBitSet(driver->pev->button, IN_ALT1))
	{
		if (m_flIgnitionHoldStart <= 0.0f)
		{
			m_flIgnitionHoldStart = gpGlobals->time;
			SendActionBar(driver, 3, m_flIgnitionHoldDuration);
			if (m_iEngineState == CAR_ENGINE_OFF) StartEngine();
		}
		else if (!m_bIgnitionLatched && gpGlobals->time - m_flIgnitionHoldStart >= m_flIgnitionHoldDuration)
		{
			m_bIgnitionLatched = TRUE;
			SendActionBar(driver, 0, 0.0f);
			if (m_iEngineState == CAR_ENGINE_STARTING)
			{
				m_iEngineState = CAR_ENGINE_RUNNING;
				m_flEngineStateUntil = 0.0f;
				m_flNextEngineSound = 0.0f;
			}
			else if (m_iEngineState == CAR_ENGINE_RUNNING) StopEngine(true);
		}
	}
	else
	{
		if (driver && m_flIgnitionHoldStart > 0.0f && !m_bIgnitionLatched)
			SendActionBar(driver, 0, 0.0f);
		if (m_iEngineState == CAR_ENGINE_STARTING && !m_bIgnitionLatched)
		{
			if (m_iszEngineStartSound != NULL_STRING)
				STOP_SOUND(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
					CHAN_ITEM, STRING(m_iszEngineStartSound));
			m_iEngineState = CAR_ENGINE_OFF;
			m_flEngineStateUntil = 0.0f;
		}
		m_flIgnitionHoldStart = 0.0f;
		m_bIgnitionLatched = FALSE;
	}
	if (m_iEngineState != CAR_ENGINE_RUNNING) return;
	const float speedScale = m_iDriveDirection < 0 ? m_flReverseSpeed : m_flMaxSpeed;
	const float speedFraction = ClampFloat(GetCarPlanarSpeed() /
		Q_max(1.0f, speedScale), 0.0f, 1.0f);
	const float loadFraction = ClampFloat(Q_max(speedFraction, fabs(m_flThrottle) * 0.35f), 0.0f, 1.0f);
	const float targetPitch = m_flEngineIdlePitch + (m_flEngineMaxPitch - m_flEngineIdlePitch) * loadFraction;
	const float pitchStep = (targetPitch > m_flEnginePitch ? m_flEnginePitchUpSpeed : m_flEnginePitchDownSpeed) * dt;
	m_flEnginePitch = CarApproach(targetPitch, m_flEnginePitch, pitchStep);
	if (gpGlobals->time < m_flNextEngineSound) return;
	m_flNextEngineSound = gpGlobals->time + Q_max(0.02f, m_flEngineSoundInterval);
	const float runBlend = ClampFloat(speedFraction * 1.5f + fabs(m_flThrottle) * 0.45f, 0.0f, 1.0f);
	// Keep the low idle layer under load. Large vehicles lose their weight if
	// the bass loop almost disappears while the short run loop takes over.
	const float idleVolume = m_flEngineVolume * (1.0f - runBlend * 0.45f);
	const float runVolume = m_flEngineVolume * Q_max(0.01f, runBlend);
	if (m_iszEngineIdleSound != NULL_STRING)
		EMIT_SOUND_DYN(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
			CHAN_WEAPON, STRING(m_iszEngineIdleSound), idleVolume,
			ATTN_NORM, SND_CHANGE_VOL | SND_CHANGE_PITCH, (int)m_flEngineIdlePitch);
	if (m_iszEngineRunSound != NULL_STRING)
		EMIT_SOUND_DYN(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
			CHAN_VOICE, STRING(m_iszEngineRunSound), runVolume,
			ATTN_NORM, SND_CHANGE_VOL | SND_CHANGE_PITCH, (int)m_flEnginePitch);
}

void CFuncCar::IgnoreExitCollision(CBasePlayer *player)
{
	if (!player) return;
	int slot = 0;
	for (int i = 0; i < EXIT_IGNORE_SLOTS; ++i)
	{
		if (m_hExitIgnorePlayers[i] == player) { slot = i; break; }
		if (m_hExitIgnorePlayers[i] == NULL || m_flExitIgnoreUntil[i] <= gpGlobals->time)
		{
			slot = i;
			break;
		}
		if (m_flExitIgnoreUntil[i] < m_flExitIgnoreUntil[slot]) slot = i;
	}
	m_hExitIgnorePlayers[slot] = player;
	m_flExitIgnoreUntil[slot] = gpGlobals->time + 0.25f;
}

bool CFuncCar::ShouldIgnoreExitCollision(CBaseEntity *other)
{
	if (!other) return false;
	for (int i = 0; i < EXIT_IGNORE_SLOTS; ++i)
		if (m_hExitIgnorePlayers[i] == other && m_flExitIgnoreUntil[i] > gpGlobals->time)
			return true;
	return false;
}

void CFuncCar::UpdateInput(float dt)
{
	float requestedThrottle = 0;
	float steeringTarget = 0;
	if (m_hDriver != NULL && CanDrive() && !m_bParkingBrakeOn)
	{
		if (FBitSet(m_hDriver->pev->button, IN_FORWARD)) requestedThrottle += 1;
		if (FBitSet(m_hDriver->pev->button, IN_BACK)) requestedThrottle -= 1;
		if (FBitSet(m_hDriver->pev->button, IN_MOVELEFT)) steeringTarget += m_flSteerAngle;
		if (FBitSet(m_hDriver->pev->button, IN_MOVERIGHT)) steeringTarget -= m_flSteerAngle;
	}

	float throttleTarget = requestedThrottle;
	const int requestedDirection = requestedThrottle > 0.0f ? 1 : (requestedThrottle < 0.0f ? -1 : 0);
	if (requestedDirection != 0)
	{
		// The first direction after starting is available immediately. Switching
		// to the opposite direction still brakes normally while the car moves;
		// only after it stops do we spend the configured time in neutral.
		if (m_iDriveDirection == 0)
			m_iDriveDirection = requestedDirection;
		else if (requestedDirection != m_iDriveDirection)
		{
			// Only motion in the currently engaged direction is still the braking
			// phase. PhysX can cross zero between two thinks; using abs(speed) here
			// mistook that small opposite drift for continued braking and allowed
			// reverse traction without ever entering the neutral delay.
			if (m_flSpeed * m_iDriveDirection > CAR_STOP_EPSILON)
			{
				m_iPendingDriveDirection = requestedDirection;
				m_flDirectionChangeUntil = 0.0f;
			}
			else
			{
				if (m_iPendingDriveDirection != requestedDirection || m_flDirectionChangeUntil <= 0.0f)
				{
					m_iPendingDriveDirection = requestedDirection;
					m_flDirectionChangeUntil = gpGlobals->time + Q_max(0.0f, m_flDirectionChangeDelay);
				}
				if (gpGlobals->time >= m_flDirectionChangeUntil)
				{
					m_iDriveDirection = requestedDirection;
					m_iPendingDriveDirection = 0;
					m_flDirectionChangeUntil = 0.0f;
					// Match each wheel to its actual contact-patch speed. This avoids
					// injecting artificial slip when the new direction engages, while a
					// genuinely stopped vehicle still naturally produces zero omega.
					const float wheelRadius = Q_max(fabs(m_flWheelRadius), 1.0f);
					for (int wheel = 0; wheel < WHEEL_COUNT; ++wheel)
						m_flWheelAngularVelocity[wheel] =
							m_flWheelGroundSpeed[wheel] / wheelRadius;
				}
				else throttleTarget = 0.0f;
			}
		}
		else
		{
			m_iPendingDriveDirection = 0;
			m_flDirectionChangeUntil = 0.0f;
		}
	}
	else
	{
		// Releasing the requested opposite direction cancels the shift. A later
		// press begins a fresh, full delay instead of inheriting an old timer.
		m_iPendingDriveDirection = 0;
		m_flDirectionChangeUntil = 0.0f;
	}
	const float riseRate = m_flThrottleRiseTime > 0.0f ? 1.0f / m_flThrottleRiseTime : 1000.0f;
	// Releasing the pedal is immediate enough to keep braking responsive; only
	// applied drive torque is ramped in.
	if (throttleTarget == 0.0f || (m_flThrottle * throttleTarget < 0.0f))
		m_flThrottle = 0.0f;
	else m_flThrottle = CarApproach(throttleTarget, m_flThrottle, riseRate * dt);
	m_flSteering = CarApproach(steeringTarget, m_flSteering, m_flSteerSpeed * dt);
	if (m_iActorType == ACTOR_DYNAMIC)
		return;

	if (m_hDriver != NULL && FBitSet(m_hDriver->pev->button, IN_JUMP))
		m_flSpeed = CarApproach(0, m_flSpeed, m_flBrakeForce * 1.25f * dt);
	else if (throttleTarget > 0)
	{
		if (m_flSpeed < -CAR_STOP_EPSILON) m_flSpeed = CarApproach(0, m_flSpeed, m_flBrakeForce * dt);
		else m_flSpeed = Q_min(m_flMaxSpeed, m_flSpeed + m_flAcceleration * dt);
	}
	else if (throttleTarget < 0)
	{
		if (m_flSpeed > CAR_STOP_EPSILON) m_flSpeed = CarApproach(0, m_flSpeed, m_flBrakeForce * dt);
		else m_flSpeed = Q_max(-m_flReverseSpeed, m_flSpeed - m_flAcceleration * dt);
	}
	else m_flSpeed = CarApproach(0, m_flSpeed, m_flDrag * dt);
	if (fabs(m_flSpeed) < CAR_STOP_EPSILON && throttleTarget == 0) m_flSpeed = 0;
}

void CFuncCar::UpdateWheels(float dt)
{
	m_iGroundedWheels = 0;
	// Suspension always acts against world gravity. Using the chassis local
	// down vector makes a tilted airborne car cast sideways and never reacquire
	// the ground after a jump.
	const Vector down(0, 0, -1);
	const float traceDistance = m_flSuspensionLength + m_flWheelRadius;
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		m_flWheelLoad[i] = 0.0f;
		m_pWheelContactMaterial[i] = NULL;
		m_flWheelMaterialLongitudinalGrip[i] = 1.0f;
		m_flWheelMaterialLateralGrip[i] = 1.0f;
		m_flWheelMaterialRollingResistance[i] = 1.0f;
		const bool wasGrounded = m_bWheelGrounded[i] != FALSE;
		m_vecWheelWorld[i] = LocalToWorld(m_vecWheelPos[i]);
		const Vector traceEnd = m_vecWheelWorld[i] + down * traceDistance;
		TraceResult trace;
		SetBits(gpGlobals->trace_flags, FTRACE_MATERIAL_TRACE);
		UTIL_TraceLine(m_vecWheelWorld[i], traceEnd, ignore_monsters, edict(), &trace);
		ClearBits(gpGlobals->trace_flags, FTRACE_MATERIAL_TRACE);
		m_flPreviousCompression[i] = m_flCompression[i];
		m_flCompression[i] = 0;
		m_bWheelGrounded[i] = FALSE;
		m_vecWheelContact[i] = trace.vecEndPos;
		m_vecWheelNormal[i] = trace.vecPlaneNormal;
		if (trace.flFraction < 1.0f && !trace.fStartSolid)
		{
			m_bWheelGrounded[i] = TRUE;
			matdef_t *contactMaterial = NULL;
			CBaseEntity *hitEntity = CBaseEntity::Instance(trace.pHit);
			if (hitEntity && UTIL_GetModelType(hitEntity->pev->modelindex) == mod_brush)
			{
				msurface_t *surface = TRACE_SURFACE(trace.pHit, m_vecWheelWorld[i], traceEnd);
				contactMaterial = COM_MatDefFromSurface(surface, trace.vecEndPos);
			}
			else if (trace.materialHash)
			{
				matdesc_t *material = COM_FindMaterial(trace.materialHash);
				contactMaterial = material ? material->effects : NULL;
			}
			if (contactMaterial)
			{
				m_pWheelContactMaterial[i] = contactMaterial;
				m_flWheelMaterialLongitudinalGrip[i] = contactMaterial->carLongitudinalGrip;
				m_flWheelMaterialLateralGrip[i] = contactMaterial->carLateralGrip;
				m_flWheelMaterialRollingResistance[i] = contactMaterial->carRollingResistance;
			}
			const float suspension = Q_max(0.0f, traceDistance * trace.flFraction - m_flWheelRadius);
			m_flCompression[i] = ClampFloat(m_flSuspensionLength - suspension, 0, m_flSuspensionLength);
			// Reacquiring a raycast contact is not an instantaneous suspension
			// compression from zero. Suppress that one-frame derivative spike;
			// the spring component still supports the wheel immediately.
			if (!wasGrounded)
				m_flPreviousCompression[i] = m_flCompression[i];
			++m_iGroundedWheels;
		}
	}

	if (m_iActorType == ACTOR_DYNAMIC)
	{
		for (int i = 0; i < WHEEL_COUNT; ++i)
		{
			if (!m_bWheelGrounded[i]) continue;
			// A delayed frame (notably synchronous screenshots) can otherwise turn a
			// small raycast displacement into an enormous one-tick damper impulse.
			const float compressionVelocity = ClampFloat(
				(m_flCompression[i] - m_flPreviousCompression[i]) / Q_max(dt, 0.001f),
				-CAR_MAX_SUSPENSION_COMPRESSION_SPEED, CAR_MAX_SUSPENSION_COMPRESSION_SPEED);
			const float suspensionAcceleration = ClampFloat(
				m_flCompression[i] * m_flSpringStrength + compressionVelocity * m_flSuspensionDamping,
				0.0f, 2400.0f);
			m_flWheelLoad[i] = suspensionAcceleration * m_flBodyMass / WHEEL_COUNT;
			const float impulse = suspensionAcceleration * m_flBodyMass * dt / WHEEL_COUNT;
			WorldPhysic->AddImpulse(this, m_vecWheelNormal[i], m_vecWheelContact[i], impulse);
		}
		return;
	}

	float springAcceleration = -CAR_GRAVITY;
	if (m_iGroundedWheels > 0)
	{
		float totalForce = 0;
		for (int i = 0; i < WHEEL_COUNT; ++i)
		{
			if (m_flCompression[i] <= 0) continue;
			const float compressionVelocity = (m_flCompression[i] - m_flPreviousCompression[i]) / Q_max(dt, 0.001f);
			totalForce += m_flCompression[i] * m_flSpringStrength + compressionVelocity * m_flSuspensionDamping;
		}
		springAcceleration += totalForce / WHEEL_COUNT;
	}
	m_flVerticalVelocity = ClampFloat(m_flVerticalVelocity + springAcceleration * dt, -800.0f, 400.0f);
}

float CFuncCar::EvaluateDriveForce(float speedFraction) const
{
	static const float speedPoints[6] = { 0.0f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f };
	const float speed = ClampFloat(speedFraction, 0.0f, 1.0f);
	for (int i = 0; i < 5; ++i)
	{
		if (speed > speedPoints[i + 1]) continue;
		const float fraction = (speed - speedPoints[i]) / (speedPoints[i + 1] - speedPoints[i]);
		return m_flDriveForceFalloff[i] +
			(m_flDriveForceFalloff[i + 1] - m_flDriveForceFalloff[i]) * fraction;
	}
	return m_flDriveForceFalloff[5];
}

float CFuncCar::EvaluateEngineTorque(float rpm) const
{
	if (rpm <= m_flEngineTorqueCurveRPM[0])
		return m_flEngineTorqueCurve[0];
	for (int i = 0; i < ENGINE_TORQUE_POINTS - 1; ++i)
	{
		if (rpm > m_flEngineTorqueCurveRPM[i + 1]) continue;
		const float range = Q_max(1.0f,
			m_flEngineTorqueCurveRPM[i + 1] - m_flEngineTorqueCurveRPM[i]);
		const float fraction = ClampFloat(
			(rpm - m_flEngineTorqueCurveRPM[i]) / range, 0.0f, 1.0f);
		return m_flEngineTorqueCurve[i] +
			(m_flEngineTorqueCurve[i + 1] - m_flEngineTorqueCurve[i]) * fraction;
	}
	return m_flEngineTorqueCurve[ENGINE_TORQUE_POINTS - 1];
}

void CFuncCar::ResetDrivetrain()
{
	m_flEngineRPM = 0.0f;
	m_flEngineTorque = 0.0f;
	m_flDrivelineRPM = 0.0f;
	m_flPerWheelDriveTorque = 0.0f;
	m_flConverterSlipRPM = 0.0f;
	m_flConverterRatio = 1.0f;
	m_flTransmittedTorque = 0.0f;
	m_iCurrentGear = 0;
	m_iTargetGear = 0;
	m_flShiftStartTime = 0.0f;
	m_flShiftEndTime = 0.0f;
}

float CFuncCar::UpdateDrivetrain(float dt, int drivenWheels, bool allowDriveTorque)
{
	float averageDrivenOmega = 0.0f;
	int sampledWheels = 0;
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		if (!IsDrivenWheel(i)) continue;
		averageDrivenOmega += m_flWheelAngularVelocity[i];
		++sampledWheels;
	}
	if (sampledWheels > 0) averageDrivenOmega /= sampledWheels;
	m_flDrivelineRPM = fabs(averageDrivenOmega) * CAR_RADIANS_TO_RPM;
	m_flPerWheelDriveTorque = 0.0f;
	m_flEngineTorque = 0.0f;
	m_flConverterSlipRPM = 0.0f;
	m_flConverterRatio = 1.0f;
	m_flTransmittedTorque = 0.0f;

	if (!CanDrive())
	{
		m_flEngineRPM = 0.0f;
		return 0.0f;
	}

	const int forwardGearCount = Q_max(1, Q_min(m_iForwardGearCount, MAX_FORWARD_GEARS));
	float gearRatio = 0.0f;
	float shiftTorqueScale = 1.0f;

	if (m_iDriveDirection < 0)
	{
		m_iCurrentGear = -1;
		m_iTargetGear = -1;
		m_flShiftStartTime = 0.0f;
		m_flShiftEndTime = 0.0f;
		gearRatio = -fabs(m_flReverseRatio);
	}
	else if (m_iDriveDirection > 0)
	{
		if (m_iCurrentGear < 1 || m_iCurrentGear > forwardGearCount)
			m_iCurrentGear = 1;
		if (m_iTargetGear < 1 || m_iTargetGear > forwardGearCount)
			m_iTargetGear = m_iCurrentGear;

		gearRatio = fabs(m_flGearRatios[m_iCurrentGear - 1]);

		const bool shifting = m_flShiftEndTime > gpGlobals->time &&
			m_flShiftEndTime > m_flShiftStartTime;
		if (!shifting)
		{
			m_flShiftStartTime = 0.0f;
			m_flShiftEndTime = 0.0f;
			m_iTargetGear = m_iCurrentGear;
			if (m_flEngineRPM >= m_flShiftUpRPM && m_iCurrentGear < forwardGearCount)
				m_iTargetGear = m_iCurrentGear + 1;
			else if (m_flEngineRPM <= m_flShiftDownRPM && m_iCurrentGear > 1)
				m_iTargetGear = m_iCurrentGear - 1;
			if (m_iTargetGear != m_iCurrentGear)
			{
				m_flShiftStartTime = gpGlobals->time;
				m_flShiftEndTime = gpGlobals->time + Q_max(0.0f, m_flShiftDuration);
			}
		}

		if (m_flShiftEndTime > gpGlobals->time &&
			m_flShiftEndTime > m_flShiftStartTime)
		{
			const float progress = ClampFloat(
				(gpGlobals->time - m_flShiftStartTime) /
				Q_max(0.001f, m_flShiftEndTime - m_flShiftStartTime), 0.0f, 1.0f);
			shiftTorqueScale = fabs(1.0f - progress * 2.0f);
			if (progress >= 0.5f)
			{
				m_iCurrentGear = m_iTargetGear;
				gearRatio = fabs(m_flGearRatios[m_iCurrentGear - 1]);
			}
		}
		else if (m_flShiftEndTime > 0.0f && gpGlobals->time >= m_flShiftEndTime)
		{
			m_iCurrentGear = m_iTargetGear;
			m_flShiftStartTime = 0.0f;
			m_flShiftEndTime = 0.0f;
		}
	}
	else
	{
		m_iCurrentGear = 0;
		m_iTargetGear = 0;
		m_flShiftStartTime = 0.0f;
		m_flShiftEndTime = 0.0f;
	}

	// The converter input speed is the wheel/driveline speed transformed through
	// the selected gearbox ratio and final drive. At low road speed the engine is
	// allowed to rise toward stall RPM instead of being rigidly clamped to it.
	const float converterInputRPM = m_flDrivelineRPM * fabs(gearRatio) * fabs(m_flFinalDrive);
	const float throttleAmount = ClampFloat(fabs(m_flThrottle), 0.0f, 1.0f);
	const float stallTargetRPM = m_flEngineIdleRPM +
		(Q_max(m_flConverterStallRPM, m_flEngineIdleRPM) - m_flEngineIdleRPM) * throttleAmount;
	float targetEngineRPM = Q_max(m_flEngineIdleRPM, converterInputRPM);
	if (gearRatio != 0.0f)
		targetEngineRPM = Q_max(targetEngineRPM, stallTargetRPM);
	const float maxEngineRPM = Q_max(m_flEngineIdleRPM,
		m_flEngineTorqueCurveRPM[ENGINE_TORQUE_POINTS - 1]);
	targetEngineRPM = Q_min(targetEngineRPM, maxEngineRPM);
	if (m_flEngineRPM <= 0.0f) m_flEngineRPM = m_flEngineIdleRPM;
	m_flEngineRPM = CarApproach(targetEngineRPM, m_flEngineRPM,
		Q_max(1.0f, m_flConverterResponseRPM) * Q_max(dt, 0.0f));
	m_flConverterSlipRPM = m_flEngineRPM - converterInputRPM;
	const float normalizedSlip = ClampFloat(Q_max(0.0f, m_flConverterSlipRPM) /
		Q_max(1.0f, m_flConverterCouplingRPM), 0.0f, 1.0f);
	const float smoothSlip = normalizedSlip * normalizedSlip * (3.0f - 2.0f * normalizedSlip);
	m_flConverterRatio = 1.0f +
		(Q_max(1.0f, m_flConverterMaxRatio) - 1.0f) * smoothSlip;
	if (!allowDriveTorque || gearRatio == 0.0f || fabs(m_flThrottle) <= 0.0f)
		return 0.0f;

	m_flEngineTorque = Q_max(0.0f, EvaluateEngineTorque(m_flEngineRPM)) *
		fabs(m_flThrottle);
	m_flTransmittedTorque = m_flEngineTorque * m_flConverterRatio;
	const float totalWheelTorque = m_flTransmittedTorque * CAR_NM_TO_GAME_TORQUE *
		gearRatio * fabs(m_flFinalDrive) * m_flTransmissionEfficiency * shiftTorqueScale;
	m_flPerWheelDriveTorque = totalWheelTorque / Q_max(1, drivenWheels);
	return m_flPerWheelDriveTorque;
}

void CFuncCar::PlayImpact(float impactSpeed)
{
	const float impactKph = impactSpeed * 0.09144f; // GoldSrc units/sec (inches) to km/h
	if (gpGlobals->time < m_flNextImpactSound || impactKph < m_flImpactSoundMinKph) return;
	{
		const int choice = RANDOM_LONG(0, 3);
		if (m_iszImpactSounds[choice] != NULL_STRING)
			EMIT_SOUND(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
				CHAN_ITEM, STRING(m_iszImpactSounds[choice]), 1.0f, ATTN_NORM);
		m_flNextImpactSound = gpGlobals->time + m_flImpactCooldown;
		m_flDriverDamageAnimUntil = gpGlobals->time + 0.7f;
	}
	if (m_hDriver != NULL && impactKph > m_flDamageThresholdKph)
	{
		const float range = Q_max(1.0f, m_flDamageReferenceKph - m_flDamageThresholdKph);
		const float t = Q_max(0.0f, (impactKph - m_flDamageThresholdKph) / range);
		// Quadratic growth keeps 41 km/h close to two damage while reaching the
		// configured reference damage at 100 km/h and continuing above it.
		const float damage = m_flDamageAtThreshold +
			(m_flDamageAtReference - m_flDamageAtThreshold) * t * t;
		m_hDriver->TakeDamage(pev, pev, damage, DMG_CRUSH);
	}
}

void CFuncCar::UpdateImpactAndLanding(const Vector &velocityBefore, int groundedBefore)
{
	Vector horizontalDelta = m_vecPreviousVelocity - velocityBefore;
	horizontalDelta.z = 0.0f;
	if (horizontalDelta.Length() > 40.0f) PlayImpact(horizontalDelta.Length());

	if (groundedBefore == 0 && m_iGroundedWheels == 0) m_bWasAirborne = TRUE;
	if (m_bWasAirborne && m_iGroundedWheels >= 2)
	{
		if (m_iszLandingSound != NULL_STRING)
			EMIT_SOUND_DYN(m_hBodyVisual != NULL ? m_hBodyVisual->edict() : edict(),
				CHAN_BODY, STRING(m_iszLandingSound), 1.0f, ATTN_NORM, 0, RANDOM_LONG(95, 105));
		m_bWasAirborne = FALSE;
	}
}

void CFuncCar::UpdateDriverVisual(float dt)
{
	CBaseAnimating *driver = m_hDriverVisual != NULL
		? static_cast<CBaseAnimating *>(static_cast<CBaseEntity *>(m_hDriverVisual)) : NULL;
	if (!driver) return;
	driver->SetLocalOrigin(m_vecDriverPos);
	int sequence = 0;
	if (gpGlobals->time < m_flDriverDamageAnimUntil) sequence = 2;
	else if (m_flSpeed < -CAR_STOP_EPSILON) sequence = 1;
	if (driver->pev->sequence != sequence)
	{
		driver->pev->sequence = sequence;
		driver->pev->frame = 0;
		driver->ResetSequenceInfo();
	}
	driver->StudioFrameAdvance(dt);
}

bool CFuncCar::IsDrivenWheel(int wheel) const
{
	if (m_iDriveType == DRIVE_FWD) return wheel == WHEEL_FL || wheel == WHEEL_FR;
	if (m_iDriveType == DRIVE_RWD) return wheel == WHEEL_RL || wheel == WHEEL_RR;
	return true;
}

float CFuncCar::EvaluateLongitudinalGrip(float slipRatio) const
{
	const float peak = Q_max(0.001f, fabs(m_flSlipPeak));
	const float normalizedSlip = fabs(slipRatio) / peak;
	if (normalizedSlip <= 1.0f)
		return normalizedSlip;
	// Smoothly descend from the peak toward the configured high-slip grip.
	const float tailGrip = ClampFloat(m_flSlipFalloff, 0.0f, 1.0f);
	return tailGrip + (1.0f - tailGrip) / normalizedSlip;
}

void CFuncCar::UpdateMotion(float dt)
{
	if (m_iActorType == ACTOR_DYNAMIC)
	{
		matrix4x4 transform = EntityToWorldTransform();
		Vector forward = transform.GetForward();
		Vector right = transform.GetRight();
		forward.z = 0;
		right.z = 0;
		if (forward.Length() > 0.001f) forward = forward.Normalize();
		if (right.Length() > 0.001f) right = right.Normalize();
		const Vector velocity = GetAbsVelocity();
		m_flSpeed = DotProduct(velocity, forward);
		// PhysX follows sv_gravity (800 by default), not the legacy fallback
		// constant used by the non-PhysX prototype. Static tyre support and load
		// normalization must use the exact same gravity as the scene.
		const float gravityMagnitude = Q_max(0.0f, CVAR_GET_FLOAT("sv_gravity"));
		const float gripGravityMagnitude = Q_max(1.0f, gravityMagnitude);

		const bool handbrake = CanDrive() && m_hDriver != NULL && FBitSet(m_hDriver->pev->button, IN_JUMP);
		const bool parkingBrake = m_bParkingBrakeOn != FALSE;
		// SPACE is a rear-wheel handbrake. X is the four-wheel latched parking
		// brake. Reversing the requested direction while moving is the ordinary
		// four-wheel service brake (serviceBrake below).
		const bool rearBrake = handbrake;
		const bool allWheelBrake = parkingBrake;

		const float radius = Q_max(fabs(m_flWheelRadius), 1.0f);
		const float automaticInertia = Q_max(1.0f, m_flBodyMass * radius * radius / WHEEL_COUNT);
		const float wheelInertia = m_flWheelInertia > 0.0f ? m_flWheelInertia : automaticInertia;
		int drivenWheels = 0;
		for (int i = 0; i < WHEEL_COUNT; ++i)
			if (IsDrivenWheel(i)) ++drivenWheels;
		drivenWheels = Q_max(1, drivenWheels);

		bool serviceBrake = false;
		bool allowDriveTorque = false;
		if (!handbrake && !parkingBrake && m_flThrottle > 0.0f)
		{
			serviceBrake = m_flSpeed < -CAR_STOP_EPSILON;
			if (!serviceBrake && m_flSpeed < m_flMaxSpeed)
				allowDriveTorque = true;
		}
		else if (!handbrake && !parkingBrake && m_flThrottle < 0.0f)
		{
			serviceBrake = m_flSpeed > CAR_STOP_EPSILON;
			if (!serviceBrake && m_flSpeed > -m_flReverseSpeed)
				allowDriveTorque = true;
		}

		const float perWheelDriveTorque = UpdateDrivetrain(dt, drivenWheels, allowDriveTorque);
		// Steering authority follows total road speed. Using only forward speed
		// restored full steering as soon as a fast turn developed lateral velocity.
		const float horizontalSpeed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y);
		const float speedFraction = ClampFloat(horizontalSpeed / Q_max(m_flMaxSpeed, 1.0f), 0.0f, 1.0f);
		const float highSpeedSteerScale = 1.0f - speedFraction * (1.0f - m_flHighSpeedSteerScale);
		const float steerRadians = m_flSteering * highSpeedSteerScale * (M_PI / 180.0f);
		const Vector bodyAngularVelocity = GetAbsAvelocity();
		const float maxSurfaceSpeed = Q_max(m_flMaxSpeed, m_flReverseSpeed) * CAR_MAX_WHEEL_SURFACE_SCALE;
		const float nominalWheelLoad = m_flBodyMass * gripGravityMagnitude / WHEEL_COUNT;
		float measuredGroundedLoad = 0.0f;
		float groundedNormalZ = 0.0f;
		int groundedWheelCount = 0;
		for (int i = 0; i < WHEEL_COUNT; ++i)
		{
			if (!m_bWheelGrounded[i]) continue;
			measuredGroundedLoad += Q_min(Q_max(0.0f, m_flWheelLoad[i]),
				nominalWheelLoad * CAR_MAX_TYRE_LOAD_SCALE);
			groundedNormalZ += Q_max(0.1f, m_vecWheelNormal[i].z);
			++groundedWheelCount;
		}
		const float averageNormalZ = groundedWheelCount > 0
			? groundedNormalZ / groundedWheelCount : 1.0f;
		// The PhysX chassis shape can carry part of the weight before the raycast
		// springs do. Spring force alone therefore under-reports the tyre normal
		// load (sometimes almost to zero) even though all four tyres touch the road.
		// Reconstruct the quasistatic support load from gravity and the road normal,
		// retaining the measured spring distribution for weight transfer.
		const float expectedGroundedLoad = groundedWheelCount > 0
			? m_flBodyMass * gripGravityMagnitude / Q_max(averageNormalZ, 0.25f) : 0.0f;
		const float effectiveGroundedLoad = Q_max(measuredGroundedLoad, expectedGroundedLoad);
		const bool directionShiftNeutral = m_iPendingDriveDirection != 0 &&
			m_flDirectionChangeUntil > gpGlobals->time;
		float totalAvailableGripForce = 0.0f;
		float totalAvailableLateralGripForce = 0.0f;
		float rearAvailableGripForce = 0.0f;
		Vector aggregateGroundNormal = g_vecZero;
		Vector aggregateDynamicTyreForce = g_vecZero;

		for (int i = 0; i < WHEEL_COUNT; ++i)
		{
			m_flWheelLongitudinalSlip[i] = 0.0f;
			m_flWheelLateralSlip[i] = 0.0f;
			m_flWheelLongitudinalForce[i] = 0.0f;
			m_flWheelLateralForce[i] = 0.0f;
			m_flWheelBrakeTorque[i] = 0.0f;
			m_bWheelLocked[i] = FALSE;
			m_flWheelGripUtilization[i] = 0.0f;
			m_flWheelGroundSpeed[i] = 0.0f;
			m_flWheelRequiredStaticForce[i] = 0.0f;
			m_flWheelMaxGripForce[i] = 0.0f;

			const bool rearAxle = i == WHEEL_RL || i == WHEEL_RR;
			if (IsDrivenWheel(i))
				m_flWheelAngularVelocity[i] += perWheelDriveTorque / wheelInertia * dt;

			// brakeforce retains its existing mapper-facing acceleration units. Convert
			// it to a real wheel torque here and integrate it once, after contact torque.
			// This lets a sufficiently strong brake hold omega at zero while the chassis
			// keeps moving, producing genuine negative longitudinal slip.
			float brakeTorque = 0.0f;
			if (serviceBrake || directionShiftNeutral || allWheelBrake)
				brakeTorque = m_flBrakeForce * wheelInertia / radius;
			if (rearBrake && rearAxle)
				brakeTorque = Q_max(brakeTorque,
					m_flBrakeForce * m_flHandbrakeStrength * wheelInertia / radius);
			m_flWheelBrakeTorque[i] = brakeTorque;
			if (m_flThrottle == 0.0f && !allWheelBrake && !(rearBrake && rearAxle))
			{
				const float rollingResistanceMultiplier = m_bWheelGrounded[i]
					? m_flWheelMaterialRollingResistance[i] : 1.0f;
				m_flWheelAngularVelocity[i] = CarApproach(0.0f, m_flWheelAngularVelocity[i],
					m_flRollingResistance * rollingResistanceMultiplier / radius * dt);
			}

			m_flWheelAngularVelocity[i] = ClampFloat(m_flWheelAngularVelocity[i],
				-maxSurfaceSpeed / radius, maxSurfaceSpeed / radius);
			if (!m_bWheelGrounded[i])
			{
				if (brakeTorque > 0.0f)
					m_flWheelAngularVelocity[i] = CarApproach(0.0f,
						m_flWheelAngularVelocity[i], brakeTorque / wheelInertia * dt);
				m_bWheelLocked[i] = brakeTorque > 0.0f &&
					fabs(m_flWheelAngularVelocity[i]) <= 0.01f;
				m_bWheelStaticLateralGrip[i] = FALSE;
				m_flWheelStaticGripBlend[i] = CarApproach(0.0f,
					m_flWheelStaticGripBlend[i], CAR_STATIC_LATERAL_BLEND_RATE * dt);
				continue;
			}

			const bool frontAxle = i == WHEEL_FL || i == WHEEL_FR;
			const float wheelSteer = frontAxle ? steerRadians : 0.0f;
			Vector wheelForward = forward * cosf(wheelSteer) + right * sinf(wheelSteer);
			Vector wheelRight = right * cosf(wheelSteer) - forward * sinf(wheelSteer);
			wheelForward -= m_vecWheelNormal[i] * DotProduct(wheelForward, m_vecWheelNormal[i]);
			wheelRight -= m_vecWheelNormal[i] * DotProduct(wheelRight, m_vecWheelNormal[i]);
			if (wheelForward.Length() > 0.001f) wheelForward = wheelForward.Normalize();
			if (wheelRight.Length() > 0.001f) wheelRight = wheelRight.Normalize();

			const Vector arm = m_vecWheelContact[i] - GetAbsOrigin();
			const Vector pointVelocity = velocity + CrossProduct(bodyAngularVelocity, arm);
			const float groundSpeed = DotProduct(pointVelocity, wheelForward);
			const float lateralSpeed = DotProduct(pointVelocity, wheelRight);
			const float surfaceSpeed = m_flWheelAngularVelocity[i] * radius;
			const float slipSpeed = surfaceSpeed - groundSpeed;
			const float slipDenominator = Q_max(CAR_SLIP_REFERENCE_SPEED,
				Q_max(fabs(surfaceSpeed), fabs(groundSpeed)));
			const float slipRatio = slipSpeed / slipDenominator;
			m_flWheelGroundSpeed[i] = groundSpeed;
			m_flWheelLongitudinalSlip[i] = slipSpeed;
			m_flWheelLateralSlip[i] = lateralSpeed;

			// Convert this wheel's spring/damper load into its available tyre force.
			// A wheel at nominal static load retains the old max_lateral_accel limit.
			float tyreLoad;
			if (measuredGroundedLoad > 0.001f)
				tyreLoad = Q_max(0.0f, m_flWheelLoad[i]) *
					effectiveGroundedLoad / measuredGroundedLoad;
			else tyreLoad = groundedWheelCount > 0
				? effectiveGroundedLoad / groundedWheelCount : 0.0f;
			tyreLoad = Q_min(tyreLoad, nominalWheelLoad * CAR_MAX_TYRE_LOAD_SCALE);
			const float maxGripForce = tyreLoad *
				m_flMaxLateralAcceleration / gripGravityMagnitude;
			const float maxLongitudinalGripForce = maxGripForce *
				m_flWheelMaterialLongitudinalGrip[i];
			const float maxLateralGripForce = maxGripForce *
				m_flWheelMaterialLateralGrip[i];
			m_flWheelMaxGripForce[i] = maxLateralGripForce;
			totalAvailableGripForce += maxGripForce;
			totalAvailableLateralGripForce += maxLateralGripForce;
			if (rearAxle)
				rearAvailableGripForce += maxGripForce;
			aggregateGroundNormal += m_vecWheelNormal[i] * tyreLoad;
			const float wheelMassShare = effectiveGroundedLoad > 0.001f
				? m_flBodyMass * tyreLoad / effectiveGroundedLoad
				: m_flBodyMass / WHEEL_COUNT;
			float longitudinalForce = 0.0f;
			if (fabs(slipSpeed) > 0.001f && maxLongitudinalGripForce > 0.0f)
				longitudinalForce = (slipSpeed > 0.0f ? 1.0f : -1.0f) * maxLongitudinalGripForce *
					EvaluateLongitudinalGrip(slipRatio) * m_flLongitudinalGrip;
			float lateralForce = ClampFloat(
				-lateralSpeed * m_flLateralGrip * m_flBodyMass / WHEEL_COUNT,
				-maxLateralGripForce, maxLateralGripForce);

			const Vector dynamicTyreForce =
				wheelForward * longitudinalForce + wheelRight * lateralForce;
			aggregateDynamicTyreForce += dynamicTyreForce;
			Vector tyreForce = dynamicTyreForce;
			const Vector gravity(0, 0, -gravityMagnitude);
			// Static translation is solved from the centre-of-mass velocity. Using
			// four point velocities includes body rotation four times; with unequal
			// wheel loads those corrections do not cancel and leave a net downhill
			// impulse. Dynamic slip still uses the true contact-point velocity above.
			const Vector staticSourceVelocity = parkingBrake ? pointVelocity : velocity;
			const Vector tangentVelocity = staticSourceVelocity -
				m_vecWheelNormal[i] * DotProduct(staticSourceVelocity, m_vecWheelNormal[i]);
			const Vector tangentGravity = gravity -
				m_vecWheelNormal[i] * DotProduct(gravity, m_vecWheelNormal[i]);
			const Vector requiredStaticForce =
				-(tangentVelocity / Q_max(dt, 0.001f) + tangentGravity) * wheelMassShare;
			const float requiredLateralForce = DotProduct(requiredStaticForce, wheelRight);
			// Parking is solved once for the whole chassis below. Four independent
			// full-vector point constraints fight each other and shake the suspension.
			const bool lateralOnlyContact = !serviceBrake && !directionShiftNeutral;
			const float longitudinalGripUsage = maxLongitudinalGripForce > 0.001f
				? longitudinalForce / maxLongitudinalGripForce : 0.0f;
			const float remainingLateralGrip = maxLateralGripForce * sqrtf(Q_max(0.0f,
				1.0f - longitudinalGripUsage * longitudinalGripUsage));
			const float staticGripLimit = lateralOnlyContact ? remainingLateralGrip : maxGripForce;
			const float requiredStaticMagnitude = lateralOnlyContact
				? fabs(requiredLateralForce) : requiredStaticForce.Length();
			const float tangentSpeed = lateralOnlyContact
				? fabs(DotProduct(velocity, wheelRight)) : tangentVelocity.Length();
			// Lateral static adhesion exists while accelerating too. Throttle consumes
			// the longitudinal share of the friction circle but must not disable the
			// tyre's ability to resist gravity or a tiny sideways slip.
			const bool wantsStaticContact = lateralOnlyContact
				? !(rearBrake && rearAxle)
				: (serviceBrake || directionShiftNeutral || (m_flThrottle == 0.0f && !rearBrake));
			const bool unattendedParking = m_hDriver == NULL;
			const bool wasStaticGrip = m_bWheelStaticLateralGrip[i] != FALSE;
			const float speedLimit = wasStaticGrip
				? CAR_STATIC_LATERAL_EXIT_SPEED : CAR_STATIC_LATERAL_ENTER_SPEED;
			m_bWheelStaticLateralGrip[i] = wantsStaticContact &&
				staticGripLimit > 0.001f && tangentSpeed <= speedLimit &&
				requiredStaticMagnitude <= staticGripLimit * (wasStaticGrip ? 1.0f : 0.95f);

			// With no driver there is no requested coasting behaviour. Use the full
			// available tyre force to recapture a parked vehicle even if it has already
			// accumulated more speed than the narrow static-entry window. This remains
			// friction-limited, so an excessive slope still wins.
			const bool forceLateralRecapture = lateralOnlyContact && m_flThrottle == 0.0f;
			float staticGripTarget = parkingBrake ? 1.0f : unattendedParking ? 1.0f :
				(m_bWheelStaticLateralGrip[i] ? 1.0f : 0.0f);
			// The aggregate PhysX constraint below owns zero-throttle recapture. Do not
			// also queue four independent wheel corrections for the same velocity.
			if (forceLateralRecapture)
				staticGripTarget = 0.0f;
			if (wantsStaticContact && !m_bWheelStaticLateralGrip[i] &&
				!unattendedParking && !forceLateralRecapture &&
				tangentSpeed < CAR_STATIC_LATERAL_CAPTURE_SPEED && staticGripLimit > 0.001f)
				staticGripTarget = ClampFloat(
					(CAR_STATIC_LATERAL_CAPTURE_SPEED - tangentSpeed) /
					(CAR_STATIC_LATERAL_CAPTURE_SPEED - CAR_STATIC_LATERAL_ENTER_SPEED), 0.0f, 1.0f);
			if (m_bWheelStaticLateralGrip[i] && !wasStaticGrip)
				m_flWheelStaticGripBlend[i] = 1.0f;
			else m_flWheelStaticGripBlend[i] = CarApproach(staticGripTarget,
				m_flWheelStaticGripBlend[i], CAR_STATIC_LATERAL_BLEND_RATE * dt);

			Vector limitedStaticForce;
			if (lateralOnlyContact)
			{
				const float heldLateralForce = ClampFloat(requiredLateralForce,
					-staticGripLimit, staticGripLimit);
				limitedStaticForce = wheelForward * longitudinalForce +
					wheelRight * heldLateralForce;
			}
			else
			{
				limitedStaticForce = requiredStaticForce;
				if (requiredStaticMagnitude > staticGripLimit && requiredStaticMagnitude > 0.001f)
					limitedStaticForce *= staticGripLimit / requiredStaticMagnitude;
			}
			tyreForce += (limitedStaticForce - tyreForce) * m_flWheelStaticGripBlend[i];
			m_flWheelRequiredStaticForce[i] = requiredLateralForce;

			longitudinalForce = DotProduct(tyreForce, wheelForward);
			lateralForce = DotProduct(tyreForce, wheelRight);
			float combinedGripUsage = 0.0f;
			if (maxLongitudinalGripForce > 0.001f)
				combinedGripUsage += longitudinalForce * longitudinalForce /
					(maxLongitudinalGripForce * maxLongitudinalGripForce);
			else if (fabs(longitudinalForce) > 0.001f)
				combinedGripUsage = 1.0e6f;
			if (maxLateralGripForce > 0.001f)
				combinedGripUsage += lateralForce * lateralForce /
					(maxLateralGripForce * maxLateralGripForce);
			else if (fabs(lateralForce) > 0.001f)
				combinedGripUsage = 1.0e6f;
			combinedGripUsage = sqrtf(combinedGripUsage);
			if (combinedGripUsage > 1.0f)
			{
				tyreForce *= 1.0f / combinedGripUsage;
				longitudinalForce = DotProduct(tyreForce, wheelForward);
				lateralForce = DotProduct(tyreForce, wheelRight);
				combinedGripUsage = 1.0f;
			}
			m_flWheelLongitudinalForce[i] = longitudinalForce;
			m_flWheelLateralForce[i] = lateralForce;
			m_flWheelGripUtilization[i] = combinedGripUsage;

			if (lateralOnlyContact)
			{
				// The static correction represents translation of the whole chassis.
				// Applying four unequal corrections at the suspension points creates an
				// artificial yaw moment and can spin a parked car by 180 degrees. Keep
				// normal tyre/slip forces at the wheel, but apply only the constraint
				// correction through the centre of mass where it cannot steer the car.
				if (dynamicTyreForce.Length() > 0.001f)
					WorldPhysic->AddImpulse(this, dynamicTyreForce, m_vecWheelWorld[i], dt);
				const Vector staticCorrection = tyreForce - dynamicTyreForce;
				if (staticCorrection.Length() > 0.001f)
					WorldPhysic->AddImpulse(this, staticCorrection, GetAbsOrigin(), dt);
			}
			else if (tyreForce.Length() > 0.001f)
				WorldPhysic->AddImpulse(this, tyreForce, m_vecWheelWorld[i], dt);

			// Equal and opposite contact torque changes the wheel itself. Clamp only
			// the global extreme; normal slip is allowed to cross zero naturally.
			m_flWheelAngularVelocity[i] -= longitudinalForce * radius / wheelInertia * dt;
			if (brakeTorque > 0.0f)
				m_flWheelAngularVelocity[i] = CarApproach(0.0f,
					m_flWheelAngularVelocity[i], brakeTorque / wheelInertia * dt);
			m_flWheelAngularVelocity[i] = ClampFloat(m_flWheelAngularVelocity[i],
				-maxSurfaceSpeed / radius, maxSurfaceSpeed / radius);
			m_bWheelLocked[i] = brakeTorque > 0.0f &&
				fabs(m_flWheelAngularVelocity[i]) <= 0.01f;
		}

		if (m_flThrottle == 0.0f && groundedWheelCount >= 2 &&
			totalAvailableGripForce > 0.001f)
		{
			Vector supportNormal = aggregateGroundNormal;
			if (supportNormal.Length() > 0.001f) supportNormal = supportNormal.Normalize();
			else supportNormal = Vector(0, 0, 1);
			const float supportSlopeDegrees = acosf(ClampFloat(supportNormal.z, -1.0f, 1.0f)) *
				(180.0f / M_PI);
			const bool slopeAllowsStaticRest = rearBrake || allWheelBrake ||
				supportSlopeDegrees <= Q_max(0.0f, m_flStationaryHoldMaxSlope);
			Vector lateralDirection = right - supportNormal * DotProduct(right, supportNormal);
			Vector longitudinalDirection = forward - supportNormal * DotProduct(forward, supportNormal);
			if (lateralDirection.Length() > 0.001f && longitudinalDirection.Length() > 0.001f)
			{
				lateralDirection = lateralDirection.Normalize();
				// Re-orthogonalize the road-plane axes. Pitch and roll can otherwise
				// leave a small overlap between them and make the two corrections fight.
				longitudinalDirection -= lateralDirection *
					DotProduct(longitudinalDirection, lateralDirection);
				longitudinalDirection = longitudinalDirection.Normalize();
				const Vector currentVelocity = GetAbsVelocity();
				const Vector gravity(0, 0, -gravityMagnitude);
				const float lateralVelocity = DotProduct(currentVelocity, lateralDirection);
				const float gravityLateral = DotProduct(gravity, lateralDirection);
				const float longitudinalVelocity = DotProduct(currentVelocity, longitudinalDirection);
				const float gravityLongitudinal = DotProduct(gravity, longitudinalDirection);
				const float tangentSpeed = sqrtf(lateralVelocity * lateralVelocity +
					longitudinalVelocity * longitudinalVelocity);
				// Persistent hysteresis is essential here. The previous one-frame test
				// alternated around its threshold, applying a full correction and then
				// none, which made a stopped chassis hunt uphill/downhill.
				const bool lowSpeedRearBrake = rearBrake &&
					tangentSpeed <= CAR_REAR_BRAKE_CAPTURE_SPEED;
				const bool lowSpeedAllWheelBrake = allWheelBrake &&
					tangentSpeed <= CAR_REAR_BRAKE_CAPTURE_SPEED;
				const float longitudinalGripForce = lowSpeedAllWheelBrake
					? totalAvailableGripForce
					: lowSpeedRearBrake
					? rearAvailableGripForce
					: totalAvailableGripForce;
				// A rear parking brake owns longitudinal holding, but the freely rolling
				// front tyres still provide lateral static adhesion. Limiting the complete
				// vector to rear grip made a braked car slide sideways more than a free one.
				const float lateralGripForce = totalAvailableLateralGripForce;
				const float requiredLateralHold = fabs(m_flBodyMass * gravityLateral);
				const float requiredLongitudinalHold = fabs(m_flBodyMass * gravityLongitudinal);
				const float holdUtilization = sqrtf(
					powf(requiredLateralHold / Q_max(lateralGripForce, 0.001f), 2.0f) +
					powf(requiredLongitudinalHold / Q_max(longitudinalGripForce, 0.001f), 2.0f));
				if (!slopeAllowsStaticRest)
					m_bStaticRestConstraint = FALSE;
				else if (m_bStaticRestConstraint)
				{
					if (tangentSpeed > CAR_STATIC_REST_EXIT_SPEED ||
						holdUtilization > 1.0f)
						m_bStaticRestConstraint = FALSE;
				}
				else if (tangentSpeed <= CAR_STATIC_REST_ENTER_SPEED &&
					holdUtilization <= CAR_STATIC_REST_ENTER_GRIP_FRACTION)
				{
					m_bStaticRestConstraint = TRUE;
				}

				// Wheel forces have already been queued for this simulation step. Ask
				// the aggregate constraint only for the remaining velocity correction;
				// otherwise both systems brake the same drift and overshoot through zero.
				const float dynamicLateralChange = DotProduct(aggregateDynamicTyreForce,
					lateralDirection) / Q_max(m_flBodyMass, 1.0f) * dt;
				const float dynamicLongitudinalChange = DotProduct(aggregateDynamicTyreForce,
					longitudinalDirection) / Q_max(m_flBodyMass, 1.0f) * dt;
				const float desiredVelocityChange =
					-(lateralVelocity + gravityLateral * dt) - dynamicLateralChange;
				// X is a four-wheel parking brake: even if a small drift has already
				// started, recapture both road-plane axes with a friction-limited force.
				// SPACE gets the same static behaviour only near rest and is limited to
				// rear-axle grip; at road speed its existing rear slip model remains.
				const bool constrainLongitudinal = m_bStaticRestConstraint ||
					lowSpeedRearBrake || lowSpeedAllWheelBrake;
				const float desiredLongitudinalChange = constrainLongitudinal
					? -(longitudinalVelocity + gravityLongitudinal * dt) - dynamicLongitudinalChange
					: 0.0f;
				const float maxLateralVelocityChange = lateralGripForce /
					Q_max(m_flBodyMass, 1.0f) * dt;
				const float maxLongitudinalVelocityChange = longitudinalGripForce /
					Q_max(m_flBodyMass, 1.0f) * dt;
				float limitedLateralChange = desiredVelocityChange;
				float limitedLongitudinalChange = desiredLongitudinalChange;
				const float correctionUtilization = sqrtf(
					powf(limitedLateralChange / Q_max(maxLateralVelocityChange, 0.0001f), 2.0f) +
					powf(limitedLongitudinalChange / Q_max(maxLongitudinalVelocityChange, 0.0001f), 2.0f));
				if (correctionUtilization > 1.0f)
				{
					limitedLateralChange /= correctionUtilization;
					limitedLongitudinalChange /= correctionUtilization;
				}
				const Vector aggregateVelocityChange = lateralDirection * limitedLateralChange +
					longitudinalDirection * limitedLongitudinalChange;
				if (aggregateVelocityChange.Length() > 0.0001f)
					WorldPhysic->AddForce(this, aggregateVelocityChange,
						IPhysicLayer::ForceMode::VelocityChange);
				// Suppress only residual rocking after static rest is established.
				// Larger angular motion from an impact remains fully physical.
				if (m_bStaticRestConstraint && bodyAngularVelocity.Length() < 1.0f)
					WorldPhysic->SetAvelocity(this, g_vecZero);
			}
		}
		else m_bStaticRestConstraint = FALSE;

		// Keep the existing coasting drag as a body resistance. Propulsion and
		// braking, unlike this aerodynamic/rolling loss, now come only from tyres.
		if (m_flThrottle == 0.0f && !handbrake && !m_bStaticRestConstraint &&
			fabs(m_flSpeed) > CAR_STOP_EPSILON)
			WorldPhysic->AddImpulse(this, forward * (m_flSpeed > 0.0f ? -1.0f : 1.0f),
				GetAbsOrigin(), m_flBodyMass * m_flDrag * dt);
		return;
	}

	const Vector oldOrigin = GetAbsOrigin();
	const Vector oldAngles = GetAbsAngles();
	Vector angles = oldAngles;
	const float speedFraction = ClampFloat(fabs(m_flSpeed) / Q_max(m_flMaxSpeed, 1.0f), 0, 1);
	const float steerEffect = speedFraction * (m_flSpeed >= 0 ? 1.0f : -1.0f);
	angles.y += m_flSteering * steerEffect * dt * 0.9f;

	bool pitchSupported = false;
	bool rollSupported = false;
	float targetPitch = 0;
	float targetRoll = 0;
	if (m_iGroundedWheels >= 2)
	{
		const bool grounded[WHEEL_COUNT] = {
			m_bWheelGrounded[WHEEL_FL] != FALSE, m_bWheelGrounded[WHEEL_FR] != FALSE,
			m_bWheelGrounded[WHEEL_RL] != FALSE, m_bWheelGrounded[WHEEL_RR] != FALSE
		};
		float frontZ = 0, rearZ = 0, leftZ = 0, rightZ = 0;
		int frontCount = 0, rearCount = 0, leftCount = 0, rightCount = 0;
		for (int i = 0; i < WHEEL_COUNT; ++i)
		{
			if (!grounded[i]) continue;
			if (i == WHEEL_FL || i == WHEEL_FR) { frontZ += m_vecWheelContact[i].z; ++frontCount; }
			else { rearZ += m_vecWheelContact[i].z; ++rearCount; }
			if (i == WHEEL_FL || i == WHEEL_RL) { leftZ += m_vecWheelContact[i].z; ++leftCount; }
			else { rightZ += m_vecWheelContact[i].z; ++rightCount; }
		}

		// Three contacts define a stable support plane. Two contacts may only
		// update the axis they actually span; a diagonal pair updates neither.
		if (frontCount > 0 && rearCount > 0 &&
			(m_iGroundedWheels >= 3 || (frontCount == 1 && rearCount == 1 &&
			 grounded[WHEEL_FL] == grounded[WHEEL_RL])))
		{
			frontZ /= frontCount;
			rearZ /= rearCount;
			const float length = Q_max(1.0f, fabs(m_vecWheelPos[WHEEL_FL].x - m_vecWheelPos[WHEEL_RL].x));
			targetPitch = ClampFloat(atan2f(rearZ - frontZ, length) * 57.29578f, -30.0f, 30.0f);
			pitchSupported = true;
		}
		if (leftCount > 0 && rightCount > 0 &&
			(m_iGroundedWheels >= 3 || (leftCount == 1 && rightCount == 1 &&
			 grounded[WHEEL_FL] == grounded[WHEEL_FR])))
		{
			leftZ /= leftCount;
			rightZ /= rightCount;
			const float width = Q_max(1.0f, fabs(m_vecWheelPos[WHEEL_FL].y - m_vecWheelPos[WHEEL_FR].y));
			targetRoll = ClampFloat(atan2f(leftZ - rightZ, width) * 57.29578f, -30.0f, 30.0f);
			rollSupported = true;
		}
	}
	// This first version has no angular rigid-body simulation. When an axis is
	// not supported by usable wheel contacts, recover it toward neutral instead
	// of preserving an arbitrary launch angle forever.
	angles.x = CarApproach(pitchSupported ? targetPitch : 0.0f, angles.x, 60.0f * dt);
	angles.z = CarApproach(rollSupported ? targetRoll : 0.0f, angles.z, 60.0f * dt);

	matrix4x4 desiredTransform(oldOrigin, angles, 1.0f);
	Vector forward = desiredTransform.GetForward();
	forward.z = 0;
	if (forward.Length() > 0.001f) forward = forward.Normalize();
	const Vector desiredOrigin = oldOrigin + forward * (m_flSpeed * dt) + Vector(0, 0, m_flVerticalVelocity * dt);

	float fraction = 1.0f;
	Vector planeNormal = g_vecZero;
	bool startSolid = false;
	const bool hit = SweepBody(oldOrigin, oldAngles, desiredOrigin, angles, fraction, planeNormal, startSolid);
	if (startSolid && m_bHasLastSafeTransform)
	{
		SetAbsOrigin(m_vecLastSafeOrigin);
		SetAbsAngles(m_vecLastSafeAngles);
		m_flSpeed *= 0.15f;
		m_flVerticalVelocity = 0;
	}
	else
	{
		const float safeFraction = hit ? Q_max(0.0f, fraction - 0.002f) : 1.0f;
		Vector finalAngles;
		for (int axis = 0; axis < 3; ++axis)
			finalAngles[axis] = oldAngles[axis] + UTIL_AngleDiff(angles[axis], oldAngles[axis]) * safeFraction;
		SetAbsOrigin(oldOrigin + (desiredOrigin - oldOrigin) * safeFraction);
		SetAbsAngles(finalAngles);
		if (hit)
		{
			// Consume the remaining translation along the contact plane. This
			// lets the chassis climb BSP slopes and slide along walls without
			// weakening the wall barrier itself.
			const Vector firstOrigin = GetAbsOrigin();
			const Vector remaining = desiredOrigin - firstOrigin;
			const Vector slide = remaining - planeNormal * DotProduct(remaining, planeNormal);
			if (slide.Length() > 0.001f)
			{
				float slideFraction;
				Vector slideNormal;
				bool slideStartSolid;
				const Vector slideEnd = firstOrigin + slide;
				const bool slideHit = SweepBody(firstOrigin, finalAngles, slideEnd, finalAngles,
					slideFraction, slideNormal, slideStartSolid);
				if (!slideStartSolid)
					SetAbsOrigin(firstOrigin + slide * (slideHit ? Q_max(0.0f, slideFraction - 0.002f) : 1.0f));
			}
			if (fabs(planeNormal.z) < 0.5f) m_flSpeed *= 0.15f;
			if (planeNormal.z > 0.5f && m_flVerticalVelocity < 0) m_flVerticalVelocity = 0;
		}
		if (BodyPositionClear(GetAbsOrigin(), GetAbsAngles()))
		{
			m_vecLastSafeOrigin = GetAbsOrigin();
			m_vecLastSafeAngles = GetAbsAngles();
			m_bHasLastSafeTransform = TRUE;
		}
	}
	RelinkEntity(TRUE);
}

bool CFuncCar::SweepBody(const Vector &oldOrigin, const Vector &oldAngles,
	const Vector &newOrigin, const Vector &newAngles, float &fraction,
	Vector &planeNormal, bool &startSolid) const
{
	fraction = 1.0f;
	planeNormal = g_vecZero;
	startSolid = false;
	matrix4x4 oldTransform(oldOrigin, oldAngles, 1.0f);
	matrix4x4 newTransform(newOrigin, newAngles, 1.0f);
	// Keep samples just inside the mathematical bbox. Exact boundary points
	// resting on BSP can otherwise be reported as startsolid and hard-lock.
	const Vector inset(1.5f, 1.5f, 1.5f);
	const Vector sampleMins = pev->mins + inset;
	const Vector sampleMaxs = pev->maxs - inset;
	const Vector middle = (sampleMins + sampleMaxs) * 0.5f;
	const float xs[3] = { sampleMins.x, middle.x, sampleMaxs.x };
	const float ys[3] = { sampleMins.y, middle.y, sampleMaxs.y };
	const float zs[3] = { sampleMins.z, middle.z, sampleMaxs.z };

	for (int x = 0; x < 3; ++x)
	for (int y = 0; y < 3; ++y)
	for (int z = 0; z < 3; ++z)
	{
		// The 3x3x3 boundary grid gives corners, edge centers and face centers.
		if (x == 1 && y == 1 && z == 1) continue;
		const Vector local(xs[x], ys[y], zs[z]);
		TraceResult trace;
		UTIL_TraceLine(oldTransform.VectorTransform(local), newTransform.VectorTransform(local),
			ignore_monsters, ENT(pev), &trace);
		if (trace.fStartSolid || trace.fAllSolid) startSolid = true;
		if (trace.flFraction < fraction)
		{
			fraction = trace.flFraction;
			planeNormal = trace.vecPlaneNormal;
		}
	}
	return fraction < 1.0f;
}

bool CFuncCar::BodyPositionClear(const Vector &origin, const Vector &angles) const
{
	float fraction;
	Vector normal;
	bool startSolid;
	SweepBody(origin, angles, origin, angles, fraction, normal, startSolid);
	return !startSolid;
}

void CFuncCar::UpdateVisuals(float dt)
{
	if (m_hBodyVisual != NULL)
		m_hBodyVisual->pev->animtime = gpGlobals->time;
	const bool chassisInverted = EntityToWorldTransform().GetUp().z < 0.0f;
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		if (m_iActorType != ACTOR_DYNAMIC)
			m_flWheelAngularVelocity[i] = m_flSpeed / Q_max(fabs(m_flWheelRadius), 1.0f);
		m_flWheelRotation[i] += m_flWheelAngularVelocity[i] * dt * 57.29578f;
		if (m_flWheelRotation[i] > 360.0f || m_flWheelRotation[i] < -360.0f)
			m_flWheelRotation[i] = fmodf(m_flWheelRotation[i], 360.0f);
		CBaseEntity *wheel = m_hWheels[i];
		if (!wheel) continue;
		wheel->pev->iuser2 = m_hBodyVisual != NULL ? m_hBodyVisual->entindex() : 0;
		wheel->pev->animtime = gpGlobals->time;
		// Re-query after the body sweep. The suspension force was already
		// calculated earlier; this trace only prevents one-frame-old visual
		// contacts from placing a wheel below BSP after collision/rotation.
		const Vector rayStart = LocalToWorld(m_vecWheelPos[i]);
		const Vector down(0, 0, -1);
		const float traceDistance = m_flSuspensionLength + m_flWheelRadius;
		// An overturned chassis rests on its roof. Keep its visual suspension fully
		// compressed so the wheels stay as close to the ground as their mounts allow,
		// instead of extending chassis-local down (world-up) into the sky.
		Vector localCenter = chassisInverted ? m_vecWheelPos[i] :
			m_vecWheelPos[i] + Vector(0, 0, -m_flSuspensionLength);
		if (!chassisInverted)
		{
			TraceResult visualTrace;
			UTIL_TraceLine(rayStart, rayStart + down * traceDistance,
				ignore_monsters, edict(), &visualTrace);
			Vector worldCenter = rayStart + down * m_flSuspensionLength;
			if (visualTrace.flFraction < 1.0f && !visualTrace.fStartSolid)
			{
				// The wheel centre may travel only from the suspension mount to full
				// droop. contact + normal * radius can move it above the mount when the
				// chassis is very close to BSP, making the wheel emerge through the hood.
				const float suspension = ClampFloat(
					traceDistance * visualTrace.flFraction - m_flWheelRadius,
					0.0f, m_flSuspensionLength);
				worldCenter = rayStart + down * suspension;
			}
			localCenter = EntityToWorldTransform().VectorITransform(worldCenter);
			// A world-down ray must never retract a visual wheel through its local
			// suspension mount when the body is heavily rolled.
			localCenter.z = Q_min(localCenter.z, m_vecWheelPos[i].z);
		}
		// Once upside down, world-down points through the roof. The compressed local
		// position above avoids both that intrusion and the full-droop skyward stretch.
		wheel->SetLocalOrigin(localCenter);
		// hummer_wheel.mdl is authored as a front-left wheel. Right wheels use
		// a true model-space X reflection supplied through startpos; see the
		// narrowly-scoped renderer handling for FUNC_CAR_WHEEL_MARKER.
		const bool rightSide = (i == WHEEL_FR || i == WHEEL_RR);
		const bool frontAxle = (i == WHEEL_FL || i == WHEEL_FR);
		const float baseYaw = rightSide ? 180.0f : 0.0f;
		const float visualRotation = rightSide ? -m_flWheelRotation[i] : m_flWheelRotation[i];
		const float visualSpeedFraction = ClampFloat(GetCarPlanarSpeed() /
			Q_max(m_flMaxSpeed, 1.0f), 0.0f, 1.0f);
		const float visualSteering = m_flSteering * (1.0f - visualSpeedFraction * (1.0f - m_flHighSpeedSteerScale));
		const Vector localWheelAngles(visualRotation, baseYaw + (frontAxle ? visualSteering : 0), 0);
		wheel->SetLocalAngles(localWheelAngles);
	}
	if (m_hViewEntity != NULL)
	{
		// The external view entity is networked independently. animtime makes
		// AddToFullPack mark it for the same interpolation as body and wheels.
		m_hViewEntity->pev->animtime = gpGlobals->time;
		m_hViewEntity->SetLocalOrigin(m_vecViewPos);
		m_hViewEntity->pev->iuser2 = m_hBodyVisual != NULL ? m_hBodyVisual->entindex() : 0;
		m_hViewEntity->pev->startpos = m_vecViewPos;
		Vector viewAngles = g_vecZero;
		if (m_hDriver != NULL)
		{
			const Vector inputAngles = m_hDriver->pev->v_angle;
			m_flDriverViewYaw = ClampFloat(m_flDriverViewYaw +
				UTIL_AngleDiff(inputAngles.y, m_vecLastDriverInputAngles.y), -90.0f, 90.0f);
			m_flDriverViewPitch = ClampFloat(m_flDriverViewPitch +
				UTIL_AngleDiff(inputAngles.x, m_vecLastDriverInputAngles.x), -60.0f, 60.0f);
			m_vecLastDriverInputAngles = inputAngles;
			viewAngles.y = m_flDriverViewYaw;
			viewAngles.x = m_flDriverViewPitch;
		}
		m_hViewEntity->SetLocalAngles(viewAngles);
		m_hViewEntity->pev->fuser2 = viewAngles.x;
		m_hViewEntity->pev->fuser3 = viewAngles.y;
	}
	if (m_hDriver != NULL)
	{
		m_hDriver->SetLocalOrigin(m_vecDriverPos);
		m_hDriver->SetLocalAngles(g_vecZero);
	}
}

void CFuncCar::DebugDraw()
{
	const int level = static_cast<int>(CVAR_GET_FLOAT("car_debug"));
	if (level <= 0) return;
	auto line = [](const Vector &start, const Vector &end, int r, int g, int b)
	{
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_COORD(start.x); WRITE_COORD(start.y); WRITE_COORD(start.z);
		WRITE_COORD(end.x); WRITE_COORD(end.y); WRITE_COORD(end.z);
		WRITE_SHORT(g_sModelIndexLaser); WRITE_BYTE(0); WRITE_BYTE(0); WRITE_BYTE(2); WRITE_BYTE(2); WRITE_BYTE(0);
		WRITE_BYTE(r); WRITE_BYTE(g); WRITE_BYTE(b); WRITE_BYTE(220); WRITE_BYTE(0);
		MESSAGE_END();
	};
	const Vector down(0, 0, -1);
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		const float surfaceSpeed = m_flWheelAngularVelocity[i] * m_flWheelRadius;
		const float slipRatio = fabs(m_flWheelLongitudinalSlip[i]) /
			Q_max(CAR_SLIP_REFERENCE_SPEED, Q_max(fabs(surfaceSpeed), fabs(m_flWheelGroundSpeed[i])));
		int r = 0, g = 255, b = 0;
		if (slipRatio > Q_max(0.001f, m_flSlipPeak)) { r = 255; g = 32; }
		else if (m_flWheelGripUtilization[i] > 0.8f) { r = 255; g = 220; }
		line(m_vecWheelWorld[i], m_vecWheelWorld[i] + down * (m_flSuspensionLength + m_flWheelRadius), r, g, b);
		line(m_vecWheelWorld[i] - Vector(2,0,0), m_vecWheelWorld[i] + Vector(2,0,0), 255, 255, 0);
		if (m_flCompression[i] > 0) line(m_vecWheelContact[i], m_vecWheelContact[i] + m_vecWheelNormal[i] * 8, 0, 255, 0);
	}
	line(LocalToWorld(m_vecDriverPos), LocalToWorld(m_vecDriverPos) + Vector(0,0,8), 0, 128, 255);
	line(LocalToWorld(m_vecViewPos), LocalToWorld(m_vecViewPos) + Vector(0,0,8), 255, 0, 255);
	line(LocalToWorld(m_vecExitPos), LocalToWorld(m_vecExitPos) + Vector(0,0,8), 0, 255, 255);
	if (level >= 2)
	{
		Vector lightForward = EntityToWorldTransform().GetForward();
		if (lightForward.Length() > 0.001f) lightForward = lightForward.Normalize();
		for (int i = 0; i < 2; ++i)
		{
			const Vector lightPosition = LocalToWorld(m_vecHeadlightPos[i]);
			line(lightPosition - Vector(3, 0, 0), lightPosition + Vector(3, 0, 0), 255, 180, 0);
			line(lightPosition, lightPosition + lightForward * 48.0f, 255, 240, 96);
		}
	}
	if (level >= 2 && gpGlobals->time >= m_flNextDebugText)
	{
		m_flNextDebugText = gpGlobals->time + 0.25f;
		ALERT(at_console, "car longitudinalSpeed %.1f planarSpeed %.1f throttle %.0f steer %.1f grounded %d compression [%.1f %.1f %.1f %.1f]\n",
			m_flSpeed, GetCarPlanarSpeed(), m_flThrottle, m_flSteering, m_iGroundedWheels,
			m_flCompression[0], m_flCompression[1], m_flCompression[2], m_flCompression[3]);
		const float debugGearRatio = m_iCurrentGear < 0 ? -fabs(m_flReverseRatio) :
			(m_iCurrentGear > 0 && m_iCurrentGear <= m_iForwardGearCount
				? m_flGearRatios[m_iCurrentGear - 1] : 0.0f);
		ALERT(at_console,
			"  drivetrain engineRPM %.0f engineTorque %.1fNm gear %d gearRatio %.3f drivelineRPM %.1f perWheelDriveTorque %.0f\n"
			"  converter slipRPM %.0f ratio %.2f transmittedTorque %.1fNm\n",
			m_flEngineRPM, m_flEngineTorque, m_iCurrentGear, debugGearRatio,
			m_flDrivelineRPM, m_flPerWheelDriveTorque,
			m_flConverterSlipRPM, m_flConverterRatio, m_flTransmittedTorque);
		ALERT(at_console, "  physicsSleep %s idleFor %.1f / %.1f sec\n",
			m_bCarPhysicsSleeping ? "active" : "inactive",
			m_flSleepCandidateSince > 0.0f ? gpGlobals->time - m_flSleepCandidateSince : 0.0f,
			CAR_SLEEP_DELAY);
		const Vector headlightLeft = LocalToWorld(m_vecHeadlightPos[0]);
		const Vector headlightRight = LocalToWorld(m_vecHeadlightPos[1]);
		ALERT(at_console, "  headlights L local [%.1f %.1f %.1f] world [%.1f %.1f %.1f] R local [%.1f %.1f %.1f] world [%.1f %.1f %.1f]\n",
			m_vecHeadlightPos[0].x, m_vecHeadlightPos[0].y, m_vecHeadlightPos[0].z,
			headlightLeft.x, headlightLeft.y, headlightLeft.z,
			m_vecHeadlightPos[1].x, m_vecHeadlightPos[1].y, m_vecHeadlightPos[1].z,
			headlightRight.x, headlightRight.y, headlightRight.z);
		static const char *wheelNames[WHEEL_COUNT] = { "FL", "FR", "RL", "RR" };
		for (int i = 0; i < WHEEL_COUNT; ++i)
		{
			const char *materialName = m_pWheelContactMaterial[i]
				? m_pWheelContactMaterial[i]->name : "unknown";
			ALERT(at_console,
				"  %s ground %d material %s materialLongGrip %.2f materialLatGrip %.2f materialRollingResistance %.2f effectiveRollingResistance %.2f load %.0f omega %.2f surfaceSpeed %.1f groundSpeed %.1f longSlip %.1f brakeTorque %.0f locked %s lateralSpeed %.1f staticGrip %s requiredStaticForce %.0f effectiveLateralGripForce %.0f longF %.0f actualLateralForce %.0f grip %.0f%%\n",
				wheelNames[i], m_bWheelGrounded[i] != FALSE, materialName,
				m_flWheelMaterialLongitudinalGrip[i],
				m_flWheelMaterialLateralGrip[i], m_flWheelMaterialRollingResistance[i],
				m_flRollingResistance * (m_bWheelGrounded[i]
					? m_flWheelMaterialRollingResistance[i] : 1.0f),
				m_flWheelLoad[i],
				m_flWheelAngularVelocity[i], m_flWheelAngularVelocity[i] * m_flWheelRadius,
				m_flWheelGroundSpeed[i], m_flWheelLongitudinalSlip[i],
				m_flWheelBrakeTorque[i], m_bWheelLocked[i] ? "yes" : "no",
				m_flWheelLateralSlip[i],
				m_bWheelStaticLateralGrip[i] ? "active" : "inactive",
				m_flWheelRequiredStaticForce[i], m_flWheelMaxGripForce[i],
				m_flWheelLongitudinalForce[i], m_flWheelLateralForce[i],
				m_flWheelGripUtilization[i] * 100.0f);
		}
	}
}

void CFuncCar::CarThink()
{
	// SV_Physics_Rigid normally synchronizes a PhysX actor only after running the
	// entity think. A static tyre constraint must use the pose and velocity from
	// the latest completed simulation step, not the previous server frame.
	if (m_iActorType == ACTOR_DYNAMIC && WorldPhysic)
		WorldPhysic->UpdateEntityTransform(this);
	// Force/impulse integration is designed for the 50 Hz car think. A synchronous
	// screenshot can stall the host and used to turn the next suspension impulse
	// into a 2.5x kick. Timers still use gpGlobals->time; only physical integration
	// is capped to one regular vehicle step.
	const float dt = ClampFloat(gpGlobals->time - m_flLastThink, 0.001f, CAR_THINK_INTERVAL);
	const Vector velocityBefore = GetAbsVelocity();
	const int groundedBefore = m_iGroundedWheels;
	m_flLastThink = gpGlobals->time;
	EnsureChildren();
	// An automatic RaceLap entry can happen at the tail of CBasePlayer::Spawn.
	// The engine restores the player's regular view after Spawn returns, so make
	// the vehicle camera authoritative from the first subsequent car frame on.
	if (m_hDriver != NULL && m_hDriver->IsPlayer() && m_hViewEntity != NULL)
		SET_VIEW(m_hDriver->edict(), m_hViewEntity->edict());
	if (m_hUseBlockedPlayer != NULL &&
		(!m_hUseBlockedPlayer->IsPlayer() || !FBitSet(m_hUseBlockedPlayer->pev->button, IN_USE)))
		m_hUseBlockedPlayer = NULL;
	UpdateUseAction();
	if (m_hDriver != NULL && (!m_hDriver->IsPlayer() || !m_hDriver->IsAlive()))
	{
		CBasePlayer *player = m_hDriver->IsPlayer() ? static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver)) : NULL;
		if (player) ExitDriver(player, true); else m_hDriver = NULL;
	}
	UpdateEngine(dt);
	UpdateHorn();
	// Entering or using a parked vehicle does not wake its rigid body. Keep the
	// low-rate interaction/sound think until the driver actually requests motion.
	if (m_bCarPhysicsSleeping)
	{
		if (DriverRequestsMovement())
			WakeCarPhysics();
		else
		{
			// Keep the sleeping actor and its network entity on the exact stored
			// suspension-supported pose. This is cheap (10 Hz) and performs no rays.
			if (m_bHasLastSafeTransform && WorldPhysic)
			{
				SetAbsOrigin(m_vecLastSafeOrigin);
				SetAbsAngles(m_vecLastSafeAngles);
				WorldPhysic->SetOrigin(this, m_vecLastSafeOrigin);
				WorldPhysic->SetAngles(this, m_vecLastSafeAngles);
				WorldPhysic->SetVelocity(this, g_vecZero);
				WorldPhysic->SetAvelocity(this, g_vecZero);
				WorldPhysic->SetBodySleeping(this, true);
			}
			if (m_hDriver != NULL && gpGlobals->time >= m_flNextVehicleHud)
				SendVehicleHud(true);
			DebugDraw();
			SetNextThink(CAR_SLEEP_THINK_INTERVAL);
			return;
		}
	}
	UpdateInput(dt);
	UpdateWheels(dt);
	UpdateMotion(dt);
	UpdateImpactAndLanding(velocityBefore, groundedBefore);
	UpdateVisuals(dt);
	UpdateDriverVisual(dt);
	UpdateSleepState();
	if (m_hDriver != NULL && gpGlobals->time >= m_flNextVehicleHud)
		SendVehicleHud(true);
	m_vecPreviousVelocity = GetAbsVelocity();
	DebugDraw();
	SetNextThink(m_bCarPhysicsSleeping ? CAR_SLEEP_THINK_INTERVAL : CAR_THINK_INTERVAL);
}
