#pragma once

#include "cbase.h"

class CDroppedMagazine : public CBaseEntity
{
	DECLARE_CLASS(CDroppedMagazine, CBaseEntity);
public:
	DECLARE_DATADESC();

	void Spawn() override;
	void Precache() override;
	int ObjectCaps() override { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_ONLYDIRECT_USE; }
	void Use(CBaseEntity *activator, CBaseEntity *caller, USE_TYPE useType, float value) override;
	void MagazineTouch(CBaseEntity *other);
	void SetMagazine(int magazineType, int ammoType, int rounds, int capacity);

	int m_iMagazineType;
	int m_iAmmoType;
	int m_iRounds;
	int m_iCapacity;
};

CDroppedMagazine *CreateDroppedMagazine(const Vector &origin, const Vector &velocity,
	int magazineType, int ammoType, int rounds, int capacity, edict_t *owner);
