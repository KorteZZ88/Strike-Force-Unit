#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gamerules/gamerules.h"
#include "func_race.h"

LINK_ENTITY_TO_CLASS(func_race, CFuncRace);

BEGIN_DATADESC(CFuncRace)
	DEFINE_FIELD(m_type, FIELD_INTEGER),
	DEFINE_FIELD(m_number, FIELD_INTEGER),
	DEFINE_FUNCTION(RaceTouch),
END_DATADESC()

void CFuncRace::Spawn()
{
	InitTrigger();
	SetTouch(&CFuncRace::RaceTouch);
}

void CFuncRace::KeyValue(KeyValueData *data)
{
	if (FStrEq(data->szKeyName, "type")) { m_type = atoi(data->szValue); data->fHandled = TRUE; }
	else if (FStrEq(data->szKeyName, "number")) { m_number = atoi(data->szValue); data->fHandled = TRUE; }
	else BaseClass::KeyValue(data);
}

void CFuncRace::RaceTouch(CBaseEntity *other)
{
	if (g_pGameRules && g_pGameRules->IsRaceMode())
		g_pGameRules->RaceTriggerTouched(this, other);
}
