#include "c2.h"
#include <utility>

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapon_c2.h"
#include "sfu_c2_charge.h"
#include "sfu_door.h"
#include "user_messages.h"
#endif

enum { C2_IDLE = 0, C2_FIDGET, C2_DRAW, C2_PLACE };
enum { C2_RADIO_IDLE = 0, C2_RADIO_FIDGET, C2_RADIO_DRAW, C2_RADIO_FIRE, C2_RADIO_HOLSTER };

#ifndef CLIENT_DLL
static bool SFUPlayerHasPlacedC2( CBasePlayer *player )
{
	CBaseEntity *charge = NULL;
	while(( charge = UTIL_FindEntityByClassname( charge, "sfu_c2_charge" )) != NULL )
		if( charge->pev->owner == player->edict() && !FBitSet( charge->pev->flags, FL_KILLME )) return true;
	return false;
}
#endif

CC2WeaponContext::CC2WeaponContext( std::unique_ptr<IWeaponLayer> &&layer ) : CBaseWeaponContext( std::move( layer ))
{
	m_iId = WEAPON_C2; m_iDefaultAmmo = 3; m_chargeReady = 0; m_flPlaceStart = 0.0f;
}

int CC2WeaponContext::GetItemInfo( ItemInfo *p ) const
{
	p->pszName = CLASSNAME_STR( C2_CLASSNAME ); p->pszAmmo1 = "C2 Charge"; p->iMaxAmmo1 = 3;
	p->pszAmmo2 = NULL; p->iMaxAmmo2 = -1; p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3; p->iPosition = 3; p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId; p->iWeight = -10; return 1;
}

bool CC2WeaponContext::IsUseable() { return m_pLayer->GetPlayerAmmo( PrimaryAmmoIndex()) > 0 || m_chargeReady != 0; }
bool CC2WeaponContext::CanDeploy() { return IsUseable(); }
bool CC2WeaponContext::Deploy()
{
#ifndef CLIENT_DLL
	CC2 *weapon = static_cast<CC2 *>( m_pLayer->GetWeaponEntity() );
	const bool placed = weapon->m_pPlayer && SFUPlayerHasPlacedC2( weapon->m_pPlayer );
	if( placed ) m_chargeReady = 1;
	else if( m_chargeReady == 1 ) m_chargeReady = 0;
#endif
	return m_chargeReady ? DefaultDeploy( "models/v_satchel_radio.mdl", "models/p_satchel_radio.mdl", C2_RADIO_DRAW, "hive" )
		: DefaultDeploy( "models/v_satchel.mdl", "models/p_satchel.mdl", C2_DRAW, "trip" );
}

void CC2WeaponContext::Holster()
{
	#ifndef CLIENT_DLL
	if( m_flPlaceStart > 0.0f )
	{
		CC2 *weapon = static_cast<CC2 *>( m_pLayer->GetWeaponEntity() );
		MESSAGE_BEGIN( MSG_ONE, gmsgActionBar, NULL, weapon->m_pPlayer->pev ); WRITE_BYTE( 0 ); WRITE_SHORT( 0 ); MESSAGE_END();
	}
	#endif
	m_flPlaceStart = 0.0f;
	m_pLayer->SetPlayerNextAttackTime( m_pLayer->GetWeaponTimeBase( UsePredicting()) + 0.5f );
	SendWeaponAnim( m_chargeReady ? C2_RADIO_HOLSTER : C2_PLACE );
}

