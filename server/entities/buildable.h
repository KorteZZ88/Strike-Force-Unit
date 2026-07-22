#pragma once

#include "cbase.h"

class CBasePlayer;

enum BuildableType
{
	BUILDABLE_SANDBAG = 1,
	BUILDABLE_AMMO_BOX = 2,
	BUILDABLE_BASE = 3,
	BUILDABLE_PLAYER_SPAWN = 4,
};

class CBuildable : public CBaseAnimating
{
	DECLARE_CLASS( CBuildable, CBaseAnimating );

public:
	void Spawn( void );
	void Precache( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	int ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void SetObjectCollisionBox( void );

	void BeginPreview( CBasePlayer *pOwner, int buildType );
	void UpdatePreview( CBasePlayer *pOwner );
	BOOL CanPlace( CBasePlayer *pOwner );
	void ConfirmPlacement( void );
	void CancelPreview( void );
	BOOL IsPreview( void ) const { return m_bPreview; }
	BOOL IsUnderConstruction( void ) const { return m_bPlaced && m_flBuildProgress < 100.0f; }
	BOOL IsPlaced( void ) const { return m_bPlaced; }
	BOOL IsAmmoBox( void ) const { return m_iBuildType == BUILDABLE_AMMO_BOX; }
	BOOL IsBase( void ) const { return m_iBuildType == BUILDABLE_BASE; }
	BOOL IsPlayerSpawn( void ) const { return m_iBuildType == BUILDABLE_PLAYER_SPAWN; }
	CBuildable *GetResourceBase( void ) { return (CBuildable *)(CBaseEntity *)m_hResourceBase; }
	BOOL IsPlayerSpawnOperational( void );
	BOOL IsFriendlyTo( CBasePlayer *player );
	float GetBuildProgress( void ) const { return m_flBuildProgress; }
	float GetBuildHealth( void ) const { return pev->health; }
	BOOL BuildWithTool( CBasePlayer *pPlayer, float amount );
	BOOL DismantleWithTool( CBasePlayer *pPlayer, float amount );
	BOOL SpendAmmoPoints( int amount );
	BOOL SpendBuildPoints( int amount );
	int GetAmmoPoints( void ) const { return m_iAmmoPoints; }
	int GetBuildPoints( void ) const { return m_iBuildPoints; }
	void SetToolHighlighted( BOOL highlighted );

	DECLARE_DATADESC();

private:
	void ApplyBuildTypeModel( void );
	void FinishBuilding( void );
	void EnableCollisionWhenClear( void );
	void BreakApart( BOOL allowAmmoExplosion = TRUE );

	EHANDLE m_hBuilder;
	string_t m_iszBuilderTeam;
	int m_iBuilderTeam;
	EHANDLE m_hResourceBase;
	BOOL m_bPreview;
	BOOL m_bPlaced;
	BOOL m_bValidPosition;
	int m_iBuildType;
	float m_flBuildProgress;
	BOOL m_bPendingSolid;
	int m_iAmmoPoints;
	int m_iBuildPoints;
};

CBuildable *FindBuildableInView( CBasePlayer *pPlayer, float range );
CBuildable *FindNearestBase( const Vector &origin );
CBuildable *FindNearestFriendlyBase( CBasePlayer *player, const Vector &origin );
CBuildable *FindSpawnForBase( CBuildable *base );
CBuildable *FindPlayerSpawnByNumber( CBasePlayer *player, int number );
