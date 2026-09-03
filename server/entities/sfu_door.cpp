#include "sfu_door.h"

#include "player.h"
#include "studio.h"
#include "user_messages.h"
#include "weapons.h"
#include "sfu_c2_charge.h"
#include "func_break.h"

static const char *SFU_DOOR_UNLOCK_SOUND = "buttons/latchunlocked1.wav";
static const char *SFU_DOOR_DEFAULT_LOCKED_SOUND = "buttons/latchlocked1.wav";
static const char *SFU_DOOR_BREAK_LOCK_SOUND = "debris/bustmetal1.wav";
static const char *SFU_DOOR_BREAK_HINGE_SOUND = "debris/bustmetal2.wav";
static const char *SFU_DOOR_LOCKPICK_SOUND = "doors/lockpick.wav";
static const char *SFU_DOOR_RAM_WOOD_SOUND = "debris/bustcrate1.wav";
static const char *SFU_DOOR_RAM_METAL_SOUND = "debris/metal1.wav";
static const int SFU_PLAYER_LOCK_VIEW = BIT( 30 );
static const float SFU_DOOR_PART_HEALTH = 50.0f;
static const float SFU_DOOR_BROKEN_LOCK_CRACK_ANGLE = 5.0f;
// GoldSrc/Xash coordinates use inches: three metres are about 118 units.
static const float SFU_DOOR_M3_BREACH_RANGE = 118.11f;

struct SFUDoorMaterialProfile
{
	float mass;
	float pushScale;
};

static const SFUDoorMaterialProfile g_SFUDoorMaterialProfiles[] =
{
	{ 40.0f, 1.0f }, // wood
	{ 80.0f, 0.6f }, // metal
};

static const SFUDoorMaterialProfile &SFUDoorMaterialSettings( int material )
{
	return g_SFUDoorMaterialProfiles[material == SFU_DOOR_MATERIAL_METAL ? 1 : 0];
}

LINK_ENTITY_TO_CLASS( sfu_door, CSFUDoor );

static float SFURayHitboxDistance( CSFUDoor *door, int hitboxIndex, const Vector &rayOrigin, const Vector &rayDirection, float maxDistance )
{
	studiohdr_t *header = (studiohdr_t *)GET_MODEL_PTR( door->edict() );
	if( !header || hitboxIndex < 0 || hitboxIndex >= header->numhitboxes ) return -1.0f;
	mstudiobbox_t *hitboxes = (mstudiobbox_t *)((byte *)header + header->hitboxindex);
	const mstudiobbox_t &hitbox = hitboxes[hitboxIndex];
	Vector boneOrigin, boneAngles;
	door->GetBonePosition( hitbox.bone, boneOrigin, boneAngles );
	const matrix3x4 boneTransform( boneOrigin, boneAngles );
	const Vector localOrigin = boneTransform.VectorITransform( rayOrigin );
	const Vector localDirection = boneTransform.VectorIRotate( rayDirection );
	float enterDistance = 0.0f;
	float exitDistance = maxDistance;
	for( int axis = 0; axis < 3; ++axis )
	{
		if( fabs( localDirection[axis] ) < 0.0001f )
		{
			if( localOrigin[axis] < hitbox.bbmin[axis] || localOrigin[axis] > hitbox.bbmax[axis] ) return -1.0f;
			continue;
		}
		float nearDistance = ( hitbox.bbmin[axis] - localOrigin[axis] ) / localDirection[axis];
		float farDistance = ( hitbox.bbmax[axis] - localOrigin[axis] ) / localDirection[axis];
		if( nearDistance > farDistance ) std::swap( nearDistance, farDistance );
		enterDistance = Q_max( enterDistance, nearDistance );
		exitDistance = Q_min( exitDistance, farDistance );
		if( enterDistance > exitDistance ) return -1.0f;
	}
	return exitDistance >= 0.0f && enterDistance <= maxDistance ? Q_max( enterDistance, 0.0f ) : -1.0f;
}

BEGIN_DATADESC( CSFUDoor )
	DEFINE_KEYFIELD( m_iOpenMode, FIELD_INTEGER, "open_mode" ),
	DEFINE_KEYFIELD( m_iDoorMaterial, FIELD_INTEGER, "door_material" ),
	DEFINE_KEYFIELD( m_flOpenAngle, FIELD_FLOAT, "open_angle" ),
	DEFINE_KEYFIELD( m_flCarefulAngle, FIELD_FLOAT, "careful_angle" ),
	DEFINE_KEYFIELD( m_iszCollisionModel, FIELD_STRING, "collision_model" ),
	DEFINE_KEYFIELD( m_iszLockedSound, FIELD_STRING, "locked_sound" ),
	DEFINE_KEYFIELD( m_iszOnOpened, FIELD_STRING, "OnOpened" ),
	DEFINE_KEYFIELD( m_iszOnClosed, FIELD_STRING, "OnClosed" ),
	DEFINE_FIELD( m_iDoorState, FIELD_INTEGER ),
	DEFINE_FIELD( m_bLocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bTriedOpposite, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flCurrentOpenAngle, FIELD_FLOAT ),
	DEFINE_FIELD( m_flOpenSign, FIELD_FLOAT ),
	DEFINE_FIELD( m_flUnlockStart, FIELD_TIME ),
	DEFINE_FIELD( m_flUnlockHealth, FIELD_FLOAT ),
	DEFINE_FIELD( m_flLockHealth, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTopHingeHealth, FIELD_FLOAT ),
	DEFINE_FIELD( m_flBottomHingeHealth, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecClosedAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecHingeOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_bLockBroken, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bTopHingeBroken, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBottomHingeBroken, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDetached, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bFreeSwing, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPhysicsClosing, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextLockedSound, FIELD_TIME ),
	DEFINE_FIELD( m_flDetachedRestStart, FIELD_TIME ),
	DEFINE_FIELD( m_bDetachedFrozen, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bC4Destroyed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hUnlocker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_pUnlockWeapon, FIELD_CLASSPTR ),
	DEFINE_FUNCTION( ArriveOpen ),
	DEFINE_FUNCTION( ArriveClosed ),
	DEFINE_FUNCTION( AutoClose ),
	DEFINE_FUNCTION( UnlockThink ),
	DEFINE_FUNCTION( FreeSwingThink ),
	DEFINE_FUNCTION( DetachedThink ),
END_DATADESC()

