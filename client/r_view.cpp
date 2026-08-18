//=======================================================================
//			Copyright (C) Shambler Team 2005
//			r_view.cpp - multipass renderer
//=======================================================================

#include "hud.h"
#include "utils.h"
#include "r_view.h"
#include "ref_params.h"
#include "pm_movevars.h"
#include "pm_defs.h"
#include "gl_local.h"
#include "r_studioint.h"
#include "gl_studio.h"
#include "gl_cvars.h"
#include "func_car_shared.h"
#include "keydefs.h"
#include <mathlib.h>

// thirdperson camera
void CAM_Think( void ) {}
extern "C" {
	void DLLEXPORT CL_CameraOffset(float *ofs) { g_vecZero.CopyToArray(ofs); }
	int DLLEXPORT CL_IsThirdPerson(void) { return ((gHUD.m_iCameraMode || gHUD.m_iIntermission) ? 1 : 0); }
}

cl_entity_t *v_intermission_spot;
float v_idlescale;
static bool is_paused = false;

cvar_t	*cl_bobcycle;
cvar_t	*cl_bob;
cvar_t	*cl_bobup;
cvar_t	*cl_waterdist;
cvar_t	*cl_chasedist;
cvar_t	*cl_weaponlag;
cvar_t	*cl_vsmoothing;
cvar_t	*v_centermove;
cvar_t	*v_centerspeed;
cvar_t	*cl_viewsize;
cvar_t	*car_thirdperson;

static bool g_bCarThirdPersonActive = false;
static float g_flCarThirdPersonDistance = 192.0f;

void V_ResetCarThirdPerson()
{
	g_bCarThirdPersonActive = false;
	CVAR_SET_FLOAT( "car_thirdperson", 0.0f );
}

bool V_CarThirdPersonKeyEvent( int down, int keynum )
{
	if( !down || !g_bCarThirdPersonActive )
		return false;

	if( keynum == K_MWHEELUP )
	{
		g_flCarThirdPersonDistance = bound( 72.0f,
			g_flCarThirdPersonDistance - 16.0f, 384.0f );
		return true;
	}
	if( keynum == K_MWHEELDOWN )
	{
		g_flCarThirdPersonDistance = bound( 72.0f,
			g_flCarThirdPersonDistance + 16.0f, 384.0f );
		return true;
	}
	return false;
}

cvar_t	v_iyaw_cycle	= { "v_iyaw_cycle", "2", 0, 2 };
cvar_t	v_iroll_cycle	= { "v_iroll_cycle", "0.5", 0, 0.5 };
cvar_t	v_ipitch_cycle	= { "v_ipitch_cycle", "1", 0, 1 };
cvar_t	v_iyaw_level  	= { "v_iyaw_level", "0.3", 0, 0.3 };
cvar_t	v_iroll_level 	= { "v_iroll_level", "0.1", 0, 0.1 };
cvar_t	v_ipitch_level	= { "v_ipitch_level", "0.3", 0, 0.3 };

//============================================================================== 
//				VIEW RENDERING 
//============================================================================== 

//==========================
// V_ThirdPerson
//
// switch to third person
//==========================
void V_ThirdPerson( void )
{
	gHUD.m_iCameraMode = 1;
}

//==========================
// V_FirstPerson
//
// switch to first person
//==========================
void V_FirstPerson( void )
{
	gHUD.m_iCameraMode = 0;
}

//==========================
// V_Init
//==========================
void V_Init( void )
{
	v_centermove	= CVAR_REGISTER( "v_centermove", "0.15", 0 );
	v_centerspeed	= CVAR_REGISTER( "v_centerspeed","500", 0 );

	cl_bobcycle	= CVAR_REGISTER( "cl_bobcycle","0.8", 0 );
	cl_bob		= CVAR_REGISTER( "cl_bob","0.01", 0 );
	cl_bobup		= CVAR_REGISTER( "cl_bobup","0.5", 0 );
	cl_waterdist	= CVAR_REGISTER( "cl_waterdist","4", 0 );
	cl_chasedist	= CVAR_REGISTER( "cl_chasedist","112", 0 );
	cl_weaponlag	= CVAR_REGISTER( "cl_weaponlag", "0.3", FCVAR_ARCHIVE );
	cl_vsmoothing = CVAR_REGISTER("cl_vsmoothing", "0.05", FCVAR_ARCHIVE);
	cl_viewsize = CVAR_GET_POINTER("viewsize");
	car_thirdperson = CVAR_REGISTER( "car_thirdperson", "0", 0 );
	
	R_InitializeConVars();
	ADD_COMMAND( "thirdperson", V_ThirdPerson );
	ADD_COMMAND( "firstperson", V_FirstPerson );
	ADD_COMMAND( "centerview", V_StartPitchDrift );
}

