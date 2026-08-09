#include "weapon_surveillance_camera.h"
#include "server_weapon_layer_impl.h"
#include "weapons/surveillance_camera.h"
#include "weapons/tripmine.h"
#include "player.h"
#include "user_messages.h"

class CSurveillanceCameraEntity;

static CSurveillanceCamera *FindCameraWeapon(CBasePlayer *player)
{
	if (!player)
		return nullptr;
	for (int slot = 0; slot < MAX_ITEM_TYPES; ++slot)
		for (CBasePlayerItem *item = player->m_rgpPlayerItems[slot]; item; item = item->m_pNext)
			if (item->iWeaponID() == WEAPON_SURVEILLANCE_CAMERA)
				return static_cast<CSurveillanceCamera *>(item);
	return nullptr;
}

class CSurveillanceCameraEntity : public CBaseAnimating
{
	DECLARE_CLASS(CSurveillanceCameraEntity, CBaseAnimating);
public:
	void Spawn() override;
	void Killed(entvars_t *attacker, int gib) override;
	void Use(CBaseEntity *activator, CBaseEntity *caller, USE_TYPE useType, float value) override;
	void FollowSurfaceThink();
	BOOL ShouldCollide(CBaseEntity *other) override;
	int ObjectCaps() override { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE; }
	void AttachToSurface(CBaseEntity *surface);
	void SetVisual(CBaseEntity *visual) { m_visual = visual; m_mountAngles = GetAbsAngles(); }
	const Vector &GetMountAngles() const { return m_mountAngles; }
	void RemoveInstalled(bool dropAsWeapon);
private:
	EHANDLE m_surface;
	EHANDLE m_visual;
	Vector m_localSurfaceOrigin;
	Vector m_localSurfaceAngles;
	Vector m_mountAngles;
};

class CSurveillanceCameraModel : public CBaseAnimating
{
	DECLARE_CLASS(CSurveillanceCameraModel, CBaseAnimating);
public:
	void Spawn() override
	{
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_NOT;
		SET_MODEL(edict(), "models/weapon/Camera/w_camera.mdl");
		pev->rendermode = kRenderNormal;
		pev->renderamt = 255;
		SetBits(pev->effects, EF_NOINTERP);
		ResetSequenceInfo();
		pev->animtime = 0.0f;
	}
};

class CSurveillanceCameraView : public CBaseEntity
{
	DECLARE_CLASS(CSurveillanceCameraView, CBaseEntity);
public:
	void Spawn() override
	{
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_NOT;
		SET_MODEL(edict(), "models/weapon/Camera/w_camera.mdl");
		SetBits(pev->effects, EF_NODRAW | EF_NOINTERP);
	}
};

LINK_ENTITY_TO_CLASS(weapon_camera, CSurveillanceCamera);
LINK_ENTITY_TO_CLASS(surveillance_camera, CSurveillanceCameraEntity);
LINK_ENTITY_TO_CLASS(surveillance_camera_view, CSurveillanceCameraView);
LINK_ENTITY_TO_CLASS(surveillance_camera_model, CSurveillanceCameraModel);

void CSurveillanceCameraEntity::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	// The camera follows moving brush geometry manually.  A solid bbox becomes
	// part of the pusher collision chain: doors then push players with it or
	// refuse to move.  A trigger keeps the use/damage bounds without applying
	// physical blocking forces.
	pev->solid = SOLID_TRIGGER;
	pev->takedamage = DAMAGE_YES;
	pev->health = 10.0f;
	SET_MODEL(edict(), "models/weapon/Camera/w_camera.mdl");
	pev->frame = 0;
	pev->framerate = 0;
	pev->rendermode = kRenderNormal;
	pev->renderamt = 255;
	ResetSequenceInfo();
	UTIL_SetSize(pev, Vector(-8, -8, -8), Vector(8, 8, 8));
	// Keep a second effect bit: AddToFullPack intentionally rejects entities
	// whose effects value is exactly EF_NODRAW, including active view entities.
	SetBits(pev->effects, EF_NODRAW | EF_NOINTERP);
	m_mountAngles = GetAbsAngles();
}

