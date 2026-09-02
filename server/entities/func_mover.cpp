#include "func_mover.h"
#include "scriptevent.h"

LINK_ENTITY_TO_CLASS( func_mover, CFuncMover );
LINK_ENTITY_TO_CLASS( prop_mover, CFuncMover );

BEGIN_DATADESC( CFuncMover )
	DEFINE_KEYFIELD( m_iPreset, FIELD_INTEGER, "preset" ),
	DEFINE_KEYFIELD( m_iVisualMode, FIELD_INTEGER, "visual_mode" ),
	DEFINE_KEYFIELD( m_iMotionMode, FIELD_INTEGER, "motion_mode" ),
	DEFINE_KEYFIELD( m_iTouchFilter, FIELD_INTEGER, "touch_filter" ),
	DEFINE_KEYFIELD( m_iMaterial, FIELD_INTEGER, "material" ),
	DEFINE_KEYFIELD( m_iGibCount, FIELD_INTEGER, "gib_count" ),
	DEFINE_KEYFIELD( m_iDamageMask, FIELD_INTEGER, "damage_mask" ),
	DEFINE_KEYFIELD( m_iDirectionMode, FIELD_INTEGER, "move_direction" ),
	DEFINE_KEYFIELD( m_flMoveSpeed, FIELD_FLOAT, "move_speed" ),
	DEFINE_KEYFIELD( m_flReturnDelay, FIELD_FLOAT, "return_delay" ),
	DEFINE_KEYFIELD( m_flTouchCooldown, FIELD_FLOAT, "touch_cooldown" ),
	DEFINE_KEYFIELD( m_flMinDamage, FIELD_FLOAT, "min_damage" ),
	DEFINE_KEYFIELD( m_flVolume, FIELD_FLOAT, "sound_volume" ),
	DEFINE_KEYFIELD( m_flAttenuation, FIELD_FLOAT, "sound_attenuation" ),
	DEFINE_KEYFIELD( m_flPendulumSpeed, FIELD_FLOAT, "pendulum_speed" ),
	DEFINE_KEYFIELD( m_flThinkInterval, FIELD_FLOAT, "think_interval" ),
	DEFINE_KEYFIELD( m_flMoveDistance, FIELD_FLOAT, "move_distance" ),
	DEFINE_KEYFIELD( m_vecMoveOffset, FIELD_VECTOR, "move_offset" ),
	DEFINE_KEYFIELD( m_vecMoveAngles, FIELD_VECTOR, "move_angles" ),
	DEFINE_KEYFIELD( m_vecAngleOffset, FIELD_VECTOR, "angle_offset" ),
	DEFINE_KEYFIELD( m_vecRotationSpeed, FIELD_VECTOR, "rotation_speed" ),
	DEFINE_KEYFIELD( m_vecStudioMins, FIELD_VECTOR, "collision_mins" ),
	DEFINE_KEYFIELD( m_vecStudioMaxs, FIELD_VECTOR, "collision_maxs" ),
	DEFINE_KEYFIELD( m_iszSequenceIdle, FIELD_STRING, "sequence_idle" ),
	DEFINE_KEYFIELD( m_iszSequenceActive, FIELD_STRING, "sequence_active" ),
	DEFINE_KEYFIELD( m_iszSequenceBreak, FIELD_STRING, "sequence_break" ),
	DEFINE_KEYFIELD( m_iszGibModel, FIELD_STRING, "gib_model" ),
	DEFINE_KEYFIELD( m_iszSoundStart, FIELD_STRING, "sound_start" ),
	DEFINE_KEYFIELD( m_iszSoundMove, FIELD_STRING, "sound_move" ),
	DEFINE_KEYFIELD( m_iszSoundStop, FIELD_STRING, "sound_stop" ),
	DEFINE_KEYFIELD( m_iszSoundTouch, FIELD_STRING, "sound_touch" ),
	DEFINE_KEYFIELD( m_iszSoundDamage, FIELD_STRING, "sound_damage" ),
	DEFINE_KEYFIELD( m_iszSoundBreak, FIELD_STRING, "sound_break" ),
	DEFINE_KEYFIELD( m_iszOnActivate, FIELD_STRING, "on_activate" ),
	DEFINE_KEYFIELD( m_iszOnDeactivate, FIELD_STRING, "on_deactivate" ),
	DEFINE_KEYFIELD( m_iszOnStart, FIELD_STRING, "on_start" ),
	DEFINE_KEYFIELD( m_iszOnStop, FIELD_STRING, "on_stop" ),
	DEFINE_KEYFIELD( m_iszOnForward, FIELD_STRING, "on_forward" ),
	DEFINE_KEYFIELD( m_iszOnReturn, FIELD_STRING, "on_return" ),
	DEFINE_KEYFIELD( m_iszOnTouch, FIELD_STRING, "on_touch" ),
	DEFINE_KEYFIELD( m_iszOnEnter, FIELD_STRING, "on_enter" ),
	DEFINE_KEYFIELD( m_iszOnExit, FIELD_STRING, "on_exit" ),
	DEFINE_KEYFIELD( m_iszOnBlocked, FIELD_STRING, "on_blocked" ),
	DEFINE_KEYFIELD( m_iszOnDamaged, FIELD_STRING, "on_damaged" ),
	DEFINE_KEYFIELD( m_iszOnBreak, FIELD_STRING, "on_break" ),
	DEFINE_FIELD( m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( m_iGibModelIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextTouchSound, FIELD_TIME ),
	DEFINE_FIELD( m_flPendulumPhase, FIELD_FLOAT ),
	DEFINE_FIELD( m_hActivator, FIELD_EHANDLE ),
	DEFINE_ARRAY( m_hTouching, FIELD_EHANDLE, MOVER_MAX_TOUCHES ),
	DEFINE_ARRAY( m_flTouchSeen, FIELD_TIME, MOVER_MAX_TOUCHES ),
	DEFINE_FIELD( m_bLoopSound, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBroken, FIELD_BOOLEAN ),
	DEFINE_FUNCTION( Use ),
	DEFINE_FUNCTION( Touch ),
	DEFINE_FUNCTION( Think ),
	DEFINE_FUNCTION( ArriveForward ),
	DEFINE_FUNCTION( ArriveReturn ),
END_DATADESC()

void CFuncMover::KeyValue( KeyValueData *pkvd )
{
	#define MOVER_INT_KEY( key, member ) if( FStrEq( pkvd->szKeyName, key ) ) { member = Q_atoi( pkvd->szValue ); pkvd->fHandled = TRUE; return; }
	#define MOVER_FLOAT_KEY( key, member ) if( FStrEq( pkvd->szKeyName, key ) ) { member = Q_atof( pkvd->szValue ); pkvd->fHandled = TRUE; return; }
	#define MOVER_STRING_KEY( key, member ) if( FStrEq( pkvd->szKeyName, key ) ) { member = ALLOC_STRING( pkvd->szValue ); pkvd->fHandled = TRUE; return; }
	#define MOVER_VECTOR_KEY( key, member ) if( FStrEq( pkvd->szKeyName, key ) ) { UTIL_StringToVector( member, pkvd->szValue ); pkvd->fHandled = TRUE; return; }

	MOVER_INT_KEY( "preset", m_iPreset );
	MOVER_INT_KEY( "visual_mode", m_iVisualMode );
	MOVER_INT_KEY( "motion_mode", m_iMotionMode );
	MOVER_INT_KEY( "touch_filter", m_iTouchFilter );
	MOVER_INT_KEY( "material", m_iMaterial );
	MOVER_INT_KEY( "gib_count", m_iGibCount );
	MOVER_INT_KEY( "damage_mask", m_iDamageMask );
	MOVER_INT_KEY( "move_direction", m_iDirectionMode );
	MOVER_FLOAT_KEY( "move_speed", m_flMoveSpeed );
	MOVER_FLOAT_KEY( "move_distance", m_flMoveDistance );
	MOVER_FLOAT_KEY( "return_delay", m_flReturnDelay );
	MOVER_FLOAT_KEY( "touch_cooldown", m_flTouchCooldown );
	MOVER_FLOAT_KEY( "min_damage", m_flMinDamage );
	MOVER_FLOAT_KEY( "sound_volume", m_flVolume );
	MOVER_FLOAT_KEY( "sound_attenuation", m_flAttenuation );
	MOVER_FLOAT_KEY( "pendulum_speed", m_flPendulumSpeed );
	MOVER_FLOAT_KEY( "think_interval", m_flThinkInterval );
	MOVER_VECTOR_KEY( "move_offset", m_vecMoveOffset );
	MOVER_VECTOR_KEY( "move_angles", m_vecMoveAngles );
	MOVER_VECTOR_KEY( "angle_offset", m_vecAngleOffset );
	MOVER_VECTOR_KEY( "rotation_speed", m_vecRotationSpeed );
	MOVER_VECTOR_KEY( "collision_mins", m_vecStudioMins );
	MOVER_VECTOR_KEY( "collision_maxs", m_vecStudioMaxs );
	MOVER_STRING_KEY( "sequence_idle", m_iszSequenceIdle );
	MOVER_STRING_KEY( "sequence_active", m_iszSequenceActive );
	MOVER_STRING_KEY( "sequence_break", m_iszSequenceBreak );
	MOVER_STRING_KEY( "gib_model", m_iszGibModel );
	MOVER_STRING_KEY( "sound_start", m_iszSoundStart );
	MOVER_STRING_KEY( "sound_move", m_iszSoundMove );
	MOVER_STRING_KEY( "sound_stop", m_iszSoundStop );
	MOVER_STRING_KEY( "sound_touch", m_iszSoundTouch );
	MOVER_STRING_KEY( "sound_damage", m_iszSoundDamage );
	MOVER_STRING_KEY( "sound_break", m_iszSoundBreak );
	MOVER_STRING_KEY( "on_activate", m_iszOnActivate );
	MOVER_STRING_KEY( "on_deactivate", m_iszOnDeactivate );
	MOVER_STRING_KEY( "on_start", m_iszOnStart );
	MOVER_STRING_KEY( "on_stop", m_iszOnStop );
	MOVER_STRING_KEY( "on_forward", m_iszOnForward );
	MOVER_STRING_KEY( "on_return", m_iszOnReturn );
	MOVER_STRING_KEY( "on_touch", m_iszOnTouch );
	MOVER_STRING_KEY( "on_enter", m_iszOnEnter );
	MOVER_STRING_KEY( "on_exit", m_iszOnExit );
	MOVER_STRING_KEY( "on_blocked", m_iszOnBlocked );
	MOVER_STRING_KEY( "on_damaged", m_iszOnDamaged );
	MOVER_STRING_KEY( "on_break", m_iszOnBreak );

	#undef MOVER_INT_KEY
	#undef MOVER_FLOAT_KEY
	#undef MOVER_STRING_KEY
	#undef MOVER_VECTOR_KEY
	BaseClass::KeyValue( pkvd );
}

void CFuncMover::ApplyPreset( void )
{
	if( m_flMoveSpeed <= 0 ) m_flMoveSpeed = 100;
	if( m_flThinkInterval <= 0 ) m_flThinkInterval = 0.05f;
	if( m_flTouchCooldown <= 0 ) m_flTouchCooldown = 0.25f;
	if( m_flVolume <= 0 ) m_flVolume = 1.0f;
	if( m_flAttenuation <= 0 ) m_flAttenuation = ATTN_NORM;
	if( m_iGibCount <= 0 ) m_iGibCount = 6;
	if( m_flPendulumSpeed <= 0 ) m_flPendulumSpeed = 1.0f;

	// Presets only fill behavior that was not selected explicitly.
	switch( m_iPreset )
	{
	case 3: // toggle appliance
		if( m_iMotionMode == MOVER_MOTION_NONE ) m_iMotionMode = MOVER_MOTION_NONE;
		pev->spawnflags |= SF_MOVER_TOGGLE;
		break;
	case 4: // sliding door/drawer
		if( m_iMotionMode == MOVER_MOTION_NONE ) m_iMotionMode = MOVER_MOTION_LINEAR;
		pev->spawnflags |= SF_MOVER_TOGGLE;
		break;
	case 5: // rotating door
		if( m_iMotionMode == MOVER_MOTION_NONE ) m_iMotionMode = MOVER_MOTION_ANGULAR;
		pev->spawnflags |= SF_MOVER_TOGGLE;
		break;
	case 6: // button
		if( m_iMotionMode == MOVER_MOTION_NONE ) m_iMotionMode = MOVER_MOTION_LINEAR;
		pev->spawnflags |= SF_MOVER_AUTO_RETURN;
		if( m_flReturnDelay == 0 ) m_flReturnDelay = 0.1f;
		break;
	case 7: // glass
		if( pev->health <= 0 ) pev->health = 20;
		m_iMaterial = matGlass;
		break;
	case 8: // breakable prop
		if( pev->health <= 0 ) pev->health = 40;
		break;
	case 10: // rotating
		m_iMotionMode = MOVER_MOTION_ROTATE;
		break;
	case 11: // pendulum
		m_iMotionMode = MOVER_MOTION_PENDULUM;
		break;
	case 12: // foliage
		pev->spawnflags |= SF_MOVER_TRACK_ENTER_EXIT;
		break;
	}
}

void CFuncMover::Precache( void )
{
	if( !FStringNull( pev->model ) ) PRECACHE_MODEL( GetModel() );
	string_t sounds[] = { m_iszSoundStart, m_iszSoundMove, m_iszSoundStop, m_iszSoundTouch, m_iszSoundDamage, m_iszSoundBreak };
	for( int i = 0; i < ARRAYSIZE( sounds ); ++i )
		if( !FStringNull( sounds[i] ) ) PRECACHE_SOUND( (char *)STRING( sounds[i] ) );

	if( !FStringNull( m_iszGibModel ) )
		m_iGibModelIndex = PRECACHE_MODEL( STRING( m_iszGibModel ) );
	else
	{
		const char *model = "models/woodgibs.mdl";
		if( m_iMaterial == matGlass || m_iMaterial == matUnbreakableGlass ) model = "models/glassgibs.mdl";
		else if( m_iMaterial == matMetal || m_iMaterial == matComputer ) model = "models/metalplategibs.mdl";
		else if( m_iMaterial == matFlesh ) model = "models/fleshgibs.mdl";
		else if( m_iMaterial == matCinderBlock || m_iMaterial == matRocks ) model = "models/cindergibs.mdl";
		m_iGibModelIndex = PRECACHE_MODEL( model );
	}
}

void CFuncMover::Spawn( void )
{
	ApplyPreset();
	Precache();
	SetupVisual();
	SetupMotion();
	SetUse( &CFuncMover::Use );
	if( FBitSet( pev->spawnflags, SF_MOVER_TOUCH_ACTIVATE | SF_MOVER_TRACK_ENTER_EXIT ) )
		SetTouch( &CFuncMover::Touch );
	if( pev->health > 0 && m_iMaterial != matUnbreakableGlass )
		pev->takedamage = DAMAGE_YES;
	else pev->takedamage = DAMAGE_NO;

	m_iState = STATE_OFF;
	SetActiveSequence( FALSE );
	SetThink( &CFuncMover::Think );
	SetNextThink( m_flThinkInterval );
	if( FBitSet( pev->spawnflags, SF_MOVER_START_ACTIVE ) )
		ActivateMover( this );
}

void CFuncMover::SetupVisual( void )
{
	const char *model = GetModel();
	if( m_iVisualMode == MOVER_VISUAL_AUTO )
		m_iVisualMode = ( model && model[0] == '*' ) ? MOVER_VISUAL_BRUSH : MOVER_VISUAL_STUDIO;

	if( m_iVisualMode != MOVER_VISUAL_INVISIBLE && model && model[0] ) SET_MODEL( edict(), model );
	pev->movetype = MOVETYPE_PUSH;
	if( FBitSet( pev->spawnflags, SF_MOVER_PASSABLE ) )
		pev->solid = FBitSet( pev->spawnflags, SF_MOVER_TOUCH_ACTIVATE | SF_MOVER_TRACK_ENTER_EXIT ) ? SOLID_TRIGGER : SOLID_NOT;
	else pev->solid = ( m_iVisualMode == MOVER_VISUAL_BRUSH ) ? SOLID_BSP : SOLID_BBOX;
	if( m_iVisualMode == MOVER_VISUAL_INVISIBLE ) pev->effects |= EF_NODRAW;
	if( m_iVisualMode == MOVER_VISUAL_STUDIO ) AutoSetStudioSize();
	UTIL_SetOrigin( this, GetLocalOrigin() );
	if( pev->solid != SOLID_NOT ) m_pUserData = WorldPhysic->CreateKinematicBodyFromEntity( this );
}

void CFuncMover::AutoSetStudioSize( void )
{
	if( m_vecStudioMaxs != g_vecZero || m_vecStudioMins != g_vecZero )
	{
		UTIL_SetSize( pev, m_vecStudioMins, m_vecStudioMaxs );
		return;
	}
	studiohdr_t *header = (studiohdr_t *)GET_MODEL_PTR( edict() );
	if( !header || header->numseq <= 0 )
	{
		UTIL_SetSize( pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) );
		return;
	}
	mstudioseqdesc_t *sequences = (mstudioseqdesc_t *)((byte *)header + header->seqindex);
	UTIL_SetSize( pev, sequences[0].bbmin, sequences[0].bbmax );
}

