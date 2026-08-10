/*
gl_mirror.cpp - draw reflected surfaces
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "hud.h"
#include "utils.h"
#include "gl_local.h"
#include "gl_world.h"
#include "mathlib.h"
#include <stringlib.h>
#include "gl_occlusion.h"
#include "gl_cvars.h"
#include "gl_studio.h"
#include "r_view.h"

#define MIRROR_PLANE_EPSILON		0.1f

/*
=============================================================

	MIRROR RENDERING

=============================================================
*/
/*
================
R_SetupMirrorView

Prepare view for mirroring
================
*/
texFlags_t R_SetupMirrorView( msurface_t *surf, ref_viewpass_t *rvp, matrix4x4 &viewmat)
{
	cl_entity_t	*ent = surf->info->parent;
	matrix3x3	matAngles;
	mplane_t	plane;
	gl_state_t	*glm;
	CViewport	viewport;
	float		d;

	// setup mirror plane
	if( FBitSet( surf->flags, SURF_PLANEBACK ))
		SetPlane( &plane, -surf->plane->normal, -surf->plane->dist );
	else SetPlane( &plane, surf->plane->normal, surf->plane->dist );

	glm = GL_GetCache( ent->hCachedMatrix );

	if (ent->hCachedMatrix != WORLD_MATRIX) {
		viewmat = glm->transform;
		viewmat.TransformPositivePlane(plane, plane);
	}
	else {
		viewmat.Identity();
	}

	// reflect view by mirror plane
	d = -2.0f * ( DotProduct( GetVieworg(), plane.normal ) - plane.dist );
	Vector origin = GetVieworg() + d * plane.normal;

	d = -2.0f * DotProduct( GetVForward(), plane.normal );
	Vector forward = ( GetVForward() + d * plane.normal ).Normalize();

	d = -2.0f * DotProduct( GetVRight(), plane.normal );
	Vector right = ( GetVRight() + d * plane.normal ).Normalize();

	d = -2.0f * DotProduct( GetVUp(), plane.normal );
	Vector up = ( GetVUp() + d * plane.normal ).Normalize();

	// compute mirror angles
	matAngles.SetForward( forward );
	matAngles.SetRight( right );
	matAngles.SetUp( up );
	Vector angles = matAngles.GetAngles();

	plane.dist += ON_EPSILON; // to prevent z-fighting with reflective water
	RI->view.frustum.SetPlane( FRUSTUM_NEAR, plane.normal, plane.dist );
	RI->clipPlane = plane;

	rvp->viewangles[0] = anglemod( angles[0] );
	rvp->viewangles[1] = anglemod( angles[1] );
	rvp->viewangles[2] = anglemod( angles[2] );
	rvp->viewangles[2] = -rvp->viewangles[2];
	rvp->vieworigin = origin;

	rvp->fov_x = RI->view.fov_x;
	rvp->fov_y = RI->view.fov_y;
	rvp->flags = RP_MIRRORVIEW | RP_CLIPPLANE | RP_MERGE_PVS;

	// put pvsorigin before the mirror plane to avoid get recursion with himself
	if( ent == GET_ENTITY( 0 )) 
		origin = surf->info->origin + (1.0f * plane.normal);
	else 
		origin = glm->transform.VectorTransform( surf->info->origin ) + (1.0f * plane.normal);
	material_t *mat = R_TextureAnimation( surf )->material;

	RI->view.pvspoint = origin;
	viewport.SetX(0);
	viewport.SetY(0);

	if( FBitSet( surf->flags, SURF_REFLECT_PUDDLE ))
	{
		viewport.SetWidth(192);
		viewport.SetHeight(192);
	}
	else
	{
		viewport.SetWidth(RI->view.port.GetWidth());
		viewport.SetHeight(RI->view.port.GetHeight());
	}

	if( GL_Support( R_ARB_TEXTURE_NPOT_EXT ))
	{
		// allow screen size
		viewport.SetWidth(Q_max(96, viewport.GetWidth()));
		viewport.SetHeight(Q_max(72, viewport.GetHeight()));
	}
	else
	{
		int w = NearestPOW(viewport.GetWidth(), true);
		int h = NearestPOW(viewport.GetHeight(), true);
		viewport.SetWidth(Q_max(128, w));
		viewport.SetHeight(Q_max(64, h));
	}

	viewport.WriteToArray(rvp->viewport);
	if( FBitSet( mat->flags, BRUSH_LIQUID ))
		SetBits( rvp->flags, RP_WATERPASS );
	else if( FBitSet( surf->flags, SURF_REFLECT_PUDDLE ))
		SetBits( rvp->flags, RP_NOSHADOWS|RP_WATERPASS|RP_NOGRASS ); // don't draw grass from puddles

	return static_cast<texFlags_t>(TF_SCREEN);
}

