#include "hud.h"
#include "usercmd.h"
#include "cvardef.h"
#include "kbutton.h"
#include "keydefs.h"
#include "input_mouse.h"
extern cvar_t		*sensitivity;
extern cvar_t		*in_joystick;

extern kbutton_t	in_strafe;
extern kbutton_t	in_mlook;
extern kbutton_t	in_speed;
extern kbutton_t	in_jlook;
extern kbutton_t	in_forward;
extern kbutton_t	in_back;
extern kbutton_t	in_moveleft;
extern kbutton_t	in_moveright;

extern cvar_t	*m_pitch;
extern cvar_t	*m_yaw;
extern cvar_t	*m_forward;
extern cvar_t	*m_side;
extern cvar_t	*lookstrafe;
extern cvar_t	*lookspring;
extern cvar_t	*cl_pitchdown;
extern cvar_t	*cl_pitchup;
extern cvar_t	*cl_yawspeed;
extern cvar_t	*cl_sidespeed;
extern cvar_t	*cl_forwardspeed;
extern cvar_t	*cl_pitchspeed;
extern cvar_t	*cl_movespeedkey;
cvar_t	*cl_laddermode;


#define F 1U<<0	// Forward
#define B 1U<<1	// Back
#define L 1U<<2	// Left
#define R 1U<<3	// Right
#define T 1U<<4	// Forward stop
#define S 1U<<5	// Side stop

#define BUTTON_DOWN		1
#define IMPULSE_DOWN	2
#define IMPULSE_UP		4

int CL_IsDead( void );
extern Vector dead_viewangles;

void IN_ToggleButtons( float forwardmove, float sidemove )
{
	static unsigned int moveflags = T | S;

	if( forwardmove )
		moveflags &= ~T;
	else
	{
		//if( in_forward.state || in_back.state ) gEngfuncs.Con_Printf("Buttons pressed f%d b%d\n", in_forward.state, in_back.state);
		if( !( moveflags & T ) )
		{
			//IN_ForwardUp();
			//IN_BackUp();
			//gEngfuncs.Con_Printf("Reset forwardmove state f%d b%d\n", in_forward.state, in_back.state);
			in_forward.state &= ~BUTTON_DOWN;
			in_back.state &= ~BUTTON_DOWN;
			moveflags |= T;
		}
	}
	if( sidemove )
		moveflags &= ~S;
	else
	{
		//gEngfuncs.Con_Printf("l%d r%d\n", in_moveleft.state, in_moveright.state);
		//if( in_moveleft.state || in_moveright.state ) gEngfuncs.Con_Printf("Buttons pressed l%d r%d\n", in_moveleft.state, in_moveright.state);
		if( !( moveflags & S ) )
		{
			//IN_MoverightUp();
			//IN_MoveleftUp();
			//gEngfuncs.Con_Printf("Reset sidemove state f%d b%d\n", in_moveleft.state, in_moveright.state);
			in_moveleft.state &= ~BUTTON_DOWN;
			in_moveright.state &= ~BUTTON_DOWN;
			moveflags |= S;
		}
	}

	if( forwardmove > 0.7f && !( moveflags & F ) )
	{
		moveflags |= F;
		in_forward.state |= BUTTON_DOWN;
	}
	if( forwardmove < 0.7f && ( moveflags & F ) )
	{
		moveflags &= ~F;
		in_forward.state &= ~BUTTON_DOWN;
	}
	if( forwardmove < -0.7f && !( moveflags & B ) )
	{
		moveflags |= B;
		in_back.state |= BUTTON_DOWN;
	}
	if( forwardmove > -0.7f && ( moveflags & B ) )
	{
		moveflags &= ~B;
		in_back.state &= ~BUTTON_DOWN;
	}
	if( sidemove > 0.9f && !( moveflags & R ) )
	{
		moveflags |= R;
		in_moveright.state |= BUTTON_DOWN;
	}
	if( sidemove < 0.9f && ( moveflags & R ) )
	{
		moveflags &= ~R;
		in_moveright.state &= ~BUTTON_DOWN;
	}
	if( sidemove < -0.9f && !( moveflags & L ) )
	{
		moveflags |= L;
		in_moveleft.state |= BUTTON_DOWN;
	}
	if( sidemove > -0.9f && ( moveflags & L ) )
	{
		moveflags &= ~L;
		in_moveleft.state &= ~BUTTON_DOWN;
	}
}

void FWGSInput::IN_ClientMoveEvent( float forwardmove, float sidemove )
{
	//gEngfuncs.Con_Printf("IN_MoveEvent\n");

	ac_forwardmove += forwardmove;
	ac_sidemove += sidemove;
	ac_movecount++;
}

void FWGSInput::IN_ClientLookEvent( float relyaw, float relpitch )
{
	rel_yaw += relyaw;
	rel_pitch += relpitch;
}

