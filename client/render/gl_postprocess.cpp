//
// written by BUzer for HL: Paranoia modification
//
//		2006

#include "gl_postprocess.h"
#include "hud.h"
#include "utils.h"
#include "const.h"
#include "gl_local.h"
#include "stringlib.h"
#include "gl_world.h"
#include "gl_studio.h"
#include "gl_cvars.h"
#include "gl_debug.h"
#include "postfx_controller.h"
#include "entity_types.h"
#include <unordered_map>

static CBasePostEffects	post;
static bool g_nightVisionEnabled = false;
static bool g_nightVisionOwned = false;
static bool g_nightVisionButtonDown = false;
static float g_nightVisionTransitionStart = -1.0f;

struct NightVisionRenderState
{
	int renderMode;
	int renderFx;
	int renderAmt;
	color24 renderColor;
	int effects;
};

static std::unordered_map<cl_entity_t *, NightVisionRenderState> g_nightVisionEntities;

static void RestoreNightVisionEntities()
{
	for (auto &entry : g_nightVisionEntities)
	{
		cl_entity_t *entity = entry.first;
		const NightVisionRenderState &state = entry.second;
		entity->curstate.rendermode = state.renderMode;
		entity->curstate.renderfx = state.renderFx;
		entity->curstate.renderamt = state.renderAmt;
		entity->curstate.rendercolor = state.renderColor;
		entity->curstate.effects = state.effects;
	}
	g_nightVisionEntities.clear();
}

static void NightVisionDown_f()
{
	g_nightVisionButtonDown = false;
}

static void NightVisionToggle_f()
{
	if (!g_nightVisionOwned)
		return;
	if (g_nightVisionButtonDown)
		return;
	g_nightVisionButtonDown = true;

	g_nightVisionEnabled = !g_nightVisionEnabled;
	g_nightVisionTransitionStart = GET_CLIENT_TIME();
	if (!g_nightVisionEnabled)
		RestoreNightVisionEntities();

	// Night vision changes the lighting variant used by world and studio
	// shaders. Invalidate their cached handles on both transitions.
	tr.glsl_valid_sequence++;
	R_StudioClearLightCache();

	// Play on the server-side player entity so nearby players (including
	// enemies) hear the device through normal spatial attenuation.
	if (g_nightVisionEnabled)
		ServerCmd("nv_sound 1\n");
	else
		ServerCmd("nv_sound 0\n");
}

void SetNightVisionOwned(bool owned)
{
	g_nightVisionOwned = owned;
	if (!owned && g_nightVisionEnabled)
	{
		g_nightVisionEnabled = false;
		g_nightVisionTransitionStart = GET_CLIENT_TIME();
		RestoreNightVisionEntities();
		tr.glsl_valid_sequence++;
		R_StudioClearLightCache();
	}
}

bool IsNightVisionEnabled()
{
	return g_nightVisionEnabled;
}

bool IsNightVisionEntityHighlighted(const cl_entity_t *entity)
{
	return g_nightVisionEnabled && entity &&
		g_nightVisionEntities.find(const_cast<cl_entity_t *>(entity)) != g_nightVisionEntities.end();
}

void ApplyNightVisionEntityHighlight(cl_entity_t *entity, int entityType)
{
	if (!g_nightVisionEnabled || !entity || !entity->model)
		return;
	auto savedState = g_nightVisionEntities.find(entity);

	// Players are identified by the entity type. GoldSrc monsters use a
	// studio model, step movement and a slide-box hull.
	const bool isPlayer = entityType == ET_PLAYER;
	const bool isMonster = entity->model->type == mod_studio &&
		entity->curstate.movetype == MOVETYPE_STEP &&
		entity->curstate.solid == SOLID_SLIDEBOX;
	const float maxDistance = 50.0f * 39.37f; // metres to GoldSrc units
	if ((!isPlayer && !isMonster) || (entity->origin - GetVieworg()).Length() > maxDistance)
	{
		if (savedState != g_nightVisionEntities.end())
		{
			const NightVisionRenderState state = savedState->second;
			entity->curstate.rendermode = state.renderMode;
			entity->curstate.renderfx = state.renderFx;
			entity->curstate.renderamt = state.renderAmt;
			entity->curstate.rendercolor = state.renderColor;
			entity->curstate.effects = state.effects;
			g_nightVisionEntities.erase(savedState);
		}
		return;
	}

	if (savedState == g_nightVisionEntities.end())
	{
		g_nightVisionEntities.emplace(entity, NightVisionRenderState{
			entity->curstate.rendermode, entity->curstate.renderfx, entity->curstate.renderamt,
			entity->curstate.rendercolor, entity->curstate.effects });
	}

	// Keep opaque depth-tested rendering. Brightness is supplied by the
	// studio shader multiplier; translucent additive rendering causes body
	// parts and characters behind each other to show through.
	entity->curstate.rendermode = kRenderNormal;
	entity->curstate.renderfx = kRenderFxNone;
	entity->curstate.renderamt = 255;
	// Feed the monochrome intensifier a maximum-energy signal. The following
	// tint pass turns this white additive model into acid green.
	entity->curstate.rendercolor.r = 255;
	entity->curstate.rendercolor.g = 255;
	entity->curstate.rendercolor.b = 255;
	entity->curstate.effects |= EF_FULLBRIGHT;
}
void V_RenderPostEffect(word hProgram);

void CBasePostEffects::InitializeTextures()
{
	InitScreencopyFramebuffer();
	InitTargetColor(0);
	InitDepthOfField();
	InitAutoExposure();
}