/*
=============================================================

	PORTAL RENDERING

=============================================================
*/
/*
================
R_SetupPortalView

Setup portal view and near plane
================
*/
texFlags_t R_SetupPortalView(msurface_t *surf, ref_viewpass_t *rvp, cl_entity_t *camera)
{
	cl_entity_t *ent = surf->info->parent;
	matrix4x4	surfaceMat, cameraMat, viewMat;
	Vector		origin, angles;
	mplane_t	plane;
	CViewport	viewport;

	if (!R_StaticEntity(ent))
	{
		if (ent->angles != g_vecZero)
			surfaceMat = matrix4x4(ent->origin, ent->angles);
		else surfaceMat = matrix4x4(ent->origin, g_vecZero);
	}
	else surfaceMat.Identity();

	if (!R_StaticEntity(camera))
	{
		if (camera->angles != g_vecZero)
			cameraMat = matrix4x4(camera->origin, camera->angles);
		else cameraMat = matrix4x4(camera->origin, g_vecZero);
	}
	else cameraMat.Identity();

	// now get the camera origin and orientation
	viewMat.SetForward(GetVForward());
	viewMat.SetRight(GetVRight());
	viewMat.SetUp(GetVUp());
	viewMat.SetOrigin(GetVieworg());

	surfaceMat = surfaceMat.Invert();
	viewMat = surfaceMat.ConcatTransforms(viewMat);
	viewMat = cameraMat.ConcatTransforms(viewMat);

	angles = viewMat.GetAngles();
	origin = viewMat.GetOrigin();

	rvp->viewangles[0] = anglemod(angles[0]);
	rvp->viewangles[1] = anglemod(angles[1]);
	rvp->viewangles[2] = anglemod(angles[2]); 
	
	rvp->vieworigin = origin;
	rvp->fov_x = RI->view.fov_x;
	rvp->fov_y = RI->view.fov_y;
	rvp->flags = RP_PORTALVIEW | RP_CLIPPLANE | RP_MERGE_PVS;

	// set clipping plane
	SetPlane(&plane, cameraMat.GetForward(), DotProduct(cameraMat.GetForward(), camera->origin));
	plane.dist += ON_EPSILON; // to prevent z-fighting with reflective water
	RI->view.frustum.SetPlane(FRUSTUM_NEAR, plane.normal, plane.dist);
	RI->clipPlane = plane;
	RI->view.pvspoint = camera->origin;

	viewport.SetX(0);
	viewport.SetY(0);
	if (GL_Support(R_ARB_TEXTURE_NPOT_EXT))
	{
		// allow screen size
		viewport.SetWidth(bound(96, RI->view.port.GetWidth(), 1024));
		viewport.SetHeight(bound(72, RI->view.port.GetHeight(), 768));
	}
	else
	{
		int w = NearestPOW(RI->view.port.GetWidth(), true);
		int h = NearestPOW(RI->view.port.GetHeight(), true);
		viewport.SetWidth(bound(128, w, 1024));
		viewport.SetHeight(bound(64, h, 512));
	}

	viewport.WriteToArray(rvp->viewport);
	return static_cast<texFlags_t>(TF_SCREEN);
}

