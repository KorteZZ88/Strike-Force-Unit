#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#define SF_SFU_DOOR_START_LOCKED BIT( 0 )
#define SF_SFU_DOOR_USABLE       BIT( 1 )
#define SF_SFU_DOOR_NO_LOCKPICK  BIT( 2 )

enum SFUDoorOpenMode
{
	SFU_DOOR_OPEN_BOTH = 0,
	SFU_DOOR_OPEN_CLOCKWISE,
	SFU_DOOR_OPEN_COUNTERCLOCKWISE,
};

enum SFUDoorState
{
	SFU_DOOR_CLOSED = 0,
	SFU_DOOR_OPENING,
	SFU_DOOR_OPEN,
	SFU_DOOR_CLOSING,
};

enum SFUDoorMaterial
{
	SFU_DOOR_MATERIAL_WOOD = 0,
	SFU_DOOR_MATERIAL_METAL,
};

class CSFUDoor : public CBaseToggle
{
	DECLARE_CLASS( CSFUDoor, CBaseToggle );

public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Touch( CBaseEntity *pOther );
	void Blocked( CBaseEntity *pOther );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType ) override;
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType ) override;
	int ObjectCaps( void );
	void SetObjectCollisionBox( void );
	bool RamHit( CBasePlayer *player );
	bool C2Breach( CBaseEntity *activator, float installedSideSign );
	bool GetChargeMount( Vector &origin ) const;
	bool GetCameraMount( Vector &origin ) const;
	Vector GetDoorNormal() const;
	bool CanPlaceC2( CBasePlayer *player ) const { return PlayerLooksAtLock( player ); }
	bool CanUseUnderDoorCamera() const;

	DECLARE_DATADESC();

private:
	void AutoSetSize( void );
	void Open( CBaseEntity *pActivator, bool careful = false );
	void Close( CBaseEntity *pActivator );
	void ArriveOpen( void );
	void ArriveClosed( void );
	void AutoClose( void );
	void UnlockThink( void );
	bool StartUnlock( CBaseEntity *pActivator );
	void CancelUnlock( void );
	bool UnlockerStillValid( void );
	bool PlayerLooksAtLock( CBasePlayer *player ) const;
	void HoldUnlockerStill( CBasePlayer *player );
	bool IsM3Attack( entvars_t *pevAttacker ) const;
	void BreakLock( CBaseEntity *pAttacker );
	void BreakHinge( bool top, const Vector &shotDirection );
	void DetachDoor( const Vector &shotDirection );
	void DetachedThink( void );
	void FreezeDetachedDoor( void );
	void BeginFreeSwing( void );
	void EndFreeSwing( void );
	void FreeSwingThink( void );
	void ApplyBodyPush( CBaseEntity *pOther );
	void ApplyBulletPush( float damage, const Vector &shotDirection, const Vector &hitPosition );
	void DestroyByC4( CBaseEntity *inflictor, CBaseEntity *attacker );
	void SpawnC4Debris();
	float SelectOpenSign( CBaseEntity *pActivator ) const;
	Vector OpenAngles( float sign ) const;

	int m_iOpenMode;
	int m_iDoorMaterial;
	int m_iDoorState;
	bool m_bLocked;
	bool m_bTriedOpposite;
	float m_flOpenAngle;
	float m_flCarefulAngle;
	float m_flCurrentOpenAngle;
	float m_flOpenSign;
	float m_flUnlockStart;
	float m_flUnlockHealth;
	float m_flLockHealth;
	float m_flTopHingeHealth;
	float m_flBottomHingeHealth;
	Vector m_vecClosedAngles;
	Vector m_vecHingeOrigin;
	bool m_bLockBroken;
	bool m_bTopHingeBroken;
	bool m_bBottomHingeBroken;
	bool m_bDetached;
	bool m_bFreeSwing;
	bool m_bPhysicsClosing;
	string_t m_iszCollisionModel;
	string_t m_iszLockedSound;
	string_t m_iszOnOpened;
	string_t m_iszOnClosed;
	float m_flNextLockedSound;
	float m_flDetachedRestStart;
	bool m_bDetachedFrozen;
	bool m_bC4Destroyed;
	EHANDLE m_hUnlocker;
	CBasePlayerItem *m_pUnlockWeapon;
};
