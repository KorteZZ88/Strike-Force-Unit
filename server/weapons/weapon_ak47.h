#pragma once
#include "weapons.h"
class CAK47 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CAK47, CBasePlayerWeapon);
public:
	CAK47(); void Spawn() override; void Precache() override; int AddToPlayer(CBasePlayer *player) override;
	void PlayViewModelSounds(int sequence);
	void CancelViewModelSounds();
	void ViewModelSoundThink();

private:
	int m_iViewModelSoundSequence = -1;
	int m_iNextViewModelSound = 0;
	float m_flViewModelSoundStart = 0.0f;
};
