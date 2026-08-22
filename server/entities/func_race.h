#pragma once
#include "triggers.h"

class CFuncRace : public CBaseTrigger
{
	DECLARE_CLASS(CFuncRace, CBaseTrigger);
public:
	enum Type { START_FINISH = 0, START_ONLY = 1, FINISH_ONLY = 2, CHECKPOINT = 3 };
	void Spawn() override;
	void KeyValue(KeyValueData *data) override;
	void RaceTouch(CBaseEntity *other);
	int GetRaceType() const { return m_type; }
	int GetCheckpointNumber() const { return m_number; }
	DECLARE_DATADESC();
private:
	int m_type = START_FINISH;
	int m_number = 0;
};