void CSFUDoor::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "open_mode" ))
	{
		m_iOpenMode = Q_atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "door_material" ))
	{
		m_iDoorMaterial = Q_atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "open_angle" ))
	{
		m_flOpenAngle = Q_atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "careful_angle" ))
	{
		m_flCarefulAngle = Q_atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "collision_model" ))
	{
		m_iszCollisionModel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "locked_sound" ))
	{
		m_iszLockedSound = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "OnOpened" ))
	{
		m_iszOnOpened = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "OnClosed" ))
	{
		m_iszOnClosed = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else BaseClass::KeyValue( pkvd );
}

void CSFUDoor::Precache( void )
{
	if( !FStringNull( pev->model ))
		PRECACHE_MODEL( GetModel() );
	if( !FStringNull( m_iszCollisionModel ))
		PRECACHE_MODEL( STRING( m_iszCollisionModel ));
	if( !FStringNull( m_iszLockedSound ))
		PRECACHE_SOUND( STRING( m_iszLockedSound ));
	PRECACHE_SOUND( SFU_DOOR_UNLOCK_SOUND );
	PRECACHE_SOUND( SFU_DOOR_BREAK_LOCK_SOUND );
	PRECACHE_SOUND( SFU_DOOR_BREAK_HINGE_SOUND );
	PRECACHE_SOUND( SFU_DOOR_LOCKPICK_SOUND );
	PRECACHE_SOUND( SFU_DOOR_RAM_WOOD_SOUND );
	PRECACHE_SOUND( SFU_DOOR_RAM_METAL_SOUND );
	PRECACHE_MODEL( "models/woodgibs.mdl" );
	PRECACHE_MODEL( "models/metalplategibs.mdl" );
}

void CSFUDoor::Spawn( void )
{
	if( FStringNull( pev->model ))
	{
		const Vector origin = GetAbsOrigin();
		ALERT( at_error, "sfu_door at (%.1f %.1f %.1f) has no model\n", origin.x, origin.y, origin.z );
		UTIL_Remove( this );
		return;
	}

	if( FStringNull( m_iszLockedSound )) m_iszLockedSound = ALLOC_STRING( SFU_DOOR_DEFAULT_LOCKED_SOUND );
	Precache();
	SET_MODEL( edict(), GetModel() );
	AutoSetSize();

	if( m_flOpenAngle <= 0.0f ) m_flOpenAngle = 90.0f;
	else if( m_flOpenAngle > 179.0f ) m_flOpenAngle = 179.0f;
	if( m_flCarefulAngle <= 0.0f ) m_flCarefulAngle = 15.0f;
	m_flCarefulAngle = bound( 1.0f, m_flCarefulAngle, m_flOpenAngle );
	if( pev->speed <= 0.0f ) pev->speed = 100.0f;
	if( m_iOpenMode < SFU_DOOR_OPEN_BOTH || m_iOpenMode > SFU_DOOR_OPEN_COUNTERCLOCKWISE )
		m_iOpenMode = SFU_DOOR_OPEN_BOTH;
	if( m_iDoorMaterial < SFU_DOOR_MATERIAL_WOOD || m_iDoorMaterial > SFU_DOOR_MATERIAL_METAL )
		m_iDoorMaterial = SFU_DOOR_MATERIAL_WOOD;

	m_vecClosedAngles = GetLocalAngles();
	m_vecHingeOrigin = GetAbsOrigin();
	m_flOpenSign = ( m_iOpenMode == SFU_DOOR_OPEN_CLOCKWISE ) ? -1.0f : 1.0f;
	m_flCurrentOpenAngle = m_flOpenAngle;
	m_iDoorState = SFU_DOOR_CLOSED;
	m_bLocked = FBitSet( pev->spawnflags, SF_SFU_DOOR_START_LOCKED ) != 0;
	m_bTriedOpposite = false;
	m_flUnlockStart = 0.0f;
	m_hUnlocker = NULL;
	m_pUnlockWeapon = NULL;
	m_flLockHealth = SFU_DOOR_PART_HEALTH;
	m_flTopHingeHealth = SFU_DOOR_PART_HEALTH;
	m_flBottomHingeHealth = SFU_DOOR_PART_HEALTH;
	m_bLockBroken = false;
	m_bTopHingeBroken = false;
	m_bBottomHingeBroken = false;
	m_bDetached = false;
	m_bFreeSwing = false;
	m_bPhysicsClosing = false;
	m_flNextLockedSound = 0.0f;
	m_flDetachedRestStart = 0.0f;
	m_bDetachedFrozen = false;
	m_bC4Destroyed = false;
	m_toggle_state = TS_AT_BOTTOM;
	pev->takedamage = DAMAGE_YES;

	pev->movetype = MOVETYPE_PUSH;
	if( WorldPhysic->Initialized() )
	{
		pev->solid = SOLID_CUSTOM;
		const string_t visualModel = pev->model;
		int collisionModelIndex = 0;
		Vector collisionMins = pev->mins;
		Vector collisionMaxs = pev->maxs;
		if( !FStringNull( m_iszCollisionModel ))
		{
			SET_MODEL( edict(), STRING( m_iszCollisionModel ));
			collisionModelIndex = pev->modelindex;
			AutoSetSize();
			collisionMins = pev->mins;
			collisionMaxs = pev->maxs;
		}
		m_pUserData = WorldPhysic->CreateKinematicBodyFromEntity( this );
		if( !FStringNull( m_iszCollisionModel ))
		{
			SET_MODEL( edict(), STRING( visualModel ));
			AutoSetSize();
		}
		// SweepTest normally uses the visible model index. Preserve the actual
		// collision model index so custom traces use the same mesh as PhysX.
		pev->iuser3 = collisionModelIndex;
		pev->vuser1 = collisionMins;
		pev->vuser2 = collisionMaxs;
		if( !m_pUserData ) pev->solid = SOLID_SLIDEBOX;
	}
	else pev->solid = SOLID_SLIDEBOX;

	UTIL_SetOrigin( this, GetLocalOrigin() );
	RelinkEntity( TRUE );
}

int CSFUDoor::ObjectCaps( void )
{
	int caps = BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	if( FBitSet( pev->spawnflags, SF_SFU_DOOR_USABLE ))
	{
		if( m_bLocked && !FBitSet( pev->spawnflags, SF_SFU_DOOR_NO_LOCKPICK ))
			caps |= FCAP_CONTINUOUS_USE | FCAP_ONLYDIRECT_USE;
		else caps |= FCAP_IMPULSE_USE;
	}
	return caps;
}