//==========================
// V_CalcBob
//
// Quakeworld bob code, this fixes jitters in the mutliplayer
// since the clock (pparams->time) isn't quite linear
//==========================
float V_CalcBob( struct ref_params_s *pparams )
{
	static double bobtime;
	static float bob, lasttime;
	float cycle;
	Vector vel;

	if( pparams->onground == -1 || pparams->time == lasttime )
		return bob;	

	lasttime = pparams->time;

	bobtime += pparams->frametime;
	cycle = bobtime - (int)( bobtime / cl_bobcycle->value ) * cl_bobcycle->value;
	cycle /= cl_bobcycle->value;
	
	if( cycle < cl_bobup->value )
	{
		cycle = M_PI * cycle / cl_bobup->value;
	}
	else
	{
		cycle = M_PI + M_PI * ( cycle - cl_bobup->value ) / ( 1.0f - cl_bobup->value );
	}

	vel = pparams->simvel;
	bob = sqrt( vel.x * vel.x + vel.y * vel.y ) * cl_bob->value;
	bob = bob * 0.3f + bob * 0.7f * sin( cycle );

	return bound( -7, bob, 4 );
}

extern cvar_t *cl_forwardspeed;

static struct
{
	float	pitchvel;
	int  	nodrift;
	float	driftmove;
	double	laststop;
} pd;

//==========================
// V_StartPitchDrift
//==========================
void V_StartPitchDrift( void )
{
	if( pd.laststop == GET_CLIENT_TIME( ))
	{
		// something else is keeping it from drifting
		return;
	}

	if( pd.nodrift || !pd.pitchvel )
	{
		pd.pitchvel = v_centerspeed->value;
		pd.driftmove = 0;
		pd.nodrift = 0;
	}
}

