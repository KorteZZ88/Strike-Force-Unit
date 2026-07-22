#pragma once

#include "cbase.h"
#include "ggrenade.h"

class CTimedSatchelBomb : public CGrenade
{
	DECLARE_CLASS(CTimedSatchelBomb, CGrenade);
public:
	void Spawn() override;
	void Precache() override;
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) override;
	void TimedDetonate();
	DECLARE_DATADESC();
};

class CTimedSatchelPreview : public CBaseEntity
{
	DECLARE_CLASS(CTimedSatchelPreview, CBaseEntity);
public:
	void Spawn() override;
	void PreviewThink();
	bool CanPlace() const { return m_bCanPlace != FALSE; }
	void PlaceBomb(int timerSeconds);
	DECLARE_DATADESC();

private:
	BOOL m_bCanPlace;
	EHANDLE m_hSurface;
};