/*
=============================================================

	SCREEN RENDERING

=============================================================
*/
/*
================
R_SetupScreenView

Setup screen view
================
*/
texFlags_t R_SetupScreenView(msurface_t *surf, ref_viewpass_t *rvp, matrix4x4 &viewmat, cl_entity_t *camera)
{
	cl_entity_t *ent = surf->info->parent;
	Vector		origin, angles;
	float		fov = 90.f;
	CViewport	viewport;

	viewmat.Identity();
	if (camera->player && UTIL_IsLocal(camera->curstate.number))
	{
		ref_params_t viewparams;
		memcpy(&viewparams, &tr.viewparams, sizeof(viewparams));

		// player seen through camera. Restore firstperson view here
		if (tr.viewparams.viewentity > tr.viewparams.maxclients)
			V_CalcFirstPersonRefdef(&viewparams);

		rvp->flags = RP_SCREENVIEW | RP_FORCE_NOPLAYER;
		RI->view.pvspoint = viewparams.vieworg;
		return TF_NOMIPMAP;
	}

	origin = camera->origin;
	angles = camera->angles;

	studiohdr_t *viewmonster = (studiohdr_t *)IEngineStudio.Mod_Extradata(camera->model);

	// it's monster
	if (viewmonster && FBitSet(camera->curstate.eflags, EFLAG_SLERP))
	{
		Vector forward;
		AngleVectors(angles, forward, NULL, NULL);

		Vector viewpos = viewmonster->eyeposition;

		if (viewpos == g_vecZero)
			viewpos = Vector(0, 0, 8.0f);// monster_cockroach
		origin += viewpos + forward * 8.0f;	// best value for humans
	}

	// smooth step stair climbing
	// lasttime goes into ent->latched.sequencetime
	// oldz goes into ent->latched.prevanimtime
	// HACKHACK we store interpolate values into func_monitor entity to avoid broke lerp
	if (origin[2] - ent->latched.prevanimtime > 0.0f)
	{
		float steptime;

		steptime = tr.time - ent->latched.sequencetime;
		if (steptime < 0) steptime = 0;

		ent->latched.prevanimtime += steptime * 150.0f;

		if (ent->latched.prevanimtime > origin[2])
			ent->latched.prevanimtime = origin[2];
		if (origin[2] - ent->latched.prevanimtime > tr.movevars->stepsize)
			ent->latched.prevanimtime = origin[2] - tr.movevars->stepsize;

		origin[2] += ent->latched.prevanimtime - origin[2];
	}
	else
	{
		ent->latched.prevanimtime = origin[2];
	}

	ent->latched.sequencetime = tr.time;

	// setup the screen fov
	if (ent->curstate.fuser2 != 0.0f)
		fov = bound(10, ent->curstate.fuser2, 120);

	if (GL_Support(R_ARB_TEXTURE_NPOT_EXT))
	{
		texture_t *t = surf->texinfo->texture;

		if ((t->width == 64) && (t->height == 64))
		{
			// HACKHACK: for default texture
			viewport.SetWidth(512);
			viewport.SetHeight(512);
		}
		else
		{
			viewport.SetWidth(bound(64, t->width * 2, 1024));
			viewport.SetHeight(bound(64, t->height * 2, 1024));
		}
	}
	else 
	{
		viewport.SetWidth(512);
		viewport.SetHeight(512);
	}

	// setup the screen FOV
	if (viewport.GetWidth() == viewport.GetHeight())
	{
		rvp->fov_x = fov;
		rvp->fov_y = fov;
	}
	else
	{
		rvp->fov_x = fov;
		rvp->fov_y = V_CalcFov(rvp->fov_x, viewport.GetWidth(), viewport.GetHeight());
	}

	viewport.SetX(0);
	viewport.SetY(0);
	viewport.WriteToArray(rvp->viewport);
	rvp->viewangles[0] = anglemod(angles[0]);
	rvp->viewangles[1] = anglemod(angles[1]);
	rvp->viewangles[2] = anglemod(angles[2]);
	rvp->vieworigin = origin;
	RI->view.pvspoint = camera->origin;

	rvp->flags = RP_SCREENVIEW;
	if (camera->player) {
		SetBits(rvp->flags, RP_FORCE_NOPLAYER); // camera set to player don't draw him
	}

	return TF_NOMIPMAP;
}