//==========================
// V_StopPitchDrift
//==========================
void V_StopPitchDrift( void )
{
	pd.laststop = GET_CLIENT_TIME();
	pd.nodrift = 1;
	pd.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
===============
*/
void V_DriftPitch( struct ref_params_s *pparams )
{
	if( gEngfuncs.IsNoClipping() || !pparams->onground || pparams->demoplayback )
	{
		pd.driftmove = 0;
		pd.pitchvel = 0;
		return;
	}

	if( pd.nodrift )
	{
		if( fabs( pparams->cmd->forwardmove ) < cl_forwardspeed->value )
		{
			pd.driftmove = 0;
		}
		else
		{
			pd.driftmove += pparams->frametime;
		}

		if( pd.driftmove > v_centermove->value )
			V_StartPitchDrift ();
		return;
	}
	
	float delta = pparams->idealpitch - pparams->cl_viewangles[PITCH];

	if( !delta )
	{
		pd.pitchvel = 0;
		return;
	}

	float move = pparams->frametime * pd.pitchvel;
	pd.pitchvel += pparams->frametime * v_centerspeed->value;
	
	if( delta > 0 )
	{
		if( move > delta )
		{
			pd.pitchvel = 0;
			move = delta;
		}

		pparams->cl_viewangles[PITCH] += move;
	}
	else if( delta < 0 )
	{
		if( move > -delta )
		{
			pd.pitchvel = 0;
			move = -delta;
		}

		pparams->cl_viewangles[PITCH] -= move;
	}
}

//==========================
// V_CalcGunAngle
//==========================
void V_CalcGunAngle( struct ref_params_s *pparams )
{	
	cl_entity_t *viewent;
	
	viewent = GET_VIEWMODEL();
	if( !viewent ) return;

	viewent->angles[YAW] = pparams->viewangles[YAW] + pparams->crosshairangle[YAW];
	viewent->angles[PITCH] = pparams->viewangles[PITCH] + pparams->crosshairangle[PITCH] * 0.25f;
	viewent->angles[ROLL] -= v_idlescale * sin( pparams->time * v_iroll_cycle.value ) * v_iroll_level.value;
	
	// don't apply all of the v_ipitch to prevent normally unseen parts of viewmodel from coming into view.
	viewent->angles[PITCH] -= v_idlescale * sin( pparams->time*v_ipitch_cycle.value ) * (v_ipitch_level.value * 0.5f);
	viewent->angles[YAW] -= v_idlescale * sin( pparams->time*v_iyaw_cycle.value ) * v_iyaw_level.value;

	viewent->latched.prevangles = viewent->angles;
	viewent->curstate.angles = viewent->angles;
}

//==========================
// V_CalcViewModelLag
//==========================
void V_CalcViewModelLag( ref_params_t *pparams, Vector &origin, Vector &angles, const Vector &original_angles )
{
	static Vector m_vecLastFacing;
	Vector vOriginalOrigin = origin;
	Vector vOriginalAngles = angles;

	// Calculate our drift
	Vector forward, right, up;

	AngleVectors( angles, forward, right, up );

	if( pparams->frametime != 0.0f )	// not in paused
	{
		Vector vDifference;

		vDifference = forward - m_vecLastFacing;

		float flSpeed = 5.0f;

		// If we start to lag too far behind, we'll increase the "catch up" speed.
		// Solves the problem with fast cl_yawspeed, m_yaw or joysticks rotating quickly.
		// The old code would slam lastfacing with origin causing the viewmodel to pop to a new position
		float flDiff = vDifference.Length();

		if(( flDiff > cl_weaponlag->value ) && ( cl_weaponlag->value > 0.0f ))
		{
			float flScale = flDiff / cl_weaponlag->value;
			flSpeed *= flScale;
		}

		// FIXME:  Needs to be predictable?
		m_vecLastFacing = m_vecLastFacing + vDifference * ( flSpeed * pparams->frametime );
		// Make sure it doesn't grow out of control!!!
		m_vecLastFacing = m_vecLastFacing.Normalize();
		origin = origin + (vDifference * -1.0f) * flSpeed;
	}

	AngleVectors( original_angles, forward, right, up );

	float pitch = original_angles[PITCH];

	if( pitch > 180.0f )
	{
		pitch -= 360.0f;
	}
	else if( pitch < -180.0f )
	{
		pitch += 360.0f;
	}

	if( cl_weaponlag->value <= 0.0f )
	{
		origin = vOriginalOrigin;
		angles = vOriginalAngles;
	}
	else
	{
		// FIXME: These are the old settings that caused too many exposed polys on some models
		origin = origin + forward * ( -pitch * 0.035f );
		origin = origin + right * ( -pitch * 0.03f );
		origin = origin + up * ( -pitch * 0.02f );
	}
}

//==========================
// V_AddIdle
//==========================
void V_AddIdle( struct ref_params_s *pparams )
{
	pparams->viewangles[ROLL] += v_idlescale * sin(pparams->time*v_iroll_cycle.value) * v_iroll_level.value;
	pparams->viewangles[PITCH] += v_idlescale * sin(pparams->time*v_ipitch_cycle.value) * v_ipitch_level.value;
	pparams->viewangles[YAW] += v_idlescale * sin(pparams->time*v_iyaw_cycle.value) * v_iyaw_level.value;
}

//==========================
// V_CalcViewRoll
//==========================
void V_CalcViewRoll( struct ref_params_s *pparams )
{
	float   sign, side, value;
	Vector  right;
	
	cl_entity_t *viewentity = GET_ENTITY( pparams->viewentity );
	if( !viewentity ) return;

	AngleVectors( viewentity->angles, NULL, right, NULL );
	side = DotProduct( pparams->simvel, right );
	sign = side < 0 ? -1 : 1;
	side = fabs( side );
	value = pparams->movevars->rollangle;

	if( side < pparams->movevars->rollspeed )
		side = side * value / pparams->movevars->rollspeed;
	else side = value;

	side = side * sign;		
	pparams->viewangles[ROLL] += side;

	if( pparams->health <= 0 && ( pparams->viewheight[2] != 0 ))
	{
		// only roll the view if the player is dead and the viewheight[2] is nonzero 
		// this is so deadcam in multiplayer will work.
		pparams->viewangles[ROLL] = 80; // dead view angle
		return;
	}
}

//==========================
// V_CalcSendOrigin
//==========================
void V_CalcSendOrigin( struct ref_params_s *pparams )
{
	// never let view origin sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// FIXME, we send origin at 1/128 now, change this?
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	pparams->vieworg[0] += 1.0f / 32;
	pparams->vieworg[1] += 1.0f / 32;
	pparams->vieworg[2] += 1.0f / 32;
}

//==========================
// V_GetChaseOrigin
//==========================
void V_GetChaseOrigin( const Vector &angles, const Vector &origin, float distance, Vector &returnvec )
{
	Vector vecStart, vecEnd;
	pmtrace_t *trace;
	int maxLoops = 8;

	Vector forward, right, up;	

	// trace back from the target using the player's view angles
	AngleVectors( angles, forward, right, up );
	forward = -forward;

	vecStart = origin;
	vecEnd = vecStart + forward * distance;

	int ignoreent = -1;	// first, ignore no entity
	cl_entity_t *ent = NULL;

	while( maxLoops > 0 )
	{
		trace = gEngfuncs.PM_TraceLine( vecStart, vecEnd, PM_TRACELINE_PHYSENTSONLY, 2, ignoreent );
		if( trace->ent <= 0 ) break; // we hit the world or nothing, stop trace

		ent = GET_ENTITY( PM_GetPhysEntInfo( trace->ent ));
		if( ent == NULL ) break;

		// hit non-player solid BSP, stop here
		if( ent->curstate.solid == SOLID_BSP && !ent->player )
			break;

		// if close enought to end pos, stop, otherwise continue trace
		if(( vecEnd - trace->endpos ).Length() < 1.0f )
		{
			break;
		}
		else
		{
			ignoreent = trace->ent;	// ignore last hit entity
			vecStart = trace->endpos;
		}
		maxLoops--;
	}  

	returnvec = trace->endpos + trace->plane.normal * 8;
}

// remaps an angular variable to a 3 band function:
// 0 <= t < start :		f(t) = 0
// start <= t <= end :	f(t) = end * spline(( t-start) / (end-start) )  // s curve between clamped and linear
// end < t :			f(t) = t
float RemapAngleRange( float startInterval, float endInterval, float value, RemapAngleRange_CurvePart_t *peCurvePart )
{
	// Fixup the roll
	value = AngleNormalize( value );
	float absAngle = fabs(value);

	// beneath cutoff?
	if( absAngle < startInterval )
	{
		if( peCurvePart )
		{
			*peCurvePart = RemapAngleRange_CurvePart_Zero;
		}
		value = 0;
	}
	// in spline range?
	else if( absAngle <= endInterval )
	{
		float newAngle = SimpleSpline(( absAngle - startInterval ) / ( endInterval-startInterval )) * endInterval;

		// grab the sign from the initial value
		if( value < 0 )
		{
			newAngle *= -1;
		}

		if( peCurvePart )
		{
			*peCurvePart = RemapAngleRange_CurvePart_Spline;
		}
		value = newAngle;
	}
	// else leave it alone, in linear range
	else if( peCurvePart )
	{
		*peCurvePart = RemapAngleRange_CurvePart_Linear;
	}

	return value;
}

float ApplyViewLocking( ref_params_t *params, float flAngleRaw, float flAngleClamped, ViewLockData_t &lockData, RemapAngleRange_CurvePart_t eCurvePart )
{
	// If we're set up to never lock this degree of freedom, return the clamped value.
	if( lockData.flLockInterval == 0 )
		return flAngleClamped;

	float flAngleOut = flAngleClamped;

	// Lock the view if we're in the linear part of the curve, and keep it locked
	// until some duration after we return to the flat (zero) part of the curve.
	if(( eCurvePart == RemapAngleRange_CurvePart_Linear ) || ( lockData.bLocked && ( eCurvePart == RemapAngleRange_CurvePart_Spline )))
	{
		lockData.bLocked = true;
		lockData.flUnlockTime = params->time + lockData.flLockInterval;
		flAngleOut = flAngleRaw;
	}
	else
	{
		if(( lockData.bLocked ) && ( params->time > lockData.flUnlockTime ))
		{
			lockData.bLocked = false;
			if ( lockData.flUnlockBlendInterval > 0 )
			{
				lockData.flUnlockTime = params->time;
			}
			else
			{
				lockData.flUnlockTime = 0;
			}
		}

		if ( !lockData.bLocked )
		{
			if( lockData.flUnlockTime != 0 )
			{
				// Blend out from the locked raw view (no remapping) to a remapped view.
				float flBlend = RemapValClamped( params->time-lockData.flUnlockTime, 0, lockData.flUnlockBlendInterval, 0, 1 );
				Msg( "BLEND %f\n", flBlend );

				flAngleOut = Lerp( flBlend, flAngleRaw, flAngleClamped );

				if( flBlend >= 1.0f )
				{
					lockData.flUnlockTime = 0;
				}
			}
			else
			{
				// Not blending out from a locked view to a remapped view.
				Msg( "CLAMPED\n" );
				flAngleOut = flAngleClamped;
			}
		}
		else
		{
			Msg( "STILL LOCKED\n" );
			flAngleOut = flAngleRaw;
		}
	}

	return flAngleOut;
}

void RemapViewAngles( struct ref_params_s *pparams, ViewSmoothingData_t *pData, Vector &vehicleEyeAngles )
{
	Vector vecEyeAnglesRemapped;

	// Clamp pitch.
	RemapAngleRange_CurvePart_t ePitchCurvePart;
	vecEyeAnglesRemapped.x = RemapAngleRange( pData->flPitchCurveZero, pData->flPitchCurveLinear, vehicleEyeAngles.x, &ePitchCurvePart );

	vehicleEyeAngles.z = vecEyeAnglesRemapped.z = AngleNormalize( vehicleEyeAngles.z );

	// Blend out the roll dampening as our pitch approaches 90 degrees, to avoid gimbal lock problems.
	float flBlendRoll = 1.0;

	if( fabs( vehicleEyeAngles.x ) > 60 )
	{
		flBlendRoll = RemapValClamped( fabs( vecEyeAnglesRemapped.x ), 60, 80, 1, 0);
	}

	RemapAngleRange_CurvePart_t eRollCurvePart;
	float flRollDamped = RemapAngleRange( pData->flRollCurveZero, pData->flRollCurveLinear, vecEyeAnglesRemapped.z, &eRollCurvePart );
	vecEyeAnglesRemapped.z = Lerp( flBlendRoll, vecEyeAnglesRemapped.z, flRollDamped );

	//Msg("PITCH ");
	vehicleEyeAngles.x = ApplyViewLocking( pparams, vehicleEyeAngles.x, vecEyeAnglesRemapped.x, pData->pitchLockData, ePitchCurvePart );

	//Msg("ROLL ");
	vehicleEyeAngles.z = ApplyViewLocking( pparams, vehicleEyeAngles.z, vecEyeAnglesRemapped.z, pData->rollLockData, eRollCurvePart );
}

//==========================
// V_CalcCameraRefdef
//==========================
static Vector g_surveillanceCameraAngles[4096];
static bool g_surveillanceCameraAnglesValid[4096] = {};
static float g_surveillanceCameraGeneration[4096] = {};

static void V_CalcCarThirdPersonRefdef( ref_params_t *pparams, cl_entity_t *view )
{
	cl_entity_t *body = view && view->curstate.iuser2 > 0
		? GET_ENTITY( view->curstate.iuser2 ) : NULL;
	Vector target = body ? body->origin : view ? view->origin : pparams->simorg;
	Vector cameraAngles = pparams->cl_viewangles;
	cameraAngles.x = bound( -85.0f, cameraAngles.x, 85.0f );
	cameraAngles.z = 0.0f;

	Vector forward;
	AngleVectors( cameraAngles, forward, NULL, NULL );
	Vector desiredOrigin = target - forward * g_flCarThirdPersonDistance;
	pmtrace_t cameraTrace;
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( target, desiredOrigin,
		PM_WORLD_ONLY, -1, &cameraTrace );
	Vector cameraOrigin = cameraTrace.endpos;
	if( cameraTrace.fraction < 1.0f )
		cameraOrigin += cameraTrace.plane.normal * 8.0f;

	pparams->vieworg = cameraOrigin;
	pparams->simorg = cameraOrigin;
	pparams->viewangles = cameraAngles;
	if( !g_bCarThirdPersonActive )
		gEngfuncs.Con_Printf( "car_thirdperson: third person enabled\n" );
	g_bCarThirdPersonActive = true;
	gEngfuncs.V_CalcShake();
	gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0f );
}

