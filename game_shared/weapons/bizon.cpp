#include "bizon.h"

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#endif

CBizonWeaponContext::CBizonWeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_BIZON;
	m_iDefaultAmmo = BIZON_DEFAULT_GIVE;
	m_usEvent = m_pLayer->PrecacheEvent("events/bizon.sc");
}

int CBizonWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(BIZON_CLASSNAME);
	p->pszAmmo1 = "9mm_bizon";
	p->iMaxAmmo1 = BIZON_MAX_CLIP * BIZON_MAX_SPARE_MAGAZINES;
	p->iMaxClip = BIZON_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 11;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = BIZON_WEIGHT;
	return 1;
}

bool CBizonWeaponContext::Deploy()
{
	return DefaultDeploy("models/weapon/Bizon/v_bizon.mdl", "models/weapon/Bizon/p_bizon.mdl", BIZON_DEPLOY, "mp5");
}

void CBizonWeaponContext::PrimaryAttack()
{
	const float cycleTime = ConfigFireInterval(60.0f / 700.0f);
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
	// P90 standing accuracy; its moving profile is the MP5 Navy profile.
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

void CBizonWeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;
	const int reloadClipSize = m_iClip > 0 ? BIZON_MAX_CLIP + 1 : BIZON_MAX_CLIP;
	if (m_iClip >= reloadClipSize)
		return;
	DefaultReload(reloadClipSize, BIZON_RELOAD, ConfigValue("reload_time", 3.35f));
}

void CBizonWeaponContext::WeaponIdle()
{
	ResetEmptySound();
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;
	SendWeaponAnim(BIZON_IDLE);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 20.0f;
}
