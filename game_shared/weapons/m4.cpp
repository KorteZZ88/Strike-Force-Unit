#include "m4.h"

#ifdef CLIENT_DLL
#else
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "ggrenade.h"
#endif

CM4WeaponContext::CM4WeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_M4;
	m_iDefaultAmmo = M4_DEFAULT_GIVE;
	m_usEvent1 = m_pLayer->PrecacheEvent("events/m4.sc");
}

int CM4WeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(M4_CLASSNAME);
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = _556_MAX_CARRY;
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M4_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = M4_WEIGHT;
	return 1;
}

int CM4WeaponContext::GetReloadClipSize(int requestedClipSize)
{
	return requestedClipSize;
}

int CM4WeaponContext::SecondaryAmmoIndex()
{
	return m_iSecondaryAmmoType;
}

bool CM4WeaponContext::Deploy()
{
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->pev->maxspeed = 230; // Замедление игрока при ношении М4
#endif
	return DefaultDeploy("models/weapon/m4/v_m4.mdl", "models/p_9mmar.mdl",
		m_bSilenced ? M4_DRAW : M4_UNSIL_DRAW, "m4");

}

void CM4WeaponContext::PrimaryAttack()
{
	// don't fire underwater
	if (m_pLayer->GetPlayerWaterlevel() == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f);
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f);
		return;
	}

	m_iClip--;

	Vector vecSrc = m_pLayer->GetGunPosition();
	matrix3x3 cameraTransform = m_pLayer->GetCameraOrientation();
	cameraTransform.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	const float spread = GetCs16AutomaticSpread(Cs16AutomaticProfile::M4A1, m_bSilenced);
	Vector vecDir = m_pLayer->FireBullets(1, vecSrc, cameraTransform, 8192, spread, BULLET_PLAYER_556, m_pLayer->GetRandomSeed());
	ApplyCs16AutomaticKickBack(Cs16AutomaticProfile::M4A1);

	WeaponEventParams params;
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usEvent1;
	params.delay = 0.0f;
	params.origin = vecSrc;
	params.angles = cameraTransform.GetAngles();
	params.fparam1 = vecDir.x;
	params.fparam2 = vecDir.y;
	params.iparam1 = 0;
	params.iparam2 = 0;
	params.bparam1 = 0;
	params.bparam2 = m_bSilenced;

	if (m_pLayer->ShouldRunFuncs()) {
		m_pLayer->PlaybackWeaponEvent(params);
	}

#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->m_iWeaponVolume = m_bSilenced ? QUIET_GUN_VOLUME : NORMAL_GUN_VOLUME;
	player->m_iWeaponFlash = m_bSilenced ? DIM_GUN_FLASH : NORMAL_GUN_FLASH;
	if (!m_bSilenced)
		player->pev->effects = (int)(player->pev->effects) | EF_MUZZLEFLASH;
	player->SetAnimation(PLAYER_ATTACK1);

	if (!m_iClip && player->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		player->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif

	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.09f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
}

void CM4WeaponContext::SecondaryAttack()
{
	m_bSilenced = !m_bSilenced;
	SendWeaponAnim(m_bSilenced ? M4_ADD_SILENCER : M4_DETACH_SILENCER);

	const float now = m_pLayer->GetWeaponTimeBase(UsePredicting());
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(2.0f);
	m_flNextSecondaryAttack = now + 2.0f;
	m_flTimeWeaponIdle = now + 2.0f;
}

void CM4WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;

	const int reloadClipSize = m_iClip > 0 ? M4_MAX_CLIP + 1 : M4_MAX_CLIP;
	if (m_iClip >= reloadClipSize)
		return;

	if (DefaultReload(reloadClipSize, m_bSilenced ? M4_RELOAD : M4_UNSIL_RELOAD, 3.5f))
	{
#ifndef CLIENT_DLL
		CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
		player->pev->maxspeed = 170; // Замедление игрока при перезарядке
#endif
	}
}

void CM4WeaponContext::WeaponIdle()
{
	ResetEmptySound();
	m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->pev->maxspeed = 230; // Замедление игрока при ношении М4
#endif
	SendWeaponAnim(m_bSilenced ? M4_IDLE : M4_UNSIL_IDLE);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
}

void CM4WeaponContext::Holster()
{
	m_fInReload = FALSE; // cancel any reload in progress.
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->pev->maxspeed = 0; //Сброс скорости игрока
#endif

	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
	SendWeaponAnim(m_bSilenced ? M4_IDLE : M4_UNSIL_IDLE);
}