void CFuncMover::SetupMotion( void )
{
	if( m_iDirectionMode != MOVER_DIR_OFFSET )
	{
		Vector direction;
		switch( m_iDirectionMode )
		{
		case MOVER_DIR_POS_X: direction = Vector( 1, 0, 0 ); break;
		case MOVER_DIR_NEG_X: direction = Vector( -1, 0, 0 ); break;
		case MOVER_DIR_POS_Y: direction = Vector( 0, 1, 0 ); break;
		case MOVER_DIR_NEG_Y: direction = Vector( 0, -1, 0 ); break;
		case MOVER_DIR_UP: direction = Vector( 0, 0, 1 ); break;
		case MOVER_DIR_DOWN: direction = Vector( 0, 0, -1 ); break;
		case MOVER_DIR_ANGLES: UTIL_MakeVectorsPrivate( m_vecMoveAngles, direction, NULL, NULL ); break;
		default: direction = g_vecZero; break;
		}
		m_vecMoveOffset = direction.Normalize() * m_flMoveDistance;
	}
	m_vecPosition1 = GetLocalOrigin();
	m_vecPosition2 = m_vecPosition1 + m_vecMoveOffset;
	m_vecAngle1 = GetLocalAngles();
	m_vecAngle2 = m_vecAngle1 + m_vecAngleOffset;
	if( m_iMotionMode == MOVER_MOTION_ROTATE && FBitSet( pev->spawnflags, SF_MOVER_START_ACTIVE ) )
		SetLocalAvelocity( m_vecRotationSpeed );
}