void CSurveillanceCameraEntity::AttachToSurface(CBaseEntity *surface)
{
	if (!surface || surface == g_pWorld)
		return;
	m_surface = surface;
	pev->owner = surface->edict();
	matrix4x4 parent(surface->GetAbsOrigin(), surface->GetAbsAngles(), 1.0f);
	matrix4x4 child(GetAbsOrigin(), GetAbsAngles(), 1.0f);
	matrix4x4 local = parent.Invert().ConcatTransforms(child);
	local.GetOrigin(m_localSurfaceOrigin);
	local.GetAngles(m_localSurfaceAngles);
	SetThink(&CSurveillanceCameraEntity::FollowSurfaceThink);
	SetNextThink(0.01f);
}

void CSurveillanceCameraEntity::FollowSurfaceThink()
{
	CBaseEntity *surface = m_surface;
	const bool brokenBrush = surface && FClassnameIs(surface->pev, "func_breakable") &&
		(surface->pev->health <= 0.0f || surface->pev->solid == SOLID_NOT || FBitSet(surface->pev->effects, EF_NODRAW));
	if (!surface || FBitSet(surface->pev->flags, FL_KILLME) || brokenBrush)
	{
		RemoveInstalled(true);
		return;
	}
	matrix4x4 parent(surface->GetAbsOrigin(), surface->GetAbsAngles(), 1.0f);
	matrix4x4 local(m_localSurfaceOrigin, m_localSurfaceAngles, 1.0f);
	matrix4x4 world = parent.ConcatTransforms(local);
	Vector origin, angles;
	world.GetOrigin(origin);
	world.GetAngles(angles);
	SetAbsOrigin(origin);
	m_mountAngles = angles;
	SetAbsVelocity(surface->GetAbsVelocity());
	pev->animtime = gpGlobals->time;
	RelinkEntity(FALSE);
	if (CBaseEntity *visual = m_visual)
	{
		visual->SetAbsOrigin(origin);
		visual->SetAbsAngles(angles);
		visual->pev->animtime = gpGlobals->time;
		visual->RelinkEntity(FALSE);
	}
	SetNextThink(0.01f);
}

BOOL CSurveillanceCameraEntity::ShouldCollide(CBaseEntity *other)
{
	if (other && m_surface && (other == static_cast<CBaseEntity *>(m_surface) || other->GetRootParent() == static_cast<CBaseEntity *>(m_surface)))
		return FALSE;
	return BaseClass::ShouldCollide(other);
}

void CSurveillanceCameraEntity::RemoveInstalled(bool dropAsWeapon)
{
	CSurveillanceCamera *weapon = FindCameraWeapon(pev->euser1
		? static_cast<CBasePlayer *>(CBaseEntity::Instance(pev->euser1)) : nullptr);
	if (weapon)
		weapon->OnCameraDestroyed(this);
	if (CBaseEntity *visual = m_visual)
		UTIL_Remove(visual);

	if (dropAsWeapon)
	{
		CBasePlayerItem *dropped = dynamic_cast<CBasePlayerItem *>(CBaseEntity::Create("weapon_camera", GetAbsOrigin(), GetAbsAngles(), nullptr));
		if (dropped)
		{
			// A detached camera is the pickup itself, not a weaponbox.  This keeps
			// the authored w_camera model visible while retaining normal weapon
			// pickup behaviour.
			SetBits(dropped->pev->spawnflags, SF_NORESPAWN);
			dropped->pev->movetype = MOVETYPE_BOUNCE;
			dropped->pev->gravity = 1.0f;
			dropped->pev->friction = 0.8f;
			dropped->SetAbsVelocity(GetAbsVelocity() + Vector(0, 0, 80));
			dropped->SetLocalAvelocity(Vector(RANDOM_FLOAT(-80,80), RANDOM_FLOAT(-80,80), RANDOM_FLOAT(-80,80)));
		}
	}

	UTIL_Remove(this);
}

