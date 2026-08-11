#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "studio.h"
#include "func_car_shared.h"
#include "func_car.h"

extern short g_sModelIndexLaser;

namespace
{
constexpr float CAR_THINK_INTERVAL = 0.02f;
constexpr float CAR_GRAVITY = 600.0f;
constexpr float CAR_STOP_EPSILON = 2.0f;
constexpr float CAR_BODY_MASS = 1800.0f;
constexpr float CAR_LATERAL_GRIP = 4.0f;

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
	int ObjectCaps() override { return FCAP_DONT_SAVE; }
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

LINK_ENTITY_TO_CLASS(func_car, CFuncCar);

BEGIN_DATADESC(CFuncCar)
	DEFINE_KEYFIELD(m_iszWheelModel, FIELD_STRING, "wheelmodel"),
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
	DEFINE_KEYFIELD(m_flSteerAngle, FIELD_FLOAT, "steerangle"),
	DEFINE_KEYFIELD(m_flSteerSpeed, FIELD_FLOAT, "steerspeed"),
	DEFINE_KEYFIELD(m_flSuspensionLength, FIELD_FLOAT, "suspension_length"),
	DEFINE_KEYFIELD(m_flSpringStrength, FIELD_FLOAT, "spring_strength"),
	DEFINE_KEYFIELD(m_flSuspensionDamping, FIELD_FLOAT, "suspension_damping"),
	DEFINE_FIELD(m_hDriver, FIELD_EHANDLE),
	DEFINE_ARRAY(m_vecWheelWorld, FIELD_POSITION_VECTOR, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_vecWheelContact, FIELD_POSITION_VECTOR, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_vecWheelNormal, FIELD_VECTOR, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flCompression, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_ARRAY(m_flPreviousCompression, FIELD_FLOAT, CFuncCar::WHEEL_COUNT),
	DEFINE_FIELD(m_flSpeed, FIELD_FLOAT),
	DEFINE_FIELD(m_flThrottle, FIELD_FLOAT),
	DEFINE_FIELD(m_flSteering, FIELD_FLOAT),
	DEFINE_FIELD(m_flWheelRotation, FIELD_FLOAT),
	DEFINE_FIELD(m_flVerticalVelocity, FIELD_FLOAT),
	DEFINE_FIELD(m_flLastThink, FIELD_TIME),
	DEFINE_FIELD(m_flNextDebugText, FIELD_TIME),
	DEFINE_FIELD(m_iGroundedWheels, FIELD_INTEGER),
	DEFINE_FIELD(m_vecLastSafeOrigin, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_vecLastSafeAngles, FIELD_VECTOR),
	DEFINE_FIELD(m_bHasLastSafeTransform, FIELD_BOOLEAN),
	DEFINE_FUNCTION(CarThink),
END_DATADESC()

void CFuncCar::KeyValue(KeyValueData *pkvd)
{
	struct WheelKey { const char *name; int index; };
	static const WheelKey keys[] = {
		{ "wheel_fl_pos", WHEEL_FL }, { "wheel_fr_pos", WHEEL_FR },
		{ "wheel_rl_pos", WHEEL_RL }, { "wheel_rr_pos", WHEEL_RR }
	};
	for (const WheelKey &key : keys)
	{
		if (FStrEq(pkvd->szKeyName, key.name))
		{
			UTIL_StringToVector(m_vecWheelPos[key.index], pkvd->szValue);
			pkvd->fHandled = TRUE;
			return;
		}
	}
	if (FStrEq(pkvd->szKeyName, "wheelmodel"))
		m_iszWheelModel = ALLOC_STRING(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "driver_pos"))
		UTIL_StringToVector(m_vecDriverPos, pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "view_pos"))
		UTIL_StringToVector(m_vecViewPos, pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "exit_pos"))
		UTIL_StringToVector(m_vecExitPos, pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "wheel_radius"))
		m_flWheelRadius = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "wheel_width"))
		m_flWheelWidth = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "maxspeed"))
		m_flMaxSpeed = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "reversespeed"))
		m_flReverseSpeed = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "acceleration"))
		m_flAcceleration = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "brakeforce"))
		m_flBrakeForce = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "drag"))
		m_flDrag = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "steerangle"))
		m_flSteerAngle = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "steerspeed"))
		m_flSteerSpeed = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "suspension_length"))
		m_flSuspensionLength = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "spring_strength"))
		m_flSpringStrength = Q_atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "suspension_damping"))
		m_flSuspensionDamping = Q_atof(pkvd->szValue);
	else
	{
		BaseClass::KeyValue(pkvd);
		return;
	}
	pkvd->fHandled = TRUE;
}

