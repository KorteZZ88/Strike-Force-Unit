#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "buildable.h"
#include "buildable_shared.h"
#include "env_explosion.h"
#include "gamerules.h"

static const char *BUILDABLE_SANDBAG_MODEL = "models/box.mdl";
static const char *BUILDABLE_AMMO_BOX_MODEL = "models/w_chainammo.mdl";
static const char *BUILDABLE_BASE_MODEL = "models/w_longjump.mdl";
static const char *BUILDABLE_PLAYER_SPAWN_MODEL = "models/SFU/blacksofa02.mdl";
static const char *BUILDABLE_PLACE_SOUND = "weapons/xbow_hit1.wav";
static const float BUILDABLE_RANGE = 100.0f;
static const float BUILDABLE_MAX_HEALTH = 100.0f;
static const float BASE_MAX_HEALTH = 1000.0f;
static const float PLAYER_SPAWN_MAX_HEALTH = 500.0f;

LINK_ENTITY_TO_CLASS( buildable, CBuildable );

BEGIN_DATADESC( CBuildable )
	DEFINE_FIELD( m_hBuilder, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iszBuilderTeam, FIELD_STRING ),
	DEFINE_FIELD( m_iBuilderTeam, FIELD_INTEGER ),
	DEFINE_FIELD( m_hResourceBase, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bPreview, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPlaced, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bValidPosition, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iBuildType, FIELD_INTEGER ),
	DEFINE_FIELD( m_flBuildProgress, FIELD_FLOAT ),
	DEFINE_FIELD( m_bPendingSolid, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iAmmoPoints, FIELD_INTEGER ),
	DEFINE_FIELD( m_iBuildPoints, FIELD_INTEGER ),
	DEFINE_FUNCTION( EnableCollisionWhenClear ),
END_DATADESC()

void CBuildable::Precache( void )
{
	PRECACHE_MODEL( BUILDABLE_SANDBAG_MODEL );
	PRECACHE_MODEL( BUILDABLE_AMMO_BOX_MODEL );
	PRECACHE_MODEL( BUILDABLE_BASE_MODEL );
	PRECACHE_MODEL( BUILDABLE_PLAYER_SPAWN_MODEL );
	PRECACHE_MODEL( "models/metalplategibs.mdl" );
	PRECACHE_SOUND( BUILDABLE_PLACE_SOUND );
	PRECACHE_SOUND( "debris/bustmetal1.wav" );
	PRECACHE_SOUND( "debris/metal1.wav" );
	PRECACHE_SOUND( "debris/metal3.wav" );
}

void CBuildable::Spawn( void )
{
	Precache();
	SET_MODEL( edict(), BUILDABLE_SANDBAG_MODEL );

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_YES;
	pev->health = BUILDABLE_MAX_HEALTH;
	pev->max_health = BUILDABLE_MAX_HEALTH;
	// The client recognizes this marker and enforces green alpha blending.
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 80;
	pev->rendercolor = Vector( 64, 255, 64 );
	pev->iuser4 = BUILDABLE_PREVIEW_INVALID_MARKER;
	m_iBuilderTeam = -1;

	Vector mins, maxs;
	if( ExtractBbox( pev->sequence, mins, maxs ))
		UTIL_SetSize( pev, mins, maxs );
	else
		UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 32 ));
}

void CBuildable::BeginPreview( CBasePlayer *pOwner, int buildType )
{
	m_hBuilder = pOwner;
	m_iszBuilderTeam = ( pOwner->TeamID() && pOwner->TeamID()[0] ) ? ALLOC_STRING( pOwner->TeamID() ) : iStringNull;
	m_iBuilderTeam = g_pGameRules && pOwner->TeamID() ? g_pGameRules->GetTeamIndex( pOwner->TeamID() ) : -1;
	m_iBuildType = buildType;
	ApplyBuildTypeModel();
	m_bPreview = TRUE;
	m_bPlaced = FALSE;
	m_bValidPosition = FALSE;
	m_flBuildProgress = 0;
	m_bPendingSolid = FALSE;
	m_iAmmoPoints = buildType == BUILDABLE_BASE ? BASE_STARTING_AMMO_POINTS : 0;
	m_iBuildPoints = buildType == BUILDABLE_BASE ? BASE_STARTING_BUILD_POINTS : 0;
	pev->health = buildType == BUILDABLE_BASE ? BASE_MAX_HEALTH :
		( buildType == BUILDABLE_PLAYER_SPAWN ? PLAYER_SPAWN_MAX_HEALTH : BUILDABLE_MAX_HEALTH );
	pev->max_health = pev->health;
	pev->owner = pOwner->edict();
	UpdatePreview( pOwner );
}