void CBasePostEffects::InitializeShaders()
{
	char options[MAX_OPTIONS_LENGTH];
	memset(options, 0, sizeof(options));

	// postprocessing
	postprocessingShader = GL_FindShader("postfx/postprocessing", "postfx/generic", "postfx/postprocessing");
	nightVisionShader = GL_FindShader("postfx/nightvision", "postfx/generic", "postfx/nightvision");

	// gaussian blur for X
	GL_SetShaderDirective(options, "BLUR_X");
	blurShader[0] = GL_FindShader("postfx/gaussblur", "postfx/generic", "postfx/gaussblur", options);

	// gaussian blur for Y
	GL_SetShaderDirective(options, "BLUR_Y");
	blurShader[1] = GL_FindShader("postfx/gaussblur", "postfx/generic", "postfx/gaussblur", options);

	// DOF with bokeh
	dofShader = GL_FindShader("postfx/dofbokeh", "postfx/generic", "postfx/dofbokeh");

	// prepare sunshafts
	genSunShafts = GL_FindShader("postfx/genshafts", "postfx/generic", "postfx/genshafts");

	// render sunshafts
	drawSunShafts = GL_FindShader("postfx/drawshafts", "postfx/generic", "postfx/drawshafts");

	// tonemapping
	if (CVAR_TO_BOOL(r_show_luminance)) {
		GL_SetShaderDirective(options, "DEBUG_LUMINANCE");
	}
	tonemapShader = GL_FindShader("postfx/tonemap", "postfx/generic", "postfx/tonemap", options);

	// automatic exposure correction
	exposureGenShader = GL_FindShader("postfx/generate_exposure", "postfx/generic", "postfx/generate_exposure");
	luminanceGenShader = GL_FindShader("postfx/generate_luminance", "postfx/generic", "postfx/generate_luminance");
	luminanceDownscaleShader = GL_FindShader("postfx/downscale_luminance", "postfx/generic", "postfx/downscale_luminance");

	// bloom effect
	blurMipShader = GL_FindShader("postfx/gaussblurmip", "postfx/generic", "postfx/gaussblurmip");
	bloomShader = GL_FindShader("postfx/bloom", "postfx/generic", "postfx/bloom");
}

void CBasePostEffects :: InitScreencopyFramebuffer()
{
	int tex_flags = 0;
	bool hdr_rendering = CVAR_TO_BOOL(gl_hdr);

	if (hdr_rendering)
		tex_flags = TF_COLORBUFFER | TF_HAS_ALPHA | TF_ARB_16BIT | TF_ARB_FLOAT;
	else
		tex_flags = TF_COLORBUFFER;

	FREE_TEXTURE(tr.screen_color);
	FREE_TEXTURE(tr.screen_depth);
	tr.screen_color = CREATE_TEXTURE( "*screencolor", glState.width, glState.height, NULL, tex_flags);
	tr.screen_depth = CREATE_TEXTURE( "*screendepth", glState.width, glState.height, NULL, TF_DEPTHBUFFER ); 

	if (hdr_rendering)
	{
		GL_FreeDrawbuffer(tr.screencopy_fbo);
		tr.screencopy_fbo = GL_AllocDrawbuffer("*screencopy_fbo", glState.width, glState.height, 1);
		GL_AttachColorTextureToFBO(tr.screencopy_fbo, tr.screen_color, 0);
		GL_AttachDepthTextureToFBO(tr.screencopy_fbo, tr.screen_depth, 0);
		GL_CheckFBOStatus(tr.screencopy_fbo);
	}
}

void CBasePostEffects :: InitTargetColor( int slot )
{
	int tex_flags = 0;
	bool hdr_rendering = CVAR_TO_BOOL(gl_hdr);

	if( target_rgb[slot].Initialized() )
	{
		FREE_TEXTURE( target_rgb[slot] );
		target_rgb[slot] = TextureHandle::Null();
	}

	if (hdr_rendering)
		tex_flags = TF_IMAGE | TF_ARB_16BIT | TF_ARB_FLOAT;
	else
		tex_flags = TF_IMAGE;

	target_rgb[slot] = CREATE_TEXTURE(va( "*target%i", slot ), TARGET_SIZE, TARGET_SIZE, NULL, tex_flags);
}

void CBasePostEffects :: RequestScreenColor( void )
{
	bool hdr_rendering = CVAR_TO_BOOL(gl_hdr);
	GL_Bind(GL_TEXTURE0, tr.screen_color);
	if (hdr_rendering)
	{
		pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, tr.screencopy_fbo->id);
		pglBlitFramebuffer(0, 0, glState.width, glState.height, 0, 0, glState.width, glState.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		pglBindFramebuffer(GL_FRAMEBUFFER_EXT, glState.frameBuffer);
	}
	else
	{
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, glState.width, glState.height);
	}
}

void CBasePostEffects :: RequestScreenDepth( void )
{
	bool hdr_rendering = CVAR_TO_BOOL(gl_hdr);
	GL_Bind(GL_TEXTURE0, tr.screen_depth);
	if (hdr_rendering)
	{
		pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, tr.screencopy_fbo->id);
		pglBlitFramebuffer(0, 0, glState.width, glState.height, 0, 0, glState.width, glState.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		pglBindFramebuffer(GL_FRAMEBUFFER_EXT, glState.frameBuffer);
	}
	else
	{
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, glState.width, glState.height);
	}
}

void CBasePostEffects :: RequestTargetCopy( int slot )
{
	GL_Bind(GL_TEXTURE0, target_rgb[slot]);
	pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, TARGET_SIZE, TARGET_SIZE);
}

void CBasePostEffects::PrintLuminanceValue()
{
	if (!CVAR_TO_BOOL(r_show_luminance))
		return;

	float luminance;
	char valueString[32];
	int	width = avg_luminance_texture.GetWidth();
	int	height = avg_luminance_texture.GetHeight();
	const int mipCount = GL_TextureMipCount(width, height);

	pglBindFramebuffer(GL_READ_FRAMEBUFFER, avg_luminance_fbo[0]->id);
	pglReadPixels(width / 2, height / 2, 1, 1, GL_GREEN, GL_FLOAT, &luminance);
	pglBindFramebuffer(GL_FRAMEBUFFER_EXT, glState.frameBuffer);

	Q_snprintf(valueString, sizeof(valueString), "%.2f", luminance);
	gEngfuncs.pfnCenterPrint(valueString);
}