bool CSFUDoor::RamHit( CBasePlayer *player )
{
	if( !player || m_bDetached || m_bDetachedFrozen ) return false;
	if( fabs( UTIL_AngleDistance( GetAbsAngles().y, m_vecClosedAngles.y )) > 5.0f ) return false;

	if( m_iDoorMaterial == SFU_DOOR_MATERIAL_METAL )
	{
		EMIT_SOUND( edict(), CHAN_BODY, SFU_DOOR_RAM_METAL_SOUND, 1.0f, ATTN_NORM );
		return true;
	}

	EMIT_SOUND( edict(), CHAN_BODY, SFU_DOOR_RAM_WOOD_SOUND, 1.0f, ATTN_NORM );
	if( !m_bLockBroken ) BreakLock( player );
	if( m_bFreeSwing ) EndFreeSwing();
	m_bLocked = false;
	// BreakLock deliberately leaves an ordinary breached door in OPEN/free-swing
	// state after cracking it. The ram must take ownership of that transition
	// and drive the slab all the way to its authored open angle.
	m_iDoorState = SFU_DOOR_CLOSED;
	m_toggle_state = TS_AT_BOTTOM;
	m_hActivator = player;
	Open( player, false );
	return true;
}

bool CSFUDoor::GetChargeMount( Vector &origin ) const
{
	Vector angles;
	return const_cast<CSFUDoor *>( this )->GetAttachment( "charge_mount", origin, angles ) >= 0;
}

bool CSFUDoor::GetCameraMount( Vector &origin ) const
{
	Vector angles;
	return const_cast<CSFUDoor *>( this )->GetAttachment( "camera", origin, angles ) >= 0;
}

bool CSFUDoor::CanUseUnderDoorCamera() const
{
	return !m_bDetached && !m_bDetachedFrozen && !m_bC4Destroyed &&
		fabs( UTIL_AngleDistance( GetAbsAngles().y, m_vecClosedAngles.y )) <= 0.25f;
}

Vector CSFUDoor::GetDoorNormal() const
{
	Vector forward;
	UTIL_MakeVectorsPrivate( GetAbsAngles(), forward, NULL, NULL );
	return forward.Normalize();
}

bool CSFUDoor::C2Breach( CBaseEntity *activator, float installedSideSign )
{
	if( m_bDetached || m_bDetachedFrozen || m_bC4Destroyed ) return false;
	if( !m_bLockBroken ) BreakLock( activator );
	m_bLocked = false;
	if( m_bFreeSwing ) EndFreeSwing();
	SetLocalAngles( m_vecClosedAngles );
	if( m_pUserData ) WorldPhysic->SetAngles( this, m_vecClosedAngles );
	RelinkEntity( TRUE );
	m_iDoorState = SFU_DOOR_CLOSED;
	m_toggle_state = TS_AT_BOTTOM;
	m_hActivator = activator;

	const float installedSign = installedSideSign < 0.0f ? -1.0f : 1.0f;
	const float allowedSign = m_iOpenMode == SFU_DOOR_OPEN_CLOCKWISE ? -1.0f : 1.0f;
	if( m_iOpenMode != SFU_DOOR_OPEN_BOTH && installedSign != allowedSign )
	{
		// A charge mounted on the non-opening side cannot blast the slab toward
		// itself. It only knocks the latch loose and lets the door drift into
		// the permitted side by a small amount.
		m_flOpenSign = allowedSign;
		m_flCurrentOpenAngle = Q_min( 25.0f, m_flOpenAngle );
		m_iDoorState = SFU_DOOR_OPENING;
		m_toggle_state = TS_GOING_UP;
		SetMoveDone( &CSFUDoor::ArriveOpen );
		AngularMove( OpenAngles( m_flOpenSign ), Q_max( 10.0f, pev->speed * 0.25f ));
	}
	else
	{
		// Select the opening direction from the side remembered at mounting
		// time. The player's position at detonation is deliberately irrelevant.
		m_flOpenSign = m_iOpenMode == SFU_DOOR_OPEN_BOTH ? installedSign : allowedSign;
		Open( NULL, false );
	}
	return true;
}

void CSFUDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( m_bDetached ) return;
	if( m_bLocked )
	{
		bool unlocking = false;
		if( !FBitSet( pev->spawnflags, SF_SFU_DOOR_NO_LOCKPICK ) && pActivator && pActivator->IsPlayer() )
			unlocking = StartUnlock( pActivator );
		if( !unlocking && gpGlobals->time >= m_flNextLockedSound && !FStringNull( m_iszLockedSound ))
		{
			EMIT_SOUND( edict(), CHAN_ITEM, STRING( m_iszLockedSound ), 1.0f, ATTN_NORM );
			m_flNextLockedSound = gpGlobals->time + 0.75f;
		}
		return;
	}
	if( m_bFreeSwing )
	{
		const bool nearClosed = fabs( UTIL_AngleDistance( GetAbsAngles().y, m_vecClosedAngles.y )) <= 2.0f;
		if( nearClosed )
		{
			EndFreeSwing();
			SetLocalAngles( m_vecClosedAngles );
			m_iDoorState = SFU_DOOR_CLOSED;
			m_toggle_state = TS_AT_BOTTOM;
			Open( pActivator, pActivator && pActivator->IsPlayer() && FBitSet( pActivator->pev->button, IN_RUN ));
		}
		else Close( pActivator );
		return;
	}

	const bool careful = pActivator && pActivator->IsPlayer() && FBitSet( pActivator->pev->button, IN_RUN );
	if( useType == USE_OFF ) Close( pActivator );
	else if( useType == USE_ON ) Open( pActivator, careful );
	else if( m_iDoorState == SFU_DOOR_CLOSED || m_iDoorState == SFU_DOOR_CLOSING ) Open( pActivator, careful );
	else Close( pActivator );
}

bool CSFUDoor::IsM3Attack( entvars_t *pevAttacker ) const
{
	CBaseEntity *attacker = pevAttacker ? CBaseEntity::Instance( ENT( pevAttacker )) : NULL;
	if( !attacker || !attacker->IsPlayer() ) return false;
	CBasePlayer *player = static_cast<CBasePlayer *>( attacker );
	return player->m_pActiveItem && FClassnameIs( player->m_pActiveItem->pev, "weapon_m3" );
}