void CFuncCar::Precache()
{
	if (pev->model != NULL_STRING) PRECACHE_MODEL(STRING(pev->model));
	if (m_iszWheelModel != NULL_STRING) PRECACHE_MODEL(STRING(m_iszWheelModel));
}

void CFuncCar::Spawn()
{
	if (m_flWheelRadius <= 0) m_flWheelRadius = 16.0f;
	if (m_flWheelWidth <= 0) m_flWheelWidth = 8.0f;
	if (m_flMaxSpeed <= 0) m_flMaxSpeed = 600.0f;
	if (m_flReverseSpeed <= 0) m_flReverseSpeed = 250.0f;
	if (m_flAcceleration <= 0) m_flAcceleration = 300.0f;
	if (m_flBrakeForce <= 0) m_flBrakeForce = 500.0f;
	if (m_flDrag <= 0) m_flDrag = 80.0f;
	if (m_flSteerAngle <= 0) m_flSteerAngle = 30.0f;
	if (m_flSteerSpeed <= 0) m_flSteerSpeed = 90.0f;
	if (m_flSuspensionLength <= 0) m_flSuspensionLength = 20.0f;
	if (m_flSpringStrength <= 0) m_flSpringStrength = 45.0f;
	if (m_flSuspensionDamping <= 0) m_flSuspensionDamping = 6.0f;

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
	pev->takedamage = DAMAGE_NO;

	studiohdr_t *header = static_cast<studiohdr_t *>(GET_MODEL_PTR(edict()));
	if (header)
	{
		mstudioseqdesc_t *sequences = reinterpret_cast<mstudioseqdesc_t *>(reinterpret_cast<byte *>(header) + header->seqindex);
		UTIL_SetSize(pev, sequences[pev->sequence].bbmin, sequences[pev->sequence].bbmax);
	}
	else UTIL_SetSize(pev, Vector(-48, -32, -16), Vector(48, 32, 32));

	if (WorldPhysic->Initialized())
	{
		pev->solid = SOLID_CUSTOM;
		pev->movetype = MOVETYPE_PHYSIC;
		m_flBodyMass = CAR_BODY_MASS;
		m_pUserData = WorldPhysic->CreateBodyFromEntity(this);
		if (m_pUserData == NULL)
		{
			pev->solid = SOLID_BBOX;
			pev->movetype = MOVETYPE_NONE;
			ALERT(at_warning, "func_car: PhysX body creation failed, using fallback motion\n");
		}
	}

	SetThink(&CFuncCar::CarThink);
	m_flLastThink = gpGlobals->time;
	m_vecLastSafeOrigin = GetAbsOrigin();
	m_vecLastSafeAngles = GetAbsAngles();
	m_bHasLastSafeTransform = TRUE;
	SetNextThink(CAR_THINK_INTERVAL);
	EnsureChildren();
}

void CFuncCar::Activate()
{
	BaseClass::Activate();
	EnsureChildren();
	if (m_hDriver != NULL && m_hDriver->IsPlayer())
	{
		CBasePlayer *player = static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver));
		player->m_pVehicle = this;
		SET_VIEW(player->edict(), GetVehicleViewEntity()->edict());
	}
}

Vector CFuncCar::LocalToWorld(const Vector &local) const
{
	return EntityToWorldTransform().VectorTransform(local);
}