int CFuncMover::ObjectCaps( void )
{
	int caps = BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	if( !FBitSet( pev->spawnflags, SF_MOVER_TOUCH_ACTIVATE ) ) caps |= FCAP_IMPULSE_USE;
	return caps;
}

void CFuncMover::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( m_bBroken ) return;
	if( FBitSet( pev->spawnflags, SF_MOVER_BREAK_ON_USE ) && pev->health > 0 )
	{
		Break( pActivator );
		return;
	}
	if( IsLockedByMaster( pActivator ) ) return;
	if( useType == USE_OFF ) DeactivateMover( pActivator );
	else if( useType == USE_ON ) ActivateMover( pActivator );
	else if( m_iState == STATE_ON || m_iState == STATE_TURN_ON ) DeactivateMover( pActivator );
	else ActivateMover( pActivator );
}

void CFuncMover::ActivateMover( CBaseEntity *pActivator )
{
	if( m_iState == STATE_ON || m_iState == STATE_TURN_ON ) return;
	m_hActivator = pActivator;
	m_iState = STATE_TURN_ON;
	FireOutput( m_iszOnActivate, pActivator, USE_ON );
	SetActiveSequence( TRUE );
	BeginForward();
}

void CFuncMover::DeactivateMover( CBaseEntity *pActivator )
{
	if( m_iState == STATE_OFF || m_iState == STATE_TURN_OFF ) return;
	m_hActivator = pActivator;
	FireOutput( m_iszOnDeactivate, pActivator, USE_OFF );
	BeginReturn();
}