// Rotate camera and add move values to usercmd
void FWGSInput::IN_Move( float frametime, usercmd_t *cmd )
{
	Vector viewangles;
	bool fLadder = false;

	if( gHUD.m_iIntermission )
	{
		// Keep look input for the orbit camera, but add no movement to usercmd.
		gEngfuncs.GetViewAngles( viewangles );
		const float lookSensitivity = gHUD.GetSensitivity() != 0 ? gHUD.GetSensitivity() : sensitivity->value;
		viewangles[YAW] += rel_yaw * lookSensitivity;
		viewangles[PITCH] += rel_pitch * lookSensitivity;
		viewangles[PITCH] = bound( -cl_pitchup->value, viewangles[PITCH], cl_pitchdown->value );
		gEngfuncs.SetViewAngles( viewangles );
		cmd->forwardmove = cmd->sidemove = cmd->upmove = 0;
		ac_sidemove = ac_forwardmove = rel_pitch = rel_yaw = 0;
		ac_movecount = 0;
		return;
	}

	if( cl_laddermode->value != 2 )
	{
		cl_entity_t *pplayer = gEngfuncs.GetLocalPlayer();
		if( pplayer )
			fLadder = pplayer->curstate.movetype == MOVETYPE_FLY;
	}
	//if(ac_forwardmove || ac_sidemove)
	//gEngfuncs.Con_Printf("Move: %f %f %f %f\n", ac_forwardmove, ac_sidemove, rel_pitch, rel_yaw);
#if 1
//	if( in_mlook.state & 1 )
	{
		V_StopPitchDrift();
	}
#endif
	if( CL_IsDead() )
	{
		viewangles = dead_viewangles; // HACKHACK: see below
	}
	else
	{
		gEngfuncs.GetViewAngles( viewangles );
	}
	if( gHUD.GetSensitivity() != 0 )
	{
		rel_yaw *= gHUD.GetSensitivity();
		rel_pitch *= gHUD.GetSensitivity();
	}
	else
	{
		rel_yaw *= sensitivity->value;
		rel_pitch *= sensitivity->value;
	}
	if( gHUD.m_bGasDelayedEffects )
	{
		const float gasTurnScale = 1.0f - 0.2f * gHUD.m_flGasStrength;
		rel_yaw *= gasTurnScale;
		rel_pitch *= gasTurnScale;
	}
	if( gHUD.m_flFlashbangEffectStart >= 0.0f )
	{
		const float elapsed = gEngfuncs.GetClientTime() - gHUD.m_flFlashbangEffectStart;
		if( elapsed >= 0.0f && elapsed < 6.0f )
		{
			float strength = gHUD.m_iFlashbangStunAlpha / 255.0f;
			// Full intensity means the grenade was within six metres.  Beyond
			// that boundary camera sway is additionally 1.6 times weaker while
			// retaining the existing distance falloff to twenty metres.
			if( strength < 1.0f )
				strength /= 1.6f;
			const float recovery = elapsed < 3.0f ? 0.0f : bound(0.0f, (elapsed - 3.0f) / 3.0f, 1.0f);
			const float mouseScale = 0.12f + 0.88f * (1.0f - strength * (1.0f - recovery));
			rel_yaw *= mouseScale;
			rel_pitch *= mouseScale;
		}
	}
	viewangles[YAW] += rel_yaw;
	if( fLadder )
	{
		if( cl_laddermode->value == 1 )
			viewangles[YAW] -= ac_sidemove * 5;
		ac_sidemove = 0;
	}
	/// TODO: modern MOTD, fix buffer overflows
//	if( gHUD.m_MOTD.m_bShow )
//		gHUD.m_MOTD.scroll += rel_pitch;
//	else
		viewangles[PITCH] += rel_pitch;

	// Apply flashbang sway to the real input view angles, not just to the
	// rendered camera.  These angles are sent in the user command, so server
	// bullet traces now follow the same moving crosshair the player sees.
	static Vector s_appliedFlashbangSway;
	static float s_trackedFlashbangStart = -1.0f;
	static unsigned int s_flashbangEffectGeneration;
	const float flashbangStart = gHUD.m_flFlashbangEffectStart;
	if( s_flashbangEffectGeneration != gHUD.m_iFlashbangEffectGeneration )
	{
		// A round reset also restores the server view angles. Do not subtract
		// an offset that belonged to the previous life from those fresh angles.
		s_appliedFlashbangSway = g_vecZero;
		s_trackedFlashbangStart = flashbangStart;
		s_flashbangEffectGeneration = gHUD.m_iFlashbangEffectGeneration;
	}
	else if( flashbangStart != s_trackedFlashbangStart )
	{
		viewangles -= s_appliedFlashbangSway;
		s_appliedFlashbangSway = g_vecZero;
		s_trackedFlashbangStart = flashbangStart;
	}

	Vector flashbangSway = g_vecZero;
	if( flashbangStart >= 0.0f )
	{
		const float elapsed = gEngfuncs.GetClientTime() - flashbangStart;
		if( elapsed >= 0.0f && elapsed < 7.5f )
		{
			const float strength = gHUD.m_iFlashbangStunAlpha / 255.0f;
			const float recovery = elapsed < 3.0f ? 0.0f :
				bound(0.0f, (elapsed - 3.0f) / 4.5f, 1.0f);
			const float amount = strength * (1.0f - recovery);
			flashbangSway = Vector(
				sinf(elapsed * 3.8f) * 4.5f * amount,
				sinf(elapsed * 3.1f + 1.2f) * 6.0f * amount,
				sinf(elapsed * 3.5f + 2.4f) * 6.75f * amount);
		}
	}
	viewangles += flashbangSway - s_appliedFlashbangSway;
	s_appliedFlashbangSway = flashbangSway;

	// Slow, large-amplitude toxic-gas sway. Applying the delta keeps the
	// server-side aim and the rendered crosshair in exact agreement.
	static Vector s_appliedGasSway;
	static unsigned int s_gasEffectGeneration;
	if( s_gasEffectGeneration != gHUD.m_iGasEffectGeneration )
	{
		s_appliedGasSway = g_vecZero;
		s_gasEffectGeneration = gHUD.m_iGasEffectGeneration;
	}
	Vector gasSway = g_vecZero;
	if( gHUD.m_flGasStrength > 0.0f )
	{
		const float t = gEngfuncs.GetClientTime();
		const float a = gHUD.m_flGasStrength;
		gasSway = Vector(sinf(t * 1.05f) * 5.5f * a,
			sinf(t * 0.82f + 1.1f) * 8.0f * a,
			sinf(t * 0.71f + 2.3f) * 7.0f * a);
	}
	viewangles += gasSway - s_appliedGasSway;
	s_appliedGasSway = gasSway;

	if( viewangles[PITCH] > cl_pitchdown->value )
		viewangles[PITCH] = cl_pitchdown->value;
	if( viewangles[PITCH] < -cl_pitchup->value )
		viewangles[PITCH] = -cl_pitchup->value;
	
	// HACKHACK: change viewangles directly in viewcode, 
	// so viewangles when player is dead will not be changed on server
	if( !CL_IsDead() )
	{
		gEngfuncs.SetViewAngles( viewangles );
	}

	dead_viewangles = viewangles; // keep them actual
	if( ac_movecount )
	{
		IN_ToggleButtons( ac_forwardmove / ac_movecount, ac_sidemove / ac_movecount );

		if( ac_forwardmove )
			cmd->forwardmove = ac_forwardmove * cl_forwardspeed->value / ac_movecount;
		if( ac_sidemove )
			cmd->sidemove  = ac_sidemove * cl_sidespeed->value / ac_movecount;
		if( ( in_speed.state & 1 ) && ( ac_sidemove || ac_forwardmove ) )
		{
			cmd->forwardmove *= cl_movespeedkey->value;
			cmd->sidemove *= cl_movespeedkey->value;
		}
	}

	ac_sidemove = ac_forwardmove = rel_pitch = rel_yaw = 0;
	ac_movecount = 0;
}