void CSFUDoor::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if( !ptr ) return;
	if( m_bDetached )
	{
		if( WorldPhysic->Initialized() && ( bitsDamageType & ( DMG_BULLET | DMG_BLAST )))
			WorldPhysic->AddImpulse( this, vecDir, ptr->vecEndPos, ( bitsDamageType & DMG_BLAST ) ? 30000.0f : 1000.0f );
		return;
	}
	if( m_bFreeSwing && ( bitsDamageType & DMG_BULLET ))
		ApplyBulletPush( flDamage, vecDir, ptr->vecEndPos );
	if( !( bitsDamageType & DMG_BULLET ) || !IsM3Attack( pevAttacker )) return;

	CBasePlayer *player = static_cast<CBasePlayer *>( CBaseEntity::Instance( ENT( pevAttacker )));
	if(( ptr->vecEndPos - player->EyePosition() ).Length() > SFU_DOOR_M3_BREACH_RANGE ) return;
	int componentHitgroup = 0;
	float nearestHit = 3001.0f;
	for( int hitbox = 1; hitbox <= 3; ++hitbox )
	{
		const float distance = SFURayHitboxDistance( this, hitbox, player->EyePosition(), vecDir, 3000.0f );
		if( distance >= 0.0f && distance < nearestHit )
		{
			nearestHit = distance;
			componentHitgroup = hitbox;
		}
	}

	switch( componentHitgroup )
	{
	case 1:
		if( !m_bLockBroken && ( m_flLockHealth -= flDamage ) <= 0.0f ) BreakLock( player );
		break;
	case 2:
		if( !m_bTopHingeBroken && ( m_flTopHingeHealth -= flDamage ) <= 0.0f ) BreakHinge( true, vecDir );
		break;
	case 3:
		if( !m_bBottomHingeBroken && ( m_flBottomHingeHealth -= flDamage ) <= 0.0f ) BreakHinge( false, vecDir );
		break;
	default:
		break;
	}
}

int CSFUDoor::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( m_bC4Destroyed ) return 0;
	if( bitsDamageType & DMG_BLAST )
	{
		CBaseEntity *inflictor = pevInflictor ? CBaseEntity::Instance( ENT( pevInflictor )) : NULL;
		if( inflictor && FClassnameIs( inflictor->pev, "timed_satchel_bomb" ))
		{
			DestroyByC4( inflictor, pevAttacker ? CBaseEntity::Instance( ENT( pevAttacker )) : NULL );
			return 1;
		}
		return 0;
	}
	return BaseClass::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSFUDoor::SpawnC4Debris()
{
	const bool metal = m_iDoorMaterial == SFU_DOOR_MATERIAL_METAL;
	const int gibIndex = MODEL_INDEX( metal ? "models/metalplategibs.mdl" : "models/woodgibs.mdl" );
	const Vector center = Center();
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, center );
		WRITE_BYTE( TE_BREAKMODEL );
		WRITE_COORD( center.x ); WRITE_COORD( center.y ); WRITE_COORD( center.z );
		WRITE_COORD( pev->size.x ); WRITE_COORD( pev->size.y ); WRITE_COORD( pev->size.z );
		WRITE_COORD( 0 ); WRITE_COORD( 0 ); WRITE_COORD( 80 );
		WRITE_BYTE( 40 );
		WRITE_SHORT( gibIndex );
		WRITE_BYTE( 12 );
		WRITE_BYTE( 40 );
		WRITE_BYTE( metal ? BREAK_METAL : BREAK_WOOD );
	MESSAGE_END();
}

void CSFUDoor::DestroyByC4( CBaseEntity *inflictor, CBaseEntity *attacker )
{
	if( m_bC4Destroyed ) return;
	m_bC4Destroyed = true;
	CancelUnlock();
	DontThink();
	SetMoveDone( NULL );

	CBaseEntity *entity = NULL;
	while(( entity = UTIL_FindEntityByClassname( entity, "sfu_c2_charge" )) != NULL )
	{
		CSFUC2Charge *charge = static_cast<CSFUC2Charge *>( entity );
		if( charge->IsAttachedTo( this )) charge->Use( attacker, inflictor, USE_ON, 0.0f );
	}

	if( inflictor && (CBaseEntity *)inflictor->m_hParent == this )
		inflictor->SetParent((CBaseEntity *)NULL );

	CBaseEntity *child = (CBaseEntity *)m_hChild;
	while( child )
	{
		CBaseEntity *next = (CBaseEntity *)child->m_hNextChild;
		child->Use( attacker, inflictor, USE_ON, 0.0f );
		child->pev->solid = SOLID_NOT;
		UTIL_Remove( child );
		child = next;
	}

	SpawnC4Debris();
	if( WorldPhysic->Initialized() && m_pUserData ) WorldPhysic->RemoveBody( edict() );
	m_pUserData = NULL;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->modelindex = 0;
	pev->model = iStringNull;
	SetBits( pev->effects, EF_NODRAW );
	RelinkEntity( TRUE );
	UTIL_Remove( this );
}

void CSFUDoor::BreakLock( CBaseEntity *pAttacker )
{
	const bool wasBeingUnlocked = (CBaseEntity *)m_hUnlocker != NULL;
	const bool wasClosed = fabs( UTIL_AngleDistance( GetAbsAngles().y, m_vecClosedAngles.y )) <= 2.0f;
	m_bLockBroken = true;
	m_flLockHealth = 0.0f;
	m_bLocked = false;
	CancelUnlock();
	if( wasBeingUnlocked ) DontThink();
	EMIT_SOUND( edict(), CHAN_BODY, SFU_DOOR_BREAK_LOCK_SOUND, 1.0f, ATTN_NORM );
	if( wasClosed )
	{
		m_flOpenSign = SelectOpenSign( pAttacker );
		Vector crackedAngles = m_vecClosedAngles;
		crackedAngles.y += m_flOpenSign * SFU_DOOR_BROKEN_LOCK_CRACK_ANGLE;
		SetLocalAngles( crackedAngles );
		if( m_pUserData ) WorldPhysic->SetAngles( this, crackedAngles );
		RelinkEntity( TRUE );
		m_iDoorState = SFU_DOOR_OPEN;
		m_toggle_state = TS_AT_TOP;
	}
	BeginFreeSwing();
	if( wasClosed && m_bFreeSwing )
	{
		// The lock gives way abruptly: after the initial visible crack, retain
		// enough angular velocity to coast approximately ten degrees farther.
		Vector angularVelocity = GetLocalAvelocity();
		angularVelocity.y = m_flOpenSign * 60.0f;
		SetLocalAvelocity( angularVelocity );
		SetMoveDoneTime( 0.03f );
	}
}

