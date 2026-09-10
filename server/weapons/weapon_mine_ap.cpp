#include "weapon_mine_ap.h"
#include "server_weapon_layer_impl.h"
#include "weapons/mine_ap.h"
#include "player.h"
#include "gamerules.h"
#include "user_messages.h"

class CPlantedMineAP : public CBaseEntity
{
 DECLARE_CLASS(CPlantedMineAP, CBaseEntity);
public:
 EHANDLE m_planter;
 EHANDLE m_defuser;
 float m_defuseEnd = 0;
 int ObjectCaps() override { return CBaseEntity::ObjectCaps() | FCAP_CONTINUOUS_USE; }
 bool CanDefuse(CBasePlayer* player)
 {
  if(!player || !player->IsAlive() || !(player->pev->button & IN_USE) ||
     (player->GetAbsOrigin()-GetAbsOrigin()).Length() > 64.0f) return false;
  const Vector target = GetAbsOrigin() + Vector(0,0,4);
  const Vector direction = target-player->EyePosition();
  UTIL_MakeVectors(player->pev->v_angle + player->pev->punchangle);
  if(DotProduct(direction.Normalize(), gpGlobals->v_forward) < 0.95f) return false;
  TraceResult tr;
  UTIL_TraceLine(player->EyePosition(), target, dont_ignore_monsters, player->edict(), &tr);
  return !tr.fStartSolid && (tr.flFraction == 1.0f || tr.pHit == edict());
 }
 void CancelDefuse()
 {
  CBaseEntity* player = m_defuser;
  if(player)
  {
   MESSAGE_BEGIN(MSG_ONE, gmsgActionBar, NULL, player->pev);
   WRITE_BYTE(0); WRITE_SHORT(0); MESSAGE_END();
  }
  m_defuser = NULL; m_defuseEnd = 0;
  SetThink(NULL); pev->nextthink = 0;
 }
 void Use(CBaseEntity* activator, CBaseEntity*, USE_TYPE, float) override
 {
  if(!activator || !activator->IsPlayer() || FBitSet(pev->flags, FL_KILLME)) return;
  CBasePlayer* player = static_cast<CBasePlayer*>(activator);
  if(!CanDefuse(player)) return;
  if(m_defuseEnd > 0) return; // A second player cannot take over partial progress.
  m_defuser = player; m_defuseEnd = gpGlobals->time + 5.0f;
  MESSAGE_BEGIN(MSG_ONE, gmsgActionBar, NULL, player->pev);
  WRITE_BYTE(2); WRITE_SHORT(50); MESSAGE_END();
  SetThink(&CPlantedMineAP::DefuseThink); pev->nextthink = gpGlobals->time + 0.05f;
 }
 void DefuseThink()
 {
  CBasePlayer* player = static_cast<CBasePlayer*>((CBaseEntity*)m_defuser);
  if(!CanDefuse(player)) { CancelDefuse(); return; }
  if(gpGlobals->time >= m_defuseEnd)
  {
   CancelDefuse(); SetTouch(NULL); pev->solid = SOLID_NOT;
   UTIL_Remove(this); return;
  }
  pev->nextthink = Q_min(m_defuseEnd, gpGlobals->time + 0.05f);
 }
 void Spawn() override
 {
  SET_MODEL(edict(), "models/weapon/MineAP/w_landmine.mdl");
  pev->movetype = MOVETYPE_NONE; pev->solid = SOLID_TRIGGER;
  UTIL_SetSize(pev, Vector(-8,-8,0), Vector(8,8,6));
  SetTouch(&CPlantedMineAP::MineTouch);
 }
 void MineTouch(CBaseEntity* other)
 {
  if(!other->IsPlayer() || !other->IsAlive()) return;
  CancelDefuse();
  SetTouch(NULL); pev->solid = SOLID_NOT;
  MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, GetAbsOrigin());
  WRITE_BYTE(TE_EXPLOSION); WRITE_COORD(GetAbsOrigin().x); WRITE_COORD(GetAbsOrigin().y); WRITE_COORD(GetAbsOrigin().z+4);
  WRITE_SHORT(g_sModelIndexFireball); WRITE_BYTE(15); WRITE_BYTE(15); WRITE_BYTE(0); MESSAGE_END();
  CBaseEntity* attacker = m_planter;
  // Contact victim takes exactly 105 health damage, without distance falloff or armor absorption.
  other->TakeDamage(pev, attacker ? attacker->pev : pev, 105.0f, DMG_BLAST | DMG_GAS_IGNORE_ARMOR);
  UTIL_Remove(this);
 }
};
LINK_ENTITY_TO_CLASS(planted_mineAP, CPlantedMineAP);
LINK_ENTITY_TO_CLASS(weapon_mineAP, CMineAP);

