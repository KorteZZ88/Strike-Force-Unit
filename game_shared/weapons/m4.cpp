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
	m_usEvent2 = m_pLayer->PrecacheEvent("events/m42.sc");
}

int CM4WeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(M4_CLASSNAME);
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = _556_MAX_CARRY;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
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
	return DefaultDeploy("models/weapon/m4/v_m4.mdl", "models/p_9mmar.mdl", M4_DEPLOY, "m4");

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
	Vector spread = VECTOR_CONE_1DEGREES;
	Vector vecDir = m_pLayer->FireBullets(1, vecSrc, cameraTransform, 8192, spread.x, BULLET_PLAYER_556, m_pLayer->GetRandomSeed());

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
	params.bparam2 = 0;

	if (m_pLayer->ShouldRunFuncs()) {
		m_pLayer->PlaybackWeaponEvent(params);
	}

#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	player->m_iWeaponFlash = NORMAL_GUN_FLASH;
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
	// don't fire underwater
	if (m_pLayer->GetPlayerWaterlevel() == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f);
		return;
	}

	if (m_pLayer->GetPlayerAmmo(m_iSecondaryAmmoType) < 1)
	{
		PlayEmptySound();
		return;
	}

	m_pLayer->SetPlayerAmmo(m_iSecondaryAmmoType, m_pLayer->GetPlayerAmmo(m_iSecondaryAmmoType) - 1);

	WeaponEventParams params;
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usEvent2;
	params.delay = 0.0f;
	params.origin = m_pLayer->GetGunPosition();
	params.angles = m_pLayer->GetViewAngles();
	params.fparam1 = 0;
	params.fparam2 = 0;
	params.iparam1 = 0;
	params.iparam2 = 0;
	params.bparam1 = 0;
	params.bparam2 = 0;

	if (m_pLayer->ShouldRunFuncs()) {
		m_pLayer->PlaybackWeaponEvent(params);
	}

#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	player->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	player->m_iExtraSoundTypes = bits_SOUND_DANGER;
	player->m_flStopExtraSoundTime = gpGlobals->time + 0.2;
	player->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(player->pev->v_angle + player->pev->punchangle);

	// we don't add in player velocity anymore.
	CGrenade::ShootContact(player->pev, player->EyePosition() + gpGlobals->v_forward * 16, gpGlobals->v_forward * 1400);

	if (!player->m_rgAmmo[m_iSecondaryAmmoType])
		// HEV suit - indicate out of ammo condition
		player->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif

	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(1.0f);
	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.f;
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 5.f; // idle pretty soon after shooting.

	// m_pPlayer->pev->punchangle.x -= 10;
}

void CM4WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;

	const int reloadClipSize = m_iClip > 0 ? M4_MAX_CLIP + 1 : M4_MAX_CLIP;
	if (m_iClip >= reloadClipSize)
		return;

	if (DefaultReload(reloadClipSize, M4_RELOAD, 3.5f))
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
	SendWeaponAnim(m_pLayer->GetRandomInt(m_pLayer->GetRandomSeed(), 0, 1) == 0 ? M4_IDLE : M4_IDLE11);
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
	SendWeaponAnim(M4_FIRE1);
}