#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>
#define WEAPON_BOMB 22
#define BOMB_CLASSNAME weapon_bomb
enum bomb_e
{
	BOMB_IDLE = 0,
	BOMB_DRAW,
	BOMB_DROP,
	BOMB_PRESSBUTTON,
};
class CBombWeaponContext : public CBaseWeaponContext
{
public:
	explicit CBombWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 5; }
	int GetItemInfo(ItemInfo *p) const override;
	bool Deploy() override;
	void Holster() override;
	void PrimaryAttack() override;
	void WeaponIdle() override;
	void PrimaryAttackReleased() override;
private:
	float m_flPlantStart;
	bool m_bPlantReady;
};