bool V_GetSurveillanceCameraAngles( int visualEntityIndex, float generation, Vector &angles )
{
	if( visualEntityIndex <= 0 || visualEntityIndex >= 4096 ||
		!g_surveillanceCameraAnglesValid[visualEntityIndex] ||
		g_surveillanceCameraGeneration[visualEntityIndex] != generation )
		return false;
	angles = g_surveillanceCameraAngles[visualEntityIndex];
	return true;
}

void V_CalcCameraRefdef( struct ref_params_s *pparams )
{
	static float lasttime, oldz = 0;

	// get viewentity and monster eyeposition
	cl_entity_t *view = GET_ENTITY( pparams->viewentity );

	if( view )
	{
		// The car view entity already contains the exact, interpolated world-space
		// transform of view_pos. Do not run generic monster-camera eye-height,
		// forward-offset or stair smoothing on top of it.
		if( view->curstate.iuser4 == FUNC_CAR_VIEW_MARKER )
		{
			if( CVAR_GET_FLOAT( "car_thirdperson" ) != 0.0f )
			{
				V_CalcCarThirdPersonRefdef( pparams, view );
				return;
			}

			g_bCarThirdPersonActive = false;
			pparams->vieworg = view->origin;
			pparams->simorg = view->origin;
			pparams->viewangles = view->angles;
			gEngfuncs.V_CalcShake();
			gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0f );
			return;
		}

		if( view->curstate.iuser4 == 0x5354434D )
		{
			pparams->vieworg = view->origin;
			pparams->simorg = view->origin;
			pparams->viewangles = pparams->cl_viewangles;
			gEngfuncs.V_CalcShake();
			gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0f );
			return;
		}

		// Surveillance cameras need local mouse angles.  The generic external
		// camera path below replaces them with the network entity angles, which
		// makes SET_VIEW cameras appear locked and lets interpolation separate
		// the eye from their visible model.
		const bool surveillanceView = view->curstate.iuser4 == 0x5346434D ||
			(view->model && strstr( view->model->name, "weapon/Camera/w_camera.mdl" ));
		if( surveillanceView )
		{
			cl_entity_t *visual = view->curstate.iuser2 > 0
				? GET_ENTITY( view->curstate.iuser2 ) : NULL;
			// Never switch the eye origin to the separately networked visual model.
			// It may enter the client snapshot a few frames later and interpolate
			// from an old position, which looked like the camera teleported after
			// viewing had already begun. The hidden view entity is authoritative.
			Vector mountOrigin = view->origin;
			Vector mountAngles = visual ? visual->angles : view->angles;
			Vector cameraAngles = pparams->cl_viewangles;
			bool clamped = false;
			for( int axis = 0; axis < 2; ++axis )
			{
				float delta = AngleNormalize( cameraAngles[axis] - mountAngles[axis] );
				float limited = bound( -45.0f, delta, 45.0f );
				clamped |= limited != delta;
				cameraAngles[axis] = mountAngles[axis] + limited;
			}
			if( clamped )
				gEngfuncs.SetViewAngles( cameraAngles );
			const int visualIndex = visual ? visual->index : view->curstate.iuser2;
			if( visualIndex > 0 && visualIndex < 4096 )
			{
				g_surveillanceCameraAngles[visualIndex] = cameraAngles;
				g_surveillanceCameraAnglesValid[visualIndex] = true;
				g_surveillanceCameraGeneration[visualIndex] = visual ? visual->curstate.fuser1 : 0.0f;
			}
			Vector forward;
			AngleVectors( mountAngles, forward, NULL, NULL );
			pparams->vieworg = mountOrigin + forward * 14.0f;
			pparams->simorg = pparams->vieworg;
			pparams->viewangles = cameraAngles;
			gEngfuncs.V_CalcShake();
			gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0f );
			return;
		}

		pparams->vieworg = view->origin;
		pparams->viewangles = view->angles;

		// Some GoldSrc-compatible engine builds position the sound listener at
		// simorg instead of the calculated view origin. A non-player view entity
		// is the authoritative camera position, so keep both origins together.
		pparams->simorg = view->origin;

		studiohdr_t *viewmonster = (studiohdr_t *)IEngineStudio.Mod_Extradata( view->model );

		if( viewmonster && view->curstate.eflags & EFLAG_SLERP &&
			view->curstate.iuser4 != FUNC_CAR_VIEW_MARKER )
		{
			Vector forward;
			AngleVectors( pparams->viewangles, forward, NULL, NULL );

			Vector viewpos = viewmonster->eyeposition;

			if( viewpos == g_vecZero )
				viewpos = Vector( 0, 0, 8 );	// monster_cockroach

			pparams->vieworg += viewpos + forward * 8;	// best value for humans
			// NOTE: fov computation moved into r_main.cpp
		}

		// this is smooth stair climbing in thirdperson mode but not affected for client model :(
		if( !pparams->smoothing && pparams->onground && view->origin[2] - oldz > 0.0f && viewmonster != NULL )
		{
			float steptime;
		
			steptime = pparams->time - lasttime;
			if( steptime < 0 ) steptime = 0;

			oldz += steptime * 150.0f;

			if( oldz > view->origin[2] )
				oldz = view->origin[2];
			if( view->origin[2] - oldz > pparams->movevars->stepsize )
				oldz = view->origin[2] - pparams->movevars->stepsize;

			pparams->vieworg[2] += oldz - view->origin[2];
		}
		else
		{
			oldz = view->origin[2];
		}

		lasttime = pparams->time;              	

		// g-cont. apply shake to camera
		gEngfuncs.V_CalcShake();
		gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0f );
	}
}