void CBasePostEffects::RenderAverageLuminance()
{
	GL_DEBUG_SCOPE();

	// render luminance to first mip
	gl_drawbuffer_t *baseLevel = avg_luminance_fbo[0];
	pglViewport(0, 0, baseLevel->width, baseLevel->height);
	pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, baseLevel->id);
	V_RenderPostEffect(luminanceGenShader);

	// downscale luminance to other mips
	GL_BindShader(&glsl_programs[luminanceDownscaleShader]);
	const int mipCount = GL_TextureMipCount(baseLevel->width, baseLevel->height);
	for (int a = 2, i = 1; i < mipCount; ++i, a *= 2)
	{
		gl_drawbuffer_t *fbo = avg_luminance_fbo[i];
		int w = Q_max(fbo->width / a, 1);
		int h = Q_max(fbo->height / a, 1);
		
		for (int j = 0; j < RI->currentshader->numUniforms; j++)
		{
			uniform_t *u = &RI->currentshader->uniforms[j];
			switch (u->type)
			{
				case UT_SCREENMAP:
					u->SetValue(avg_luminance_texture.ToInt());
					break;
				case UT_SCREENSIZEINV:
					u->SetValue(1.0f / (float)w, 1.0f / (float)h);
					break;
				case UT_MIPLOD:
					u->SetValue((float)(i - 1));
					break;
				case UT_TEXCOORDCLAMP:
					u->SetValue(0.0f, 0.0f, 1.0f, 1.0f);
					break;
			}
		}

		pglViewport(0, 0, w, h);
		pglBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo->id);
		RenderFSQ(glState.width, glState.height);
	}

	// restore GL state
	GL_CleanupAllTextureUnits();
	pglViewport(0, 0, glState.width, glState.height);
	pglBindFramebuffer(GL_FRAMEBUFFER_EXT, glState.frameBuffer);
}

TextureHandle CBasePostEffects::RenderExposureStorage()
{
	static int fboIndex = 0;
	const int sourceIndex = fboIndex % 2;
	const int destIndex = (fboIndex + 1) % 2;
	gl_drawbuffer_t *fbo = avg_luminance_fbo[0];
	const float mipCount = static_cast<float>(GL_TextureMipCount(fbo->width, fbo->height));

	GL_DEBUG_SCOPE();
	GL_BindShader(&glsl_programs[exposureGenShader]);
	fboIndex = (fboIndex + 1) % 2;

	for (int j = 0; j < RI->currentshader->numUniforms; j++)
	{
		uniform_t *u = &RI->currentshader->uniforms[j];
		switch (u->type)
		{
			case UT_SCREENMAP:
				u->SetValue(avg_luminance_texture.ToInt());
				break;
			case UT_NORMALMAP:
				u->SetValue(exposure_storage_texture[sourceIndex].ToInt());
				break;
			case UT_MIPLOD:
				u->SetValue(mipCount - 1);
				break;
			case UT_TIMEDELTA: 
				u->SetValue((float)tr.frametime);
				break;
		}
	}

	pglViewport(0, 0, 1, 1);
	pglBindFramebuffer(GL_FRAMEBUFFER_EXT, exposure_storage_fbo[destIndex]->id);
	RenderFSQ(glState.width, glState.height);

	// restore GL state
	pglViewport(0, 0, glState.width, glState.height);
	pglBindFramebuffer(GL_FRAMEBUFFER_EXT, glState.frameBuffer);
	GL_CleanupAllTextureUnits();
	return exposure_storage_texture[destIndex];
}

void CBasePostEffects :: SetNormalViewport( void )
{
	RI->glstate.viewport.SetAsCurrent();
}

void CBasePostEffects :: SetTargetViewport( void )
{
	pglViewport( 0, 0, TARGET_SIZE, TARGET_SIZE );
}

void CBasePostEffects :: InitDepthOfField( void )
{

}
 
void CBasePostEffects::InitAutoExposure()
{
	const int texWidth = Q_min(1280, glState.width);
	const int texHeight = Q_min(720, glState.height);
	const int mipCount = GL_TextureMipCount(texWidth, texHeight);
	const vec3_t colorBlack = vec3_t(0.0f, 0.0f, 0.0f);

	// make this textures TF_LUMINANCE for using just one color channel
	FREE_TEXTURE(avg_luminance_texture);
	avg_luminance_texture = CREATE_TEXTURE("*screen_avg_luminance", texWidth, texHeight, NULL, TF_ARB_16BIT | TF_ARB_FLOAT); 
	GL_Bind(GL_TEXTURE0, avg_luminance_texture);
	pglGenerateMipmap(GL_TEXTURE_2D);
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GL_Bind(GL_TEXTURE0, TextureHandle::Null());

	if (avg_luminance_fbo)
	{
		for (int i = 0; i < mipCount; ++i) {
			GL_FreeDrawbuffer(avg_luminance_fbo[i]);
		}
		delete[] avg_luminance_fbo;
		avg_luminance_fbo = nullptr;
	}

	avg_luminance_fbo = new gl_drawbuffer_t*[mipCount];
	for (int i = 0; i < mipCount; ++i)
	{
		avg_luminance_fbo[i] = GL_AllocDrawbuffer("*screen_avg_luminance_fbo", texWidth, texHeight, 1);
		GL_AttachColorTextureToFBO(avg_luminance_fbo[i], avg_luminance_texture, 0, 0, i);
		GL_CheckFBOStatus(avg_luminance_fbo[i]);
	}

	for (int i = 0; i < 2; ++i) 
	{
		FREE_TEXTURE(exposure_storage_texture[i]);
		GL_FreeDrawbuffer(exposure_storage_fbo[i]);
		exposure_storage_texture[i] = CREATE_TEXTURE(va("*exposure_storage%d", i), 1, 1, NULL, TF_ARB_16BIT | TF_ARB_FLOAT);

		GL_Bind(GL_TEXTURE0, exposure_storage_texture[i]);
		pglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGB, GL_FLOAT, &colorBlack.x);
		GL_Bind(GL_TEXTURE0, TextureHandle::Null());

		exposure_storage_fbo[i] = GL_AllocDrawbuffer("*exposure_storage_fbo", 1, 1, 1);
		GL_AttachColorTextureToFBO(exposure_storage_fbo[i], exposure_storage_texture[i], 0);
		GL_CheckFBOStatus(exposure_storage_fbo[i]);
	}
}

