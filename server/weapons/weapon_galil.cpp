#include "weapon_galil.h"
#include "weapons/galil.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_galil, CGalil);
CGalil::CGalil() { m_pWeaponContext = std::make_unique<CGalilWeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this)); }
void CGalil::Spawn() { pev->classname = MAKE_STRING(CLASSNAME_STR(GALIL_CLASSNAME)); Precache(); SET_MODEL(ENT(pev), "models/weapon/Galil/w_galil.mdl"); FallInit(); }
void CGalil::Precache()
{
	PRECACHE_MODEL("models/weapon/Galil/v_galil.mdl"); PRECACHE_MODEL("models/weapon/Galil/p_galil.mdl");
	PRECACHE_MODEL("models/weapon/Galil/w_galil.mdl"); PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/Galil/galil-1.wav"); PRECACHE_SOUND("weapons/Galil/galil_clipout.wav");
	PRECACHE_SOUND("weapons/Galil/galil_clipin.wav"); PRECACHE_SOUND("weapons/Galil/galil_boltpull.wav");
	PRECACHE_SOUND("weapons/Galil/sg552-2.wav");
	PrecacheViewModelSounds("models/weapon/Galil/v_galil.mdl");
}
int CGalil::AddToPlayer(CBasePlayer* player)
{
	if (!CBasePlayerWeapon::AddToPlayer(player))
		return FALSE;

	// Guarantee that late-created Galils have a client weapon description.
	// Without WeaponList the gun can still fire, but neither its selection icon
	// nor its current clip/ammo HUD can be resolved by the client.
	ItemInfo info = {};
	if (GetItemInfo(&info) && info.iId > 0 && info.iId < MAX_WEAPONS)
		CBaseWeaponContext::ItemInfoArray[info.iId] = info;
	player->m_fKnownItem = FALSE;
	return TRUE;
}