#define MAX_SPOTS	16

//==========================
// V_FindIntermisionSpot
//==========================
cl_entity_t *V_FindIntermisionSpot( struct ref_params_s *pparams )
{
	int i, j, spotindex[MAX_SPOTS];
	cl_entity_t *ent;

	// found intermission point
	for( i = 0, j = 0; i < pparams->max_entities; i++ )
	{
		ent = GET_ENTITY( i );

		if( ent && ent->curstate.eflags & EFLAG_INTERMISSION )
		{
			spotindex[j] = ent->index;
			if( ++j >= MAX_SPOTS ) break; // full
		}
	}	
	
	// ok, we have list of intermission spots
	if( j != 0 )
	{
		const int selectedSpot = ( j > 1 ) ? RANDOM_LONG( 0, j - 1 ) : 0;
		ent = GET_ENTITY( spotindex[selectedSpot] );
	}
	else
	{
		// defaulted to player view
		ent = gEngfuncs.GetLocalPlayer();
	}	

	return ent;
}

//==========================
// V_CalcIntermisionRefdef
//==========================
void V_CalcIntermisionRefdef( struct ref_params_s *pparams )
{
	// Orbit the frozen local player and keep the chase camera out of geometry.
	pparams->vieworg = pparams->simorg + pparams->viewheight;
	pparams->viewangles = pparams->cl_viewangles;
	V_GetChaseOrigin( pparams->viewangles, pparams->vieworg, cl_chasedist->value, pparams->vieworg );

	cl_entity_t *view = GET_VIEWMODEL();
	if( view )
		view->model = NULL;

	// allways idle in intermission
	float old = v_idlescale;
	v_idlescale = 1;
	V_AddIdle( pparams );
	v_idlescale = old;
}

