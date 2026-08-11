#pragma once

#include "cbase.h"

class CBasePlayer;

class CFuncCar : public CBaseAnimating
{
	DECLARE_CLASS(CFuncCar, CBaseAnimating);

public:
	void Spawn() override;
	void Precache() override;
	void Activate() override;
	void KeyValue(KeyValueData *pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	void OnRemove() override;
	int ObjectCaps() override { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE | FCAP_ONLYDIRECT_USE; }
	CBaseEntity *GetVehicleDriver() override { return m_hDriver; }
	CBaseEntity *GetVehicleViewEntity() override;

	DECLARE_DATADESC();
	enum { WHEEL_FL, WHEEL_FR, WHEEL_RL, WHEEL_RR, WHEEL_COUNT };

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

	string_t m_iszWheelModel;
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
	float m_flSteerAngle;
	float m_flSteerSpeed;
	float m_flSuspensionLength;
	float m_flSpringStrength;
	float m_flSuspensionDamping;

	EHANDLE m_hDriver;
	EHANDLE m_hWheels[WHEEL_COUNT];
	EHANDLE m_hViewEntity;
	Vector m_vecWheelWorld[WHEEL_COUNT];
	Vector m_vecWheelContact[WHEEL_COUNT];
	Vector m_vecWheelNormal[WHEEL_COUNT];
	BOOL m_bWheelGrounded[WHEEL_COUNT];
	float m_flCompression[WHEEL_COUNT];
	float m_flPreviousCompression[WHEEL_COUNT];
	float m_flSpeed;
	float m_flThrottle;
	float m_flSteering;
	float m_flWheelRotation;
	float m_flVerticalVelocity;
	float m_flLastThink;
	float m_flNextDebugText;
	int m_iGroundedWheels;
	Vector m_vecLastSafeOrigin;
	Vector m_vecLastSafeAngles;
	BOOL m_bHasLastSafeTransform;
};
