#pragma once
#include "weapons.h"
class CMineAP : public CBasePlayerWeapon
{
 DECLARE_CLASS(CMineAP, CBasePlayerWeapon);
public:
 CMineAP();
 void Spawn() override;
 void Precache() override;
 void StartPlant();
 void CancelPlant();
 void PlantThink();
 bool FindGround(TraceResult& tr);
private:
 float m_plantEnd = 0;
 Vector m_plantPoint;
 Vector m_plantNormal;
};

bool TryDefuseMineAP(CBasePlayer* player);