bool CBasePostEffects :: ProcessDepthOfField( void )
{
	if( !CVAR_TO_BOOL( r_dof ) ) // or no iron sight on weapon
		return false; // disabled or unitialized

	if( m_flDOFStartTime == 0.0f )
		return false; // iron sight disabled

	if( !Begin( )) 
		return false;

	if( m_flDOFStartTime != 0.0f )
	{
		float flDegree = (tr.time - m_flDOFStartTime) / 0.3f;

		if( flDegree >= 1.0f )
		{
			// all done. holds the final value
			m_flLastLength = m_flStartLength + m_flOffsetLength;
			m_flDOFStartTime = 0.0f; // done
		}
		else
		{
			// evaluate focal length
			m_flLastLength = m_flStartLength + m_flOffsetLength * flDegree;
		}
	}

	float zNear = Z_NEAR; // fixed
	float zFar = RI->view.farClip;
	float depthValue = 0.0f;

	// get current depth value
	// TODO refactor it to get rid of glReadPixels, because it's slow as fuck
	// 1. do trace from camera to point along view direction
	// 2. convert collision point coordinates to view space
	// 3. compute depth value using formula (z - near) / (far - near) (in this case we don't need linearization)
	pglReadPixels( glState.width >> 1, glState.height >> 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthValue );
	depthValue = -zFar * zNear / ( depthValue * ( zFar - zNear ) - zFar ); // linearize it
	float holdTime = bound( 0.01f, r_dof_hold_time->value, 0.5f );
	float changeTime = bound( 0.1f, r_dof_change_time->value, 2.0f );

	if( Q_round( m_flCachedDepth, 10 ) != Q_round( depthValue, 10 ))
	{
		m_flStartTime = 0.0f; // cancelling changes
		m_flDelayTime = tr.time; // make sure what focal point is not changed more than 0.5 secs
		m_flStartDepth = m_flLastDepth; // get last valid depth
		m_flOffsetDepth = depthValue - m_flStartDepth;
		m_flCachedDepth = depthValue;
	}

	if(( tr.time - m_flDelayTime ) > holdTime && m_flStartTime == 0.0f && m_flDelayTime != 0.0f )
	{
		// begin the change depth
		m_flStartTime = tr.time;
	}

	if( m_flStartTime != 0.0f )
	{
		float flDegree = (tr.time - m_flStartTime) / changeTime;

		if( flDegree >= 1.0f )
		{
			// all done. holds the final value
			m_flLastDepth = m_flStartDepth + m_flOffsetDepth;
			m_flStartTime = m_flDelayTime = 0.0f;
		}
		else
		{
			// evaluate focal depth
			m_flLastDepth = m_flStartDepth + m_flOffsetDepth * flDegree;
		}
	}

	return true;
}

bool CBasePostEffects :: ProcessSunShafts( void )
{
	if( !CVAR_TO_BOOL( v_sunshafts ))
		return false;

	if( !FBitSet( world->features, WORLD_HAS_SKYBOX ))
		return false;

	if( tr.sky_normal == g_vecZero )
		return false;

	// update blur params
	blurFactor[0] = 0.15f;
	blurFactor[1] = 0.15f;

	if( tr.sun_light_enabled ) m_vecSunLightColor = tr.sun_diffuse;
	else m_vecSunLightColor = tr.sky_ambient * (1.0f/128.0f) * tr.diffuseFactor;
	Vector sunPos = tr.cached_vieworigin + tr.sky_normal * 1000.0;
	ColorNormalize( m_vecSunLightColor, m_vecSunLightColor );

	Vector ndc, view;

	// project sunpos to screen 
	R_TransformWorldToDevice( sunPos, ndc );
	R_TransformDeviceToScreen( ndc, m_vecSunPosition );
	m_vecSunPosition.z = DotProduct( -tr.sky_normal, GetVForward( ));

	if( m_vecSunPosition.z < 0.01f )
		return false; // fade out

	// convert to screen pixels
	m_vecSunPosition.x = m_vecSunPosition.x / glState.width;
	m_vecSunPosition.y = m_vecSunPosition.y / glState.height;

	return Begin();
}

bool CBasePostEffects :: Begin( void )
{
	// we are in cubemap rendering mode
	if( !RP_NORMALPASS( ))
		return false;

	if( !CVAR_TO_BOOL( v_posteffects ))
		return false;

	GL_Setup2D();

	return true;
}

void CBasePostEffects :: End( void )
{
	GL_CleanUpTextureUnits( 0 );
	GL_BindShader( NULL );
	GL_Setup3D();
}

