#include "sfu_c2_charge.h"
#include "sfu_door.h"
#include "player.h"
#include "weapons.h"
#include "env_explosion.h"
#include "skill.h"

static const float SFU_C2_NEAR_RADIUS = 39.37f;
static const float SFU_C2_FAR_RADIUS = 196.85f;
static const float SFU_C2_SURFACE_OFFSET = 3.0f;
static const float SFU_C2_MAX_DAMAGE = 200.0f;

LINK_ENTITY_TO_CLASS( sfu_c2_charge, CSFUC2Charge );

BEGIN_DATADESC( CSFUC2Charge )
	DEFINE_FIELD( m_hDoor, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flSideSign, FIELD_FLOAT ),
	DEFINE_FIELD( m_bDetonated, FIELD_BOOLEAN ),
	DEFINE_FUNCTION( FollowDoorThink ),
END_DATADESC()

void CSFUC2Charge::Precache()
{
	PRECACHE_MODEL( "models/w_satchel.mdl" );
}

void CSFUC2Charge::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/w_satchel.mdl" );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->sequence = 1;
	m_bDetonated = false;
	SetThink( &CSFUC2Charge::FollowDoorThink );
	pev->nextthink = gpGlobals->time + 0.05f;
}

bool CSFUC2Charge::AttachToDoor( CSFUDoor *door, CBasePlayer *owner, float sideSign )
{
	Vector mount;
	if( !door || !owner || !door->GetChargeMount( mount )) return false;
	m_hDoor = door;
	m_flSideSign = sideSign < 0.0f ? -1.0f : 1.0f;
	pev->owner = owner->edict();
	FollowDoorThink();
	return true;
}

void CSFUC2Charge::FollowDoorThink()
{
	CSFUDoor *door = static_cast<CSFUDoor *>((CBaseEntity *)m_hDoor);
	Vector mount;
	if( !door || !door->GetChargeMount( mount )) { UTIL_Remove( this ); return; }
	const Vector normal = door->GetDoorNormal() * m_flSideSign;
	UTIL_SetOrigin( this, mount + normal * SFU_C2_SURFACE_OFFSET );
	SetAbsAngles( UTIL_VecToAngles( normal ));
	pev->nextthink = gpGlobals->time + 0.05f;
}

bool CSFUC2Charge::VisibleForBlast( CBaseEntity *target, const Vector &source )
{
	TraceResult tr;
	UTIL_TraceLine( source, target->BodyTarget( source ), dont_ignore_monsters, edict(), &tr );
	return tr.flFraction == 1.0f || tr.pHit == target->edict();
}

void CSFUC2Charge::DirectedDamage( const Vector &mount, const Vector &nearNormal )
{
	CBaseEntity *door = (CBaseEntity *)m_hDoor;
	CBaseEntity *owner = pev->owner ? CBaseEntity::Instance( pev->owner ) : this;
	CBaseEntity *target = NULL;
	while(( target = UTIL_FindEntityInSphere( target, mount, SFU_C2_FAR_RADIUS )) != NULL )
	{
		// C2 breaches only the door it is physically mounted on. Other SFU doors
		// must not fall through CBaseEntity's generic blast-death path and vanish;
		// their dedicated C4 destruction will be implemented separately.
		if( target == this || target == door || FClassnameIs( target->pev, "sfu_door" ) || target->pev->takedamage == DAMAGE_NO ) continue;
		Vector closest;
		closest.x = bound( target->pev->absmin.x, mount.x, target->pev->absmax.x );
		closest.y = bound( target->pev->absmin.y, mount.y, target->pev->absmax.y );
		closest.z = bound( target->pev->absmin.z, mount.z, target->pev->absmax.z );
		const Vector delta = closest - mount;
		const bool nearSide = DotProduct( delta, nearNormal ) >= 0.0f;
		const float radius = nearSide ? SFU_C2_NEAR_RADIUS : SFU_C2_FAR_RADIUS;
		const float distance = delta.Length();
		if( distance >= radius ) continue;
		const Vector source = mount + nearNormal * ( nearSide ? 6.0f : -6.0f );
		if( !VisibleForBlast( target, source )) continue;
		const float damage = SFU_C2_MAX_DAMAGE * ( 1.0f - distance / radius );
		target->TakeDamage( pev, owner->pev, damage, DMG_BLAST );
	}
}

void CSFUC2Charge::Use( CBaseEntity *, CBaseEntity *, USE_TYPE, float )
{
	if( m_bDetonated ) return;
	m_bDetonated = true;
	CSFUDoor *door = static_cast<CSFUDoor *>((CBaseEntity *)m_hDoor);
	Vector mount;
	if( !door || !door->GetChargeMount( mount )) { UTIL_Remove( this ); return; }
	const Vector nearNormal = door->GetDoorNormal() * m_flSideSign;
	DirectedDamage( mount, nearNormal );
	door->C2Breach( pev->owner ? CBaseEntity::Instance( pev->owner ) : this, m_flSideSign );
	ExplosionCreate( mount + nearNormal * 2.0f, GetAbsAngles(), pev->owner, 100, FALSE );
	UTIL_Remove( this );
}
