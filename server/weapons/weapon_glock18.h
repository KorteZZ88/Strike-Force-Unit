#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
class CGlock18 : public CBasePlayerWeapon { DECLARE_CLASS(CGlock18,CBasePlayerWeapon); public: CGlock18(); void Spawn() override; void Precache() override; };