void InitPostEffects()
{
	gEngfuncs.pfnAddCommand("+nv", NightVisionToggle_f);
	gEngfuncs.pfnAddCommand("-nv", NightVisionDown_f);
	gEngfuncs.pfnClientCmd("bind n +nv\n");
	v_posteffects = CVAR_REGISTER( "gl_posteffects", "1", FCVAR_ARCHIVE );
	v_sunshafts = CVAR_REGISTER( "gl_sunshafts", "1", FCVAR_ARCHIVE );
	r_postfx_enable = CVAR_REGISTER("r_postfx_enable", "1.0", 0);
	r_tonemap = CVAR_REGISTER("r_tonemap", "1", FCVAR_ARCHIVE);
	r_bloom = CVAR_REGISTER("r_bloom", "1", FCVAR_ARCHIVE);
	r_bloom_scale = CVAR_REGISTER("r_bloom_scale", "0.7", FCVAR_ARCHIVE);
	memset( &post, 0, sizeof( post ));
	post.InitializeShaders();
}

void InitPostTextures()
{
	post.InitializeTextures();
}

void InitPostprocessShaders()
{
	post.InitializeShaders();
}

// rectangle version
void RenderFSQ( void )
{
	float screenWidth = (float)glState.width;
	float screenHeight = (float)glState.height;

	pglBegin( GL_QUADS );
		pglTexCoord2f( 0.0f, screenHeight );
		pglVertex2f( 0.0f, 0.0f );
		pglTexCoord2f( screenWidth, screenHeight );
		pglVertex2f( screenWidth, 0.0f );
		pglTexCoord2f( screenWidth, 0.0f );
		pglVertex2f( screenWidth, screenHeight );
		pglTexCoord2f( 0.0f, 0.0f );
		pglVertex2f( 0.0f, screenHeight );
	pglEnd();
}

void RenderFSQ( int wide, int tall )
{
	float screenWidth = (float)wide;
	float screenHeight = (float)tall;

	pglBegin( GL_QUADS );
		pglTexCoord2f( 0.0f, 1.0f );
		pglNormal3fv( tr.screen_normals[0] );
		pglVertex2f( 0.0f, 0.0f );
		pglTexCoord2f( 1.0f, 1.0f );
		pglNormal3fv( tr.screen_normals[1] );
		pglVertex2f( screenWidth, 0.0f );
		pglTexCoord2f( 1.0f, 0.0f );
		pglNormal3fv( tr.screen_normals[2] );
		pglVertex2f( screenWidth, screenHeight );
		pglTexCoord2f( 0.0f, 0.0f );
		pglNormal3fv( tr.screen_normals[3] );
		pglVertex2f( 0.0f, screenHeight );
	pglEnd();
}

void GL_DrawScreenSpaceQuad( void )
{
	pglBegin( GL_QUADS );
		pglTexCoord2f( 0.0f, 1.0f );
		pglNormal3fv( tr.screen_normals[0] );
		pglVertex3f( 0.0f, 0.0f, 0.0f );
		pglNormal3fv( tr.screen_normals[1] );
		pglTexCoord2f( 0.0f, 0.0f );
		pglVertex3f( 0.0f, glState.height, 0.0f );
		pglNormal3fv( tr.screen_normals[2] );
		pglTexCoord2f( 1.0f, 0.0f );
		pglVertex3f( glState.width, glState.height, 0.0f );
		pglNormal3fv( tr.screen_normals[3] );
		pglTexCoord2f( 1.0f, 1.0f );
		pglVertex3f( glState.width, 0.0f, 0.0f );
	pglEnd();
}

void V_RenderPostEffect( word hProgram )
{
	if( hProgram <= 0 || hProgram >= num_glsl_programs ||
		!glsl_programs[hProgram].initialized ||
		!FBitSet(glsl_programs[hProgram].status, SHADER_PROGRAM_LINKED) )
	{
		GL_BindShader( NULL );
		return; // bad shader?
	}

	if( RI->currentshader != &glsl_programs[hProgram] )
	{
		// force to bind new shader
		GL_BindShader( &glsl_programs[hProgram] );
	}

	glsl_program_t *shader = RI->currentshader;
	if (!shader)
		return;

	// setup specified uniforms (and texture bindings)
	for( int i = 0; i < shader->numUniforms; i++ )
	{
		uniform_t *u = &shader->uniforms[i];

		switch( u->type )
		{
		case UT_SCREENMAP:
			if( post.m_bUseTarget ) // HACKHACK
				u->SetValue( post.target_rgb[0].ToInt() );
			else u->SetValue( tr.screen_color.ToInt() );
			break;
		case UT_DEPTHMAP:
			u->SetValue( tr.screen_depth.ToInt() );
			break;
		case UT_COLORMAP:
			u->SetValue( post.target_rgb[0].ToInt() );
			break;
		case UT_BRIGHTNESS:
			u->SetValue(post.fxParameters.GetBrightness());
			break;
		case UT_SATURATION:
			u->SetValue(post.fxParameters.GetSaturation());
			break;
		case UT_CONTRAST:
			u->SetValue(post.fxParameters.GetContrast());
			break;
		case UT_COLORLEVELS:
			u->SetValue(post.fxParameters.GetRedLevel(), post.fxParameters.GetGreenLevel(), post.fxParameters.GetBlueLevel());
			break;
		case UT_VIGNETTESCALE:
			u->SetValue(post.fxParameters.GetVignetteScale());
			break;
		case UT_FILMGRAINSCALE:
			u->SetValue(post.fxParameters.GetFilmGrainScale());
			break;
		case UT_ACCENTCOLOR:
		{
			const Vector &accentColor = post.fxParameters.GetAccentColor();
			u->SetValue(accentColor.x, accentColor.y, accentColor.z, post.fxParameters.GetColorAccentScale());
			break;
		}
		case UT_BLURFACTOR:
			u->SetValue( post.blurFactor[0], post.blurFactor[1] );
			break;
		case UT_SCREENSIZEINV:
			u->SetValue( 1.0f / (float)glState.width, 1.0f / (float)glState.height );
			break;
		case UT_SCREENWIDTH:
			u->SetValue( (float)glState.width );
			break;
		case UT_SCREENHEIGHT:
			u->SetValue( (float)glState.height );
			break;
		case UT_FOCALDEPTH:
			u->SetValue( post.m_flLastDepth );
			break;
		case UT_FOCALLENGTH:
			u->SetValue( post.m_flLastLength );
			break;
		case UT_DOFDEBUG:
			u->SetValue( CVAR_TO_BOOL( r_dof_debug ));
			break;
		case UT_FSTOP:
			u->SetValue( r_dof_fstop->value );
			break;
		case UT_ZFAR:
			u->SetValue( RI->view.farClip );
			break;
		case UT_LIGHTGAMMA:
			u->SetValue( tr.light_gamma );
			break;
		case UT_AMBIENTFACTOR:
			u->SetValue( tr.ambientFactor );
			break;
		case UT_SUNREFRACT:
			u->SetValue( tr.sun_refract );
			break;
		case UT_REALTIME:
			u->SetValue( (float)tr.time );
			break;
		case UT_LIGHTDIFFUSE:
			u->SetValue( post.m_vecSunLightColor.x, post.m_vecSunLightColor.y, post.m_vecSunLightColor.z );
			break;
		case UT_LIGHTORIGIN:
			u->SetValue( post.m_vecSunPosition.x, post.m_vecSunPosition.y, post.m_vecSunPosition.z ); 
			break;
		case UT_FOGPARAMS:
			u->SetValue( tr.fogColor[0], tr.fogColor[1], tr.fogColor[2], tr.fogDensity );
			break;
		default:
			ALERT( at_error, "%s: unhandled uniform %s\n", RI->currentshader->name, u->name );
			break;
		}
	}

	// render a fullscreen quad
	RenderFSQ( glState.width, glState.height );
}