/*
================
R_AllocateSubviewTexture

Allocate the screen texture and make copy
================
*/
int R_AllocateSubviewTexture(const CViewport &viewport, texFlags_t texFlags)
{
	int	i;

	// first, search for available mirror texture
	for ( i = 0; i < tr.num_subview_used; i++ )
	{
		if( tr.subviewTextures[i].texframe == tr.realframecount )
			continue;	// already used for this frame

		if( viewport.GetWidth() != tr.subviewTextures[i].texturenum.GetWidth())
			continue;	// width mismatched

		if( viewport.GetHeight() != tr.subviewTextures[i].texturenum.GetHeight())
			continue;	// height mismatched

		// screens don't want textures with 'clamp' modifier
		if( FBitSet( texFlags, TF_CLAMP ) != FBitSet(tr.subviewTextures[i].texturenum.GetFlags(), TF_CLAMP))
			continue;	// mismatch texture flags

		// found a valid spot
		tr.subviewTextures[i].texframe = tr.realframecount; // now used
		break;
	}

	if( i == tr.num_subview_used )
	{
		if( i == MAX_SUBVIEW_TEXTURES )
		{
			ALERT( at_error, "R_AllocSubviewTexture: texture limit exceeded (per frame)!\n" );
			return 0;	// disable
		}

		// use HDR textures for subviews too
		bool hdr_rendering = CVAR_TO_BOOL(gl_hdr);
		if (hdr_rendering) {
			texFlags = static_cast<texFlags_t>(texFlags | TF_ARB_FLOAT | TF_ARB_16BIT);
		}

		// create new mirror texture
		tr.subviewTextures[i].texturenum = CREATE_TEXTURE( va( "*subview%i", i ), viewport.GetWidth(), viewport.GetHeight(), NULL, texFlags ); 
		tr.subviewTextures[i].texframe = tr.realframecount; // now used
		if( GL_Support( R_FRAMEBUFFER_OBJECT ))
			tr.subviewTextures[i].framebuffer = R_AllocFrameBuffer( viewport );
		tr.num_subview_used++; // allocate new one
	}

	if( GL_Support( R_FRAMEBUFFER_OBJECT ))
 	{
		GL_BindFrameBuffer( tr.subviewTextures[i].framebuffer, tr.subviewTextures[i].texturenum );
	}
	else 
	{
		GL_Bind( GL_TEXTURE0, tr.subviewTextures[i].texturenum );
		pglCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, viewport.GetX(), viewport.GetY(), viewport.GetWidth(), viewport.GetHeight(), 0 );
	}

	return (i+1);
}

static TextureHandle g_cameraFeedTexture;
static int g_cameraFeedFrame = -1;

TextureHandle R_GetCameraFeedTexture(void)
{
	// Keep the last completed feed available.  The renderer advances its real
	// frame counter at the end of GL_BackendEndFrame, while studio material
	// batches may be consumed on either side of that boundary.
	return g_cameraFeedTexture;
}

