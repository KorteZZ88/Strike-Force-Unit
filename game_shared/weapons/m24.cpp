#include "m24.h"

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

// check VECTOR_CONE_1DEGREES macro
#define CONE_1DEGREES 0.00373

CM24WeaponContext::CM24WeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_M24;
	m_iDefaultAmmo = M24_DEFAULT_GIVE;
	m_usEvent1 = m_pLayer->PrecacheEvent("events/m24.sc");
	m_fInZoom = false;
}

int CM24WeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(M24_CLASSNAME);
	p->pszAmmo1 = "762";
	p->iMaxAmmo1 = _762_MAX_CARRY;
	p->iMaxClip = M24_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = M24_WEIGHT;
	return 1;
}

int CM24WeaponContext::GetReloadClipSize(int requestedClipSize)
{
	return requestedClipSize;
}

bool CM24WeaponContext::Deploy()
{	
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->pev->maxspeed = 220; // same movement speed as the AK-47
#endif
	return DefaultDeploy("models/weapon/m24/v_m24.mdl", "models/p_9mmAR.mdl", M24_DEPLOY, "m24");
}


void CM24WeaponContext::Holster()
{
	m_fInReload = FALSE; // cancel any reload in progress.
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->pev->maxspeed = 0;
#endif

	if (m_fInZoom) {
		SecondaryAttack();
	}

	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
	SendWeaponAnim(M24_HOLSTER);
}

void CM24WeaponContext::SecondaryAttack()
{	
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
#endif

	if (m_pLayer->GetPlayerFOV() != 0.0f)
	{
		m_pLayer->SetPlayerFOV(0.0f); // 0 means reset to default fov
		m_fInZoom = false;
#ifndef CLIENT_DLL
		player->pev->maxspeed = 220; // сброс
#endif
	}
	else
	{
		m_pLayer->SetPlayerFOV(40.0f);
		m_fInZoom = true; 
#ifndef CLIENT_DLL
		player->pev->maxspeed = 150; // замедление при зуме
#endif
	}

	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f;
}

void CM24WeaponContext::PrimaryAttack()
{	
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f);
		}

		return;
	}

	m_iClip--;

	Vector vecSrc = m_pLayer->GetGunPosition();
	matrix3x3 cameraTransform = m_pLayer->GetCameraOrientation();
	cameraTransform.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));//
	const float speed = m_pLayer->GetPlayerVelocity().Length2D();
	float spread = !IsPlayerOnGround() ? 0.2f :
		speed > 170.0f ? 0.075f :
		IsPlayerDucking() ? 0.0f : 0.007f;
	if (m_pLayer->GetPlayerFOV() == 0.0f) spread += 0.025f;
	Vector vecDir = m_pLayer->FireBullets(1, vecSrc, cameraTransform, 8192, spread, BULLET_PLAYER_762, m_pLayer->GetRandomSeed());
	KickBack(2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);

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
	player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	player->pev->effects = (int)(player->pev->effects) | EF_MUZZLEFLASH;
	player->SetAnimation(PLAYER_ATTACK1);

	if (!m_iClip && player->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		player->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif

	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(1.25f);
}

void CM24WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;

	const int reloadClipSize = m_iClip > 0 ? M24_MAX_CLIP + 1 : M24_MAX_CLIP;
	if (m_iClip >= reloadClipSize)
		return;

	if (DefaultReload(reloadClipSize, M24_RELOAD, 3.0f))
	{
#ifndef CLIENT_DLL
		CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
		player->pev->maxspeed = 150; // Замедление игрока при перезарядке
#endif
		m_pLayer->SetPlayerFOV(0.0f); // Сбросить зум
		m_fInZoom = false;
	}
}

//void CM24WeaponContext::Reload()
//{
//	bool addclip = (m_iClip > 0);
//
//	if (DefaultReload(M24_MAX_CLIP, M24_RELOAD, 2.0f))
//	{
//		if (addclip)
//			m_iClip++; // так как перед перезарядкой был 1 патрон в обойме, добавить 1 патрон в обойму ПОСЛЕ перезарядки
//#ifndef CLIENT_DLL
//		CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
//		player->pev->maxspeed = 100; // Замедление игрока при перезарядке
//#endif
//		m_pLayer->SetPlayerFOV(0.0f); // Сбросить зум
//		m_fInZoom = false;
//	}
//}

void CM24WeaponContext::WeaponIdle()
{
	ResetEmptySound();

	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;

#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->pev->maxspeed = 220;
	m_fWasReloading = false;
#endif
	SendWeaponAnim(M24_IDLE);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
}