void CFuncMover::BeginForward( void )
{
	PlaySound( m_iszSoundStart );
	StartLoopSound();
	FireOutput( m_iszOnStart, m_hActivator, USE_ON );
	SetMoveDone( &CFuncMover::ArriveForward );
	switch( m_iMotionMode )
	{
	case MOVER_MOTION_LINEAR: LinearMove( m_vecPosition2, m_flMoveSpeed ); break;
	case MOVER_MOTION_ANGULAR: AngularMove( m_vecAngle2, m_flMoveSpeed ); break;
	case MOVER_MOTION_COMPLEX: ComplexMove( m_vecPosition2, m_vecAngle2, m_flMoveSpeed ); break;
	case MOVER_MOTION_ROTATE: SetLocalAvelocity( m_vecRotationSpeed ); ArriveForward(); break;
	case MOVER_MOTION_PENDULUM: ArriveForward(); break;
	default: ArriveForward(); break;
	}
}

void CFuncMover::BeginReturn( void )
{
	m_iState = STATE_TURN_OFF;
	PlaySound( m_iszSoundStart );
	StartLoopSound();
	FireOutput( m_iszOnStart, m_hActivator, USE_OFF );
	SetMoveDone( &CFuncMover::ArriveReturn );
	switch( m_iMotionMode )
	{
	case MOVER_MOTION_LINEAR: LinearMove( m_vecPosition1, m_flMoveSpeed ); break;
	case MOVER_MOTION_ANGULAR: AngularMove( m_vecAngle1, m_flMoveSpeed ); break;
	case MOVER_MOTION_COMPLEX: ComplexMove( m_vecPosition1, m_vecAngle1, m_flMoveSpeed ); break;
	case MOVER_MOTION_ROTATE: SetLocalAvelocity( g_vecZero ); ArriveReturn(); break;
	case MOVER_MOTION_PENDULUM: ArriveReturn(); break;
	default: ArriveReturn(); break;
	}
}

