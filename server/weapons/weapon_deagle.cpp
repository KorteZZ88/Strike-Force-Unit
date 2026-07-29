#include "weapon_deagle.h"
#include "weapon_layer.h"
#include "weapons/deagle.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_deagle, CDeagle);

CDeagle::CDeagle()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CDeagleWeaponContext>(std::move(layer));
}

void CDeagle::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(DEAGLE_CLASSNAME));
	Precache();
	SET_MODEL(edict(), "models/weapon/DEagle/w_deagle.mdl");
	FallInit();
}

void CDeagle::Precache()
{
	PRECACHE_MODEL("models/weapon/DEagle/v_deagle.mdl");
	PRECACHE_MODEL("models/weapon/DEagle/p_deagle.mdl");
	PRECACHE_MODEL("models/weapon/DEagle/w_deagle.mdl");
	PrecacheViewModelSounds("models/weapon/DEagle/v_deagle.mdl");
	PRECACHE_SOUND("weapons/DEagle/deagle-1.wav");
	PRECACHE_SOUND("weapons/DEagle/deagle-2.wav");
	PRECACHE_SOUND("weapons/DEagle/de_clipin.wav");
	PRECACHE_SOUND("weapons/DEagle/de_clipout.wav");
	PRECACHE_SOUND("weapons/DEagle/de_deploy.wav");
}
