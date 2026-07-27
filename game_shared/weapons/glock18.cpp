#include "glock18.h"
#include <utility>
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "user_messages.h"
#endif

CGlock18WeaponContext::CGlock18WeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_GLOCK18; m_iDefaultAmmo = GLOCK18_DEFAULT_GIVE; m_usFire = m_pLayer->PrecacheEvent("events/glock18.sc");
}
int CGlock18WeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName=CLASSNAME_STR(GLOCK18_CLASSNAME);p->pszAmmo1="9mm_glock18";p->iMaxAmmo1=80;p->pszAmmo2=NULL;p->iMaxAmmo2=-1;
	p->iMaxClip=GLOCK18_MAX_CLIP;p->iSlot=1;p->iPosition=0;p->iFlags=0;p->iId=m_iId;p->iWeight=GLOCK18_WEIGHT;return 1;
}
bool CGlock18WeaponContext::Deploy(){return DefaultDeploy("models/weapon/glock18/v_glock18.mdl","models/weapon/glock18/p_glock18.mdl",GLOCK18_DRAW,"onehanded");}
void CGlock18WeaponContext::PrimaryAttack(){Fire(m_bFullAuto?0.06f:0.025f,m_bFullAuto?0.05f:0.20f);}
void CGlock18WeaponContext::SecondaryAttack()
{
	m_bFullAuto=!m_bFullAuto;const float now=m_pLayer->GetWeaponTimeBase(UsePredicting());m_flNextSecondaryAttack=now+0.3f;
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	MESSAGE_BEGIN(MSG_ONE, gmsgTextMsg, NULL, player->pev);
		WRITE_BYTE(HUD_PRINTCENTER);
		WRITE_STRING(m_bFullAuto ? "Fire mode: Full-auto" : "Fire mode: Semi-auto");
		WRITE_STRING("");
		WRITE_STRING("");
		WRITE_STRING("");
		WRITE_STRING("");
	MESSAGE_END();
#endif
}
void CGlock18WeaponContext::Fire(float spread,float cycleTime)
{
	if(m_iClip<=0){if(m_fFireOnEmpty){PlayEmptySound();m_flNextPrimaryAttack=GetNextPrimaryAttackDelay(0.2f);}return;}--m_iClip;SendWeaponAnim(m_iClip?GLOCK18_SHOOT:GLOCK18_SHOOT_EMPTY);
#ifndef CLIENT_DLL
	CBasePlayer*p=m_pLayer->GetWeaponEntity()->m_pPlayer;p->SetAnimation(PLAYER_ATTACK1);p->pev->effects|=EF_MUZZLEFLASH;p->m_iWeaponVolume=NORMAL_GUN_VOLUME;p->m_iWeaponFlash=NORMAL_GUN_FLASH;
#endif
	Vector src=m_pLayer->GetGunPosition();matrix3x3 aim=m_pLayer->GetCameraOrientation();Vector dir=m_pLayer->FireBullets(1,src,aim,8192,spread,BULLET_PLAYER_9MM,m_pLayer->GetRandomSeed());m_flNextPrimaryAttack=GetNextPrimaryAttackDelay(cycleTime);m_flNextSecondaryAttack=m_flNextPrimaryAttack;
	WeaponEventParams e{};e.flags=WeaponEventFlags::NotHost;e.eventindex=m_usFire;e.origin=src;e.angles=aim.GetAngles();e.fparam1=dir.x;e.fparam2=dir.y;e.bparam1=m_iClip==0;e.bparam2=true;if(m_pLayer->ShouldRunFuncs())m_pLayer->PlaybackWeaponEvent(e);m_flTimeWeaponIdle=m_pLayer->GetWeaponTimeBase(UsePredicting())+2.0f;
}
void CGlock18WeaponContext::Reload(){if(DefaultReload(GLOCK18_MAX_CLIP,GLOCK18_RELOAD,3.1f)){
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed=190.0f;
#endif
}}
void CGlock18WeaponContext::WeaponIdle(){ResetEmptySound();if(m_flTimeWeaponIdle>m_pLayer->GetWeaponTimeBase(UsePredicting()))return;
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed=250.0f;
#endif
	if(m_iClip){SendWeaponAnim(GLOCK18_IDLE);m_flTimeWeaponIdle=m_pLayer->GetWeaponTimeBase(UsePredicting())+5.0f;}}