void CSFUDoor::BreakHinge( bool top, const Vector &shotDirection )
{
	EMIT_SOUND( edict(), CHAN_BODY, SFU_DOOR_BREAK_HINGE_SOUND, 1.0f, ATTN_NORM );
	if( top )
	{
		m_bTopHingeBroken = true;
		m_flTopHingeHealth = 0.0f;
	}
	else
	{
		m_bBottomHingeBroken = true;
		m_flBottomHingeHealth = 0.0f;
	}
	if( m_bTopHingeBroken && m_bBottomHingeBroken ) DetachDoor( shotDirection );
}

void CSFUDoor::DetachDoor( const Vector &shotDirection )
{
	if( m_bDetached ) return;
	const bool wasFreeSwing = m_bFreeSwing;
	m_bDetached = true;
	m_bFreeSwing = false;
	m_bPhysicsClosing = false;
	m_bLocked = false;
	CancelUnlock();
	DontThink();
	SetMoveDone( NULL );
	SetLocalVelocity( g_vecZero );
	SetLocalAvelocity( g_vecZero );
	if( wasFreeSwing )
	{
		SetAbsOrigin( m_vecHingeOrigin );
		WorldPhysic->SetOrigin( this, m_vecHingeOrigin );
	}
	// The closed door is authored at floor level and its collision hull may
	// extend slightly below the origin.  Start a newly detached dynamic body
	// clear of the floor instead of asking the solver to resolve it downward.
	const float detachLift = bound( 1.0f, -pev->vuser1.z + 0.5f, 4.0f );
	Vector horizontalShot( shotDirection.x, shotDirection.y, 0.0f );
	if( horizontalShot.Length() > 0.01f ) horizontalShot = horizontalShot.Normalize();
	else UTIL_MakeVectorsPrivate( GetAbsAngles(), horizontalShot, NULL, NULL );
	const bool detachedWhileClosed = fabs( UTIL_AngleDistance( GetAbsAngles().y, m_vecClosedAngles.y )) <= 5.0f;
	const Vector frameClearance = detachedWhileClosed ? horizontalShot * 12.0f : g_vecZero;
	SetAbsOrigin( GetAbsOrigin() + frameClearance + Vector( 0.0f, 0.0f, detachLift ));

	if( WorldPhysic->Initialized() )
	{
		WorldPhysic->RemoveBody( edict() );
		const string_t visualModel = pev->model;
		if( !FStringNull( m_iszCollisionModel ))
		{
			SET_MODEL( edict(), STRING( m_iszCollisionModel ));
			AutoSetSize();
		}
		pev->movetype = MOVETYPE_PHYSIC;
		pev->solid = SOLID_CUSTOM;
		m_flBodyMass = SFUDoorMaterialSettings( m_iDoorMaterial ).mass;
		m_pUserData = WorldPhysic->CreateBodyFromEntity( this );
		if( m_pUserData )
		{
			WorldPhysic->SetVelocity( this, g_vecZero );
			WorldPhysic->SetAvelocity( this, g_vecZero );
			// The pellet that destroys the second hinge must finish separating a
			// closed slab from the jamb; otherwise static contact friction can pin
			// the freshly detached body in the doorway.
			WorldPhysic->AddImpulse( this, horizontalShot, Center(), 1200.0f );
		}
		if( !FStringNull( m_iszCollisionModel ))
		{
			SET_MODEL( edict(), STRING( visualModel ));
			AutoSetSize();
		}
		if( !m_pUserData )
		{
			pev->movetype = MOVETYPE_TOSS;
			pev->solid = SOLID_SLIDEBOX;
		}
	}
	else
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->solid = SOLID_SLIDEBOX;
	}
	RelinkEntity( TRUE );
	if( m_pUserData )
	{
		SetThink( &CSFUDoor::DetachedThink );
		SetNextThink( 0.1f );
	}
}

void CSFUDoor::DetachedThink( void )
{
	if( !m_bDetached || m_bDetachedFrozen || !m_pUserData || !WorldPhysic->Initialized() )
	{
		DontThink();
		return;
	}

	WorldPhysic->UpdateEntityAABB( this );
	const Vector bounds = pev->absmax - pev->absmin;
	const float horizontalExtent = Q_max( bounds.x, bounds.y );
	const bool lyingFlat = horizontalExtent > 1.0f && bounds.z <= horizontalExtent * 0.35f;
	const bool stationary = WorldPhysic->IsBodySleeping( this );

	if( lyingFlat && stationary )
	{
		if( m_flDetachedRestStart <= 0.0f ) m_flDetachedRestStart = gpGlobals->time;
		else if( gpGlobals->time - m_flDetachedRestStart >= 3.0f )
		{
			FreezeDetachedDoor();
			return;
		}
	}
	else m_flDetachedRestStart = 0.0f;

	SetNextThink( 0.1f );
}

void CSFUDoor::FreezeDetachedDoor( void )
{
	if( !m_pUserData || !WorldPhysic->Initialized() ) return;

	WorldPhysic->RemoveBody( edict() );
	m_pUserData = NULL;
	SetLocalVelocity( g_vecZero );
	SetLocalAvelocity( g_vecZero );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	m_bDetachedFrozen = true;
	DontThink();
	RelinkEntity( TRUE );
}

void CSFUDoor::BeginFreeSwing( void )
{
	if( m_bDetached || m_bFreeSwing ) return;
	DontThink();
	SetMoveDone( NULL );
	SetLocalAvelocity( g_vecZero );
	m_bFreeSwing = true;
	m_bPhysicsClosing = false;
	m_iDoorState = SFU_DOOR_OPEN;
	m_toggle_state = TS_AT_TOP;
	SetThink( &CSFUDoor::FreeSwingThink );
	SetNextThink( 0.02f );
	SetMoveDoneTime( 0.03f );
	RelinkEntity( TRUE );
}

void CSFUDoor::EndFreeSwing( void )
{
	if( !m_bFreeSwing ) return;
	m_bPhysicsClosing = false;
	m_bFreeSwing = false;
	SetLocalAvelocity( g_vecZero );
	DontThink();
	RelinkEntity( TRUE );
}

