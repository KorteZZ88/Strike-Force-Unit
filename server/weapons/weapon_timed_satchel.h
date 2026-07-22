#pragma once

#include "weapons.h"

class CTimedSatchelPreview;

class CTimedSatchel : public CBasePlayerWeapon
{
	DECLARE_CLASS(CTimedSatchel, CBasePlayerWeapon);
public:
	DECLARE_DATADESC();
	CTimedSatchel();
	void Spawn() override;
	void Precache() override;
	void CreatePreview();
	void RemovePreview();
	CTimedSatchelPreview *GetPreview();

private:
	EHANDLE m_hPreview;
};