void CBuildable::ApplyBuildTypeModel( void )
{
	const char *model = m_iBuildType == BUILDABLE_AMMO_BOX ? BUILDABLE_AMMO_BOX_MODEL :
		( m_iBuildType == BUILDABLE_BASE ? BUILDABLE_BASE_MODEL :
		( m_iBuildType == BUILDABLE_PLAYER_SPAWN ? BUILDABLE_PLAYER_SPAWN_MODEL : BUILDABLE_SANDBAG_MODEL ));
	SET_MODEL( edict(), model );
	Vector mins, maxs;
	if( ExtractBbox( pev->sequence, mins, maxs ))
		UTIL_SetSize( pev, mins, maxs );
}

void CBuildable::UpdatePreview( CBasePlayer *pOwner )
{
	if( !m_bPreview || !pOwner || !pOwner->IsAlive() )
	{
		CancelPreview();
		return;
	}

	UTIL_MakeVectors( pOwner->pev->v_angle );
	const Vector start = pOwner->EyePosition();
	const Vector end = start + gpGlobals->v_forward * BUILDABLE_RANGE;
	TraceResult tr;
	UTIL_TraceLine( start, end, dont_ignore_monsters, pOwner->edict(), &tr );

	const BOOL hitSurface = tr.flFraction < 1.0f && tr.pHit != edict();
	Vector origin = hitSurface ? tr.vecEndPos + tr.vecPlaneNormal * 0.5f : end;
	if( hitSurface ) origin.z -= pev->mins.z;
	SetAbsAngles( Vector( 0, pOwner->pev->v_angle.y, 0 ));
	UTIL_SetOrigin( this, origin );

	if( !hitSurface )
	{
		m_bValidPosition = FALSE;
		pev->iuser4 = BUILDABLE_PREVIEW_INVALID_MARKER;
		pev->rendercolor = Vector( 255, 80, 80 );
		return;
	}
	// A Base may be placed on surfaces inclined by up to 45 degrees.
	const float minimumNormalZ = IsBase() ? 0.70710678f : 0.55f;
	m_bValidPosition = tr.vecPlaneNormal.z >= minimumNormalZ;
	pev->iuser4 = m_bValidPosition ? BUILDABLE_PREVIEW_MARKER : BUILDABLE_PREVIEW_INVALID_MARKER;
	pev->rendercolor = m_bValidPosition ? Vector( 160, 210, 255 ) : Vector( 255, 80, 80 );
	if( !m_bValidPosition ) return;
	if( IsBase() )
	{
		CBuildable *nearbyBase = FindNearestFriendlyBase( pOwner, GetAbsOrigin() );
		if( nearbyBase && ( nearbyBase->Center() - GetAbsOrigin() ).Length() <= 1000.0f )
		{
			m_bValidPosition = FALSE;
			pev->iuser4 = BUILDABLE_PREVIEW_INVALID_MARKER;
			pev->rendercolor = Vector( 255, 80, 80 );
			return;
		}
	}
	else
	{
		CBuildable *nearbyBase = FindNearestFriendlyBase( pOwner, GetAbsOrigin() );
		const int cost = IsPlayerSpawn() ? PLAYER_SPAWN_BUILD_COST :
			( IsAmmoBox() ? AMMO_BOX_BUILD_COST : BOX_BUILD_COST );
		if( !nearbyBase || ( nearbyBase->Center() - GetAbsOrigin() ).Length() > 500.0f ||
			nearbyBase->GetBuildPoints() < cost ||
			( IsPlayerSpawn() && ( !g_pGameRules->IsMultiplayer() || FindSpawnForBase( nearbyBase ) != NULL )))
		{
			m_bValidPosition = FALSE;
			pev->iuser4 = BUILDABLE_PREVIEW_INVALID_MARKER;
			pev->rendercolor = Vector( 255, 80, 80 );
			return;
		}
	}

	CBaseEntity *entities[32];
	const int count = UTIL_EntitiesInBox( entities, ARRAYSIZE( entities ), pev->absmin, pev->absmax, 0 );
	for( int i = 0; i < count; ++i )
	{
		CBuildable *otherBuildable = NULL;
		if( entities[i] != this && FClassnameIs( entities[i]->pev, "buildable" ))
			otherBuildable = (CBuildable *)entities[i];
		const BOOL blockedByAmmoBox = otherBuildable && otherBuildable->IsPlaced() && otherBuildable->IsAmmoBox();
		if( entities[i] != this && entities[i] != pOwner &&
			(( entities[i]->pev->solid != SOLID_NOT && !otherBuildable ) || blockedByAmmoBox ))
		{
			m_bValidPosition = FALSE;
			pev->iuser4 = BUILDABLE_PREVIEW_INVALID_MARKER;
			pev->rendercolor = Vector( 255, 80, 80 );
			break;
		}
	}
}

