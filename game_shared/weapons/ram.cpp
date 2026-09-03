#include "ram.h"
#include <utility>

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapon_ram.h"
#include "sfu_door.h"
#endif

CRamWeaponContext::CRamWeaponContext( std::unique_ptr<IWeaponLayer> &&layer ) :
	CBaseWeaponContext( std::move( layer ))
{
	m_iId = WEAPON_RAM;
	m_iClip = -1;
	m_usRam = m_pLayer->PrecacheEvent( "events/crowbar.sc" );
}

int CRamWeaponContext::GetItemInfo( ItemInfo *p ) const
{
	p->pszName = CLASSNAME_STR( RAM_CLASSNAME );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 2;
	p->iId = m_iId;
	p->iWeight = RAM_WEIGHT;
	return 1;
}

bool CRamWeaponContext::Deploy()
{
	return DefaultDeploy( "models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar" );
}

void CRamWeaponContext::Holster()
{
	m_pLayer->SetPlayerNextAttackTime( m_pLayer->GetWeaponTimeBase( UsePredicting() ) + 0.5f );
	SendWeaponAnim( CROWBAR_HOLSTER );
}

void CRamWeaponContext::PrimaryAttack()
{
	PlaybackEvent();
	SendWeaponAnim( CROWBAR_ATTACK1HIT );

#ifndef CLIENT_DLL
	CRam *weapon = static_cast<CRam *>( m_pLayer->GetWeaponEntity() );
	CBasePlayer *player = weapon->m_pPlayer;
	UTIL_MakeVectors( player->pev->v_angle + player->pev->punchangle );
	const Vector start = player->GetGunPosition();
	const Vector end = start + gpGlobals->v_forward * 72.0f;
	TraceResult trace;
	UTIL_TraceLine( start, end, dont_ignore_monsters, player->edict(), &trace );
	if( trace.flFraction < 1.0f )
	{
		CBaseEntity *hit = CBaseEntity::Instance( trace.pHit );
		if( hit && FClassnameIs( hit->pev, "sfu_door" ))
			static_cast<CSFUDoor *>( hit )->RamHit( player );
	}
	player->SetAnimation( PLAYER_ATTACK1 );
	player->m_iWeaponVolume = 1024;
#endif

	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay( 1.0f );
}

void CRamWeaponContext::PlaybackEvent()
{
	WeaponEventParams params;
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usRam;
	params.delay = 0.0f;
	params.origin = m_pLayer->GetGunPosition();
	params.angles = m_pLayer->GetViewAngles();
	params.fparam1 = params.fparam2 = 0.0f;
	params.iparam1 = params.iparam2 = 0;
	params.bparam1 = params.bparam2 = 0;
	if( m_pLayer->ShouldRunFuncs() ) m_pLayer->PlaybackWeaponEvent( params );
}
