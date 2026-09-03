#pragma once

#include "cbase.h"

class CSFUDoor;

class CSFUC2Charge : public CBaseAnimating
{
	DECLARE_CLASS( CSFUC2Charge, CBaseAnimating );
public:
	void Spawn();
	void Precache();
	void Use( CBaseEntity *activator, CBaseEntity *caller, USE_TYPE useType, float value );
	void FollowDoorThink();
	bool AttachToDoor( CSFUDoor *door, CBasePlayer *owner, float sideSign );
	bool IsAttachedTo( CBaseEntity *door ) { return (CBaseEntity *)m_hDoor == door; }
	DECLARE_DATADESC();
private:
	void DirectedDamage( const Vector &mount, const Vector &nearNormal );
	bool VisibleForBlast( CBaseEntity *target, const Vector &source );
	EHANDLE m_hDoor;
	float m_flSideSign;
	bool m_bDetonated;
};