//==========================
// V_CalcThirdPersonRefdef
//==========================
void V_CalcThirdPersonRefdef( struct ref_params_s * pparams )
{
	static float lasttime, oldz = 0;

	pparams->vieworg = pparams->simorg;
	pparams->vieworg += pparams->viewheight;
	pparams->viewangles = pparams->cl_viewangles;

	V_CalcSendOrigin( pparams );

	// this is smooth stair climbing in thirdperson mode but not affected for client model :(
	if( !pparams->smoothing && pparams->onground && pparams->simorg[2] - oldz > 0.0f )
	{
		float steptime;
		
		steptime = pparams->time - lasttime;
		if( steptime < 0 ) steptime = 0;

		oldz += steptime * 150.0f;

		if( oldz > pparams->simorg[2] )
			oldz = pparams->simorg[2];
		if( pparams->simorg[2] - oldz > pparams->movevars->stepsize )
			oldz = pparams->simorg[2] - pparams->movevars->stepsize;

		pparams->vieworg[2] += oldz - pparams->simorg[2];
	}
	else
	{
		oldz = pparams->simorg[2];
	}

	lasttime = pparams->time;

	V_GetChaseOrigin( pparams->viewangles, pparams->vieworg, cl_chasedist->value, pparams->vieworg );

	float pitch = pparams->viewangles[PITCH];

	// normalize angles
	if( pitch > 180.0f ) 
		pitch -= 360.0f;
	else if( pitch < -180.0f )
		pitch += 360.0f;

	// player pitch is inverted
	pitch /= -3.0;

	cl_entity_t *ent = GET_LOCAL_PLAYER();

	// slam local player's pitch value
	ent->angles[PITCH] = pitch;
	ent->curstate.angles[PITCH] = pitch;
	ent->prevstate.angles[PITCH] = pitch;
	ent->latched.prevangles[PITCH] = pitch;
}

