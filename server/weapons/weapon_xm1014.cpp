#include "weapon_xm1014.h"
#include "weapons/xm1014.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_xm1014, CXM1014);

CXM1014::CXM1014()
{
	m_pWeaponContext = std::make_unique<CXM1014WeaponContext>(
		std::make_unique<CServerWeaponLayerImpl>(this));
}

void CXM1014::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(XM1014_CLASSNAME));
	Precache();
	SET_MODEL(ENT(pev), "models/weapon/XM1014/w_xm1014.mdl");
	FallInit();
}

void CXM1014::Precache()
{
	PRECACHE_MODEL("models/weapon/XM1014/v_xm1014.mdl");
	PRECACHE_MODEL("models/weapon/XM1014/w_xm1014.mdl");
	PRECACHE_MODEL("models/weapon/XM1014/p_xm1014.mdl");
	PRECACHE_MODEL("models/shotgunshell.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("weapons/XM1014/xm1014-1.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PrecacheViewModelSounds("models/weapon/XM1014/v_xm1014.mdl");
}

int CXM1014::AddToPlayer(CBasePlayer* player)
{
	if (!CBasePlayerWeapon::AddToPlayer(player)) return FALSE;
	ItemInfo info{};
	if (GetItemInfo(&info) && info.iId > 0 && info.iId < MAX_WEAPONS)
		CBaseWeaponContext::ItemInfoArray[info.iId] = info;
	player->m_fKnownItem = FALSE;
	return TRUE;
}
