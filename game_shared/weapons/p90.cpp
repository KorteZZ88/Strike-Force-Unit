#include "p90.h"

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#endif

CP90WeaponContext::CP90WeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_P90;
	m_iDefaultAmmo = P90_DEFAULT_GIVE;
	m_usEvent = m_pLayer->PrecacheEvent("events/p90.sc");
}

int CP90WeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(P90_CLASSNAME);
	p->pszAmmo1 = "57mm_p90";
	p->iMaxAmmo1 = P90_MAX_CLIP * P90_MAX_SPARE_MAGAZINES;
	p->iMaxClip = P90_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 10;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = P90_WEIGHT;
	return 1;
}

bool CP90WeaponContext::Deploy()
{
	return DefaultDeploy("models/weapon/P90/v_p90.mdl", "models/weapon/P90/p_p90.mdl", P90_DEPLOY, "mp5");
}

void CP90WeaponContext::PrimaryAttack()
{
	const float cycleTime = ConfigFireInterval(60.0f / 800.0f);
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(cycleTime);
		return;
	}

	--m_iClip;
	Vector source = m_pLayer->GetGunPosition();
	matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	const float spread = GetCs16AutomaticSpread(Cs16AutomaticProfile::MP5Navy);
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192, spread, BULLET_PLAYER_MP5, m_pLayer->GetRandomSeed());
	ApplyCs16AutomaticKickBack(Cs16AutomaticProfile::MP5Navy);

	WeaponEventParams event{};
	event.flags = WeaponEventFlags::NotHost;
	event.eventindex = m_usEvent;
	event.origin = source;
	event.angles = camera.GetAngles();
	event.fparam1 = direction.x;
	event.fparam2 = direction.y;
	if (m_pLayer->ShouldRunFuncs())
		m_pLayer->PlaybackWeaponEvent(event);

#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	player->m_iWeaponFlash = NORMAL_GUN_FLASH;
	player->pev->effects |= EF_MUZZLEFLASH;
	player->SetAnimation(PLAYER_ATTACK1);
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(cycleTime);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
}

void CP90WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;
	const int reloadClipSize = m_iClip > 0 ? P90_MAX_CLIP + 1 : P90_MAX_CLIP;
	if (m_iClip >= reloadClipSize)
		return;
	DefaultReload(reloadClipSize, P90_RELOAD_EMPTY, ConfigValue("reload_time", 3.33f));
}

void CP90WeaponContext::WeaponIdle()
{
	ResetEmptySound();
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;
	SendWeaponAnim(P90_IDLE);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 20.0f;
}
