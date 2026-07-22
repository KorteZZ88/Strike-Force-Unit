#include "weapon_wrench.h"
#include "weapon_layer.h"
#include "weapons/wrench.h"
#include "server_weapon_layer_impl.h"
#include <utility>

LINK_ENTITY_TO_CLASS(weapon_wrench, CWrench);

CWrench::CWrench()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CWrenchWeaponContext>(std::move(layer));
}

void CWrench::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(WRENCH_CLASSNAME));
	Precache();
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	FallInit();
}

void CWrench::Precache()
{
	PRECACHE_MODEL("models/weapon/wrench/v_wrench.mdl");
	PRECACHE_MODEL("models/w_crowbar.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
}