void CSurveillanceCameraEntity::Killed(entvars_t *, int)
{
	UTIL_Sparks(GetAbsOrigin());
	EMIT_SOUND(edict(), CHAN_BODY, "buttons/spark5.wav", 1.0f, ATTN_NORM);
	RemoveInstalled(false);
}

void CSurveillanceCameraEntity::Use(CBaseEntity *activator, CBaseEntity *, USE_TYPE, float)
{
	if (!activator || !activator->IsPlayer() || !activator->IsAlive())
		return;
	CBasePlayer *player = static_cast<CBasePlayer *>(activator);
	CSurveillanceCamera *weapon = FindCameraWeapon(player);
	if (weapon)
	{
		const int ammo = weapon->m_pWeaponContext->PrimaryAmmoIndex();
		if (ammo >= 0 && player->m_rgAmmo[ammo] > 0)
			return; // only one unplaced camera may be carried
	}
	player->GiveNamedItem("weapon_camera");
	weapon = FindCameraWeapon(player);
	if (!weapon)
		return;
	const int ammo = weapon->m_pWeaponContext->PrimaryAmmoIndex();
	if (ammo < 0 || player->m_rgAmmo[ammo] <= 0)
		return;
	EMIT_SOUND(player->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1.0f, ATTN_NORM);
	RemoveInstalled(false);
}

CSurveillanceCamera::CSurveillanceCamera()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CSurveillanceCameraWeaponContext>(std::move(layer));
}

void CSurveillanceCamera::Spawn(){Precache();SET_MODEL(edict(),"models/weapon/Camera/w_camera.mdl");FallInit();}
void CSurveillanceCamera::Precache(){PRECACHE_MODEL("models/weapon/Camera/v_camera.mdl");PRECACHE_MODEL("models/weapon/Camera/v_tablet.mdl");PRECACHE_MODEL("models/weapon/Camera/w_camera.mdl");PRECACHE_MODEL("models/p_satchel_radio.mdl");PRECACHE_SOUND("weapons/mine_deploy.wav");PRECACHE_SOUND("buttons/spark5.wav");PRECACHE_SOUND("items/gunpickup2.wav");UTIL_PrecacheOther("surveillance_camera");UTIL_PrecacheOther("surveillance_camera_model");}

int CSurveillanceCamera::AddToPlayer(CBasePlayer*player){ItemInfo info={};if(GetItemInfo(&info)&&info.iId>0&&info.iId<MAX_WEAPONS)CBaseWeaponContext::ItemInfoArray[info.iId]=info;if(!CBasePlayerWeapon::AddToPlayer(player))return FALSE;MESSAGE_BEGIN(MSG_ONE,gmsgWeaponList,NULL,player->pev);WRITE_STRING(info.pszName);WRITE_BYTE(player->GetAmmoIndex(info.pszAmmo1));WRITE_BYTE(info.iMaxAmmo1);WRITE_BYTE(player->GetAmmoIndex(info.pszAmmo2));WRITE_BYTE(info.iMaxAmmo2);WRITE_BYTE(info.iSlot);WRITE_BYTE(info.iPosition);WRITE_BYTE(info.iId);WRITE_BYTE(info.iFlags);MESSAGE_END();MESSAGE_BEGIN(MSG_ONE,gmsgWeapons,NULL,player->pev);for(int byteIndex=0;byteIndex<MAX_WEAPON_BYTES;++byteIndex){byte owned=0;for(int bit=0;bit<8;++bit){int weaponId=byteIndex*8+bit;if(weaponId<MAX_WEAPONS&&player->HasWeapon(weaponId))owned|=1<<bit;}WRITE_BYTE(owned);}MESSAGE_END();int ammo=m_pWeaponContext->PrimaryAmmoIndex();if(ammo>=0&&player->m_rgAmmo[ammo]<1)player->m_rgAmmo[ammo]=1;return TRUE;}
int CSurveillanceCamera::AddDuplicate(CBasePlayerItem*original){auto*owned=static_cast<CSurveillanceCamera*>(original);int ammo=owned->m_pWeaponContext->PrimaryAmmoIndex();if(ammo>=0&&owned->m_pPlayer->m_rgAmmo[ammo]<1){owned->m_pPlayer->m_rgAmmo[ammo]=1;return TRUE;}return FALSE;}

