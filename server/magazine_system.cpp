#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "magazine_system.h"

LINK_ENTITY_TO_CLASS(item_dropped_magazine, CDroppedMagazine);

BEGIN_DATADESC(CDroppedMagazine)
	DEFINE_FIELD(m_iMagazineType, FIELD_INTEGER),
	DEFINE_FIELD(m_iAmmoType, FIELD_INTEGER),
	DEFINE_FIELD(m_iRounds, FIELD_INTEGER),
	DEFINE_FIELD(m_iCapacity, FIELD_INTEGER),
	DEFINE_FUNCTION(MagazineTouch),
END_DATADESC()

void CDroppedMagazine::Precache()
{
	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

void CDroppedMagazine::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-8, -8, 0), Vector(8, 8, 8));
	SetTouch(NULL);
}

void CDroppedMagazine::SetMagazine(int magazineType, int ammoType, int rounds, int capacity)
{
	m_iMagazineType = magazineType;
	m_iAmmoType = ammoType;
	m_iRounds = Q_max(0, Q_min(rounds, capacity));
	m_iCapacity = capacity;
	if (m_iRounds == 0)
		UTIL_Remove(this);
}

void CDroppedMagazine::Use(CBaseEntity *activator, CBaseEntity *, USE_TYPE, float)
{
	MagazineTouch(activator);
}

void CDroppedMagazine::MagazineTouch(CBaseEntity *other)
{
	if (!other || !other->IsPlayer() || !other->IsAlive() || m_iRounds <= 0)
		return;

	CBasePlayer *player = static_cast<CBasePlayer *>(other);
	int remaining = m_iRounds;
	if (player->AddMagazine(m_iMagazineType, m_iAmmoType, m_iRounds, m_iCapacity, &remaining) <= 0)
		return;

	m_iRounds = remaining;
	EMIT_SOUND(ENT(player->pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
	if (m_iRounds <= 0)
		UTIL_Remove(this);
}

CDroppedMagazine *CreateDroppedMagazine(const Vector &origin, const Vector &velocity,
	int magazineType, int ammoType, int rounds, int capacity, edict_t *owner)
{
	if (rounds <= 0 || capacity <= 0)
		return NULL;

	CDroppedMagazine *magazine = static_cast<CDroppedMagazine *>(
		CBaseEntity::Create("item_dropped_magazine", origin, g_vecZero, owner));
	if (!magazine)
		return NULL;
	magazine->SetMagazine(magazineType, ammoType, rounds, capacity);
	magazine->SetAbsVelocity(velocity);
	return magazine;
}
