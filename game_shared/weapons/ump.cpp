#include "ump.h"

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#endif

CUMPWeaponContext::CUMPWeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_UMP;
	m_iDefaultAmmo = UMP_DEFAULT_GIVE;
	m_usEvent = m_pLayer->PrecacheEvent("events/ump.sc");
}

int CUMPWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(UMP_CLASSNAME);
	p->pszAmmo1 = "45acp_ump";
	p->iMaxAmmo1 = UMP_MAX_CLIP * UMP_MAX_SPARE_MAGAZINES;
	p->iMaxClip = UMP_MAX_CLIP;
	// HUD weapon-selection slot 1 (zero-based index 0), first free subslot.
	p->iSlot = 0;
	p->iPosition = 9;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = UMP_WEIGHT;
	return 1;
}

bool CUMPWeaponContext::Deploy()
{
	return DefaultDeploy("models/weapon/UMP/v_ump45.mdl", "models/weapon/UMP/p_ump45.mdl", UMP_DEPLOY, "mp5");
}

void CUMPWeaponContext::PrimaryAttack()
{
	const float cycleTime = 60.0f / 600.0f;
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
	// Midway between the MP5 and MAC-10, both standing and moving.
	float spread = Q_max(0.003f, GetCs16AutomaticSpread(Cs16AutomaticProfile::MP5Navy)) * 1.075f;
	if (m_pLayer->GetPlayerVelocity().Length2D() > 0.0f)
		spread *= 1.525f;
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

void CUMPWeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;
	const int reloadClipSize = m_iClip > 0 ? UMP_MAX_CLIP + 1 : UMP_MAX_CLIP;
	if (m_iClip >= reloadClipSize)
		return;
	DefaultReload(reloadClipSize, UMP_RELOAD, 3.5f);
}

void CUMPWeaponContext::WeaponIdle()
{
	ResetEmptySound();
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;
	SendWeaponAnim(UMP_IDLE);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 20.0f;
}
