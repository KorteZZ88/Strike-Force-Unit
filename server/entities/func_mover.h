#pragma once

#include "cbase.h"
#include "func_break.h"

// Independent mapper-facing features. Exclusive behavior is selected by keyvalues.
#define SF_MOVER_START_ACTIVE       BIT( 0 )
#define SF_MOVER_USE_ONLY          BIT( 1 )
#define SF_MOVER_TOUCH_ACTIVATE    BIT( 2 )
#define SF_MOVER_TRACK_ENTER_EXIT  BIT( 3 )
#define SF_MOVER_AUTO_RETURN       BIT( 4 )
#define SF_MOVER_TOGGLE            BIT( 5 )
#define SF_MOVER_PASSABLE          BIT( 6 )
#define SF_MOVER_BLAST_ONLY        BIT( 7 )
#define SF_MOVER_BREAK_ON_USE      BIT( 8 )

enum MoverVisualMode
{
	MOVER_VISUAL_AUTO = 0,
	MOVER_VISUAL_BRUSH,
	MOVER_VISUAL_STUDIO,
	MOVER_VISUAL_INVISIBLE
};

enum MoverMotionMode
{
	MOVER_MOTION_NONE = 0,
	MOVER_MOTION_LINEAR,
	MOVER_MOTION_ANGULAR,
	MOVER_MOTION_COMPLEX,
	MOVER_MOTION_ROTATE,
	MOVER_MOTION_PENDULUM
};

enum MoverTouchFilter
{
	MOVER_TOUCH_PLAYERS = 0,
	MOVER_TOUCH_PLAYERS_MONSTERS,
	MOVER_TOUCH_ALL
};

enum MoverDirectionMode
{
	MOVER_DIR_OFFSET = 0,
	MOVER_DIR_POS_X,
	MOVER_DIR_NEG_X,
	MOVER_DIR_POS_Y,
	MOVER_DIR_NEG_Y,
	MOVER_DIR_UP,
	MOVER_DIR_DOWN,
	MOVER_DIR_ANGLES
};

#define MOVER_MAX_TOUCHES 16

class CFuncMover : public CBaseToggle
{
	DECLARE_CLASS( CFuncMover, CBaseToggle );
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Touch( CBaseEntity *pOther );
	void Blocked( CBaseEntity *pOther );
	void Think( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	STATE GetState( void ) { return (STATE)m_iState; }
	int ObjectCaps( void );

	DECLARE_DATADESC();

private:
	void ApplyPreset( void );
	void SetupVisual( void );
	void SetupMotion( void );
	void ActivateMover( CBaseEntity *pActivator );
	void DeactivateMover( CBaseEntity *pActivator );
	void BeginForward( void );
	void BeginReturn( void );
	void ArriveForward( void );
	void ArriveReturn( void );
	void StartLoopSound( void );
	void StopLoopSound( void );
	void PlaySound( string_t sound, int channel = CHAN_ITEM );
	void SetActiveSequence( BOOL active );
	void FireOutput( string_t output, CBaseEntity *pActivator, USE_TYPE useType = USE_TOGGLE, float value = 0 );
	BOOL CanTouchMover( CBaseEntity *pOther ) const;
	void RegisterTouch( CBaseEntity *pOther );
	void PruneTouches( void );
	void Break( CBaseEntity *pActivator );
	void CreateBreakEffect( void );
	void AutoSetStudioSize( void );

	int m_iPreset;
	int m_iVisualMode;
	int m_iMotionMode;
	int m_iTouchFilter;
	int m_iState;
	int m_iMaterial;
	int m_iGibModelIndex;
	int m_iGibCount;
	int m_iDamageMask;
	int m_iDirectionMode;
	float m_flMoveSpeed;
	float m_flReturnDelay;
	float m_flTouchCooldown;
	float m_flNextTouchSound;
	float m_flMinDamage;
	float m_flVolume;
	float m_flAttenuation;
	float m_flPendulumSpeed;
	float m_flPendulumPhase;
	float m_flThinkInterval;
	float m_flMoveDistance;
	Vector m_vecMoveOffset;
	Vector m_vecMoveAngles;
	Vector m_vecAngleOffset;
	Vector m_vecRotationSpeed;
	Vector m_vecStudioMins;
	Vector m_vecStudioMaxs;
	string_t m_iszSequenceIdle;
	string_t m_iszSequenceActive;
	string_t m_iszSequenceBreak;
	string_t m_iszGibModel;
	string_t m_iszSoundStart;
	string_t m_iszSoundMove;
	string_t m_iszSoundStop;
	string_t m_iszSoundTouch;
	string_t m_iszSoundDamage;
	string_t m_iszSoundBreak;
	string_t m_iszOnActivate;
	string_t m_iszOnDeactivate;
	string_t m_iszOnStart;
	string_t m_iszOnStop;
	string_t m_iszOnForward;
	string_t m_iszOnReturn;
	string_t m_iszOnTouch;
	string_t m_iszOnEnter;
	string_t m_iszOnExit;
	string_t m_iszOnBlocked;
	string_t m_iszOnDamaged;
	string_t m_iszOnBreak;
	EHANDLE m_hActivator;
	EHANDLE m_hTouching[MOVER_MAX_TOUCHES];
	float m_flTouchSeen[MOVER_MAX_TOUCHES];
	BOOL m_bLoopSound;
	BOOL m_bBroken;
};
