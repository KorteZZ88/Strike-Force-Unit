#include "flashbang_grenade.h"
#include "player.h"
#include "basemonster.h"
#include "soundent.h"

LINK_ENTITY_TO_CLASS(flashbang_grenade, CFlashbangGrenade);

static constexpr char FLASHBANG_EXPLODE_SOUND[] = "weapons/flashbang/flashbang.wav";
static constexpr char FLASHBANG_HIT_SOUND[] = "weapons/flashbang/flashbang_hit.wav";

BEGIN_DATADESC(CFlashbangGrenade)
	DEFINE_FUNCTION(DetonateFlash),
END_DATADESC()

CFlashbangGrenade* CFlashbangGrenade::ShootTimed(entvars_t* owner, Vector start, Vector velocity, float time)
{
	CFlashbangGrenade* grenade = GetClassPtr((CFlashbangGrenade*)NULL);
	grenade->Spawn();
	grenade->pev->classname = MAKE_STRING("flashbang_grenade");
	SET_MODEL(grenade->edict(), "models/weapon/flashbang/w_flashbang.mdl");
	UTIL_SetOrigin(grenade, start);
	grenade->SetAbsVelocity(velocity);
	grenade->SetLocalAngles(UTIL_VecToAngles(velocity));
	grenade->pev->owner = ENT(owner);
	grenade->pev->dmgtime = gpGlobals->time + time;
	grenade->pev->gravity = 0.5f;
	grenade->pev->friction = 0.8f;
	grenade->SetTouch(&CGrenade::BounceTouch);
	grenade->SetThink(&CFlashbangGrenade::DetonateFlash);
	grenade->SetNextThink(time);
	return grenade;
}

void CFlashbangGrenade::BounceSound()
{
	EMIT_SOUND(edict(), CHAN_VOICE, FLASHBANG_HIT_SOUND, 1.0f, ATTN_NORM);
	EMIT_SOUND(edict(), CHAN_BODY, FLASHBANG_HIT_SOUND, 0.5f, ATTN_NORM);
}

void CFlashbangGrenade::DetonateFlash()
{
	const Vector origin = GetAbsOrigin();
	EMIT_SOUND(edict(), CHAN_WEAPON, FLASHBANG_EXPLODE_SOUND, 1.0f, ATTN_NORM);
	EMIT_SOUND(edict(), CHAN_ITEM, FLASHBANG_EXPLODE_SOUND, 0.5f, ATTN_NORM);
	// Keep the invisible explosion visual event, but suppress its stock sound;
	// the flashbang sample above must be the only explosion sound.
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, origin);
		WRITE_BYTE(TE_EXPLOSION);
		WRITE_COORD(origin.x); WRITE_COORD(origin.y); WRITE_COORD(origin.z);
		WRITE_SHORT(g_sModelIndexFireball);
		WRITE_BYTE(0);
		WRITE_BYTE(15);
		WRITE_BYTE(TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOSOUND | TE_EXPLFLAG_NOPARTICLES);
	MESSAGE_END();

	// A short, intense white flash.  TE_DLIGHT encodes radius in tens of units
	// and lifetime in tenths of a second.
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(origin.x); WRITE_COORD(origin.y); WRITE_COORD(origin.z);
		WRITE_BYTE(30);  // 300-unit radius
		WRITE_BYTE(255); // red
		WRITE_BYTE(255); // green
		WRITE_BYTE(255); // blue
		WRITE_BYTE(3);   // 0.3-second lifetime
		WRITE_BYTE(100); // fast decay
	MESSAGE_END();

	// Several overlapping spark showers make the burst denser than the stock
	// single TE_SPARKS effect while retaining its physical-looking trajectories.
	UTIL_Sparks(origin);
	UTIL_Sparks(origin + Vector(6.0f, -4.0f, 3.0f));
	UTIL_Sparks(origin + Vector(-5.0f, 5.0f, -2.0f));
	for (int i = 0; i < 5; ++i)
	{
		Vector direction(RANDOM_FLOAT(-0.55f, 0.55f), RANDOM_FLOAT(-0.55f, 0.55f),
			RANDOM_FLOAT(0.75f, 1.0f));
		direction = direction.Normalize();
		CBaseEntity* shower = Create("spark_shower", origin, direction, NULL);
		if (shower)
		{
			Vector velocity = shower->GetAbsVelocity();
			velocity.z /= 3.0f;
			shower->SetAbsVelocity(velocity);
			shower->pev->gravity = 0.35f;
			shower->pev->speed = 0.9f;
		}
	}

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer* player = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(i));
		if (!player || !player->IsAlive()) continue;
		const Vector eyes = player->pev->origin + player->pev->view_ofs;
		const float distance = (eyes - origin).Length();
		if (distance > 20.0f * 39.3701f) continue;
		TraceResult trace;
		UTIL_TraceLine(origin, eyes, ignore_monsters, edict(), &trace);
		if (trace.flFraction < 1.0f && trace.pHit != player->edict()) continue;
		const float distanceMeters = distance / 39.3701f;
		const float intensity = distanceMeters <= 6.0f ? 1.0f : 1.0f - (distanceMeters - 6.0f) / 14.0f;
		UTIL_MakeVectors(player->pev->v_angle);
		const float facing = DotProduct(gpGlobals->v_forward, (origin - eyes).Normalize());
		const float visualScale = facing >= 0.5f ? 1.0f : (facing <= -0.5f ? 0.15f : 0.5f);
		player->ApplyFlashbang(intensity, visualScale);
	}

	CBaseEntity* entity = NULL;
	while ((entity = UTIL_FindEntityInSphere(entity, origin, 20.0f * 39.3701f)) != NULL)
	{
		if (entity->IsPlayer()) continue;
		CBaseMonster* monster = entity->MyMonsterPointer();
		if (!monster || !monster->IsAlive()) continue;
		const Vector eyes = monster->EyePosition();
		const float distance = (eyes - origin).Length();
		TraceResult trace;
		UTIL_TraceLine(origin, eyes, ignore_monsters, edict(), &trace);
		if (trace.flFraction < 1.0f && trace.pHit != monster->edict()) continue;
		const float distanceMeters = distance / 39.3701f;
		const float intensity = distanceMeters <= 6.0f ? 1.0f :
			1.0f - (distanceMeters - 6.0f) / 14.0f;
		monster->ApplyFlashbang(intensity);
	}
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	UTIL_Remove(this);
}
