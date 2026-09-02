#include "xm1014.h"

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#endif

namespace
{
constexpr float XM1014_FIRE_INTERVAL = 60.0f / 170.0f;
constexpr float XM1014_BASE_SPREAD = 0.0675f * 1.07f;
}

CXM1014WeaponContext::CXM1014WeaponContext(std::unique_ptr<IWeaponLayer>&& layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_XM1014;
	m_iDefaultAmmo = XM1014_DEFAULT_GIVE;
	m_usFireEvent = m_pLayer->PrecacheEvent("events/xm1014.sc");
}

int CXM1014WeaponContext::GetItemInfo(ItemInfo* info) const
{
	info->pszName = CLASSNAME_STR(XM1014_CLASSNAME);
	info->pszAmmo1 = "buckshot";
	info->iMaxAmmo1 = XM1014_MAX_CARRY;
	info->pszAmmo2 = nullptr;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = XM1014_MAX_CLIP;
	info->iSlot = 0;
	info->iPosition = 18;
	info->iFlags = 0;
	info->iId = m_iId;
	info->iWeight = XM1014_WEIGHT;
	return 1;
}

bool CXM1014WeaponContext::Deploy()
{
	return DefaultDeploy("models/weapon/XM1014/v_xm1014.mdl",
		"models/weapon/XM1014/p_xm1014.mdl", XM1014_DRAW, "shotgun");
}

void CXM1014WeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f);
		return;
	}

	if (m_iClip <= 0)
	{
		if (m_pLayer->ShouldAutoReload()) Reload();
		PlayEmptySound();
		return;
	}

	--m_iClip;
	Vector source = m_pLayer->GetGunPosition();
	matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	float spreadCoefficient = XM1014_BASE_SPREAD;
	if (m_pLayer->GetPlayerVelocity().Length2D() > 0.0f)
		spreadCoefficient *= 1.30f;
	Vector spread = m_pLayer->FireBullets(8, source, camera, 3000.0f,
		spreadCoefficient, BULLET_PLAYER_BUCKSHOT, m_pLayer->GetRandomSeed());

	const bool onGround = IsPlayerOnGround();
	KickBack(static_cast<float>(m_pLayer->GetRandomInt(m_pLayer->GetRandomSeed() + 1,
		onGround ? 3 : 7, onGround ? 5 : 10)), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);

	WeaponEventParams params{};
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usFireEvent;
	params.origin = source;
	params.angles = camera.GetAngles();
	params.fparam1 = spread.x;
	params.fparam2 = spread.y;
	params.iparam1 = m_pLayer->GetRandomInt(m_pLayer->GetRandomSeed() + 5, 0, 1);
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);

#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = NORMAL_GUN_FLASH;
	SetBits(player->pev->effects, EF_MUZZLEFLASH);
	if (!m_iClip && player->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		player->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif

	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(ConfigFireInterval(XM1014_FIRE_INTERVAL));
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) +
		(m_iClip ? 2.0f : 0.75f);
	m_fInSpecialReload = 0;
}

void CXM1014WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0 || m_iClip == XM1014_MAX_CLIP ||
		m_flNextPrimaryAttack > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;

	if (m_fInSpecialReload == 0)
	{
		m_fInSpecialReload = 1;
		SendWeaponAnim(XM1014_START_RELOAD);
		m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f;
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.5f);
		m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
		m_fInSpecialReload = 2;
		SendWeaponAnim(XM1014_RELOAD);
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.48f;
	}
	else
	{
		++m_iClip;
		m_pLayer->SetPlayerAmmo(m_iPrimaryAmmoType, m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) - 1);
		m_fInSpecialReload = 1;
	}
}

void CXM1014WeaponContext::WeaponIdle()
{
	ResetEmptySound();
	m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	if (m_flTimeWeaponIdle >= m_pLayer->GetWeaponTimeBase(UsePredicting())) return;

	if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pLayer->ShouldAutoReload() &&
		m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
	{
		Reload();
	}
	else if (m_fInSpecialReload != 0)
	{
		if (m_iClip < XM1014_MAX_CLIP && m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
			Reload();
		else
		{
			SendWeaponAnim(XM1014_AFTER_RELOAD);
			m_fInSpecialReload = 0;
			m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.8f;
		}
	}
	else
	{
		SendWeaponAnim(XM1014_IDLE);
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 3.0f;
	}
}