BOOL CBuildable::CanPlace( CBasePlayer *pOwner )
{
	return m_bPreview && m_bValidPosition && m_hBuilder.Get() == pOwner->edict();
}

void CBuildable::ConfirmPlacement( void )
{
	if( !m_bPreview || !m_bValidPosition )
		return;
	if( !IsBase() )
		m_hResourceBase = FindNearestFriendlyBase( (CBasePlayer *)(CBaseEntity *)m_hBuilder, GetAbsOrigin() );

	m_bPreview = FALSE;
	m_bPlaced = TRUE;
	pev->owner = NULL;
	// From this point the object is under construction, not an active placement
	// preview. Keep its translucent render mode, but stop suppressing the
	// builder's weapon prediction on the client.
	pev->iuser4 = 0;
	pev->rendercolor = Vector( 255, 255, 255 );
	if( IsBase() )
	{
		m_flBuildProgress = 100.0f;
		FinishBuilding();
		EMIT_SOUND( edict(), CHAN_BODY, BUILDABLE_PLACE_SOUND, 1.0, ATTN_NORM );
		return;
	}
	// A placed object starts visually at 20% construction, while its actual
	// build progress remains zero.
	pev->renderamt = 139;
	EMIT_SOUND( edict(), CHAN_BODY, BUILDABLE_PLACE_SOUND, 1.0, ATTN_NORM );
}

void CBuildable::CancelPreview( void )
{
	if( m_bPreview )
		UTIL_Remove( this );
}

int CBuildable::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( m_bPreview || !m_bPlaced )
		return 0;

	if( m_flBuildProgress < 100.0f )
		return 0;

	const int acceptedDamage = DMG_BLAST | ( IsAmmoBox() ? DMG_BULLET : 0 );
	if( !( bitsDamageType & acceptedDamage ))
		return 0;
	if( bitsDamageType & DMG_BLAST )
	{
		const BOOL isC4 = pevInflictor && FClassnameIs( pevInflictor, "timed_satchel_bomb" );
		// The regular box resists grenades and underbarrel grenades threefold,
		// while C4 only receives 1.5x resistance so a directly attached charge
		// reliably destroys a fully repaired box despite radial damage falloff.
		flDamage *= isC4 ? ( IsBase() ? 4.0f : ( IsPlayerSpawn() ? 1.5f : ( 1.0f / 1.5f ))) :
			( IsAmmoBox() ? 0.5f : ( 1.0f / 3.0f ));
	}

	pev->health -= flDamage;
	if( pev->health <= 0 )
		BreakApart();
	return 1;
}