void CFuncCar::EnsureChildren()
{
	if (m_iszWheelModel != NULL_STRING)
	{
		for (int i = 0; i < WHEEL_COUNT; ++i)
		{
			if (m_hWheels[i] != NULL) continue;
			CBaseEntity *wheel = CBaseEntity::Create("func_car_child", GetAbsOrigin(), GetAbsAngles(), edict());
			if (!wheel) continue;
			SET_MODEL(wheel->edict(), STRING(m_iszWheelModel));
			wheel->pev->iuser4 = FUNC_CAR_WHEEL_MARKER;
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
			view->pev->effects |= EF_NOINTERP | EF_MERGE_VISIBILITY;
			view->SetParent(this);
			view->SetLocalOrigin(m_vecViewPos);
			view->SetLocalAngles(g_vecZero);
			m_hViewEntity = view;
		}
	}
}

CBaseEntity *CFuncCar::GetVehicleViewEntity()
{
	return m_hViewEntity != NULL ? static_cast<CBaseEntity *>(m_hViewEntity) : this;
}

void CFuncCar::RemoveChildren()
{
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		if (m_hWheels[i] != NULL) UTIL_Remove(m_hWheels[i]);
		m_hWheels[i] = NULL;
	}
	if (m_hViewEntity != NULL) UTIL_Remove(m_hViewEntity);
	m_hViewEntity = NULL;
}

void CFuncCar::OnRemove()
{
	if (m_hDriver != NULL && m_hDriver->IsPlayer())
		ExitDriver(static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver)), true);
	RemoveChildren();
}

void CFuncCar::Use(CBaseEntity *pActivator, CBaseEntity *, USE_TYPE useType, float)
{
	if (!pActivator || !pActivator->IsPlayer()) return;
	CBasePlayer *player = static_cast<CBasePlayer *>(pActivator);
	if (m_hDriver != NULL)
	{
		if (m_hDriver == player && (useType == USE_OFF || useType == USE_TOGGLE || useType == USE_REMOVE))
			ExitDriver(player, useType == USE_REMOVE);
		return;
	}
	if (useType != USE_OFF) EnterDriver(player);
}

void CFuncCar::EnterDriver(CBasePlayer *player)
{
	if (!player || player->m_pVehicle != NULL || !player->EnterVehicle(this)) return;
	m_hDriver = player;
	player->SetLocalOrigin(m_vecDriverPos);
	player->SetLocalAngles(g_vecZero);
	player->pev->v_angle = GetAbsAngles();
	player->pev->fixangle = TRUE;
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
	m_hDriver = NULL;
	Vector angles = GetAbsAngles();
	angles.x = angles.z = 0;
	player->LeaveVehicle(exitPoint, angles);
}