void RenderBlur( float blurX, float blurY )
{
	if( !blurX && !blurY )
		return;

	// update blur params
	post.blurFactor[0] = blurX;
	post.blurFactor[1] = blurY;

	if( !post.Begin( )) return;

	// do vertical blur
	post.RequestScreenColor();
	V_RenderPostEffect( post.blurShader[0] );

	// do horizontal blur
	post.RequestScreenColor();
	V_RenderPostEffect( post.blurShader[1] );

	post.End();
}

void RenderPostprocessing()
{
	// we are in cubemap rendering mode
	if (!RP_NORMALPASS())
		return;

	GL_DEBUG_SCOPE();
	if (CVAR_TO_BOOL(r_postfx_enable)) {
		post.fxParameters = g_PostFxController.GetState();
	}
	else
	{
		// use default values
		post.fxParameters = CPostFxParameters::Defaults();
	}

	GL_Setup2D();
	post.RequestScreenColor();
	V_RenderPostEffect( post.postprocessingShader );
	post.End();
}

void RenderNightVision()
{
	if (!g_nightVisionEnabled || !post.Begin())
		return;
	const bool dedicatedShaderAvailable = post.nightVisionShader > 0 &&
		post.nightVisionShader < num_glsl_programs &&
		glsl_programs[post.nightVisionShader].initialized &&
		FBitSet(glsl_programs[post.nightVisionShader].status, SHADER_PROGRAM_LINKED);
	const bool fallbackShaderAvailable = post.postprocessingShader > 0 &&
		post.postprocessingShader < num_glsl_programs &&
		glsl_programs[post.postprocessingShader].initialized &&
		FBitSet(glsl_programs[post.postprocessingShader].status, SHADER_PROGRAM_LINKED);

	if (!dedicatedShaderAvailable && !fallbackShaderAvailable)
	{
		ALERT(at_error, "Night vision post-processing shaders are unavailable.\n");
		post.End();
		return;
	}

	GL_DEBUG_SCOPE();
	post.RequestScreenColor();
	if (dedicatedShaderAvailable)
	{
		post.RequestScreenDepth();
		V_RenderPostEffect(post.nightVisionShader);
	}
	else
	{
		// Portable fallback for installations where the new GLSL asset has not
		// been copied into the active game directory yet. The first pass acts
		// as the image intensifier: desaturate, amplify weak values and compress
		// contrast so details do not remain crushed in black.
		post.fxParameters = CPostFxParameters::Defaults();
		post.fxParameters.SetBrightness(0.0f);
		post.fxParameters.SetSaturation(0.0f);
		post.fxParameters.SetContrast(1.18f);
		post.fxParameters.SetRedLevel(1.0f);
		post.fxParameters.SetGreenLevel(1.0f);
		post.fxParameters.SetBlueLevel(1.0f);
		V_RenderPostEffect(post.postprocessingShader);

		// Tint the intensified monochrome signal in a separate pass. Keeping
		// this separate is important: tinting before luminance extraction loses
		// dark red/blue texture detail.
		post.RequestScreenColor();
		post.fxParameters = CPostFxParameters::Defaults();
		post.fxParameters.SetBrightness(0.0f);
		post.fxParameters.SetSaturation(1.0f);
		post.fxParameters.SetContrast(1.18f);
		post.fxParameters.SetRedLevel(0.08f);
		post.fxParameters.SetGreenLevel(0.98f);
		post.fxParameters.SetBlueLevel(0.12f);
		post.fxParameters.SetVignetteScale(0.22f);
		post.fxParameters.SetFilmGrainScale(0.17f);
		V_RenderPostEffect(post.postprocessingShader);

		// Thin animated scan lines are intentionally drawn without a texture,
		// so this part also works without the dedicated shader.
		GL_BindShader(NULL);
		GL_CleanupAllTextureUnits();
		GL_Blend(GL_TRUE);
		GL_Cull(0);
		pglShadeModel(GL_SMOOTH);
		pglBlendFunc(GL_SRC_ALPHA, GL_ONE);
		pglColor4f(0.10f, 1.0f, 0.12f, 0.09f);
		const int offset = (int)(tr.time * 34.0f) % 8;
		pglBegin(GL_QUADS);
		for (int y = offset; y < glState.height; y += 8)
		{
			pglVertex2f(0.0f, (float)y);
			pglVertex2f((float)glState.width, (float)y);
			pglVertex2f((float)glState.width, (float)(y + 2));
			pglVertex2f(0.0f, (float)(y + 2));
		}
		pglEnd();

		// A slower, wider interference band prevents the scan pattern from
		// looking like a perfectly static screen-space grid.
		const float bandY = fmod(tr.time * 83.0f, (float)(glState.height + 80)) - 40.0f;
		pglColor4f(0.18f, 1.0f, 0.20f, 0.12f);
		pglBegin(GL_QUADS);
			pglVertex2f(0.0f, bandY);
			pglVertex2f((float)glState.width, bandY);
			pglVertex2f((float)glState.width, bandY + 5.0f);
			pglVertex2f(0.0f, bandY + 5.0f);
		pglEnd();

		// Dynamic sensor noise. The LCG changes every frame and avoids the very
		// regular appearance of the scan lines.
		pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		const float w = (float)glState.width;
		const float h = (float)glState.height;
		uint noiseSeed = (uint)(tr.time * 1000.0f) ^ 0x9E3779B9u;
		pglBegin(GL_QUADS);
		for (int i = 0; i < 5000; ++i)
		{
			noiseSeed = noiseSeed * 1664525u + 1013904223u;
			const float x = (float)(noiseSeed % (uint)glState.width);
			noiseSeed = noiseSeed * 1664525u + 1013904223u;
			const float y = (float)(noiseSeed % (uint)glState.height);
			noiseSeed = noiseSeed * 1664525u + 1013904223u;
			if (noiseSeed & 1u) pglColor4f(0.15f, 1.0f, 0.18f, 0.16f);
			else pglColor4f(0.0f, 0.0f, 0.0f, 0.20f);
			pglVertex2f(x, y); pglVertex2f(x + 2.0f, y);
			pglVertex2f(x + 2.0f, y + 2.0f); pglVertex2f(x, y + 2.0f);
		}
		pglEnd();

		// Strong four-sided optical mask. Overlap makes the corners darker and
		// leaves only a deliberately narrow central field fully unobstructed.
		const float vx = w * 0.42f;
		const float vy = h * 0.43f;
		// Draw twice: alpha compositing squares the remaining transmission and
		// produces a much denser optical mask without a hard inner seam.
		for (int vignettePass = 0; vignettePass < 2; ++vignettePass)
		{
		pglBegin(GL_QUADS);
			pglColor4f(0, 0, 0, 1.0f); pglVertex2f(0, 0); pglVertex2f(0, h);
			pglColor4f(0, 0, 0, 0.0f); pglVertex2f(vx, h); pglVertex2f(vx, 0);
			pglColor4f(0, 0, 0, 0.0f); pglVertex2f(w - vx, 0); pglVertex2f(w - vx, h);
			pglColor4f(0, 0, 0, 1.0f); pglVertex2f(w, h); pglVertex2f(w, 0);
			pglColor4f(0, 0, 0, 1.0f); pglVertex2f(0, 0); pglVertex2f(w, 0);
			pglColor4f(0, 0, 0, 0.0f); pglVertex2f(w, vy); pglVertex2f(0, vy);
			pglColor4f(0, 0, 0, 0.0f); pglVertex2f(0, h - vy); pglVertex2f(w, h - vy);
			pglColor4f(0, 0, 0, 1.0f); pglVertex2f(w, h); pglVertex2f(0, h);
		pglEnd();
		}
		pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		GL_Blend(GL_FALSE);
		GL_Cull(GL_FRONT);
	}
	post.End();
}

