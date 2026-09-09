#include "stick_camera_shared.h"
#include "weapon_stick_camera.h"
#include "server_weapon_layer_impl.h"
#include "weapons/stick_camera.h"
#include "player.h"
#include "sfu_door.h"
#include "physic.h"
#include "trace.h"

namespace { constexpr float CAMERA_RADIUS=12.0f,CAMERA_DISTANCE=100.0f,TURN_LIMIT=STICK_CAMERA_TURN_LIMIT,ZOOM_FOV=30.0f; float Normalize(float a){while(a>180)a-=360;while(a<-180)a+=360;return a;} }
class CStickCameraView:public CBaseEntity{DECLARE_CLASS(CStickCameraView,CBaseEntity);public:void Spawn()override{pev->movetype=MOVETYPE_NOCLIP;pev->solid=SOLID_NOT;SET_MODEL(edict(),"models/w_shotgun.mdl");UTIL_SetSize(this,Vector(-CAMERA_RADIUS,-CAMERA_RADIUS,-CAMERA_RADIUS),Vector(CAMERA_RADIUS,CAMERA_RADIUS,CAMERA_RADIUS));pev->iuser4=STICK_CAMERA_MARKER;pev->rendermode=kRenderNormal;pev->renderamt=255;SetBits(pev->effects,EF_NOINTERP|EF_MERGE_VISIBILITY);}};
LINK_ENTITY_TO_CLASS(weapon_stickcamera,CStickCamera);LINK_ENTITY_TO_CLASS(stick_camera_view,CStickCameraView);
CStickCamera::CStickCamera(){auto layer=std::make_unique<CServerWeaponLayerImpl>(this);m_pWeaponContext=std::make_unique<CStickCameraWeaponContext>(std::move(layer));}
void CStickCamera::Spawn(){Precache();SET_MODEL(edict(),"models/w_shotgun.mdl");FallInit();}
void CStickCamera::Precache(){PRECACHE_MODEL("models/weapon/StickCamera/v_stickcamera.mdl");PRECACHE_MODEL("models/p_shotgun.mdl");PRECACHE_MODEL("models/w_shotgun.mdl");UTIL_PrecacheOther("stick_camera_view");}
CBaseEntity*CStickCamera::EnsureViewEntity(){CBaseEntity*view=m_hViewEntity;if(!view){view=CBaseEntity::Create("stick_camera_view",m_pPlayer->GetGunPosition(),g_vecZero,m_pPlayer->edict());if(view){view->pev->iuser1=m_pPlayer->entindex();view->pev->iuser4=STICK_CAMERA_MARKER;view->pev->colormap=m_pPlayer->entindex();}m_hViewEntity=view;}return view;}
CSFUDoor*CStickCamera::FindUsableDoor(float &sideSign){
	if(!m_pPlayer)return NULL;
	UTIL_MakeVectors(m_pPlayer->pev->v_angle+m_pPlayer->pev->punchangle);
	const Vector start=m_pPlayer->GetGunPosition();TraceResult tr;
	UTIL_TraceLine(start,start+gpGlobals->v_forward*CAMERA_DISTANCE,dont_ignore_monsters,m_pPlayer->edict(),&tr);
	CBaseEntity*hit=tr.flFraction<1.0f?CBaseEntity::Instance(tr.pHit):NULL;
	if(!hit||!FClassnameIs(hit->pev,"sfu_door"))return NULL;
	CSFUDoor*door=static_cast<CSFUDoor*>(hit);
	if(!door->CanUseUnderDoorCamera())return NULL;
	const float height=door->pev->absmax.z-door->pev->absmin.z;
	if(height<=1.0f||tr.vecEndPos.z>door->pev->absmin.z+height*0.30f)return NULL;
	Vector mount;if(!door->GetCameraMount(mount))return NULL;
	sideSign=DotProduct(m_pPlayer->EyePosition()-mount,door->GetDoorNormal())<0.0f?-1.0f:1.0f;
	return door;
}
void CStickCamera::ToggleCameraView()
{
	if(m_bViewingCamera){LeaveCameraView();return;}
	if(!m_pPlayer)return;
	float sideSign=1.0f;
	CSFUDoor*door=FindUsableDoor(sideSign);
	if(!EnsureViewEntity())return;
	m_hCameraDoor=door;
	m_flDoorSideSign=sideSign;
	m_vecPlayerBodyAngles=m_pPlayer->pev->angles;m_bViewingCamera=TRUE;
	m_bCameraZoom=FALSE;
	m_vecPlayerViewAngles=m_pPlayer->pev->v_angle;
	if(door)
		m_vecCameraStartAngles=UTIL_VecToAngles(door->GetDoorNormal()*-m_flDoorSideSign);
	else
	{
		m_vecCameraStartAngles=m_vecPlayerViewAngles;
		m_vecCameraStartAngles.y=Normalize(m_vecCameraStartAngles.y+(m_bLookRight?-90.0f:90.0f));
	}
	m_pPlayer->pev->fov=m_pPlayer->m_iFOV=0;
	m_pPlayer->pev->v_angle=m_vecCameraStartAngles;
	m_pPlayer->pev->fixangle=TRUE;
	UpdateCameraView();
	if(m_bViewingCamera)SET_VIEW(m_pPlayer->edict(),m_hViewEntity->edict());
}
void CStickCamera::LeaveCameraView(){if(!m_pPlayer)return;if(m_bViewingCamera){m_pPlayer->pev->fov=m_pPlayer->m_iFOV=0;SET_VIEW(m_pPlayer->edict(),m_pPlayer->edict());m_pPlayer->pev->v_angle=m_pPlayer->pev->angles=m_vecPlayerViewAngles;m_pPlayer->pev->fixangle=TRUE;}m_bViewingCamera=FALSE;m_bCameraZoom=FALSE;m_hCameraDoor=NULL;}
void CStickCamera::ToggleCameraSide(){m_bLookRight=!m_bLookRight;if(!m_bViewingCamera){UpdateCameraView();return;}m_vecCameraStartAngles=m_vecPlayerViewAngles;m_vecCameraStartAngles.y=Normalize(m_vecCameraStartAngles.y+(m_bLookRight?-90.0f:90.0f));m_pPlayer->pev->v_angle=m_vecCameraStartAngles;m_hViewEntity->pev->fuser2 += 1.0f;UpdateCameraView();}
void CStickCamera::ToggleCameraZoom(){if(!m_pPlayer||!m_bViewingCamera)return;m_bCameraZoom=!m_bCameraZoom;m_pPlayer->pev->fov=m_pPlayer->m_iFOV=m_bCameraZoom?ZOOM_FOV:0;}
void CStickCamera::UpdateCameraView()
{
	if(!m_pPlayer)return;
	CBaseEntity*view=EnsureViewEntity();
	if(!view)return;
	if(!m_bViewingCamera)
	{
		m_vecPlayerViewAngles=m_pPlayer->pev->v_angle;
		m_vecCameraStartAngles=m_vecPlayerViewAngles;
		m_vecCameraStartAngles.y=Normalize(m_vecCameraStartAngles.y+(m_bLookRight?-90.0f:90.0f));
	}
	CSFUDoor*door=static_cast<CSFUDoor*>((CBaseEntity*)m_hCameraDoor);
	if(door)
	{
		Vector mount;
		if(!door->CanUseUnderDoorCamera()||!door->GetCameraMount(mount)){LeaveCameraView();return;}
		const Vector outward=door->GetDoorNormal()*m_flDoorSideSign;
		// Place the lens beyond the door slab on the side being inspected.
		view->SetAbsOrigin(mount-outward*6.0f+Vector(0,0,2.0f));
	}
	else
	{
		// The pole camera remains available when no usable door was selected.
		UTIL_MakeVectors(m_vecPlayerViewAngles);
		const Vector start=m_pPlayer->GetGunPosition();
		TraceResult tr;
		// Sweep the camera volume against all solids, including custom door meshes.
		// Ignore the owner, but keep the view entity itself non-solid.
		TRACE_MONSTER_HULL(view->edict(),start,start+gpGlobals->v_forward*CAMERA_DISTANCE,
			dont_ignore_monsters,m_pPlayer->edict(),&tr);
		// Engine sweeps can omit SOLID_CUSTOM actors. Test their collision meshes directly.
		const Vector end=start+gpGlobals->v_forward*CAMERA_DISTANCE;
		for(int i=1;i<gpGlobals->maxEntities;++i)
		{
			edict_t*entity=INDEXENT(i);
			if(!entity||entity->free||entity==m_pPlayer->edict()||entity->v.solid!=SOLID_CUSTOM)continue;
			CBaseEntity*obstacle=CBaseEntity::Instance(entity);
			if(!obstacle||!obstacle->m_pUserData)continue;
			trace_t hit={};
			hit.fraction=1.0f;
			hit.endpos=end;
			WorldPhysic->SweepTest(obstacle,start,view->pev->mins,view->pev->maxs,end,&hit);
			if(hit.startsolid||hit.allsolid)
			{
				tr.fStartSolid=TRUE;
				break;
			}
			if(hit.fraction<tr.flFraction)tr.flFraction=hit.fraction;
		}
		Vector cameraOrigin=start;
		if(!tr.fStartSolid&&!tr.fAllSolid)
		{
			const float distance=Q_max(0.0f,CAMERA_DISTANCE*tr.flFraction-(tr.flFraction<1.0f?0.5f:0.0f));
			cameraOrigin=start+gpGlobals->v_forward*distance;
		}
		view->SetAbsOrigin(cameraOrigin);
	}
	Vector angles=m_vecCameraStartAngles;
	if(m_bViewingCamera)
	{
		Vector delta=m_pPlayer->pev->v_angle-m_vecCameraStartAngles;
		delta.x=Normalize(delta.x);
		delta.y=Normalize(delta.y);
		angles.x+=Q_max(-TURN_LIMIT,Q_min(TURN_LIMIT,delta.x));
		angles.y=Normalize(angles.y+Q_max(-TURN_LIMIT,Q_min(TURN_LIMIT,delta.y)));
		// The client limits mouse input immediately. Do not send delayed angle
		// corrections at the stop; they rewind newer input and cause snapping.
	}
	view->SetAbsAngles(angles);
	// endpos is networked with enough range for the fixed pan/tilt reference.
	view->pev->endpos=m_vecCameraStartAngles;
	view->pev->fuser1=door?m_flDoorSideSign:(m_bLookRight?1.0f:0.0f);
	view->pev->playerclass=m_bCameraZoom?1:0;
}

void CStickCamera::PreservePlayerBodyAngles()
{
    if(m_pPlayer && m_bViewingCamera)
        m_pPlayer->SetAbsAngles(m_vecPlayerBodyAngles);
}
