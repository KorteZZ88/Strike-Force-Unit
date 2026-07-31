#include "tmp.h"

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#endif

CTMPWeaponContext::CTMPWeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_TMP;
	m_iDefaultAmmo = TMP_DEFAULT_GIVE;
	m_usEvent = m_pLayer->PrecacheEvent("events/tmp.sc");
}

int CTMPWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(TMP_CLASSNAME);
	p->pszAmmo1 = "9mm_tmp";
	p->iMaxAmmo1 = TMP_MAX_CLIP * TMP_MAX_SPARE_MAGAZINES;
	p->iMaxClip = TMP_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = TMP_WEIGHT;
	return 1;
}

bool CTMPWeaponContext::Deploy()
{
	return DefaultDeploy("models/weapon/TMP/v_tmp.mdl", "models/weapon/TMP/w_tmp.mdl", TMP_DEPLOY, "mp5");
}

void CTMPWeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(60.0f / 850.0f);
		return;
	}

	--m_iClip;
	Vector source = m_pLayer->GetGunPosition();
	matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	// Halfway between the MP5 and this mod's MAC-10, including movement spread.
	float spread = Q_max(0.002f, GetCs16AutomaticSpread(Cs16AutomaticProfile::MP5Navy)) * 1.05f;
	if (m_pLayer->GetPlayerVelocity().Length2D() > 0.0f)
		spread *= 1.35f;
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
	player->m_iWeaponVolume = QUIET_GUN_VOLUME;
	player->m_iWeaponFlash = 0;
	player->pev->effects &= ~EF_MUZZLEFLASH;
	player->SetAnimation(PLAYER_ATTACK1);
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(60.0f / 850.0f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
}

void CTMPWeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;
	const int reloadClipSize = m_iClip > 0 ? TMP_MAX_CLIP + 1 : TMP_MAX_CLIP;
	if (m_iClip >= reloadClipSize)
		return;
	DefaultReload(reloadClipSize, TMP_RELOAD, 2.4f);
}

void CTMPWeaponContext::WeaponIdle()
{
	ResetEmptySound();
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;
	SendWeaponAnim(TMP_IDLE);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
}