void CSFUDoor::FreeSwingThink( void )
{
	if( !m_bFreeSwing ) return;
	// An intact lock still has a working latch. Once body pushing brings the
	// slab back near its authored closed angle, snap it shut and stop all free
	// motion. A destroyed lock deliberately skips this path forever.
	if( !m_bLockBroken && fabs( UTIL_AngleDistance( GetAbsAngles().y, m_vecClosedAngles.y )) <= 1.5f )
	{
		EndFreeSwing();
		SetLocalAngles( m_vecClosedAngles );
		ArriveClosed();
		return;
	}

	// A stationary MOVETYPE_PUSH does not reliably receive Touch callbacks.
	// Sample nearby character hulls so walking into an unlatched slab can start
	// its rotation from rest.
	for( int playerIndex = 1; playerIndex <= gpGlobals->maxClients; ++playerIndex )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( UTIL_PlayerByIndex( playerIndex ));
		if( !player || !player->IsAlive() ) continue;
		const Vector contactPadding( 10.0f, 10.0f, 4.0f );
		if( WorldPhysic->EntityIntersectsBox( this, player->GetAbsOrigin(), player->pev->mins - contactPadding, player->pev->maxs + contactPadding ))
			ApplyBodyPush( player );
	}
	CBaseEntity *nearby = NULL;
	while(( nearby = UTIL_FindEntityInSphere( nearby, m_vecHingeOrigin, 192.0f )) != NULL )
	{
		if( !nearby->MyMonsterPointer() ) continue;
		const Vector contactPadding( 10.0f, 10.0f, 4.0f );
		if( WorldPhysic->EntityIntersectsBox( this, nearby->GetAbsOrigin(), nearby->pev->mins - contactPadding, nearby->pev->maxs + contactPadding ))
			ApplyBodyPush( nearby );
	}

	float minimumYaw = -m_flOpenAngle;
	float maximumYaw = m_flOpenAngle;
	if( m_iOpenMode == SFU_DOOR_OPEN_CLOCKWISE ) maximumYaw = 0.0f;
	else if( m_iOpenMode == SFU_DOOR_OPEN_COUNTERCLOCKWISE ) minimumYaw = 0.0f;

	Vector angles = GetLocalAngles();
	const float relativeYaw = UTIL_AngleDistance( angles.y, m_vecClosedAngles.y );
	const float clampedYaw = bound( minimumYaw, relativeYaw, maximumYaw );
	Vector angularVelocity = GetLocalAvelocity();
	if( clampedYaw != relativeYaw )
	{
		angles = m_vecClosedAngles;
		angles.y += clampedYaw;
		SetLocalAngles( angles );
		angularVelocity = g_vecZero;
	}
	else
	{
		angularVelocity.y *= 0.90f;
		if( fabs( angularVelocity.y ) < 1.0f ) angularVelocity.y = 0.0f;
	}
	angularVelocity.x = angularVelocity.z = 0.0f;
	SetLocalAvelocity( angularVelocity );
	// MOVETYPE_PUSH only integrates velocity while its local move timer is
	// positive. Keep a short rolling window instead of assigning a destination.
	SetMoveDoneTime( 0.03f );
	SetNextThink( 0.02f );
}

void CSFUDoor::Touch( CBaseEntity *pOther )
{
	ApplyBodyPush( pOther );
}

void CSFUDoor::ApplyBodyPush( CBaseEntity *pOther )
{
	if( !m_bFreeSwing || !pOther || !( pOther->IsPlayer() || pOther->MyMonsterPointer() )) return;
	Vector pushVelocity = pOther->GetAbsVelocity();
	if( pOther->IsPlayer() && pushVelocity.Length() < 5.0f )
	{
		Vector forward, right;
		UTIL_MakeVectorsPrivate( pOther->pev->v_angle, forward, right, NULL );
		const float wishSpeed = pOther->pev->maxspeed > 0.0f ? pOther->pev->maxspeed : 200.0f;
		if( FBitSet( pOther->pev->button, IN_FORWARD )) pushVelocity += forward * wishSpeed;
		if( FBitSet( pOther->pev->button, IN_BACK )) pushVelocity -= forward * wishSpeed;
		if( FBitSet( pOther->pev->button, IN_MOVERIGHT )) pushVelocity += right * wishSpeed;
		if( FBitSet( pOther->pev->button, IN_MOVELEFT )) pushVelocity -= right * wishSpeed;
		pushVelocity.z = 0.0f;
	}
	Vector lever = pOther->Center() - m_vecHingeOrigin;
	lever.z = 0.0f;
	const float leverLengthSquared = DotProduct( lever, lever );
	if( leverLengthSquared < 64.0f ) return;
	const float angularPush = RAD2DEG(( lever.x * pushVelocity.y - lever.y * pushVelocity.x ) / leverLengthSquared );
	if( fabs( angularPush ) < 2.0f ) return;
	Vector angularVelocity = GetLocalAvelocity();
	angularVelocity.y = bound( -120.0f, angularVelocity.y + angularPush * 0.45f * SFUDoorMaterialSettings( m_iDoorMaterial ).pushScale, 120.0f );
	SetLocalAvelocity( angularVelocity );
	SetMoveDoneTime( 0.03f );
	SetNextThink( 0.02f );
}

void CSFUDoor::ApplyBulletPush( float damage, const Vector &shotDirection, const Vector &hitPosition )
{
	if( !m_bFreeSwing || damage <= 0.0f ) return;

	Vector lever = hitPosition - m_vecHingeOrigin;
	lever.z = 0.0f;
	const float leverLength = lever.Length();
	if( leverLength < 4.0f ) return;

	Vector direction = shotDirection;
	direction.z = 0.0f;
	const float directionLength = direction.Length();
	if( directionLength < 0.001f ) return;
	direction = direction / directionLength;

	// Cross product around the vertical hinge axis gives both the rotation
	// direction and how tangentially the bullet struck. The lever factor makes
	// hits near the lock edge much more effective than hits beside the hinges.
	const float tangentialForce = ( lever.x * direction.y - lever.y * direction.x ) / leverLength;
	const float leverFactor = bound( 0.1f, leverLength / 64.0f, 1.5f );
	const float angularImpulse = tangentialForce * leverFactor * damage * 0.8f * SFUDoorMaterialSettings( m_iDoorMaterial ).pushScale;

	Vector angularVelocity = GetLocalAvelocity();
	angularVelocity.y = bound( -120.0f, angularVelocity.y + angularImpulse, 120.0f );
	angularVelocity.x = angularVelocity.z = 0.0f;
	SetLocalAvelocity( angularVelocity );
	SetMoveDoneTime( 0.03f );
	SetNextThink( 0.02f );
}