void CFuncMover::ArriveForward( void )
{
	m_iState = STATE_ON;
	StopLoopSound();
	PlaySound( m_iszSoundStop );
	FireOutput( m_iszOnStop, m_hActivator, USE_ON );
	FireOutput( m_iszOnForward, m_hActivator, USE_ON );
	if( FBitSet( pev->spawnflags, SF_MOVER_AUTO_RETURN ) )
	{
		if( m_flReturnDelay <= 0 ) BeginReturn();
		else m_flActivateFinished = gpGlobals->time + m_flReturnDelay;
	}
}

void CFuncMover::ArriveReturn( void )
{
	m_iState = STATE_OFF;
	m_flActivateFinished = 0;
	StopLoopSound();
	PlaySound( m_iszSoundStop );
	SetActiveSequence( FALSE );
	FireOutput( m_iszOnStop, m_hActivator, USE_OFF );
	FireOutput( m_iszOnReturn, m_hActivator, USE_OFF );
}

void CFuncMover::Think( void )
{
	if( m_bBroken ) return;
	float interval = StudioFrameAdvance();
	DispatchAnimEvents( interval );
	if( m_iMotionMode == MOVER_MOTION_PENDULUM && m_iState == STATE_ON )
	{
		m_flPendulumPhase += m_flPendulumSpeed * m_flThinkInterval;
		Vector angles = m_vecAngle1 + m_vecAngleOffset * sin( m_flPendulumPhase );
		UTIL_SetAngles( this, angles );
	}
	if( m_flActivateFinished > 0 && gpGlobals->time >= m_flActivateFinished ) BeginReturn();
	if( FBitSet( pev->spawnflags, SF_MOVER_TRACK_ENTER_EXIT ) ) PruneTouches();
	SetNextThink( m_flThinkInterval );
}

