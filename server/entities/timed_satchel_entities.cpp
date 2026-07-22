#include "timed_satchel_entities.h"
#include "player.h"

LINK_ENTITY_TO_CLASS(timed_satchel_bomb, CTimedSatchelBomb);
LINK_ENTITY_TO_CLASS(timed_satchel_preview, CTimedSatchelPreview);

BEGIN_DATADESC(CTimedSatchelBomb)
	DEFINE_FUNCTION(TimedDetonate),
END_DATADESC()

BEGIN_DATADESC(CTimedSatchelPreview)
	DEFINE_FIELD(m_bCanPlace, FIELD_BOOLEAN),
	DEFINE_FIELD(m_hSurface, FIELD_EHANDLE),
	DEFINE_FUNCTION(PreviewThink),
END_DATADESC()

void CTimedSatchelBomb::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->health = 1;
	pev->dmg = gSkillData.plrDmgC4;
	SET_MODEL(edict(), "models/w_satchel.mdl");
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));
	SetThink(&CTimedSatchelBomb::TimedDetonate);
	pev->nextthink = gpGlobals->time + 10.0f;
}

void CTimedSatchelBomb::Precache()
{
	PRECACHE_MODEL("models/w_satchel.mdl");
}

int CTimedSatchelBomb::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker,
	float flDamage, int bitsDamageType)
{
	if (bitsDamageType & DMG_BLAST)
	{
		if (!pevInflictor || !FClassnameIs(pevInflictor, "timed_satchel_bomb"))
		{
			// Hand grenades, underbarrel grenades and other explosions cannot
			// remove an installed C4.
			return 0;
		}

		if (flDamage > 0.0f && pev->takedamage != DAMAGE_NO)
		{
			// A neighbouring C4 starts a short delayed chain reaction instead of
			// exploding in the same frame: "ba-bang" rather than one merged blast.
			pev->takedamage = DAMAGE_NO;
			SetThink(&CTimedSatchelBomb::TimedDetonate);
			pev->nextthink = gpGlobals->time + 0.13f;
			return 1;
		}
		return 0;
	}

	return BaseClass::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CTimedSatchelBomb::TimedDetonate()
{
	constexpr float C4_SHAKE_RADIUS = 25.0f * 39.3701f;
	UTIL_ScreenShake(GetAbsOrigin(), 5.0f, 75.0f, 0.8f, C4_SHAKE_RADIUS, true);
	pev->owner = pev->euser1;
	Detonate();
}

void CTimedSatchelPreview::Spawn()
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 110;
	pev->rendercolor = Vector(80, 255, 80);
	SET_MODEL(edict(), "models/w_satchel.mdl");
	SetThink(&CTimedSatchelPreview::PreviewThink);
	pev->nextthink = gpGlobals->time;
}

void CTimedSatchelPreview::PreviewThink()
{
	CBasePlayer *owner = static_cast<CBasePlayer*>(CBaseEntity::Instance(pev->owner));
	if (!owner || !owner->IsAlive())
	{
		UTIL_Remove(this);
		return;
	}

	UTIL_MakeVectors(owner->pev->v_angle + owner->pev->punchangle);
	const Vector start = owner->GetGunPosition();
	TraceResult tr;
	UTIL_TraceLine(start, start + gpGlobals->v_forward * 128.0f, ignore_monsters, owner->edict(), &tr);
	CBaseEntity *surface = tr.flFraction < 1.0f ? CBaseEntity::Instance(tr.pHit) : nullptr;
	m_bCanPlace = surface && (surface->IsBSPModel() || surface->IsCustomModel()) && !(surface->pev->flags & FL_CONVEYOR);
	m_hSurface = m_bCanPlace ? surface : nullptr;

	if (m_bCanPlace)
	{
		SetAbsOrigin(tr.vecEndPos + tr.vecPlaneNormal * 4.0f);
		SetAbsAngles(UTIL_VecToAngles(tr.vecPlaneNormal));
		pev->rendercolor = Vector(80, 255, 80);
		ClearBits(pev->effects, EF_NODRAW);
	}
	else
	{
		if( !surface )
		{
			// Do not show a C4 placement ghost suspended in empty space.
			SetBits(pev->effects, EF_NODRAW);
		}
		else
		{
			SetAbsOrigin(tr.vecEndPos + tr.vecPlaneNormal * 4.0f);
			SetAbsAngles(UTIL_VecToAngles(tr.vecPlaneNormal));
			pev->rendercolor = Vector(255, 80, 80);
			ClearBits(pev->effects, EF_NODRAW);
		}
	}

	pev->nextthink = gpGlobals->time + 0.05f;
}

void CTimedSatchelPreview::PlaceBomb(int timerSeconds)
{
	if (!m_bCanPlace)
		return;

	CTimedSatchelBomb *bomb = static_cast<CTimedSatchelBomb*>(CBaseEntity::Create(
		"timed_satchel_bomb", GetAbsOrigin(), GetAbsAngles(), pev->owner));
	if (!bomb)
		return;
	bomb->pev->nextthink = gpGlobals->time + (timerSeconds == 30 ? 30.0f : 10.0f);
	EMIT_SOUND(bomb->edict(), CHAN_BODY, "weapons/mine_deploy.wav", 1.0f, ATTN_NORM);

	// Preserve kill credit if the bomb is parented and its engine owner changes.
	bomb->pev->euser1 = pev->owner;
	CBaseEntity *surface = (CBaseEntity *)m_hSurface;
	if (surface && surface != g_pWorld)
		bomb->SetParent(surface);
}