void R_RenderCameraFeed(void)
{
	if (!RP_NORMALPASS() || !gHUD.m_Ammo.IsCameraFeedWeaponActive())
		return;
	const bool stickCameraActive = gHUD.m_Ammo.IsStickCameraWeaponActive();
	// Predict the Stick Camera side directly from the local RMB edge. Waiting
	// for the hidden view entity's 180-degree network update made the embedded
	// screen appear permanently left-facing on interpolated client frames.
	static bool stickLookRight = false;
	static bool stickRmbWasDown = false;
	if (stickCameraActive)
	{
		const bool rmbDown = (gHUD.m_iKeyBits & IN_ATTACK2) != 0;
		if (rmbDown && !stickRmbWasDown)
			stickLookRight = !stickLookRight;
		stickRmbWasDown = rmbDown;
	}
	else
	{
		stickLookRight = false;
		stickRmbWasDown = false;
	}
	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	if (!local)
		return;
	cl_entity_t *activeView = GET_ENTITY(tr.viewparams.viewentity);
	if (activeView && activeView != local &&
		(activeView->curstate.iuser4 == 0x5346434D || activeView->curstate.iuser4 == 0x5354434D ||
		(activeView->model && activeView->model->name &&
		 Q_stristr(activeView->model->name, "weapon/Camera/w_camera.mdl"))))
		return; // the tablet is hidden during full-screen camera viewing
	cl_entity_t *camera = NULL;
	cl_entity_t *fallback = NULL;
	for (int index = GET_MAX_CLIENTS() + 1; index < 4096; ++index)
	{
		cl_entity_t *candidate = GET_ENTITY(index);
		// GET_ENTITY keeps the last state in its slot after the server removes an
		// entity. Only entities present in the current packet are still cameras.
		if (!candidate || candidate->curstate.messagenum != r_currentMessageNum)
			continue;
		const bool visibleCameraModel = stickCameraActive
			? candidate->curstate.iuser4 == 0x5354434D && candidate->curstate.colormap == local->index
			: candidate->model && candidate->model->name &&
				Q_stristr(candidate->model->name, "weapon/Camera/w_camera.mdl") &&
				candidate->curstate.colormap == local->index;
		if (!visibleCameraModel)
			continue;
		if (stickCameraActive)
		{
			camera = candidate;
			break;
		}
		const int cameraIndex = candidate->curstate.skin + 1;
		const int fallbackIndex = fallback ? fallback->curstate.skin + 1 : 999;
		if (!fallback || cameraIndex < fallbackIndex)
			fallback = candidate;
		if (candidate->curstate.fuser2 > 0.5f)
		{
			camera = candidate;
			break;
		}
	}
	// During the first packet after drawing the weapon the selected marker may
	// not have arrived yet.  Showing the first owned camera is preferable to a
	// permanently blank tablet, and the marked camera takes over next frame.
	if (!camera)
		camera = fallback;
	const bool localStickFallback = stickCameraActive && !camera;
	if (!camera)
	{
		if (localStickFallback)
		{
			// The handheld feed must not depend on the first server snapshot.
			// Its networked view entity replaces these values as soon as it arrives.
		}
		else
		{
		// Do not leave the last live picture bound to the tablet after the camera
		// has been picked up, destroyed, or otherwise left the current snapshot.
		g_cameraFeedTexture = TextureHandle();
		g_cameraFeedFrame = -1;
		static bool reportedMissing = false;
		if (!reportedMissing)
		{
			gEngfuncs.Con_Printf("[camera-feed] no networked camera entity found\n");
			reportedMissing = true;
		}
		return;
		}
	}

	ref_viewpass_t rvp = {};
	Vector forward;
	Vector cameraAngles = localStickFallback ? tr.viewparams.cl_viewangles : camera->angles;
	if (localStickFallback)
		cameraAngles.y = AngleNormalize(cameraAngles.y + (stickLookRight ? -90.0f : 90.0f));
	else if (stickCameraActive)
	{
		// Build the idle screen direction from the current player view and the
		// locally predicted side so RMB changes the feed in the same frame.
		cameraAngles = tr.viewparams.cl_viewangles;
		cameraAngles.y = AngleNormalize(cameraAngles.y +
			(stickLookRight ? -90.0f : 90.0f));
	}
	// Prefer the exact angles that this client actually used in the full-screen
	// camera view. This avoids every server delta/interpolation path and makes
	// the tablet continue from precisely the last LMB view.
	if (!stickCameraActive)
		V_GetSurveillanceCameraAngles(camera->index, camera->curstate.fuser1, cameraAngles);
	Vector mountAngles = localStickFallback ? tr.viewparams.cl_viewangles : camera->angles;
	AngleVectors(mountAngles, forward, NULL, NULL);
	cl_entity_t *cameraView = !localStickFallback && camera->curstate.iuser2 > 0
		? GET_ENTITY(camera->curstate.iuser2) : NULL;
	// Use network state coordinates, not cl_entity_t::origin. SET_VIEW transitions
	// can temporarily clear/interpolate the latter to the map origin even though
	// curstate still contains the correct installed position.
	Vector feedOrigin;
	if (localStickFallback)
	{
		Vector stickForward;
		AngleVectors(tr.viewparams.cl_viewangles, stickForward, NULL, NULL);
		feedOrigin = tr.viewparams.simorg + tr.viewparams.viewheight + stickForward * 100.0f;
	}
	else
		feedOrigin = camera->curstate.origin;
	if (cameraView && cameraView->curstate.iuser4 == 0x5346434D &&
		cameraView->curstate.origin != g_vecZero)
		feedOrigin = cameraView->curstate.origin;
	rvp.vieworigin = stickCameraActive ? feedOrigin : feedOrigin + forward * 14.0f;
	rvp.viewangles = cameraAngles;
	// A secondary scene only needs the explicit origin/angles above. Keeping a
	// camera entity as viewentity makes the engine suppress or rewrite it across
	// SET_VIEW transitions. Use the local player as the harmless pass owner.
	rvp.viewentity = local->index;
	const float sourceFovX = stickCameraActive
		? (!localStickFallback && camera->curstate.playerclass != 0 ? 30.0f : 90.0f)
		: (!localStickFallback && camera->curstate.playerclass != 0
			? 22.0f : bound(10.0f, RI->view.fov_x, 120.0f));
	rvp.fov_x = sourceFovX;
	// Preserve the full-screen horizontal field of view without stretching the
	// image into the tablet's 3:2 screen.
	rvp.fov_y = RAD2DEG(2.0f * atan(tan(DEG2RAD(rvp.fov_x) * 0.5f) / (480.0f / 320.0f)));
	rvp.flags = RP_SCREENVIEW | RP_CAMERA_FEED;
	CViewport viewport;
	viewport.SetX(0); viewport.SetY(0);
	viewport.SetWidth(480); viewport.SetHeight(320);
	viewport.WriteToArray(rvp.viewport);

	const unsigned int oldFBO = glState.frameBuffer;
	R_PushRefState();
	RI->view.pvspoint = rvp.vieworigin;
	const int subview = R_AllocateSubviewTexture(viewport, TF_NOMIPMAP);
	if (subview > 0)
	{
		// R_Clear normally clears only depth unless the global r_clear path is
		// active. A persistent camera target must always have every pixel replaced;
		// otherwise geometry that leaves the frame exposes pieces of an older view.
		// Also force the complete target viewport because this pass is launched
		// from inside the main backend and inherits its current GL rectangle.
		pglViewport(0, 0, viewport.GetWidth(), viewport.GetHeight());
		pglDisable(GL_SCISSOR_TEST);
		pglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		pglDepthMask(GL_TRUE);
		pglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		R_RenderScene(&rvp, (RefParams)rvp.flags);
		g_cameraFeedTexture = tr.subviewTextures[subview - 1].texturenum;
		g_cameraFeedFrame = tr.realframecount;
		static bool reportedRender = false;
		if (!reportedRender)
		{
			gEngfuncs.Con_Printf("[camera-feed] render target ready: %d\n", g_cameraFeedTexture.ToInt());
			reportedRender = true;
		}
	}
	GL_BindFBO(oldFBO);
	R_ResetRefState();
	R_PopRefState();
	// R_RenderScene leaves the 256x256 camera viewport and its matrices active.
	// Restore the caller's full-screen 3D state before the tablet viewmodel and
	// post-processing are drawn.
	GL_Setup3D();
}

