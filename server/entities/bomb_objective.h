#pragma once
#include "cbase.h"
class CObjectiveBomb : public CBaseEntity
{
	DECLARE_CLASS(CObjectiveBomb, CBaseEntity);
public:
	void Spawn() override; void Precache() override; int ObjectCaps() override { return BaseClass::ObjectCaps() | FCAP_CONTINUOUS_USE; }
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	void BombThink(); void Explode(); DECLARE_DATADESC();
	int SecondsRemaining() const { return Q_max(0, (int)ceilf(m_flExplodeTime - gpGlobals->time)); }
private: EHANDLE m_hDefuser; float m_flDefuseStart; float m_flExplodeTime; float m_flNextFreq; float m_flNextFreqInterval; float m_flNextBeep; int m_iCurWave;
};