bool CSFUDoor::StartUnlock( CBaseEntity *pActivator )
{
	CBasePlayer *player = static_cast<CBasePlayer *>( pActivator );
	if( (CBaseEntity *)m_hUnlocker == player )
		return true;
	if( !PlayerLooksAtLock( player ))
		return false;

	CancelUnlock();
	m_hUnlocker = player;
	m_pUnlockWeapon = player->m_pActiveItem;
	m_flUnlockHealth = player->pev->health;
	m_flUnlockStart = gpGlobals->time;
	g_engfuncs.pfnSetPhysicsKeyValue( player->edict(), "sfu_lockpick", "1" );
	SetBits( player->pev->iuser4, SFU_PLAYER_LOCK_VIEW );
	HoldUnlockerStill( player );
	EMIT_SOUND( edict(), CHAN_ITEM, SFU_DOOR_LOCKPICK_SOUND, 1.0f, ATTN_NORM );
	MESSAGE_BEGIN( MSG_ONE, gmsgActionBar, NULL, player->pev );
		WRITE_BYTE( 2 );
		WRITE_SHORT( 30 );
	MESSAGE_END();
	SetThink( &CSFUDoor::UnlockThink );
	SetNextThink( 0.05f );
	return true;
}

void CSFUDoor::CancelUnlock( void )
{
	CBasePlayer *player = (CBasePlayer *)(CBaseEntity *)m_hUnlocker;
	if( player )
	{
		STOP_SOUND( edict(), CHAN_ITEM, SFU_DOOR_LOCKPICK_SOUND );
		MESSAGE_BEGIN( MSG_ONE, gmsgActionBar, NULL, player->pev );
			WRITE_BYTE( 0 );
			WRITE_SHORT( 0 );
		MESSAGE_END();
		g_engfuncs.pfnSetPhysicsKeyValue( player->edict(), "sfu_lockpick", "0" );
		ClearBits( player->pev->iuser4, SFU_PLAYER_LOCK_VIEW );
	}
	m_hUnlocker = NULL;
	m_pUnlockWeapon = NULL;
	m_flUnlockStart = 0.0f;
}

bool CSFUDoor::PlayerLooksAtLock( CBasePlayer *player ) const
{
	if( !player ) return false;

	Vector lockPosition;
	matrix3x4 lockBoneTransform;
	const mstudiobbox_t *lockHitbox = NULL;
	CSFUDoor *door = const_cast<CSFUDoor *>( this );
	studiohdr_t *studioHeader = (studiohdr_t *)GET_MODEL_PTR( door->edict() );
	if( studioHeader && studioHeader->numhitboxes > 1 )
	{
		mstudiobbox_t *hitboxes = (mstudiobbox_t *)((byte *)studioHeader + studioHeader->hitboxindex);
		lockHitbox = &hitboxes[1];
		Vector boneOrigin, boneAngles;
		door->GetBonePosition( lockHitbox->bone, boneOrigin, boneAngles );
		lockBoneTransform = matrix3x4( boneOrigin, boneAngles );
	}
	else
	{
		ALERT( at_error, "sfu_door model %s has no hbox 1 for its lock\n", GetModel() );
		return false;
	}

	const Vector eye = player->EyePosition();
	Vector forward;
	UTIL_MakeVectorsPrivate( player->pev->v_angle + player->pev->punchangle, forward, NULL, NULL );

	// Test the actual crosshair ray against hbox 1 in the hitbox bone's local
	// space. This deliberately has no angular tolerance around the lock.
	const Vector localEye = lockBoneTransform.VectorITransform( eye );
	const Vector localDirection = lockBoneTransform.VectorIRotate( forward );
	float enterDistance = 0.0f;
	float exitDistance = 128.0f;
	for( int axis = 0; axis < 3; ++axis )
	{
		if( fabs( localDirection[axis] ) < 0.0001f )
		{
			if( localEye[axis] < lockHitbox->bbmin[axis] || localEye[axis] > lockHitbox->bbmax[axis] )
				return false;
			continue;
		}

		float nearDistance = ( lockHitbox->bbmin[axis] - localEye[axis] ) / localDirection[axis];
		float farDistance = ( lockHitbox->bbmax[axis] - localEye[axis] ) / localDirection[axis];
		if( nearDistance > farDistance ) std::swap( nearDistance, farDistance );
		enterDistance = Q_max( enterDistance, nearDistance );
		exitDistance = Q_min( exitDistance, farDistance );
		if( enterDistance > exitDistance ) return false;
	}
	if( exitDistance < 0.0f || enterDistance > 128.0f ) return false;
	lockPosition = lockBoneTransform.VectorTransform( localEye + localDirection * Q_max( enterDistance, 0.0f ));

	TraceResult trace;
	UTIL_TraceLine( eye, lockPosition, dont_ignore_monsters, player->edict(), &trace );
	return trace.flFraction == 1.0f || trace.pHit == door->edict();
}

void CSFUDoor::HoldUnlockerStill( CBasePlayer *player )
{
	if( !player ) return;
	player->SetAbsVelocity( g_vecZero );
	player->pev->basevelocity = g_vecZero;
}

bool CSFUDoor::UnlockerStillValid( void )
{
	CBasePlayer *player = (CBasePlayer *)(CBaseEntity *)m_hUnlocker;
	if( !player || !player->IsAlive() || !FBitSet( player->pev->button, IN_USE ) || FBitSet( player->pev->button, IN_JUMP ))
		return false;
	if( player->pev->health < m_flUnlockHealth || player->m_pActiveItem != m_pUnlockWeapon )
		return false;
	if(( player->Center() - Center() ).Length() > 96.0f )
		return false;
	return true;
}

void CSFUDoor::UnlockThink( void )
{
	CBasePlayer *player = (CBasePlayer *)(CBaseEntity *)m_hUnlocker;
	HoldUnlockerStill( player );

	if( !m_bLocked || !UnlockerStillValid() )
	{
		CancelUnlock();
		DontThink();
		return;
	}

	if( gpGlobals->time - m_flUnlockStart >= 3.0f )
	{
		m_bLocked = false;
		CancelUnlock();
		EMIT_SOUND( edict(), CHAN_ITEM, SFU_DOOR_UNLOCK_SOUND, 1.0f, ATTN_NORM );
		m_hActivator = player;
		DontThink();
		return;
	}

	SetNextThink( 0.05f );
}