void FWGSInput::IN_MouseEvent( int mstate )
{
	static int mouse_oldbuttonstate;
	// perform button actions
	for( int i = 0; i < 5; i++ )
	{
		if( ( mstate & ( 1 << i ) ) && !( mouse_oldbuttonstate & ( 1 << i ) ) )
		{
			gEngfuncs.Key_Event( K_MOUSE1 + i, 1 );
		}

		if( !( mstate & ( 1 << i ) ) && ( mouse_oldbuttonstate & ( 1 << i ) ) )
		{
			gEngfuncs.Key_Event( K_MOUSE1 + i, 0 );
		}
	}	

	mouse_oldbuttonstate = mstate;
}

// Stubs

void FWGSInput::IN_ClearStates( void )
{
	//gEngfuncs.Con_Printf( "IN_ClearStates\n" );
}

void FWGSInput::IN_ActivateMouse( void )
{
	//gEngfuncs.Con_Printf( "IN_ActivateMouse\n" );
}

void FWGSInput::IN_DeactivateMouse( void )
{
	//gEngfuncs.Con_Printf( "IN_DeactivateMouse\n" );
}

void FWGSInput::IN_Accumulate( void )
{
	//gEngfuncs.Con_Printf( "IN_Accumulate\n" );
}

void FWGSInput::IN_Commands( void )
{
	//gEngfuncs.Con_Printf( "IN_Commands\n" );
}

void FWGSInput::IN_Shutdown( void )
{
}

// Register cvars and reset data
void FWGSInput::IN_Init( void )
{
	sensitivity = gEngfuncs.pfnRegisterVariable( "sensitivity", "3", FCVAR_ARCHIVE );
	in_joystick = gEngfuncs.pfnRegisterVariable( "joystick", "0", FCVAR_ARCHIVE );
	cl_laddermode = gEngfuncs.pfnRegisterVariable( "cl_laddermode", "2", FCVAR_ARCHIVE );
	ac_forwardmove = ac_sidemove = rel_yaw = rel_pitch = 0;
}
