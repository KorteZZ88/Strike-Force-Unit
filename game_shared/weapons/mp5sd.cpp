#include "mp5sd.h"

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#endif

CMP5SDWeaponContext::CMP5SDWeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_MP5SD;
	m_iDefaultAmmo = MP5SD_DEFAULT_GIVE;
	m_usEvent1 = m_pLayer->PrecacheEvent("events/mp5sd.sc");
}

int CMP5SDWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(MP5SD_CLASSNAME);
	p->pszAmmo1 = "9mm_mp5";
	p->iMaxAmmo1 = MP5_9MM_MAX_CARRY;
	p->iMaxClip = MP5SD_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = MP5SD_WEIGHT;
	return 1;
}

int CMP5SDWeaponContext::GetReloadClipSize(int requestedClipSize)
{
	return requestedClipSize;
}

bool CMP5SDWeaponContext::Deploy()
{
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 250;
#endif
	return DefaultDeploy("models/weapon/MP-5SD/v_mp5sd.mdl", "models/weapon/MP-5SD/p_mp5sd.mdl", MP5SD_DEPLOY, "mp5");
}

void CMP5SDWeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.086f);
		return;
	}

	--m_iClip;
	Vector vecSrc = m_pLayer->GetGunPosition();
	matrix3x3 cameraTransform = m_pLayer->GetCameraOrientation();
	cameraTransform.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	const float spread = GetCs16AutomaticSpread(Cs16AutomaticProfile::MP5Navy);
	Vector vecDir = m_pLayer->FireBullets(1, vecSrc, cameraTransform, 8192, spread, BULLET_PLAYER_MP5, m_pLayer->GetRandomSeed());
	ApplyCs16AutomaticKickBack(Cs16AutomaticProfile::MP5Navy);

	WeaponEventParams params{};
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usEvent1;
	params.origin = vecSrc;
	params.angles = cameraTransform.GetAngles();
	params.fparam1 = vecDir.x;
	params.fparam2 = vecDir.y;
	if (m_pLayer->ShouldRunFuncs())
		m_pLayer->PlaybackWeaponEvent(params);

#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->m_iWeaponVolume = QUIET_GUN_VOLUME;
	player->m_iWeaponFlash = 0;
	player->pev->effects &= ~EF_MUZZLEFLASH;
	player->SetAnimation(PLAYER_ATTACK1);
	if (!m_iClip && player->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		player->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif

	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.07f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
}

void CMP5SDWeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;
	const int reloadClipSize = m_iClip > 0 ? MP5SD_MAX_CLIP + 1 : MP5SD_MAX_CLIP;
	if (m_iClip >= reloadClipSize)
		return;
	if (DefaultReload(reloadClipSize, MP5SD_RELOAD, 3.0f))
	{
#ifndef CLIENT_DLL
		m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 190;
#endif
	}
}

void CMP5SDWeaponContext::WeaponIdle()
{
	ResetEmptySound();
	m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 250;
#endif
	SendWeaponAnim(m_pLayer->GetRandomInt(m_pLayer->GetRandomSeed(), 0, 1) == 0 ? MP5SD_LONGIDLE : MP5SD_IDLE1);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
}

void CMP5SDWeaponContext::Holster()
{
	m_fInReload = FALSE;
	m_iReloadClipSize = 0;
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 0;
#endif
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
	SendWeaponAnim(MP5SD_FIRE1);
}