BOOL CFuncMover::CanTouchMover( CBaseEntity *pOther ) const
{
	if( !pOther || pOther == this ) return FALSE;
	if( m_iTouchFilter == MOVER_TOUCH_PLAYERS ) return pOther->IsPlayer();
	if( m_iTouchFilter == MOVER_TOUCH_PLAYERS_MONSTERS ) return pOther->IsPlayer() || pOther->IsMonster();
	return TRUE;
}

void CFuncMover::Touch( CBaseEntity *pOther )
{
	if( !CanTouchMover( pOther ) || m_bBroken ) return;
	if( FBitSet( pev->spawnflags, SF_MOVER_TRACK_ENTER_EXIT ) ) RegisterTouch( pOther );
	if( gpGlobals->time >= m_flNextTouchSound )
	{
		PlaySound( m_iszSoundTouch, CHAN_BODY );
		FireOutput( m_iszOnTouch, pOther );
		m_flNextTouchSound = gpGlobals->time + m_flTouchCooldown;
	}
	if( FBitSet( pev->spawnflags, SF_MOVER_TOUCH_ACTIVATE ) && !FBitSet( pev->spawnflags, SF_MOVER_USE_ONLY ) && m_iState == STATE_OFF ) ActivateMover( pOther );
}

void CFuncMover::RegisterTouch( CBaseEntity *pOther )
{
	int empty = -1;
	for( int i = 0; i < MOVER_MAX_TOUCHES; ++i )
	{
		if( m_hTouching[i] == pOther ) { m_flTouchSeen[i] = gpGlobals->time; return; }
		if( !m_hTouching[i] && empty < 0 ) empty = i;
	}
	if( empty < 0 ) return;
	m_hTouching[empty] = pOther;
	m_flTouchSeen[empty] = gpGlobals->time;
	FireOutput( m_iszOnEnter, pOther, USE_ON );
}