void CBuildable::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir,
	TraceResult *ptr, int bitsDamageType )
{
	if( !m_bPreview && m_bPlaced && ptr && ( bitsDamageType & ( DMG_BULLET | DMG_CLUB )))
	{
		UTIL_Sparks( ptr->vecEndPos );
		EMIT_SOUND( edict(), CHAN_BODY,
			RANDOM_LONG( 0, 1 ) ? "debris/metal1.wav" : "debris/metal3.wav",
			0.8f, ATTN_NORM );
	}

	BaseClass::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

BOOL CBuildable::BuildWithTool( CBasePlayer *pPlayer, float amount )
{
	if( !pPlayer || m_bPreview || !m_bPlaced )
		return FALSE;

	if( m_flBuildProgress < 100.0f )
	{
		m_flBuildProgress = Q_min( 100.0f, m_flBuildProgress + amount );
		pev->renderamt = 139 + (int)( m_flBuildProgress * 1.16f );
		EMIT_SOUND( edict(), CHAN_BODY, BUILDABLE_PLACE_SOUND, 0.7, ATTN_NORM );
		if( m_flBuildProgress >= 100.0f )
			FinishBuilding();
		return TRUE;
	}

	if( pev->health >= pev->max_health )
		return FALSE;
	const float repairAmount = IsBase() ? amount * 2.0f : amount;
	pev->health = Q_min( pev->max_health, pev->health + repairAmount );
	EMIT_SOUND( edict(), CHAN_BODY, BUILDABLE_PLACE_SOUND, 0.7, ATTN_NORM );
	return TRUE;
}

BOOL CBuildable::DismantleWithTool( CBasePlayer *pPlayer, float amount )
{
	if( !pPlayer || m_bPreview || !m_bPlaced )
		return FALSE;
	EMIT_SOUND( edict(), CHAN_BODY,
		RANDOM_LONG( 0, 1 ) ? "debris/metal1.wav" : "debris/metal3.wav",
		0.8f, ATTN_NORM );

	if( m_flBuildProgress < 100.0f )
	{
		m_flBuildProgress -= amount;
		if( m_flBuildProgress <= 0.0f )
		{
			BreakApart( FALSE );
			return TRUE;
		}
		pev->renderamt = 139 + (int)( m_flBuildProgress * 1.16f );
		return TRUE;
	}

	pev->health -= amount;
	if( pev->health <= 0.0f )
		BreakApart( FALSE );
	return TRUE;
}

BOOL CBuildable::SpendAmmoPoints( int amount )
{
	if( !IsBase() || amount < 0 || m_iAmmoPoints < amount ) return FALSE;
	m_iAmmoPoints -= amount;
	return TRUE;
}

BOOL CBuildable::SpendBuildPoints( int amount )
{
	if( !IsBase() || amount < 0 || m_iBuildPoints < amount ) return FALSE;
	m_iBuildPoints -= amount;
	return TRUE;
}

void CBuildable::SetToolHighlighted( BOOL highlighted )
{
	if( m_bPreview || !m_bPlaced || IsUnderConstruction() )
		return;

	pev->rendermode = highlighted ? kRenderTransColor : kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->rendercolor = highlighted ? Vector( 255, 0, 0 ) : Vector( 255, 255, 255 );
	pev->renderamt = 255;
}

void CBuildable::FinishBuilding( void )
{
	pev->rendermode = kRenderNormal;
	pev->renderamt = 255;
	pev->iuser4 = 0;
	pev->health = IsBase() ? BASE_MAX_HEALTH :
		( IsPlayerSpawn() ? PLAYER_SPAWN_MAX_HEALTH : BUILDABLE_MAX_HEALTH );
	pev->max_health = pev->health;
	UTIL_SetOrigin( this, GetAbsOrigin() );
	if( IsPlayerSpawn() )
	{
		pev->solid = SOLID_NOT;
		m_bPendingSolid = FALSE;
		SetThink( NULL );
		pev->nextthink = 0;
		RelinkEntity( TRUE );
		return;
	}
	m_bPendingSolid = TRUE;
	SetThink( &CBuildable::EnableCollisionWhenClear );
	pev->nextthink = gpGlobals->time;
}

void CBuildable::EnableCollisionWhenClear( void )
{
	CBaseEntity *entities[32];
	const int count = UTIL_EntitiesInBox( entities, ARRAYSIZE( entities ), pev->absmin, pev->absmax, FL_CLIENT );
	for( int i = 0; i < count; ++i )
	{
		if( entities[i] && entities[i]->IsPlayer() )
		{
			pev->nextthink = gpGlobals->time + 0.1f;
			return;
		}
	}

	if( WorldPhysic->Initialized() )
	{
		pev->solid = SOLID_CUSTOM;
		m_pUserData = WorldPhysic->CreateStaticBoxFromEntity( this );
		if( !m_pUserData )
			pev->solid = SOLID_BBOX;
	}
	else
	{
		pev->solid = SOLID_BBOX;
	}
	m_bPendingSolid = FALSE;
	SetThink( NULL );
	pev->nextthink = 0;
	UTIL_SetOrigin( this, GetAbsOrigin() );
	RelinkEntity( TRUE );
}

void CBuildable::SetObjectCollisionBox( void )
{
	// SOLID_CUSTOM uses the rotated triangle mesh for narrow-phase collision.
	// Its broad-phase bounds must enclose that same rotated model.
	TransformAABB( EntityToWorldTransform(), pev->mins, pev->maxs, pev->absmin, pev->absmax );
	pev->absmin -= Vector( 1, 1, 1 );
	pev->absmax += Vector( 1, 1, 1 );
}

void CBuildable::BreakApart( BOOL allowAmmoExplosion )
{
	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;

	if( IsBase() )
	{
		CBaseEntity *candidate = NULL;
		while(( candidate = UTIL_FindEntityInSphere( candidate, Center(), 500.0f )) != NULL )
		{
			if( candidate == this || !FClassnameIs( candidate->pev, "buildable" )) continue;
			CBuildable *buildable = (CBuildable *)candidate;
			if( !buildable->IsPlaced() || buildable->IsPreview() ||
				( !buildable->IsAmmoBox() && !buildable->IsPlayerSpawn() ) ||
				buildable->GetResourceBase() != this ) continue;
			UTIL_Remove( buildable );
		}
	}

	const Vector center = ( pev->absmin + pev->absmax ) * 0.5f;
	if( IsAmmoBox() && allowAmmoExplosion )
		ExplosionCreate( center, GetAbsAngles(), edict(), 30, TRUE );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, center );
		WRITE_BYTE( TE_BREAKMODEL );
		WRITE_COORD( center.x );
		WRITE_COORD( center.y );
		WRITE_COORD( center.z );
		WRITE_COORD( pev->size.x );
		WRITE_COORD( pev->size.y );
		WRITE_COORD( pev->size.z );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_BYTE( 10 );
		WRITE_SHORT( MODEL_INDEX( "models/metalplategibs.mdl" ));
		WRITE_BYTE( 12 );
		WRITE_BYTE( 25 );
		WRITE_BYTE( BREAK_METAL );
	MESSAGE_END();

	EMIT_SOUND( edict(), CHAN_BODY, "debris/bustmetal1.wav", 1.0, ATTN_NORM );
	UTIL_Remove( this );
}