bool R_CheckMirrorClone( msurface_t *surf, msurface_t *check )
{
	if( !FBitSet( surf->flags, SURF_REFLECT|SURF_REFLECT_PUDDLE ))
		return false;

	if( !FBitSet( check->flags, SURF_REFLECT|SURF_REFLECT_PUDDLE ))
		return false;

	if( !check->info->subtexture[glState.stack_position-1] )
		return false;

	if( surf->info->parent != check->info->parent )
		return false;

	if( FBitSet( surf->flags, SURF_PLANEBACK ) != FBitSet( check->flags, SURF_PLANEBACK ))
		return false;

	if( surf->plane->normal != check->plane->normal )
		return false;

	if( Q_sign( surf->plane->dist ) != Q_sign( check->plane->dist ))
		return false;

	if( fabs( surf->plane->dist - check->plane->dist ) > MIRROR_PLANE_EPSILON )
		return false;

	// just reuse the handle
	surf->info->subtexture[glState.stack_position-1] = check->info->subtexture[glState.stack_position-1];

	return true;
}

bool R_CheckScreenSource(msurface_t *surf, msurface_t *check)
{
	if (!FBitSet(surf->flags, SURF_SCREEN))
		return false;

	if (!FBitSet(check->flags, SURF_SCREEN))
		return false;

	if (!check->info->subtexture[glState.stack_position - 1])
		return false;

	// different fov
	if (surf->info->parent->curstate.fuser2 != check->info->parent->curstate.fuser2)
		return false;

	// different cameras
	if (surf->info->parent->curstate.sequence != check->info->parent->curstate.sequence)
		return false;

	// just reuse the handle
	surf->info->subtexture[glState.stack_position - 1] = check->info->subtexture[glState.stack_position - 1];

	return true;
}