//==========================
// V_CalcWaterLevel
//==========================
float V_CalcWaterLevel( struct ref_params_s *pparams )
{
	float waterOffset = 0.0f;
	
	if( pparams->waterlevel >= 2 )
	{
		int waterEntity = WATER_ENTITY( pparams->simorg );
		float waterDist = cl_waterdist->value;

		if( waterEntity >= 0 && waterEntity < pparams->max_entities )
		{
			cl_entity_t *pwater = GET_ENTITY( waterEntity );
			if( pwater && ( pwater->model != NULL ))
				waterDist += ( pwater->curstate.scale * 16.0f );
		}

		Vector point = pparams->vieworg;

		// eyes are above water, make sure we're above the waves
		if( pparams->waterlevel == 2 )	
		{
			point.z -= waterDist;

			for( int i = 0; i < waterDist; i++ )
			{
				int contents = POINT_CONTENTS( point );
				if( contents > CONTENTS_WATER )
					break;
				point.z += 1;
			}
			waterOffset = (point.z + waterDist) - pparams->vieworg[2];
		}
		else
		{
			// eyes are under water. Make sure we're far enough under
			point[2] += waterDist;

			for( int i = 0; i < waterDist; i++ )
			{
				int contents = POINT_CONTENTS( point );
				if( contents <= CONTENTS_WATER )
					break;

				point.z -= 1;
			}
			waterOffset = (point.z - waterDist) - pparams->vieworg[2];
		}
	}

	return waterOffset;
}

#define ORIGIN_BACKUP	64
#define ORIGIN_MASK		( ORIGIN_BACKUP - 1 )

typedef struct 
{
	Vector	Origins[ORIGIN_BACKUP];
	float	OriginTime[ORIGIN_BACKUP];

	Vector	Angles[ORIGIN_BACKUP];
	float	AngleTime[ORIGIN_BACKUP];

	int	CurrentOrigin;
	int	CurrentAngle;
} viewinterp_t;

//==========================
// V_InterpolatePos
//==========================
void V_InterpolatePos( struct ref_params_s *pparams )
{
	static Vector lastorg;
	static viewinterp_t	ViewInterp;

	// view is the weapon model (only visible from inside body )
	cl_entity_t *view = GET_VIEWMODEL();

	Vector delta = pparams->simorg - lastorg;

	if( delta.Length() != 0.0f )
	{
		ViewInterp.Origins[ViewInterp.CurrentOrigin & ORIGIN_MASK] = pparams->simorg;
		ViewInterp.OriginTime[ViewInterp.CurrentOrigin & ORIGIN_MASK] = pparams->time;
		ViewInterp.CurrentOrigin++;

		lastorg = pparams->simorg;
	}

	if( cl_vsmoothing->value && pparams->smoothing && ( pparams->maxclients > 1 ))
	{
		int i, foundidx;
		float t;

		if( cl_vsmoothing->value < 0.0f )
			CVAR_SET_FLOAT( "cl_vsmoothing", 0.0 );

		t = pparams->time - cl_vsmoothing->value;

		for( i = 1; i < ORIGIN_MASK; i++ )
		{
			foundidx = ViewInterp.CurrentOrigin - 1 - i;

			if( ViewInterp.OriginTime[foundidx & ORIGIN_MASK] <= t )
				break;
		}

		if( i < ORIGIN_MASK &&  ViewInterp.OriginTime[foundidx & ORIGIN_MASK] != 0.0f )
		{
			// Interpolate
			Vector neworg;
			double dt, frac;

			dt = ViewInterp.OriginTime[(foundidx + 1) & ORIGIN_MASK] - ViewInterp.OriginTime[foundidx & ORIGIN_MASK];
			if( dt > 0.0f )
			{
				frac = ( t - ViewInterp.OriginTime[foundidx & ORIGIN_MASK] ) / dt;
				delta = ViewInterp.Origins[( foundidx + 1 ) & ORIGIN_MASK] - ViewInterp.Origins[foundidx & ORIGIN_MASK];
				frac = Q_min( 1.0, frac );

				neworg = ViewInterp.Origins[foundidx & ORIGIN_MASK] + delta * frac;

				// dont interpolate large changes (less than 64 units)
				if( delta.Length() < 64 )
				{
					delta = neworg - pparams->simorg;
					pparams->simorg += delta;
					pparams->vieworg += delta;
					view->origin += delta;
				}
			}
		}
	}
}