CBuildable *FindBuildableInView( CBasePlayer *pPlayer, float range )
{
	if( !pPlayer || range <= 0.0f ) return NULL;
	UTIL_MakeVectors( pPlayer->pev->v_angle );
	const Vector start = pPlayer->GetGunPosition();
	const Vector direction = gpGlobals->v_forward;
	const Vector end = start + direction * range;

	TraceResult obstruction;
	UTIL_TraceLine( start, end, dont_ignore_monsters, pPlayer->edict(), &obstruction );
	float maximumAlong = obstruction.flFraction * range;
	if( obstruction.flFraction < 1.0f && obstruction.pHit )
	{
		CBaseEntity *hit = CBaseEntity::Instance( obstruction.pHit );
		if( hit && FClassnameIs( hit->pev, "buildable" ))
			return (CBuildable *)hit;
	}

	CBuildable *best = NULL;
	float bestAlong = maximumAlong;
	CBaseEntity *candidate = NULL;
	const Vector midpoint = start + direction * ( range * 0.5f );
	while(( candidate = UTIL_FindEntityInSphere( candidate, midpoint, range * 0.5f + 64.0f )) != NULL )
	{
		if( !FClassnameIs( candidate->pev, "buildable" )) continue;
		CBuildable *buildable = (CBuildable *)candidate;
		if( !buildable->IsPlaced() ) continue;

		const Vector toCenter = buildable->Center() - start;
		const float along = DotProduct( toCenter, direction );
		if( along < 0.0f || along > range || along > bestAlong ) continue;
		const Vector nearest = start + direction * along;
		const float radius = Q_min( 40.0f, buildable->pev->size.Length() * 0.5f + 12.0f );
		if(( buildable->Center() - nearest ).Length() <= radius )
		{
			best = buildable;
			bestAlong = along;
		}
	}
	return best;
}