void CFuncMover::PruneTouches( void )
{
	for( int i = 0; i < MOVER_MAX_TOUCHES; ++i )
	{
		CBaseEntity *other = m_hTouching[i];
		if( !other ) continue;
		if( gpGlobals->time - m_flTouchSeen[i] > m_flThinkInterval * 2.5f || !Intersects( other ) )
		{
			FireOutput( m_iszOnExit, other, USE_OFF );
			m_hTouching[i] = NULL;
			m_flTouchSeen[i] = 0;
		}
	}
}

void CFuncMover::Blocked( CBaseEntity *pOther )
{
	FireOutput( m_iszOnBlocked, pOther );
	if( pev->dmg > 0 ) pOther->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );
}

int CFuncMover::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( m_bBroken || pev->takedamage == DAMAGE_NO ) return 0;
	if( FBitSet( pev->spawnflags, SF_MOVER_BLAST_ONLY ) && !( bitsDamageType & DMG_BLAST ) ) return 0;
	if( m_iDamageMask && !( bitsDamageType & m_iDamageMask ) ) return 0;
	if( flDamage < m_flMinDamage ) return 0;
	pev->health -= flDamage;
	PlaySound( m_iszSoundDamage, CHAN_BODY );
	FireOutput( m_iszOnDamaged, CBaseEntity::Instance( pevAttacker ), USE_SET, pev->health );
	if( pev->health <= 0 ) Break( CBaseEntity::Instance( pevAttacker ) );
	return 1;
}

