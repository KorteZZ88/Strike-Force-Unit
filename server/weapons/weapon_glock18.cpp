#include "weapon_glock18.h"
#include "weapons/glock18.h"
#include "server_weapon_layer_impl.h"
LINK_ENTITY_TO_CLASS(weapon_glock18,CGlock18);
CGlock18::CGlock18(){m_pWeaponContext=std::make_unique<CGlock18WeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this));}
void CGlock18::Spawn(){pev->classname=MAKE_STRING(CLASSNAME_STR(GLOCK18_CLASSNAME));Precache();SET_MODEL(edict(),"models/weapon/glock18/w_glock18.mdl");FallInit();}
void CGlock18::Precache(){PRECACHE_MODEL("models/weapon/glock18/v_glock18.mdl");PRECACHE_MODEL("models/weapon/glock18/p_glock18.mdl");PRECACHE_MODEL("models/weapon/glock18/w_glock18.mdl");PRECACHE_MODEL("models/shell.mdl");PRECACHE_SOUND("weapons/Glock18/glock18-1.wav");PRECACHE_SOUND("items/9mmclip1.wav");}
