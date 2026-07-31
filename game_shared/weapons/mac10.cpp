#include "mac10.h"

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#endif

CMac10WeaponContext::CMac10WeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_MAC10;
	m_iDefaultAmmo = MAC10_DEFAULT_GIVE;
	m_usEvent = m_pLayer->PrecacheEvent("events/mac10.sc");
}

int CMac10WeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(MAC10_CLASSNAME);
	p->pszAmmo1 = "9mm_mac10";
	p->iMaxAmmo1 = MAC10_MAX_CLIP * MAC10_MAX_SPARE_MAGAZINES;
	p->iMaxClip = MAC10_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = MAC10_WEIGHT;
	return 1;
}

bool CMac10WeaponContext::Deploy()
{
	return DefaultDeploy("models/weapon/Mac-10/v_mac10.mdl", "models/weapon/Mac-10/p_mac10.mdl", MAC10_DEPLOY, "mp5");
}

void CMac10WeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.086f);
		return;
	}

	--m_iClip;
	Vector source = m_pLayer->GetGunPosition();
	matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	// Ten percent less accurate than the MP-5. Its zero-spread first shot gets
	// a small baseline cone as well, so it can no longer land dead-center.
	float spread = Q_max(0.004f, GetCs16AutomaticSpread(Cs16AutomaticProfile::MP5Navy)) * 1.1f;
	if (m_pLayer->GetPlayerVelocity().Length2D() > 0.0f)
		spread *= 1.7f;
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
	// 1000 rounds per minute.
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.06f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
}

void CMac10WeaponContext::Reload()
{
	if (m_iClip >= MAC10_MAX_CLIP || m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;
	DefaultReload(MAC10_MAX_CLIP, MAC10_RELOAD, 3.15f);
}

void CMac10WeaponContext::WeaponIdle()
{
	ResetEmptySound();
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;
	SendWeaponAnim(MAC10_IDLE1);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 20.0f;
}
