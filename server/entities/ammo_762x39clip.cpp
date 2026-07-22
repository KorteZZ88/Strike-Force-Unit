#include "ammo_762x39clip.h"
#include "weapons/ak47.h"
#include "player.h"
LINK_ENTITY_TO_CLASS(ammo_762x39clip, C762x39AmmoClip);
void C762x39AmmoClip::Spawn() { Precache(); SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl"); if (pev->health <= 0) pev->health = AK47_MAX_CLIP; CBasePlayerAmmo::Spawn(); }
void C762x39AmmoClip::Precache() { PRECACHE_MODEL("models/w_9mmARclip.mdl"); PRECACHE_SOUND("items/9mmclip1.wav"); }
BOOL C762x39AmmoClip::AddAmmo(CBaseEntity *other)
{
	CBasePlayer *player = static_cast<CBasePlayer *>(other); const int ammoType = player->GetAmmoIndex("762x39");
	const int rounds = Q_max(0, Q_min((int)pev->health, AK47_MAX_CLIP)); int remaining = rounds;
	const BOOL added = player->AddMagazine(WEAPON_AK47, ammoType, rounds, AK47_MAX_CLIP, &remaining) > 0;
	if (added) EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
	pev->health = remaining == 0 ? AK47_MAX_CLIP : remaining; return added && remaining == 0;
}
