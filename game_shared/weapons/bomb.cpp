#include "bomb.h"
#include <utility>
#ifndef CLIENT_DLL
#include "weapon_bomb.h"
#include "player.h"
#include "gamerules.h"
#include "user_messages.h"
#endif
CBombWeaponContext::CBombWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer)), m_flPlantStart(0), m_bPlantReady(false) { m_iId = WEAPON_BOMB; m_iDefaultAmmo = 1; }
int CBombWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(BOMB_CLASSNAME); p->pszAmmo1 = nullptr; p->iMaxAmmo1 = -1; p->pszAmmo2 = nullptr; p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP; p->iSlot = 4; p->iPosition = 7; p->iFlags = ITEM_FLAG_LIMITINWORLD; p->iId = m_iId; p->iWeight = -20; return 1;
}
bool CBombWeaponContext::Deploy() { m_flPlantStart = 0; m_bPlantReady = false; return DefaultDeploy("models/weapon/Bomb/v_c4.mdl", "models/weapon/Bomb/p_c4.mdl", BOMB_DRAW, "trip"); }
void CBombWeaponContext::Holster()
{
	m_flPlantStart = 0;
	m_bPlantReady = false;
#ifndef CLIENT_DLL
	auto *w = static_cast<CBombWeapon*>(m_pLayer->GetWeaponEntity()); MESSAGE_BEGIN(MSG_ONE, gmsgActionBar, NULL, w->m_pPlayer->pev); WRITE_BYTE(0); WRITE_SHORT(0); MESSAGE_END();
#endif
}
void CBombWeaponContext::PrimaryAttack()
{
#ifndef CLIENT_DLL
	auto *w = static_cast<CBombWeapon*>(m_pLayer->GetWeaponEntity()); CBasePlayer *p = w->m_pPlayer;
	if (!g_pGameRules || !g_pGameRules->CanPlantBomb(p)) { if(m_flPlantStart>0){p->pev->maxspeed=0;SendWeaponAnim(m_bPlantReady?BOMB_DRAW:BOMB_IDLE);MESSAGE_BEGIN(MSG_ONE,gmsgActionBar,NULL,p->pev);WRITE_BYTE(0);WRITE_SHORT(0);MESSAGE_END();}m_flPlantStart=0;m_bPlantReady=false;return; }
	if (m_flPlantStart <= 0) { m_flPlantStart = gpGlobals->time;m_bPlantReady=false;SendWeaponAnim(BOMB_PRESSBUTTON);EMIT_SOUND(p->edict(),CHAN_WEAPON,"weapons/Bomb/c4_click.wav",1.0f,ATTN_NORM);MESSAGE_BEGIN(MSG_ONE,gmsgActionBar,NULL,p->pev);WRITE_BYTE(1);WRITE_SHORT(30);MESSAGE_END(); }
	Vector velocity = p->GetAbsVelocity();
	velocity.x = velocity.y = 0;
	p->SetAbsVelocity(velocity);
	p->pev->maxspeed = 1;
	const float elapsed=gpGlobals->time-m_flPlantStart;
	if(elapsed>=2.25f&&!m_bPlantReady){m_bPlantReady=true;SendWeaponAnim(BOMB_DROP);}
	if (elapsed >= 3.0f) { m_flPlantStart=0;m_bPlantReady=false;MESSAGE_BEGIN(MSG_ONE,gmsgActionBar,NULL,p->pev);WRITE_BYTE(0);WRITE_SHORT(0);MESSAGE_END();g_pGameRules->PlantBomb(p);return; }
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.05f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.1f;
}
void CBombWeaponContext::PrimaryAttackReleased()
{
#ifndef CLIENT_DLL
	if(m_flPlantStart>0){auto*w=static_cast<CBombWeapon*>(m_pLayer->GetWeaponEntity());w->m_pPlayer->pev->maxspeed=0;SendWeaponAnim(m_bPlantReady?BOMB_DRAW:BOMB_IDLE);MESSAGE_BEGIN(MSG_ONE,gmsgActionBar,NULL,w->m_pPlayer->pev);WRITE_BYTE(0);WRITE_SHORT(0);MESSAGE_END();m_flPlantStart=0;m_bPlantReady=false;}
#endif
}
void CBombWeaponContext::WeaponIdle() {
#ifndef CLIENT_DLL
	// ItemPostFrame calls WeaponIdle immediately when attack is released, even
	// while the idle animation timer is still in the future. Cancel planting
	// before that timer check so partial progress can never survive a release.
	if(m_flPlantStart>0){auto*w=static_cast<CBombWeapon*>(m_pLayer->GetWeaponEntity());w->m_pPlayer->pev->maxspeed=0;SendWeaponAnim(m_bPlantReady?BOMB_DRAW:BOMB_IDLE);MESSAGE_BEGIN(MSG_ONE,gmsgActionBar,NULL,w->m_pPlayer->pev);WRITE_BYTE(0);WRITE_SHORT(0);MESSAGE_END();m_flPlantStart=0;m_bPlantReady=false;}
#endif
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	SendWeaponAnim(BOMB_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 31.0f / 12.0f; }
