#include "weapon_tmp.h"
#include "weapon_layer.h"
#include "weapons/tmp.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_tmp, CTMP);

CTMP::CTMP()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CTMPWeaponContext>(std::move(layer));
}

void CTMP::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(TMP_CLASSNAME));
	Precache();
	SET_MODEL(edict(), "models/weapon/TMP/w_tmp.mdl");
	FallInit();
}

void CTMP::Precache()
{
	PRECACHE_MODEL("models/weapon/TMP/v_tmp.mdl");
	PRECACHE_MODEL("models/weapon/TMP/w_tmp.mdl");
	PrecacheViewModelSounds("models/weapon/TMP/v_tmp.mdl");
	PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/TMP/tmp-1.wav");
	PRECACHE_SOUND("weapons/TMP/tmp_clipin.wav");
	PRECACHE_SOUND("weapons/TMP/tmp_clipin2.wav");
	PRECACHE_SOUND("weapons/TMP/tmp_clipout.wav");
	PRECACHE_SOUND("weapons/TMP/tmp_release.wav");
}