void CC2WeaponContext::PrimaryAttack()
{
#ifndef CLIENT_DLL
	CC2 *weapon = static_cast<CC2 *>( m_pLayer->GetWeaponEntity() );
	CBasePlayer *player = weapon->m_pPlayer;
	if( m_chargeReady == 0 && SFUPlayerHasPlacedC2( player )) m_chargeReady = 1;
	if( m_chargeReady == 0 && m_pLayer->GetPlayerAmmo( PrimaryAmmoIndex()) > 0 )
	{
		UTIL_MakeVectors( player->pev->v_angle + player->pev->punchangle );
		const Vector start = player->GetGunPosition();
		TraceResult tr;
		UTIL_TraceLine( start, start + gpGlobals->v_forward * 96.0f, dont_ignore_monsters, player->edict(), &tr );
		CBaseEntity *hit = tr.flFraction < 1.0f ? CBaseEntity::Instance( tr.pHit ) : NULL;
		bool validMount = false;
		if( hit && FClassnameIs( hit->pev, "sfu_door" ))
		{
			CSFUDoor *door = static_cast<CSFUDoor *>( hit );
			Vector mount;
			if( door->CanPlaceC2( player ) && door->GetChargeMount( mount ) && ( mount - player->EyePosition()).Length() <= 96.0f )
			{
				validMount = true;
				if( m_flPlaceStart <= 0.0f )
				{
					m_flPlaceStart = gpGlobals->time;
					MESSAGE_BEGIN( MSG_ONE, gmsgActionBar, NULL, player->pev ); WRITE_BYTE( 1 ); WRITE_SHORT( 20 ); MESSAGE_END();
				}
				if( gpGlobals->time - m_flPlaceStart >= 2.0f )
				{
					const float side = DotProduct( player->EyePosition() - mount, door->GetDoorNormal()) < 0.0f ? -1.0f : 1.0f;
					CSFUC2Charge *charge = static_cast<CSFUC2Charge *>( CBaseEntity::Create( "sfu_c2_charge", mount, g_vecZero, player->edict() ));
					if( charge && charge->AttachToDoor( door, player, side ))
					{
						MESSAGE_BEGIN( MSG_ONE, gmsgActionBar, NULL, player->pev ); WRITE_BYTE( 0 ); WRITE_SHORT( 0 ); MESSAGE_END();
						m_pLayer->SetPlayerAmmo( PrimaryAmmoIndex(), m_pLayer->GetPlayerAmmo( PrimaryAmmoIndex()) - 1 );
						m_chargeReady = 1; m_flPlaceStart = 0.0f;
						player->pev->weaponmodel = MAKE_STRING( "models/p_satchel_radio.mdl" );
						m_pLayer->SetPlayerViewmodel( "models/v_satchel_radio.mdl" ); SendWeaponAnim( C2_RADIO_DRAW );
						player->SetAnimation( PLAYER_ATTACK1 );
					}
				}
			}
		}
		if( !validMount && m_flPlaceStart > 0.0f )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgActionBar, NULL, player->pev ); WRITE_BYTE( 0 ); WRITE_SHORT( 0 ); MESSAGE_END();
			m_flPlaceStart = 0.0f;
		}
	}
	else if( m_chargeReady == 1 )
	{
		SendWeaponAnim( C2_RADIO_FIRE );
		CBaseEntity *charge = NULL;
		while(( charge = UTIL_FindEntityByClassname( charge, "sfu_c2_charge" )) != NULL )
			if( charge->pev->owner == player->edict()) charge->Use( player, player, USE_ON, 0.0f );
		m_chargeReady = 2;
	}
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay( m_chargeReady == 0 ? 0.05f : 0.6f );
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase( UsePredicting()) + 0.1f;
}

void CC2WeaponContext::WeaponIdle()
{
	if( m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase( UsePredicting())) return;
#ifndef CLIENT_DLL
	CC2 *weapon = static_cast<CC2 *>( m_pLayer->GetWeaponEntity() );
	if( m_chargeReady == 0 && weapon->m_pPlayer && !FBitSet( weapon->m_pPlayer->pev->button, IN_ATTACK ))
	{
		if( m_flPlaceStart > 0.0f )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgActionBar, NULL, weapon->m_pPlayer->pev ); WRITE_BYTE( 0 ); WRITE_SHORT( 0 ); MESSAGE_END();
		}
		m_flPlaceStart = 0.0f;
	}
#endif
	if( m_chargeReady == 2 )
	{
		m_chargeReady = 0;
#ifndef CLIENT_DLL
		weapon->RetireWeapon();
#endif
		return;
	}
	SendWeaponAnim( m_chargeReady ? C2_RADIO_FIDGET : C2_FIDGET );
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase( UsePredicting()) + 10.0f;
}