CMineAP::CMineAP()
{ m_pWeaponContext = std::make_unique<CMineAPWeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this)); }
void CMineAP::Spawn() { Precache(); SET_MODEL(edict(), "models/weapon/MineAP/w_landmine_drop.mdl"); FallInit(); }
void CMineAP::Precache()
{ PRECACHE_MODEL("models/weapon/MineAP/v_landmine.mdl"); PRECACHE_MODEL("models/weapon/MineAP/w_landmine.mdl"); PRECACHE_MODEL("models/weapon/MineAP/w_landmine_drop.mdl"); }
bool CMineAP::FindGround(TraceResult& tr)
{
 if(!m_pPlayer || !m_pPlayer->IsAlive()) return false;
 UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
 const Vector start = m_pPlayer->GetGunPosition();
 UTIL_TraceLine(start, start + gpGlobals->v_forward * 96, dont_ignore_monsters, m_pPlayer->edict(), &tr);
 return !tr.fStartSolid && !tr.fAllSolid && tr.flFraction < 1 && tr.vecPlaneNormal.z >= 0.85f && tr.pHit == INDEXENT(0);
}
void CMineAP::StartPlant()
{
 if(m_plantEnd > 0 || !m_pPlayer || m_pPlayer->m_rgAmmo[m_pWeaponContext->PrimaryAmmoIndex()] <= 0) return;
 TraceResult tr; if(!FindGround(tr)) return;
 m_plantPoint = tr.vecEndPos; m_plantNormal = tr.vecPlaneNormal;
 m_plantEnd = gpGlobals->time + MINE_AP_PLANT_TIME;
 m_pPlayer->pev->weaponanim = 2;
 MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev); WRITE_BYTE(2); WRITE_BYTE(0); MESSAGE_END();
 MESSAGE_BEGIN(MSG_ONE, gmsgActionBar, NULL, m_pPlayer->pev); WRITE_BYTE(5); WRITE_SHORT(14); MESSAGE_END();
 SetThink(&CMineAP::PlantThink); pev->nextthink = gpGlobals->time + 0.01f;
}
void CMineAP::CancelPlant()
{
 if(m_plantEnd <= 0) return;
 m_plantEnd = 0; SetThink(NULL); pev->nextthink = 0;
 if(m_pPlayer)
 {
  MESSAGE_BEGIN(MSG_ONE, gmsgActionBar, NULL, m_pPlayer->pev); WRITE_BYTE(0); WRITE_SHORT(0); MESSAGE_END();
  m_pPlayer->pev->weaponanim = 0;
  MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev); WRITE_BYTE(0); WRITE_BYTE(0); MESSAGE_END();
 }
}
void CMineAP::PlantThink()
{
 TraceResult tr;
 if(!m_pPlayer || m_pPlayer->m_pActiveItem != this || !(m_pPlayer->pev->button & IN_ATTACK) ||
    !FindGround(tr) || (tr.vecEndPos-m_plantPoint).Length() > 8) { CancelPlant(); return; }
 if(gpGlobals->time < m_plantEnd) { pev->nextthink = Q_min(m_plantEnd, gpGlobals->time + 0.01f); return; }
 // Model bounds are z=0..3.49: put its middle plane on the ground.
 Vector angles(acosf(bound(-1.0f, m_plantNormal.z, 1.0f)) * 180.0f / 3.14159265f,
 atan2f(m_plantNormal.y, m_plantNormal.x) * 180.0f / 3.14159265f, 0);
 auto* mine = static_cast<CPlantedMineAP*>(CBaseEntity::Create("planted_mineAP", m_plantPoint-m_plantNormal*1.745f, angles, NULL));
 if(!mine) { CancelPlant(); return; }
 mine->m_planter = m_pPlayer;
 CancelPlant();
 m_pPlayer->m_rgAmmo[m_pWeaponContext->PrimaryAmmoIndex()] = 0;
 g_pGameRules->GetNextBestWeapon(m_pPlayer, this);
 m_pPlayer->RemoveWeapon(WEAPON_MINE_AP);
 SetThink(&CMineAP::DestroyItem); pev->nextthink = gpGlobals->time + 0.1f;
}