void CFuncCar::UpdateInput(float dt)
{
	float throttleTarget = 0;
	float steeringTarget = 0;
	if (m_hDriver != NULL)
	{
		if (FBitSet(m_hDriver->pev->button, IN_FORWARD)) throttleTarget += 1;
		if (FBitSet(m_hDriver->pev->button, IN_BACK)) throttleTarget -= 1;
		if (FBitSet(m_hDriver->pev->button, IN_MOVELEFT)) steeringTarget += m_flSteerAngle;
		if (FBitSet(m_hDriver->pev->button, IN_MOVERIGHT)) steeringTarget -= m_flSteerAngle;
	}
	m_flThrottle = throttleTarget;
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
		m_vecWheelWorld[i] = LocalToWorld(m_vecWheelPos[i]);
		TraceResult trace;
		UTIL_TraceLine(m_vecWheelWorld[i], m_vecWheelWorld[i] + down * traceDistance, ignore_monsters, edict(), &trace);
		m_flPreviousCompression[i] = m_flCompression[i];
		m_flCompression[i] = 0;
		m_bWheelGrounded[i] = FALSE;
		m_vecWheelContact[i] = trace.vecEndPos;
		m_vecWheelNormal[i] = trace.vecPlaneNormal;
		if (trace.flFraction < 1.0f && !trace.fStartSolid)
		{
			m_bWheelGrounded[i] = TRUE;
			const float suspension = Q_max(0.0f, traceDistance * trace.flFraction - m_flWheelRadius);
			m_flCompression[i] = ClampFloat(m_flSuspensionLength - suspension, 0, m_flSuspensionLength);
			++m_iGroundedWheels;
		}
	}

	if (m_iActorType == ACTOR_DYNAMIC)
	{
		for (int i = 0; i < WHEEL_COUNT; ++i)
		{
			if (!m_bWheelGrounded[i]) continue;
			const float compressionVelocity = (m_flCompression[i] - m_flPreviousCompression[i]) / Q_max(dt, 0.001f);
			const float suspensionAcceleration = ClampFloat(
				m_flCompression[i] * m_flSpringStrength + compressionVelocity * m_flSuspensionDamping,
				0.0f, 2400.0f);
			const float impulse = suspensionAcceleration * CAR_BODY_MASS * dt / WHEEL_COUNT;
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

		if (m_iGroundedWheels > 0)
		{
			float longitudinalAcceleration = 0;
			if (m_flThrottle > 0)
			{
				longitudinalAcceleration = m_flSpeed < -CAR_STOP_EPSILON ? m_flBrakeForce : m_flAcceleration;
				if (m_flSpeed >= m_flMaxSpeed) longitudinalAcceleration = 0;
			}
			else if (m_flThrottle < 0)
			{
				longitudinalAcceleration = m_flSpeed > CAR_STOP_EPSILON ? -m_flBrakeForce : -m_flAcceleration;
				if (m_flSpeed <= -m_flReverseSpeed) longitudinalAcceleration = 0;
			}
			else if (fabs(m_flSpeed) > CAR_STOP_EPSILON)
				longitudinalAcceleration = m_flSpeed > 0 ? -m_flDrag : m_flDrag;

			if (m_hDriver != NULL && FBitSet(m_hDriver->pev->button, IN_JUMP))
				longitudinalAcceleration = -m_flSpeed * 8.0f;
			const Vector angularVelocity = GetAbsAvelocity();
			const float impulseScale = CAR_BODY_MASS * dt / m_iGroundedWheels;
			// Full steering lock is useful while manoeuvring, but at road speed it
			// creates an unrealistically large instantaneous lateral impulse.
			const float speedFraction = ClampFloat(fabs(m_flSpeed) / Q_max(m_flMaxSpeed, 1.0f), 0.0f, 1.0f);
			const float highSpeedSteerScale = 1.0f - speedFraction * 0.65f;
			const float steerRadians = m_flSteering * highSpeedSteerScale * (M_PI / 180.0f);
			for (int i = 0; i < WHEEL_COUNT; ++i)
			{
				if (!m_bWheelGrounded[i]) continue;
				const bool frontAxle = i == WHEEL_FL || i == WHEEL_FR;
				const float wheelSteer = frontAxle ? steerRadians : 0.0f;
				Vector wheelForward = forward * cosf(wheelSteer) + right * sinf(wheelSteer);
				Vector wheelRight = right * cosf(wheelSteer) - forward * sinf(wheelSteer);
				wheelForward -= m_vecWheelNormal[i] * DotProduct(wheelForward, m_vecWheelNormal[i]);
				wheelRight -= m_vecWheelNormal[i] * DotProduct(wheelRight, m_vecWheelNormal[i]);
				if (wheelForward.Length() > 0.001f) wheelForward = wheelForward.Normalize();
				if (wheelRight.Length() > 0.001f) wheelRight = wheelRight.Normalize();

				const Vector arm = m_vecWheelContact[i] - GetAbsOrigin();
				const Vector pointVelocity = velocity + CrossProduct(angularVelocity, arm);
				const float lateralSpeed = DotProduct(pointVelocity, wheelRight);
				const float lateralAcceleration = ClampFloat(-lateralSpeed * CAR_LATERAL_GRIP, -500.0f, 500.0f);
				const Vector wheelAcceleration = wheelForward * longitudinalAcceleration + wheelRight * lateralAcceleration;
				// Apply tyre forces at hub height. The contact patch is still used by
				// suspension, but using it for the full lateral impulse produced an
				// excessive roll lever and could flip the Hummer on flat ground.
				WorldPhysic->AddImpulse(this, wheelAcceleration, m_vecWheelWorld[i], impulseScale);
			}
		}
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
	m_flWheelRotation += (m_flSpeed / Q_max(m_flWheelRadius, 1.0f)) * dt * 57.29578f;
	if (m_flWheelRotation > 360 || m_flWheelRotation < -360) m_flWheelRotation = fmodf(m_flWheelRotation, 360.0f);
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		CBaseEntity *wheel = m_hWheels[i];
		if (!wheel) continue;
		// Re-query after the body sweep. The suspension force was already
		// calculated earlier; this trace only prevents one-frame-old visual
		// contacts from placing a wheel below BSP after collision/rotation.
		const Vector rayStart = LocalToWorld(m_vecWheelPos[i]);
		const Vector down(0, 0, -1);
		const float traceDistance = m_flSuspensionLength + m_flWheelRadius;
		TraceResult visualTrace;
		UTIL_TraceLine(rayStart, rayStart + down * traceDistance, ignore_monsters, edict(), &visualTrace);
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
		wheel->SetLocalOrigin(EntityToWorldTransform().VectorITransform(worldCenter));
		// hummer_wheel.mdl is authored as a front-left wheel. Right wheels use
		// a true model-space X reflection supplied through startpos; see the
		// narrowly-scoped renderer handling for FUNC_CAR_WHEEL_MARKER.
		const bool rightSide = (i == WHEEL_FR || i == WHEEL_RR);
		const bool frontAxle = (i == WHEEL_FL || i == WHEEL_FR);
		const float baseYaw = rightSide ? 180.0f : 0.0f;
		const float visualRotation = rightSide ? -m_flWheelRotation : m_flWheelRotation;
		const float visualSpeedFraction = ClampFloat(fabs(m_flSpeed) / Q_max(m_flMaxSpeed, 1.0f), 0.0f, 1.0f);
		const float visualSteering = m_flSteering * (1.0f - visualSpeedFraction * 0.65f);
		wheel->SetLocalAngles(Vector(visualRotation, baseYaw + (frontAxle ? -visualSteering : 0), 0));
	}
	if (m_hViewEntity != NULL)
	{
		m_hViewEntity->SetLocalOrigin(m_vecViewPos);
		m_hViewEntity->SetLocalAngles(g_vecZero);
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
		line(m_vecWheelWorld[i], m_vecWheelWorld[i] + down * (m_flSuspensionLength + m_flWheelRadius), 255, 180, 0);
		line(m_vecWheelWorld[i] - Vector(2,0,0), m_vecWheelWorld[i] + Vector(2,0,0), 255, 255, 0);
		if (m_flCompression[i] > 0) line(m_vecWheelContact[i], m_vecWheelContact[i] + m_vecWheelNormal[i] * 8, 0, 255, 0);
	}
	line(LocalToWorld(m_vecDriverPos), LocalToWorld(m_vecDriverPos) + Vector(0,0,8), 0, 128, 255);
	line(LocalToWorld(m_vecViewPos), LocalToWorld(m_vecViewPos) + Vector(0,0,8), 255, 0, 255);
	line(LocalToWorld(m_vecExitPos), LocalToWorld(m_vecExitPos) + Vector(0,0,8), 0, 255, 255);
	if (level >= 2 && gpGlobals->time >= m_flNextDebugText)
	{
		m_flNextDebugText = gpGlobals->time + 0.25f;
		ALERT(at_console, "car speed %.1f throttle %.0f steer %.1f grounded %d compression [%.1f %.1f %.1f %.1f]\n",
			m_flSpeed, m_flThrottle, m_flSteering, m_iGroundedWheels,
			m_flCompression[0], m_flCompression[1], m_flCompression[2], m_flCompression[3]);
	}
}

void CFuncCar::CarThink()
{
	const float dt = ClampFloat(gpGlobals->time - m_flLastThink, 0.001f, 0.05f);
	m_flLastThink = gpGlobals->time;
	EnsureChildren();
	if (m_hDriver != NULL && (!m_hDriver->IsPlayer() || !m_hDriver->IsAlive()))
	{
		CBasePlayer *player = m_hDriver->IsPlayer() ? static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_hDriver)) : NULL;
		if (player) ExitDriver(player, true); else m_hDriver = NULL;
	}
	UpdateInput(dt);
	UpdateWheels(dt);
	UpdateMotion(dt);
	UpdateVisuals(dt);
	DebugDraw();
	SetNextThink(CAR_THINK_INTERVAL);
}