void CFuncMover::Break( CBaseEntity *pActivator )
{
	if( m_bBroken ) return;
	m_bBroken = TRUE;
	pev->takedamage = DAMAGE_NO;
	SetLocalVelocity( g_vecZero );
	SetLocalAvelocity( g_vecZero );
	StopLoopSound();
	PlaySound( m_iszSoundBreak, CHAN_BODY );
	FireOutput( m_iszOnBreak, pActivator );
	CreateBreakEffect();
	if( !FStringNull( m_iszSequenceBreak ) && m_iVisualMode == MOVER_VISUAL_STUDIO )
	{
		int sequence = LookupSequence( STRING( m_iszSequenceBreak ) );
		if( sequence >= 0 ) { pev->sequence = sequence; pev->frame = 0; ResetSequenceInfo(); }
		pev->solid = SOLID_NOT;
		return;
	}
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	SetTouch( NULL );
	DontThink();
}

void CFuncMover::CreateBreakEffect( void )
{
	Vector center = Center();
	int flags = BREAK_WOOD;
	if( m_iMaterial == matGlass || m_iMaterial == matUnbreakableGlass ) flags = BREAK_GLASS;
	else if( m_iMaterial == matMetal || m_iMaterial == matComputer ) flags = BREAK_METAL;
	else if( m_iMaterial == matFlesh ) flags = BREAK_FLESH;
	else if( m_iMaterial == matCinderBlock || m_iMaterial == matRocks ) flags = BREAK_CONCRETE;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, center );
		WRITE_BYTE( TE_BREAKMODEL );
		WRITE_COORD( center.x ); WRITE_COORD( center.y ); WRITE_COORD( center.z );
		WRITE_COORD( pev->size.x ); WRITE_COORD( pev->size.y ); WRITE_COORD( pev->size.z );
		WRITE_COORD( 0 ); WRITE_COORD( 0 ); WRITE_COORD( 0 );
		WRITE_BYTE( 10 ); WRITE_SHORT( m_iGibModelIndex ); WRITE_BYTE( bound( 1, m_iGibCount, 255 ) ); WRITE_BYTE( 25 ); WRITE_BYTE( flags );
	MESSAGE_END();
}

void CFuncMover::SetActiveSequence( BOOL active )
{
	if( m_iVisualMode != MOVER_VISUAL_STUDIO ) return;
	string_t name = active ? m_iszSequenceActive : m_iszSequenceIdle;
	if( FStringNull( name ) ) return;
	int sequence = LookupSequence( STRING( name ) );
	if( sequence < 0 ) { ALERT( at_warning, "%s %s: unknown sequence %s\n", GetClassname(), GetDebugName(), STRING( name ) ); return; }
	pev->sequence = sequence;
	pev->frame = 0;
	ResetSequenceInfo();
}

void CFuncMover::HandleAnimEvent( MonsterEvent_t *event )
{
	if( event->event == SCRIPT_EVENT_SOUND || event->event == SCRIPT_EVENT_SOUND_VOICE )
		EMIT_SOUND( edict(), event->event == SCRIPT_EVENT_SOUND ? CHAN_BODY : CHAN_VOICE, event->options, m_flVolume, m_flAttenuation );
	else if( event->event == SCRIPT_EVENT_FIREEVENT )
		UTIL_FireTargets( event->options, m_hActivator, this, USE_TOGGLE, 0 );
}

void CFuncMover::PlaySound( string_t sound, int channel )
{
	if( !FStringNull( sound ) ) EMIT_SOUND( edict(), channel, STRING( sound ), m_flVolume, m_flAttenuation );
}

void CFuncMover::StartLoopSound( void )
{
	if( m_bLoopSound || FStringNull( m_iszSoundMove ) ) return;
	EMIT_SOUND( edict(), CHAN_STATIC, STRING( m_iszSoundMove ), m_flVolume, m_flAttenuation );
	m_bLoopSound = TRUE;
}

void CFuncMover::StopLoopSound( void )
{
	if( !m_bLoopSound || FStringNull( m_iszSoundMove ) ) return;
	STOP_SOUND( edict(), CHAN_STATIC, STRING( m_iszSoundMove ) );
	m_bLoopSound = FALSE;
}

void CFuncMover::FireOutput( string_t output, CBaseEntity *pActivator, USE_TYPE useType, float value )
{
	if( !FStringNull( output ) ) UTIL_FireTargets( output, pActivator ? pActivator : this, this, useType, value );
}