CBuildable *FindNearestBase( const Vector &origin )
{
	CBuildable *nearest = NULL;
	float nearestDistance = 1.0e30f;
	CBaseEntity *candidate = NULL;
	while(( candidate = UTIL_FindEntityByClassname( candidate, "buildable" )) != NULL )
	{
		CBuildable *buildable = (CBuildable *)candidate;
		if( !buildable->IsPlaced() || buildable->IsPreview() || !buildable->IsBase() ) continue;
		const float distance = ( buildable->Center() - origin ).Length();
		if( distance < nearestDistance ) { nearest = buildable; nearestDistance = distance; }
	}
	return nearest;
}

CBuildable *FindSpawnForBase( CBuildable *base )
{
	if( !base ) return NULL;
	CBaseEntity *candidate = NULL;
	while(( candidate = UTIL_FindEntityByClassname( candidate, "buildable" )) != NULL )
	{
		CBuildable *buildable = (CBuildable *)candidate;
		if( buildable->IsPlayerSpawn() && buildable->IsPlaced() && !buildable->IsPreview() &&
			buildable->GetResourceBase() == base ) return buildable;
	}
	return NULL;
}

CBuildable *FindPlayerSpawnByNumber( CBasePlayer *player, int number )
{
	if( number < 1 ) return NULL;
	int current = 0;
	CBaseEntity *candidate = NULL;
	while(( candidate = UTIL_FindEntityByClassname( candidate, "buildable" )) != NULL )
	{
		CBuildable *buildable = (CBuildable *)candidate;
		if( !buildable->IsPlayerSpawn() || !buildable->IsFriendlyTo( player ) ||
			!buildable->IsPlaced() || buildable->IsPreview() ||
			buildable->IsUnderConstruction() ) continue;
		if( ++current == number ) return buildable;
	}
	return NULL;
}

CBuildable *FindNearestFriendlyBase( CBasePlayer *player, const Vector &origin )
{
	CBuildable *nearest = NULL;
	float nearestDistance = 1.0e30f;
	CBaseEntity *candidate = NULL;
	while(( candidate = UTIL_FindEntityByClassname( candidate, "buildable" )) != NULL )
	{
		CBuildable *base = (CBuildable *)candidate;
		if( !base->IsBase() || !base->IsPlaced() || base->IsPreview() || !base->IsFriendlyTo( player )) continue;
		const float distance = ( base->Center() - origin ).Length();
		if( distance < nearestDistance ) { nearest = base; nearestDistance = distance; }
	}
	return nearest;
}

BOOL CBuildable::IsFriendlyTo( CBasePlayer *player )
{
	if( !player ) return FALSE;
	CBasePlayer *builder = (CBasePlayer *)(CBaseEntity *)m_hBuilder;
	const char *playerTeamName = player->TeamID();
	// The saved name is authoritative. Some game-rule implementations may
	// expose incomplete team-index tables and map two different teams to the
	// same numeric index.
	if( !FStringNull( m_iszBuilderTeam ))
		return playerTeamName && playerTeamName[0] &&
			!stricmp( STRING( m_iszBuilderTeam ), playerTeamName );
	if( builder && builder->TeamID() && builder->TeamID()[0] )
		return playerTeamName && playerTeamName[0] &&
			!stricmp( builder->TeamID(), playerTeamName );
	if( builder == player ) return TRUE;
	const int playerTeam = g_pGameRules && player->TeamID() ? g_pGameRules->GetTeamIndex( player->TeamID() ) : -1;
	if( m_iBuilderTeam >= 0 || playerTeam >= 0 )
		return m_iBuilderTeam >= 0 && playerTeam == m_iBuilderTeam;
	return FALSE;
}

BOOL CBuildable::IsPlayerSpawnOperational( void )
{
	if( !IsPlayerSpawn() || !IsPlaced() || IsPreview() || IsUnderConstruction() ||
		pev->health < PLAYER_SPAWN_MAX_HEALTH * 0.60f ) return FALSE;
	CBuildable *base = GetResourceBase();
	return base && base->IsPlaced() && base->GetBuildHealth() >= 750.0f;
}