void RenderNightVisionTransition()
{
	if (g_nightVisionTransitionStart < 0.0f)
		return;

	const float elapsed = GET_CLIENT_TIME() - g_nightVisionTransitionStart;
	const float blackoutTime = 0.2f;
	const float recoveryTime = 1.0f;
	if (elapsed >= blackoutTime + recoveryTime)
	{
		g_nightVisionTransitionStart = -1.0f;
		return;
	}

	float alpha = 1.0f;
	if (elapsed > blackoutTime)
		alpha = 1.0f - (elapsed - blackoutTime) / recoveryTime;
	alpha = bound(0.0f, alpha, 1.0f);

	GL_Setup2D();
	GL_BindShader(NULL);
	GL_CleanupAllTextureUnits();
	GL_Cull(0);
	GL_Blend(GL_TRUE);
	pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	pglColor4f(0.0f, 0.0f, 0.0f, alpha);
	pglBegin(GL_QUADS);
		pglVertex2f(0.0f, 0.0f);
		pglVertex2f((float)glState.width, 0.0f);
		pglVertex2f((float)glState.width, (float)glState.height);
		pglVertex2f(0.0f, (float)glState.height);
	pglEnd();
	pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	GL_Blend(GL_FALSE);
	GL_Cull(GL_FRONT);
	GL_Setup3D();
}

void RenderUnderwaterBlur( void )
{
	if( !CVAR_TO_BOOL( cv_water ) || tr.waterlevel < 3 )
		return;

	float factor = sin( tr.time * 0.1f * ( M_PI * 2.7f ));
	float blurX = RemapVal( factor, -1.0f, 1.0f, 0.18f, 0.23f );
	float blurY = RemapVal( factor, -1.0f, 1.0f, 0.15f, 0.24f );

	GL_DEBUG_SCOPE();
	RenderBlur( blurX, blurY );
}