//==========================
// V_CalcFirstPersonRefdef
//==========================
void V_CalcFirstPersonRefdef( struct ref_params_s *pparams )
{
	V_DriftPitch( pparams );

	float bob = V_CalcBob( pparams );

	pparams->vieworg = pparams->simorg;
	pparams->vieworg += pparams->viewheight;
	pparams->vieworg.z += bob;

	pparams->viewangles = pparams->cl_viewangles;

	gEngfuncs.V_CalcShake();
	gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0f );

	V_CalcSendOrigin( pparams );

	float waterOffset = V_CalcWaterLevel( pparams );
	pparams->vieworg[2] += waterOffset;

	V_CalcViewRoll( pparams );
	V_AddIdle( pparams );

	// offsets
	AngleVectors( pparams->cl_viewangles, pparams->forward, pparams->right, pparams->up );

	cl_entity_t *view = GET_VIEWMODEL();
	Vector lastAngles = view->angles = pparams->cl_viewangles;

	V_CalcGunAngle( pparams );

	// use predicted origin as view origin.
	view->origin = pparams->simorg;      
	view->origin += pparams->viewheight;
	view->origin.z += waterOffset;

	// Let the viewmodel shake at about 10% of the amplitude
	gEngfuncs.V_ApplyShake( view->origin, view->angles, 0.9f );

	view->origin += pparams->forward * bob * 0.4f;
	view->origin.z += bob;

	view->angles[PITCH] -= bob * 0.3f;
	view->angles[YAW] -= bob * 0.5f;
	view->angles[ROLL] -= bob * 1.0f;
	view->origin.z -= 1;

	// fudge position around to keep amount of weapon visible
	// roughly equal with different FOV
	if( pparams->viewsize == 110 )
	{
		view->origin[2] += 1;
	}
	else if( pparams->viewsize == 100 )
	{
		view->origin[2] += 2;
	}
	else if( pparams->viewsize == 90 )
	{
		view->origin[2] += 1;
	}
	else if( pparams->viewsize == 80 )
	{
		view->origin[2] += 0.5;
	}

	V_CalcViewModelLag( pparams, view->origin, view->angles, lastAngles );
		
	pparams->viewangles += pparams->punchangle;

	static float lasttime, oldz = 0;

	if( !pparams->smoothing && pparams->onground && pparams->simorg[2] - oldz > 0.0f )
	{
		float steptime;
		
		steptime = pparams->time - lasttime;
		if( steptime < 0 ) steptime = 0;

		oldz += steptime * 150.0f;

		if( oldz > pparams->simorg[2] )
			oldz = pparams->simorg[2];
		if( pparams->simorg[2] - oldz > pparams->movevars->stepsize )
			oldz = pparams->simorg[2] - pparams->movevars->stepsize;

		pparams->vieworg[2] += oldz - pparams->simorg[2];
		view->origin.z += oldz - pparams->simorg[2];
	}
	else
	{
		oldz = pparams->simorg[2];
	}

	lasttime = pparams->time;

	// smooth player view in multiplayer
	V_InterpolatePos( pparams );
}

//==========================
// V_CalcRefdef
//==========================
void V_CalcRefdef( struct ref_params_s *pparams )
{
	extern bool g_bSurveillanceCameraView;
	extern Vector g_vecSurveillanceCameraOrigin;

	// store a local copy in case we need to calc firstperson later
	memcpy( &tr.viewparams, pparams, sizeof( ref_params_t ));
	g_bSurveillanceCameraView = false;
	is_paused = pparams->paused != 0;
	if( is_paused ) 
		return;

	if( pparams->intermission )
	{
		V_CalcIntermisionRefdef( pparams );
	}
	else if( gHUD.m_Car.IsVisible() && CVAR_GET_FLOAT( "car_thirdperson" ) != 0.0f )
	{
		cl_entity_t *view = pparams->viewentity > pparams->maxclients
			? GET_ENTITY( pparams->viewentity ) : NULL;
		V_CalcCarThirdPersonRefdef( pparams, view );
	}
	else if( pparams->viewentity > pparams->maxclients )
	{
		g_bSurveillanceCameraView = true;
		cl_entity_t *view = GET_ENTITY( pparams->viewentity );
		if( view )
			g_vecSurveillanceCameraOrigin = view->origin;
		V_CalcCameraRefdef( pparams );
	}
	else if( gHUD.m_iCameraMode )
	{
		V_CalcThirdPersonRefdef( pparams );
	}
	else
	{
		V_CalcFirstPersonRefdef( pparams );
	}

}
