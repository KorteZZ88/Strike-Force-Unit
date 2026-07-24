#pragma once
#include "ggrenade.h"
class CGasGrenade : public CGrenade
{
	DECLARE_CLASS(CGasGrenade, CGrenade);
public:
	static CGasGrenade* ShootTimed(entvars_t*,Vector,Vector,float); void DetonateGas(); void GasThink(); void BounceSound() override;
	float m_flGasEnd; float m_flModelEnd; float m_flNextCloud;
	DECLARE_DATADESC();
};