CSurveillanceCameraEntity*CSurveillanceCamera::FindCamera(int ordinal,int*count)const{int found=0;CBaseEntity*e=nullptr;CSurveillanceCameraEntity*r=nullptr;while((e=UTIL_FindEntityByClassname(e,"surveillance_camera"))!=nullptr)if(!FBitSet(e->pev->flags,FL_KILLME)&&e->pev->euser1==(m_pPlayer?m_pPlayer->edict():nullptr)){++found;if(e->pev->iuser3==ordinal+1)r=static_cast<CSurveillanceCameraEntity*>(e);}if(count)*count=found;return r;}
bool CSurveillanceCamera::HasCameras()const{int count=0;FindCamera(-1,&count);return count>0;}

bool CSurveillanceCamera::PlaceCamera()
{
	int count=0;FindCamera(-1,&count);
	if(count>=3)
	{
		CSurveillanceCameraEntity *oldest=nullptr;
		CBaseEntity *entity=nullptr;
		while((entity=UTIL_FindEntityByClassname(entity,"surveillance_camera"))!=nullptr)
		{
			auto *candidate=static_cast<CSurveillanceCameraEntity *>(entity);
			if(candidate->pev->euser1!=m_pPlayer->edict())continue;
			if(!oldest||candidate->pev->fuser1<oldest->pev->fuser1||
				(candidate->pev->fuser1==oldest->pev->fuser1&&candidate->entindex()<oldest->entindex()))oldest=candidate;
		}
		if(oldest)oldest->RemoveInstalled(false);
	}
	int cameraIndex=0;while(cameraIndex<3&&FindCamera(cameraIndex))++cameraIndex;if(cameraIndex>=3)return false;
	UTIL_MakeVectors(m_pPlayer->pev->v_angle+m_pPlayer->pev->punchangle);TraceResult tr;Vector start=m_pPlayer->GetGunPosition();UTIL_TraceLine(start,start+gpGlobals->v_forward*128.0f,ignore_monsters,m_pPlayer->edict(),&tr);
	if(tr.flFraction>=1.0f)return false;CBaseEntity*surface=tr.pHit?CBaseEntity::Instance(tr.pHit):g_pWorld;if(!surface)surface=g_pWorld;if(!(surface->IsBSPModel()||surface->IsCustomModel())||FBitSet(surface->pev->flags,FL_CONVEYOR))return false;
	Vector angles=UTIL_VecToAngles(tr.vecPlaneNormal);angles.z=0;
	// A tiny outward epsilon keeps the entity linked into the visible side of
	// the wall leaf while remaining visually flush with the authored origin.
	auto*camera=static_cast<CSurveillanceCameraEntity*>(CBaseEntity::Create("surveillance_camera",tr.vecEndPos+tr.vecPlaneNormal,angles,m_pPlayer->edict()));if(!camera)return false;
	auto*visual=CBaseEntity::Create("surveillance_camera_model",camera->GetAbsOrigin(),angles,m_pPlayer->edict());if(!visual){camera->RemoveInstalled(false);return false;}
	camera->SetVisual(visual);camera->pev->euser1=m_pPlayer->edict();camera->pev->euser2=nullptr;camera->pev->iuser1=m_pPlayer->entindex();camera->pev->iuser2=visual->entindex();camera->pev->iuser3=cameraIndex+1;camera->pev->iuser4=0x5346434D;camera->pev->fuser1=gpGlobals->time;camera->pev->vuser2=g_vecZero;visual->pev->euser1=camera->edict();visual->pev->iuser1=m_pPlayer->entindex();visual->pev->iuser2=camera->entindex();visual->pev->iuser3=cameraIndex+1;visual->pev->iuser4=0x5346434D;visual->pev->fuser1=camera->pev->fuser1;visual->pev->colormap=m_pPlayer->entindex();visual->pev->skin=cameraIndex;
	if(surface!=g_pWorld)camera->AttachToSurface(surface);
	EMIT_SOUND(camera->edict(),CHAN_BODY,"weapons/mine_deploy.wav",1.0f,ATTN_NORM);m_iSelectedCamera=cameraIndex;m_bViewingCamera=FALSE;return true;
}

