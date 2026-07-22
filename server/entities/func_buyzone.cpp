#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "triggers.h"
#include "gamerules/gamerules.h"
#include "gamerules/bomb_gamerules.h"

class CFuncBuyZone : public CBaseTrigger
{
	DECLARE_CLASS(CFuncBuyZone,CBaseTrigger);
public:
	void Spawn() override { if(m_team>=0)pev->team=m_team;InitTrigger(); SetTouch(&CFuncBuyZone::BuyTouch); }
	void KeyValue(KeyValueData *pkvd) override
	{
		if(!Q_stricmp(pkvd->szKeyName,"team")){m_team=bound(0,atoi(pkvd->szValue),2);pev->team=m_team;pkvd->fHandled=TRUE;}
		else CBaseTrigger::KeyValue(pkvd);
	}
	void BuyTouch(CBaseEntity *other)
	{
		if(!other||!other->IsPlayer()||!g_pGameRules||!g_pGameRules->IsBombMode())return;
		static_cast<CBombGameRules*>(g_pGameRules)->TouchBuyZone(static_cast<CBasePlayer*>(other),m_team);
	}
private:
	int m_team=-1;
};
LINK_ENTITY_TO_CLASS(func_buyzone,CFuncBuyZone);
