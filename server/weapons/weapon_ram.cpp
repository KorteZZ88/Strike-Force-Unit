#include "weapon_ram.h"
#include "weapon_layer.h"
#include "weapons/ram.h"
#include "server_weapon_layer_impl.h"
#include <utility>

LINK_ENTITY_TO_CLASS( weapon_ram, CRam );

CRam::CRam()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>( this );
	m_pWeaponContext = std::make_unique<CRamWeaponContext>( std::move( layer ));
}

void CRam::Spawn()
{
	pev->classname = MAKE_STRING( CLASSNAME_STR( RAM_CLASSNAME ));
	Precache();
	SET_MODEL( ENT( pev ), "models/w_crowbar.mdl" );
	FallInit();
}

void CRam::Precache()
{
	PRECACHE_MODEL( "models/v_crowbar.mdl" );
	PRECACHE_MODEL( "models/w_crowbar.mdl" );
	PRECACHE_MODEL( "models/p_crowbar.mdl" );
}