void RenderNerveGasBlur( void )
{
	float blurAmount = gHUD.m_flGasStrength * 0.24f;
	if (blurAmount <= 0.0f )
		return;

	float factor = sin( tr.time * 0.4f * ( M_PI * 1.7f ));
	float blurX = RemapVal( factor, -1.0f, 1.0f, 0.0f, 0.3f );
	float blurY = RemapVal( factor, -1.0f, 1.0f, 0.0f, 0.3f );

	blurX = bound( 0.0f, blurX, blurAmount);
	blurY = bound( 0.0f, blurY, blurAmount);

	GL_DEBUG_SCOPE();
	RenderBlur( blurX, blurY );
}

void RenderDOF( void )
{
	if( !post.ProcessDepthOfField( ))
		return;

	GL_DEBUG_SCOPE();
	post.RequestScreenColor();
	post.RequestScreenDepth();
	V_RenderPostEffect( post.dofShader );
	post.End();
}

void RenderSunShafts( void )
{
	if( !post.ProcessSunShafts( ))
		return;

	GL_DEBUG_SCOPE();
	post.RequestScreenColor();
	post.RequestScreenDepth();

	// we operate in small window to increase speedup
	post.SetTargetViewport();
	V_RenderPostEffect( post.genSunShafts );
	post.RequestTargetCopy( 0 );

	post.m_bUseTarget = true;
	V_RenderPostEffect( post.blurShader[0] );
	post.RequestTargetCopy( 0 );
	V_RenderPostEffect( post.blurShader[1] );
	post.RequestTargetCopy( 0 );
	post.m_bUseTarget = false;

	// back to normal size
	post.SetNormalViewport();
	V_RenderPostEffect( post.drawSunShafts );
	post.End();
}

void RenderBloom()
{
	if (!CVAR_TO_BOOL(r_bloom) || CVAR_TO_BOOL(r_show_luminance)) 
		return;

	if (FBitSet(RI->params, RP_ENVVIEW)) // no bloom in cubemaps
		return;

	if (post.blurMipShader <= 0)
	{
		GL_BindShader(NULL);
		return; // bad shader?
	}

	if (post.bloomShader <= 0)
	{
		GL_BindShader(NULL);
		return; // bad shader?
	}

	GL_DEBUG_SCOPE();
	GL_BindShader(&glsl_programs[post.blurMipShader]);
	GL_Bind(GL_TEXTURE0, tr.screen_hdr_fbo->colortarget[0]);
	GL_Setup2D();

	int w = glState.width;
	int h = glState.height;
	glsl_program_t *shader = RI->currentshader;
	const int mipCount = GL_TextureMipCount(w, h);

	// render and blur mips
	for (int i = 0; i < (mipCount - 1); i++)
	{
		w = Q_max(1, w / 2);
		h = Q_max(1, h / 2);

		pglViewport(0, 0, w, h);
		for (int j = 0; j < shader->numUniforms; j++)
		{
			uniform_t *u = &shader->uniforms[j];
			switch (u->type)
			{
				case UT_SCREENSIZEINV:
					u->SetValue(1.0f / (float)w, 1.0f / (float)h);
					break;
				case UT_MIPLOD:
					u->SetValue((float)i);
					break;
				case UT_TEXCOORDCLAMP:
					u->SetValue(0.0f, 0.0f, 1.0f, 1.0f);
					break;
				case UT_BLOOMFIRSTPASS:
					u->SetValue((i < 1) ? 1 : 0);
					break;
				case UT_REFRACTSCALE:
					u->SetValue(r_bloom_scale->value);
					break;
			}
		}

		pglBindFramebuffer(GL_FRAMEBUFFER_EXT, tr.screen_hdr_fbo_mip[i]);
		RenderFSQ(glState.width, glState.height);
	}

	w = glState.width / 2;
	h = glState.height / 2;

	pglViewport(0, 0, w, h);
	pglBindFramebuffer(GL_FRAMEBUFFER_EXT, tr.screen_hdr_fbo_mip[0]);
	GL_BindShader(&glsl_programs[post.bloomShader]);

	shader = RI->currentshader;
	for (int j = 0; j < shader->numUniforms; j++)
	{
		uniform_t *u = &shader->uniforms[j];
		switch (u->type)
		{
			case UT_SCREENSIZEINV:
				u->SetValue((float)w, (float)h);	//actual screen size, not inverted
				break;
		}
	}
	RenderFSQ(glState.width, glState.height);

	// blend screen image & first mip together
	pglViewport(0, 0, glState.width, glState.height);
	pglBindFramebuffer(GL_FRAMEBUFFER_EXT, glState.frameBuffer);
	GL_BindShader(NULL);
	GL_Blend(GL_TRUE);
	pglBlendFunc(GL_ONE, GL_ONE);
	pglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 1.0);
	RenderFSQ(glState.width, glState.height);
	pglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0.0);
	GL_Blend(GL_FALSE);
	post.End();
}

void RenderTonemap()
{
	if (!CVAR_TO_BOOL(r_tonemap))
		return;

	GL_DEBUG_SCOPE();
	GL_Setup2D();
	post.RequestScreenColor();
	TextureHandle exposureTexture = post.RenderExposureStorage();

	GL_BindShader(&glsl_programs[post.tonemapShader]);
	glsl_program_t *shader = RI->currentshader;
	for (int i = 0; i < shader->numUniforms; i++)
	{
		uniform_t *u = &shader->uniforms[i];
		switch (u->type)
		{
			case UT_SCREENMAP:
				if (post.m_bUseTarget) // HACKHACK
					u->SetValue(post.target_rgb[0].ToInt());
				else 
					u->SetValue(tr.screen_color.ToInt());
				break;
			case UT_COLORMAP:
				u->SetValue(exposureTexture.ToInt());
				break;
		}
	}
	RenderFSQ(glState.width, glState.height);
	post.PrintLuminanceValue();
	post.End();
}

void RenderAverageLuminance()
{
	ZoneScoped;
	GL_DEBUG_SCOPE();
	GL_Setup2D();
	post.RequestScreenColor();
	post.RenderAverageLuminance();
	post.End();
}
