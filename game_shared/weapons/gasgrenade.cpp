#include "gasgrenade.h"
#include <utility>
#ifndef CLIENT_DLL
#include "weapon_gasgrenade.h"
#include "gas_grenade.h"
#endif

enum gasgrenade_e { GAS_IDLE, GAS_FIDGET, GAS_PINPULL, GAS_THROW1, GAS_THROW2, GAS_THROW3, GAS_HOLSTER, GAS_DRAW };

CGasGrenadeWeaponContext::CGasGrenadeWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) :
	CBaseWeaponContext(std::move(layer)), m_flStartThrow(0), m_flReleaseThrow(0), m_bWeakThrow(false)
{
	m_iId = WEAPON_GASGRENADE;
	m_iDefaultAmmo = GASGRENADE_DEFAULT_GIVE;
}

int CGasGrenadeWeaponContext::GetItemInfo(ItemInfo* p) const
{
	p->pszName = CLASSNAME_STR(GASGRENADE_CLASSNAME); p->pszAmmo1 = "GasGrenade";
	p->iMaxAmmo1 = 1; p->pszAmmo2 = NULL; p->iMaxAmmo2 = -1; p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3; p->iPosition = 6; p->iId = m_iId; p->iWeight = GASGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE; return 1;
}

bool CGasGrenadeWeaponContext::Deploy() { m_flReleaseThrow = -1; m_bWeakThrow = false; return DefaultDeploy("models/weapon/flashbang/v_flashbang.mdl", "models/weapon/flashbang/w_flashbang.mdl", GAS_DRAW, "crowbar"); }
bool CGasGrenadeWeaponContext::CanHolster() { return m_flStartThrow == 0; }
void CGasGrenadeWeaponContext::Holster() { m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + .5f); if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0) SendWeaponAnim(GAS_HOLSTER); }
void CGasGrenadeWeaponContext::PrimaryAttack() { if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType)<=0) {
#ifndef CLIENT_DLL
	static_cast<CGasGrenadeWeapon*>(m_pLayer->GetWeaponEntity())->RetireWeapon();
#endif
	return; } if (!m_flStartThrow) { m_bWeakThrow=false; m_flStartThrow=m_pLayer->GetTime(); m_flReleaseThrow=0; m_flTimeWeaponIdle=m_pLayer->GetWeaponTimeBase(UsePredicting())+.5f; SendWeaponAnim(GAS_PINPULL); } }
void CGasGrenadeWeaponContext::SecondaryAttack() { if (!m_flStartThrow && m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0) { m_bWeakThrow=true; m_flStartThrow=m_pLayer->GetTime(); m_flReleaseThrow=0; m_flTimeWeaponIdle=m_pLayer->GetWeaponTimeBase(UsePredicting())+.5f; SendWeaponAnim(GAS_PINPULL); } }

void CGasGrenadeWeaponContext::WeaponIdle()
{
	if (m_flReleaseThrow == 0 && m_flStartThrow) m_flReleaseThrow = m_pLayer->GetTime();
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	if (m_flStartThrow) {
		Vector ang=m_pLayer->GetViewAngles(); ang.x=ang.x<0?-10+ang.x*(80.f/90.f):-10+ang.x*(100.f/90.f);
		float velocity=(90-ang.x)*6.5f; if (velocity>1000) velocity=1000; if (m_bWeakThrow) velocity*=.5f;
#ifndef CLIENT_DLL
		CGasGrenadeWeapon* weapon=static_cast<CGasGrenadeWeapon*>(m_pLayer->GetWeaponEntity()); UTIL_MakeVectors(ang);
		CGasGrenade::ShootTimed(weapon->m_pPlayer->pev, weapon->m_pPlayer->pev->origin+weapon->m_pPlayer->pev->view_ofs+gpGlobals->v_forward*16, gpGlobals->v_forward*velocity+weapon->m_pPlayer->pev->velocity, 3.f);
		weapon->m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#endif
		SendWeaponAnim(velocity<500?GAS_THROW1:(velocity<1000?GAS_THROW2:GAS_THROW3)); m_flStartThrow=0; m_bWeakThrow=false;
		m_flNextPrimaryAttack=GetNextPrimaryAttackDelay(.5f); m_flTimeWeaponIdle=m_pLayer->GetWeaponTimeBase(UsePredicting())+.5f;
		m_pLayer->SetPlayerAmmo(m_iPrimaryAmmoType,m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType)-1); return;
	}
	if (m_flReleaseThrow>0) { m_flStartThrow=0; if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType)>0) SendWeaponAnim(GAS_DRAW);
#ifndef CLIENT_DLL
		else { CGasGrenadeWeapon* w=static_cast<CGasGrenadeWeapon*>(m_pLayer->GetWeaponEntity()); w->RetireWeapon(); return; }
#endif
		m_flReleaseThrow=-1; m_flTimeWeaponIdle=m_pLayer->GetWeaponTimeBase(UsePredicting())+10; return; }
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType)>0) { SendWeaponAnim(GAS_IDLE); m_flTimeWeaponIdle=m_pLayer->GetWeaponTimeBase(UsePredicting())+10; }
}
