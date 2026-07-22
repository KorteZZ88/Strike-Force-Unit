#include "wrench.h"
#include <utility>

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapon_wrench.h"
#include "buildable.h"
#endif

CWrenchWeaponContext::CWrenchWeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_WRENCH;
	m_iClip = -1;
	m_usWrench = m_pLayer->PrecacheEvent("events/crowbar.sc");
}

int CWrenchWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(WRENCH_CLASSNAME);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iId = m_iId;
	p->iWeight = WRENCH_WEIGHT;
	return 1;
}

bool CWrenchWeaponContext::Deploy()
{
	return DefaultDeploy("models/weapon/wrench/v_wrench.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar");
}

void CWrenchWeaponContext::Holster()
{
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
	SendWeaponAnim(CROWBAR_HOLSTER);
}

void CWrenchWeaponContext::PrimaryAttack()
{
	UseTool(false);
}

void CWrenchWeaponContext::SecondaryAttack()
{
	UseTool(true);
}

void CWrenchWeaponContext::UseTool(bool dismantle)
{
	PlaybackEvent();
#ifndef CLIENT_DLL
	CWrench *weapon = static_cast<CWrench*>(m_pLayer->GetWeaponEntity());
	CBasePlayer *player = weapon->m_pPlayer;
	if( CBuildable *buildable = FindBuildableInView(player, 100.0f) )
	{
		if( dismantle )
			buildable->DismantleWithTool(player, 15.0f);
		else
			buildable->BuildWithTool(player, 15.0f);
		player->SetAnimation(PLAYER_ATTACK1);
	}
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.5f);
	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f;
}

void CWrenchWeaponContext::PlaybackEvent()
{
	WeaponEventParams params;
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usWrench;
	params.delay = 0.0f;
	params.origin = m_pLayer->GetGunPosition();
	params.angles = m_pLayer->GetViewAngles();
	params.fparam1 = params.fparam2 = 0.0f;
	params.iparam1 = params.iparam2 = 0;
	params.bparam1 = params.bparam2 = 0;
	if( m_pLayer->ShouldRunFuncs() )
		m_pLayer->PlaybackWeaponEvent(params);
}