bool R_CheckPortalSource(msurface_t *surf, msurface_t *check)
{
	if (!FBitSet(surf->flags, SURF_PORTAL))
		return false;

	if (!FBitSet(check->flags, SURF_PORTAL))
		return false;

	if (!check->info->subtexture[glState.stack_position - 1])
		return false;

	// different cameras
	if (surf->info->parent->curstate.sequence != check->info->parent->curstate.sequence)
		return false;

	// just reuse the handle
	surf->info->subtexture[glState.stack_position - 1] = check->info->subtexture[glState.stack_position - 1];

	return true;
}

bool R_CanSkipPass( int end, msurface_t *surf, ref_instance_t *prevRI )
{
	// IMPORTANT: limit the recursion depth
	if( surf == prevRI->reject_face )
		return true;

	for (int i = 0; i < end; i++)
	{
		if (R_CheckMirrorClone(surf, prevRI->frame.subview_faces[i]))
			return true;

		if (R_CheckPortalSource(surf, prevRI->frame.subview_faces[i]))
			return true;

		if (R_CheckScreenSource(surf, prevRI->frame.subview_faces[i]))
			return true;
	}

	// occluded by query
	if( FBitSet( surf->flags, SURF_OCCLUDED ))
	{
		// don't use me
		surf->info->subtexture[glState.stack_position-1] = 0;
		return true;
    }

	return false;
}

static void R_CalcMirrorSubviewMatrix(const ref_viewpass_t &rvp, int subview, const matrix4x4 &viewmatrix, cl_entity_t *camera)
{
	matrix4x4	worldView, modelView, projection;

	worldView.CreateModelview();
	worldView.ConcatRotate(-rvp.viewangles[2], 1, 0, 0);
	worldView.ConcatRotate(-rvp.viewangles[0], 0, 1, 0);
	worldView.ConcatRotate(-rvp.viewangles[1], 0, 0, 1);
	worldView.ConcatTranslate(-rvp.vieworigin[0], -rvp.vieworigin[1], -rvp.vieworigin[2]);
	projection.CreateProjection(rvp.fov_x, rvp.fov_y, Z_NEAR, RI->view.farClip);

	// create personal projection matrix for mirror
	if (R_StaticEntity(camera))
	{
		tr.subviewTextures[subview - 1].matrix = projection.Concat(worldView);
	}
	else
	{
		modelView = worldView.ConcatTransforms(viewmatrix);
		tr.subviewTextures[subview - 1].matrix = projection.Concat(modelView); // RI->view.worldProjectionMatrix;
	}
}

static void R_CalcPortalSubviewMatrix(const ref_viewpass_t &rvp, int subview)
{
	tr.subviewTextures[subview - 1].matrix = RI->view.projectionMatrix.Concat(RI->view.worldMatrix);
}

void R_CalcSubviewMatrix(const ref_viewpass_t &rvp, msurface_t *surf, int subview, const matrix4x4 &viewmatrix, cl_entity_t *camera)
{
	// we don't need to calc subview matrix for screen pass because it's not used
	if (FBitSet(surf->flags, SURF_REFLECT | SURF_REFLECT_PUDDLE)) {
		R_CalcMirrorSubviewMatrix(rvp, subview, viewmatrix, camera);
	}
	else if (FBitSet(surf->flags, SURF_PORTAL)) {
		R_CalcPortalSubviewMatrix(rvp, subview);
	}	
}

/*
================
R_CheckOutside

check if main view is outside level
================
*/
bool R_CheckOutside( void )
{
	// only main view should be tested
	if( !RP_NORMALPASS( ))
		return false;

	mleaf_t	*leaf = Mod_PointInLeaf( RI->view.origin, worldmodel->nodes );

	if( RP_OUTSIDE( leaf ))
		return true;
	return false;
}

