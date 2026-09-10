#include "mine_ap.h"
#ifndef CLIENT_DLL
#include "weapon_mine_ap.h"
#endif

CMineAPWeaponContext::CMineAPWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{ m_iId = WEAPON_MINE_AP; m_iDefaultAmmo = 1; }
int CMineAPWeaponContext::GetItemInfo(ItemInfo* p) const
{
 p->pszName = "weapon_mineAP"; p->pszAmmo1 = "AP Mine"; p->iMaxAmmo1 = 1;
 p->pszAmmo2 = nullptr; p->iMaxAmmo2 = -1; p->iMaxClip = WEAPON_NOCLIP;
 p->iSlot = 4; p->iPosition = 2; p->iId = m_iId; p->iWeight = -10;
 p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE | ITEM_FLAG_NOAUTORELOAD;
 return 1;
}
bool CMineAPWeaponContext::Deploy()
{
 const bool result = DefaultDeploy("models/weapon/MineAP/v_landmine.mdl", "models/weapon/MineAP/w_landmine.mdl", 1, "trip");
 m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f);
 m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f;
 return result;
}
void CMineAPWeaponContext::Holster() { PrimaryAttackReleased(); CBaseWeaponContext::Holster(); }
void CMineAPWeaponContext::PrimaryAttack()
{
#ifndef CLIENT_DLL
 static_cast<CMineAP*>(m_pLayer->GetWeaponEntity())->StartPlant();
#endif
 m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.05f);
 m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.2f;
}
void CMineAPWeaponContext::PrimaryAttackReleased()
{
#ifndef CLIENT_DLL
 static_cast<CMineAP*>(m_pLayer->GetWeaponEntity())->CancelPlant();
#endif
}
void CMineAPWeaponContext::WeaponIdle()
{
 if(m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
 SendWeaponAnim(0);
 m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 5.0f;
}
