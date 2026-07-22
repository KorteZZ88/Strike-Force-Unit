#include "ammo_762clip.h"
#include "weapons/m24.h"
#include "player.h"

LINK_ENTITY_TO_CLASS(ammo_762clip, C762AmmoClip);

void C762AmmoClip::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
	if (pev->health <= 0)
		pev->health = M24_MAX_CLIP;
	CBasePlayerAmmo::Spawn();
}

void C762AmmoClip::Precache()
{
	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL C762AmmoClip::AddAmmo(CBaseEntity *other)
{
	CBasePlayer *player = static_cast<CBasePlayer *>(other);
	const int ammoType = player->GetAmmoIndex("762");
	const int rounds = Q_max(0, Q_min((int)pev->health, M24_MAX_CLIP));
	int remaining = rounds;
	const BOOL added = player->AddMagazine(WEAPON_M24, ammoType, rounds, M24_MAX_CLIP, &remaining) > 0;
	if (added)
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
	pev->health = remaining == 0 ? M24_MAX_CLIP : remaining;
	return added && remaining == 0;
}