/*
================
R_RenderSubview

Draw scene from another points
e.g. for planar reflection,
remote cameras etc
================
*/
void R_RenderSubview()
{
	ZoneScoped;

	ref_instance_t *prevRI;
	unsigned int oldFBO;
	ref_viewpass_t rvp;
	matrix4x4 viewmatrix;
	bool has_mirrors = FBitSet(world->features, WORLD_HAS_MIRRORS);
	bool has_screens = FBitSet(world->features, WORLD_HAS_SCREENS);
	bool has_portals = FBitSet(world->features, WORLD_HAS_PORTALS);

	// don't render subviews in overview mode
	if (FBitSet(RI->params, RP_DRAW_OVERVIEW))
		return;

	// no mirrors, portals or screens anyway
	if (!has_mirrors && !has_screens && !has_portals)
		return;

	// too deep...
	if (glState.stack_position > (unsigned int)r_recursion_depth->value)
		return; 

	// nothing to render
	if (!RI->frame.num_subview_faces)
		return; 

	// player is outside world. Don't draw subview for speedup reasons
	if (R_CheckOutside())
		return;

	GL_DEBUG_SCOPE();
	R_PushRefState(); // make refinst backup
	prevRI = R_GetPrevInstance();

	// draw the subviews through the list
	for( int i = 0; i < prevRI->frame.num_subview_faces; i++ )
	{
		msurface_t *surf = prevRI->frame.subview_faces[i];
		cl_entity_t *e = RI->currententity = surf->info->parent;
		mextrasurf_t *es = surf->info;
		texFlags_t texFlags = TF_COLORMAP;
		cl_entity_t *camera;
		int subview = 0;
		RI->currentmodel = e->model;

		if( R_CanSkipPass( i, surf, prevRI ))
			continue;

		ASSERT( RI->currententity != NULL );
		ASSERT( RI->currentmodel != NULL );

		if (!FBitSet(surf->flags, SURF_REFLECT | SURF_REFLECT_PUDDLE))
		{
			if (e->curstate.sequence <= 0)
				continue;	// target is missed
			camera = GET_ENTITY(e->curstate.sequence);
		}
		else camera = e; // mirror camera is himself

		// NOTE: we can do additionaly culling here by PVS
		if( !e || e->curstate.messagenum != r_currentMessageNum )
			continue; // bad camera, ignore

		// setup view apropriate by type
		if (FBitSet(surf->flags, SURF_REFLECT | SURF_REFLECT_PUDDLE))
		{
			texFlags = R_SetupMirrorView(surf, &rvp, viewmatrix);
			r_stats.c_mirror_passes++;
		}
		else if (FBitSet(surf->flags, SURF_PORTAL))
		{
			texFlags = R_SetupPortalView(surf, &rvp, camera);
			r_stats.c_portal_passes++;
		}
		else if (FBitSet(surf->flags, SURF_SCREEN))
		{
			texFlags = R_SetupScreenView(surf, &rvp, viewmatrix, camera);
			r_stats.c_screen_passes++;
		}
		else continue; // ???

		oldFBO = glState.frameBuffer;
		CViewport viewport = CViewport(rvp.viewport);
		if( GL_Support( R_FRAMEBUFFER_OBJECT ))
		{
			if(( subview = R_AllocateSubviewTexture(viewport, texFlags )) == 0 )
				continue;
			R_CalcSubviewMatrix(rvp, surf, subview, viewmatrix, camera);
		}

		// reset the subinfo
		surf->info->subtexture[glState.stack_position-1] = 0;
		RI->reject_face = surf;
		R_RenderScene( &rvp, (RefParams)rvp.flags );
		RI->reject_face = NULL;

		if (!GL_Support(R_FRAMEBUFFER_OBJECT)) 
		{
			subview = R_AllocateSubviewTexture(viewport, texFlags);
			R_CalcSubviewMatrix(rvp, surf, subview, viewmatrix, camera);
		}

		surf->info->subtexture[glState.stack_position-1] = subview; // now it's valid
		GL_BindFBO( oldFBO );
		R_ResetRefState();
	}
	R_PopRefState(); // restore ref instance
}
