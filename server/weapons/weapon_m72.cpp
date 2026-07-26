#include "weapon_m72.h"
#include "weapon_layer.h"
#include "weapons/m72.h"
#include "server_weapon_layer_impl.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS(weapon_m72, CM72);

class CSpentM72 : public CBaseEntity
{
	DECLARE_CLASS(CSpentM72, CBaseEntity);
public:
	void Spawn() override
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->solid = SOLID_TRIGGER;
		SetTouch(NULL); // visual-only spent tube; it cannot be picked up
		SET_MODEL(edict(), "models/weapon/m72/w_law.mdl");
		UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 8));
	}
};

LINK_ENTITY_TO_CLASS(spent_m72, CSpentM72);

CM72::CM72()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CM72WeaponContext>(std::move(layer));
}

void CM72::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(M72_CLASSNAME));
	Precache();
	SET_MODEL(edict(), "models/weapon/m72/w_law-closed.mdl");
	FallInit();
}

void CM72::Precache()
{
	PRECACHE_MODEL("models/weapon/m72/v_law.mdl");
	PRECACHE_MODEL("models/weapon/m72/p_law.mdl");
	PRECACHE_MODEL("models/weapon/m72/w_law-closed.mdl");
	PRECACHE_MODEL("models/weapon/m72/w_law.mdl");
	PRECACHE_MODEL("models/weapon/m72/lawrocket.mdl");
	PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND("weapons/rocketfire1.wav");
	PRECACHE_SOUND("weapons/rocket1.wav");
	UTIL_PrecacheOther("m72_rocket");
}

int CM72::AddToPlayer(CBasePlayer* player)
{
	return CBasePlayerWeapon::AddToPlayer(player);
}

void CM72::RetireSpentLauncher()
{
	if (!m_pPlayer)
		return;

	CBasePlayer* player = m_pPlayer;
	CBaseEntity* spent = CBaseEntity::Create("spent_m72", player->GetAbsOrigin() + Vector(0, 0, 12), Vector(0, player->pev->v_angle.y, 0), player->edict());
	if (spent)
	{
		UTIL_MakeVectors(Vector(0, player->pev->v_angle.y, 0));
		spent->SetAbsVelocity(gpGlobals->v_forward * 80 + Vector(0, 0, 40));
	}

	g_pGameRules->GetNextBestWeapon(player, this);
	player->RemoveWeapon(WEAPON_M72);
	SetThink(&CM72::DestroyItem);
	pev->nextthink = gpGlobals->time + 0.1f;
}