void CSurveillanceCamera::ToggleCameraView(){if(m_bViewingCamera){LeaveCameraView();return;}auto*camera=FindCamera(m_iSelectedCamera);if(!camera){for(int i=0;i<3&&!camera;++i)if((camera=FindCamera(i))!=nullptr)m_iSelectedCamera=i;}if(!camera)return;m_bViewingCamera=TRUE;m_bCameraZoom=camera->pev->fuser3>0.5f;m_pPlayer->pev->fov=m_pPlayer->m_iFOV=m_bCameraZoom?22:0;Vector base=camera->GetMountAngles();Vector viewAngles=base+camera->pev->vuser2;UTIL_MakeVectors(base);camera->pev->view_ofs=gpGlobals->v_forward*14.0f;camera->SetAbsAngles(viewAngles);m_vecViewStartAngles=base;m_vecLastMountAngles=base;m_pPlayer->pev->v_angle=viewAngles;m_pPlayer->pev->angles=viewAngles;m_pPlayer->pev->fixangle=TRUE;SET_VIEW(m_pPlayer->edict(),camera->edict());m_flNextCameraLabel=0;}
void CSurveillanceCamera::LeaveCameraView(){if(!m_pPlayer||!m_bViewingCamera)return;m_bViewingCamera=FALSE;m_pPlayer->pev->fov=m_pPlayer->m_iFOV=0;SET_VIEW(m_pPlayer->edict(),m_pPlayer->edict());ClientPrint(m_pPlayer->pev,HUD_PRINTCENTER,"");}
void CSurveillanceCamera::ToggleCameraZoom(){if(!m_pPlayer||!m_bViewingCamera)return;auto*camera=FindCamera(m_iSelectedCamera);if(!camera)return;m_bCameraZoom=!m_bCameraZoom;camera->pev->fuser3=m_bCameraZoom?1.0f:0.0f;if(camera->pev->iuser2>0){CBaseEntity*visual=CBaseEntity::Instance(INDEXENT(camera->pev->iuser2));if(visual){visual->pev->fuser3=camera->pev->fuser3;visual->pev->playerclass=m_bCameraZoom?1:0;}}m_pPlayer->pev->fov=m_pPlayer->m_iFOV=m_bCameraZoom?22:0;}
void CSurveillanceCamera::SelectNextCamera(){int next=-1;for(int offset=1;offset<=3;++offset){int candidate=(m_iSelectedCamera+offset)%3;if(FindCamera(candidate)){next=candidate;break;}}if(next<0)return;m_iSelectedCamera=next;UpdateViewModels();if(m_bViewingCamera){m_bViewingCamera=FALSE;ToggleCameraView();}}

