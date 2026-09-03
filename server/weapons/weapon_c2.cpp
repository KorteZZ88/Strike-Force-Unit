#include "weapon_c2.h"
#include "weapon_layer.h"
#include "weapons/c2.h"
#include "server_weapon_layer_impl.h"
#include <utility>

LINK_ENTITY_TO_CLASS( weapon_c2, CC2 );

#define DEFINE_C2_FIELD( x, ft ) DEFINE_CUSTOM_FIELD( x, ft, [](CBaseEntity *e, void *d, size_t s) { auto *w = static_cast<CBasePlayerWeapon *>(e); auto *c = w->m_pWeaponContext->As<CC2WeaponContext>(); std::memcpy(d, &c->x, s); }, [](CBaseEntity *e, const void *d, size_t s) { auto *w = static_cast<CBasePlayerWeapon *>(e); auto *c = w->m_pWeaponContext->As<CC2WeaponContext>(); std::memcpy(&c->x, d, s); })
BEGIN_DATADESC( CC2 )
	DEFINE_C2_FIELD( m_chargeReady, FIELD_INTEGER ),
	DEFINE_C2_FIELD( m_flPlaceStart, FIELD_TIME ),
END_DATADESC()

CC2::CC2()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>( this );
	m_pWeaponContext = std::make_unique<CC2WeaponContext>( std::move( layer ));
}
void CC2::Spawn() { Precache(); SET_MODEL( edict(), "models/w_satchel.mdl" ); FallInit(); }
void CC2::Precache()
{
	PRECACHE_MODEL( "models/v_satchel.mdl" ); PRECACHE_MODEL( "models/v_satchel_radio.mdl" );
	PRECACHE_MODEL( "models/w_satchel.mdl" ); PRECACHE_MODEL( "models/p_satchel.mdl" ); PRECACHE_MODEL( "models/p_satchel_radio.mdl" );
	UTIL_PrecacheOther( "sfu_c2_charge" );
}