float CSFUDoor::SelectOpenSign( CBaseEntity *pActivator ) const
{
	if( m_iOpenMode == SFU_DOOR_OPEN_CLOCKWISE ) return -1.0f;
	if( m_iOpenMode == SFU_DOOR_OPEN_COUNTERCLOCKWISE ) return 1.0f;
	if( !pActivator ) return m_flOpenSign == 0.0f ? 1.0f : m_flOpenSign;

	Vector closedForward;
	UTIL_MakeVectorsPrivate( m_vecClosedAngles, closedForward, NULL, NULL );
	Vector toActivator = pActivator->Center() - GetAbsOrigin();
	return DotProduct( toActivator, closedForward ) >= 0.0f ? 1.0f : -1.0f;
}

Vector CSFUDoor::OpenAngles( float sign ) const
{
	Vector angles = m_vecClosedAngles;
	angles.y += m_flCurrentOpenAngle * sign;
	return angles;
}

void CSFUDoor::Open( CBaseEntity *pActivator, bool careful )
{
	if( m_bLocked || m_iDoorState == SFU_DOOR_OPEN ) return;

	DontThink();
	m_hActivator = pActivator;
	if( m_iDoorState == SFU_DOOR_CLOSED ) m_flOpenSign = SelectOpenSign( pActivator );
	// Leave a tiny clearance from the authored maximum before handing the door
	// to PhysX. At the exact limit a nearby wall and the joint can fight each
	// other and make an otherwise stationary open door vibrate.
	m_flCurrentOpenAngle = careful ? m_flCarefulAngle : Q_max( 1.0f, m_flOpenAngle - 1.0f );
	m_bTriedOpposite = false;
	m_iDoorState = SFU_DOOR_OPENING;
	m_toggle_state = TS_GOING_UP;
	SetMoveDone( &CSFUDoor::ArriveOpen );
	AngularMove( OpenAngles( m_flOpenSign ), pev->speed );
}

void CSFUDoor::Blocked( CBaseEntity *pOther )
{
	SetLocalAvelocity( g_vecZero );
	DontThink();
	if( m_bFreeSwing )
	{
		m_iDoorState = SFU_DOOR_OPEN;
		m_toggle_state = TS_AT_TOP;
		SetThink( &CSFUDoor::FreeSwingThink );
		SetNextThink( 0.02f );
		return;
	}

	if( m_iDoorState == SFU_DOOR_OPENING && m_iOpenMode == SFU_DOOR_OPEN_BOTH && !m_bTriedOpposite )
	{
		m_bTriedOpposite = true;
		m_flOpenSign = -m_flOpenSign;
		SetMoveDone( &CSFUDoor::ArriveOpen );
		AngularMove( OpenAngles( m_flOpenSign ), pev->speed );
		return;
	}
	if( m_iDoorState == SFU_DOOR_OPENING )
	{
		// A one-way door, or a two-way door blocked on both sides, gives up and
		// returns to its latched position.
		Close( pOther );
		return;
	}

	// Closing cannot force a character through geometry, so remain at the
	// angle actually reached and allow another Use attempt later.
	m_iDoorState = SFU_DOOR_OPEN;
	m_toggle_state = TS_AT_TOP;
}

void CSFUDoor::Close( CBaseEntity *pActivator )
{
	if( m_iDoorState == SFU_DOOR_CLOSED ) return;
	if( m_bFreeSwing ) EndFreeSwing();

	DontThink();
	m_hActivator = pActivator;
	m_iDoorState = SFU_DOOR_CLOSING;
	m_toggle_state = TS_GOING_DOWN;
	SetMoveDone( &CSFUDoor::ArriveClosed );
	Vector nearestClosed = m_vecClosedAngles;
	nearestClosed.y = GetAbsAngles().y - UTIL_AngleDistance( GetAbsAngles().y, m_vecClosedAngles.y );
	AngularMove( nearestClosed, pev->speed );
}

void CSFUDoor::ArriveOpen( void )
{
	m_iDoorState = SFU_DOOR_OPEN;
	m_toggle_state = TS_AT_TOP;
	if( !FStringNull( m_iszOnOpened )) UTIL_FireTargets( m_iszOnOpened, m_hActivator, this, USE_ON, 0.0f );
	// Once E has delivered the slab to its requested open angle, release it so
	// characters can continue moving it with their bodies. With an intact lock
	// it can latch again near closed; with a broken lock it never can.
	BeginFreeSwing();
	if( m_bFreeSwing ) return;
	if( m_flWait >= 0.0f )
	{
		SetThink( &CSFUDoor::AutoClose );
		SetNextThink( m_flWait );
	}
}

void CSFUDoor::ArriveClosed( void )
{
	SetLocalAngles( m_vecClosedAngles );
	if( m_pUserData ) WorldPhysic->SetAngles( this, m_vecClosedAngles );
	m_iDoorState = SFU_DOOR_CLOSED;
	m_toggle_state = TS_AT_BOTTOM;
	m_flOpenSign = ( m_iOpenMode == SFU_DOOR_OPEN_CLOCKWISE ) ? -1.0f : 1.0f;
	m_flCurrentOpenAngle = m_flOpenAngle;
	if( !FStringNull( m_iszOnClosed )) UTIL_FireTargets( m_iszOnClosed, m_hActivator, this, USE_OFF, 0.0f );
	if( m_bLockBroken ) BeginFreeSwing();
}

void CSFUDoor::AutoClose( void )
{
	SetThink( NULL );
	Close( m_hActivator );
}

void CSFUDoor::AutoSetSize( void )
{
	studiohdr_t *studioHeader = (studiohdr_t *)GET_MODEL_PTR( edict() );
	if( !studioHeader || studioHeader->numseq <= 0 )
	{
		ALERT( at_error, "sfu_door: unable to obtain bounds for %s\n", GetModel() );
		UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ));
		return;
	}

	mstudioseqdesc_t *sequence = (mstudioseqdesc_t *)((byte *)studioHeader + studioHeader->seqindex);
	UTIL_SetSize( pev, sequence[pev->sequence].bbmin, sequence[pev->sequence].bbmax );
}

void CSFUDoor::SetObjectCollisionBox( void )
{
	TransformAABB( EntityToWorldTransform(), pev->mins, pev->maxs, pev->absmin, pev->absmax );
	pev->absmin -= Vector( 1, 1, 1 );
	pev->absmax += Vector( 1, 1, 1 );
}