void CSurveillanceCamera::UpdateCameraView()
{
	if(!m_bViewingCamera||!m_pPlayer)return;
	auto*camera=FindCamera(m_iSelectedCamera);if(!camera){LeaveCameraView();return;}
	if(gpGlobals->time>=m_flNextCameraLabel){ClientPrint(m_pPlayer->pev,HUD_PRINTCENTER,UTIL_VarArgs("Camera %02d",m_iSelectedCamera+1));m_flNextCameraLabel=gpGlobals->time+1.0f;}
	Vector mountAngles=camera->GetMountAngles();
	Vector mountDelta=mountAngles-m_vecLastMountAngles;for(int i=0;i<3;++i){while(mountDelta[i]>180)mountDelta[i]-=360;while(mountDelta[i]<-180)mountDelta[i]+=360;}
	bool mountMoved=mountDelta.Length()>0.01f;
	if(mountMoved){m_vecViewStartAngles+=mountDelta;m_pPlayer->pev->v_angle+=mountDelta;}
	Vector delta=m_pPlayer->pev->v_angle-m_vecViewStartAngles;
	bool clamped=false;
	for(int i=0;i<2;++i){while(delta[i]>180)delta[i]-=360;while(delta[i]<-180)delta[i]+=360;float limited=Q_max(-45.0f,Q_min(45.0f,delta[i]));clamped|=limited!=delta[i];delta[i]=limited;camera->pev->vuser2[i]=limited;}
	Vector viewAngles=m_vecViewStartAngles+Vector(delta.x,delta.y,0);
	UTIL_MakeVectors(mountAngles);
	camera->pev->view_ofs=gpGlobals->v_forward*14.0f;
	camera->SetAbsAngles(viewAngles);
	if(clamped||mountMoved){m_pPlayer->pev->v_angle=viewAngles;m_pPlayer->pev->angles=viewAngles;m_pPlayer->pev->fixangle=TRUE;}
	m_vecLastMountAngles=mountAngles;
}

void CSurveillanceCamera::UpdateViewModels(){if(!m_pPlayer)return;m_pPlayer->pev->viewmodel=MAKE_STRING("models/weapon/Camera/v_tablet.mdl");CBaseEntity*entity=nullptr;while((entity=UTIL_FindEntityByClassname(entity,"surveillance_camera"))!=nullptr)if(entity->pev->euser1==m_pPlayer->edict()){const float selected=(entity->pev->iuser3==m_iSelectedCamera+1)?1.0f:0.0f;entity->pev->fuser2=selected;if(entity->pev->iuser2>0){CBaseEntity*visual=CBaseEntity::Instance(INDEXENT(entity->pev->iuser2));if(visual)visual->pev->fuser2=selected;}}UpdateCameraIndicators(m_pPlayer->m_pActiveItem==this);}
void CSurveillanceCamera::UpdateCameraIndicators(bool active){CBaseEntity*entity=nullptr;while((entity=UTIL_FindEntityByClassname(entity,"surveillance_camera"))!=nullptr)if(entity->pev->euser1==(m_pPlayer?m_pPlayer->edict():nullptr)){const float lit=active&&entity->pev->iuser3==m_iSelectedCamera+1?1.0f:0.0f;entity->pev->fuser4=lit;if(entity->pev->iuser2>0){CBaseEntity*visual=CBaseEntity::Instance(INDEXENT(entity->pev->iuser2));if(visual){visual->pev->fuser2=lit;visual->pev->fuser4=lit;}}}}
void CSurveillanceCamera::OnCameraDestroyed(CSurveillanceCameraEntity*camera){const int removedIndex=camera->pev->iuser3-1;if(m_bViewingCamera&&m_iSelectedCamera==removedIndex)LeaveCameraView();if(m_iSelectedCamera==removedIndex){m_iSelectedCamera=0;for(int offset=1;offset<=3;++offset){int candidate=(removedIndex+offset)%3;CSurveillanceCameraEntity*remaining=FindCamera(candidate);if(remaining&&remaining!=camera){m_iSelectedCamera=candidate;break;}}}UpdateViewModels();}
