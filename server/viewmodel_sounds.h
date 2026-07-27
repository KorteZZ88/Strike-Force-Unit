#pragma once

class CBasePlayerWeapon;

void PrecacheViewModelSounds(const char* modelName);
void PlayViewModelSounds(CBasePlayerWeapon* weapon, int sequence);
void CancelViewModelSounds(CBasePlayerWeapon* weapon);
void UpdateViewModelSounds();
