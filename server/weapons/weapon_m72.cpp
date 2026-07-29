#include "weapon_m72.h"
#include "weapon_layer.h"
#include "weapons/m72.h"
#include "server_weapon_layer_impl.h"
#include "gamerules.h"

namespace
{
constexpr int M72_SEQUENCE_COUNT = 4;
constexpr int M72_MAX_SOUND_EVENTS = 8;

struct M72SoundEvent
{
	float delay;
	char sample[64];
};

M72SoundEvent g_M72SoundEvents[M72_SEQUENCE_COUNT][M72_MAX_SOUND_EVENTS];
int g_M72SoundEventCount[M72_SEQUENCE_COUNT] = {};
bool g_M72SoundEventsLoaded = false;

int ReadMdlInt(const byte* data, int offset)
{
	int value;
	memcpy(&value, data + offset, sizeof(value));
	return value;
}

float ReadMdlFloat(const byte* data, int offset)
{
	float value;
	memcpy(&value, data + offset, sizeof(value));
	return value;
}

void LoadM72SoundEvents()
{
	if (g_M72SoundEventsLoaded)
		return;

	g_M72SoundEventsLoaded = true;
	int length = 0;
	byte* data = LOAD_FILE("models/weapon/m72/v_law.mdl", &length);
	if (!data)
		return;

	// GoldSrc MDL v10: header fields and sequence/event records have fixed layouts.
	if (length >= 244 && ReadMdlInt(data, 0) == ('I' | ('D' << 8) | ('S' << 16) | ('T' << 24)) && ReadMdlInt(data, 4) == 10)
	{
		const int numSequences = ReadMdlInt(data, 164);
		const int sequenceIndex = ReadMdlInt(data, 168);
		if (numSequences >= 0 && sequenceIndex >= 0 && sequenceIndex <= length && numSequences <= (length - sequenceIndex) / 176)
		{
			for (int sequence = 0; sequence < numSequences && sequence < M72_SEQUENCE_COUNT; ++sequence)
			{
				const int descriptor = sequenceIndex + sequence * 176;
				const float fps = ReadMdlFloat(data, descriptor + 32);
				const int numEvents = ReadMdlInt(data, descriptor + 48);
				const int eventIndex = ReadMdlInt(data, descriptor + 52);
				if (fps <= 0.0f || numEvents < 0 || eventIndex < 0 || eventIndex > length || numEvents > (length - eventIndex) / 76)
					continue;

				for (int eventNumber = 0; eventNumber < numEvents && g_M72SoundEventCount[sequence] < M72_MAX_SOUND_EVENTS; ++eventNumber)
				{
					const int eventOffset = eventIndex + eventNumber * 76;
					if (ReadMdlInt(data, eventOffset + 4) != 5004)
						continue;

					M72SoundEvent& soundEvent = g_M72SoundEvents[sequence][g_M72SoundEventCount[sequence]++];
					soundEvent.delay = ReadMdlInt(data, eventOffset) / fps;
					memcpy(soundEvent.sample, data + eventOffset + 12, sizeof(soundEvent.sample));
					soundEvent.sample[sizeof(soundEvent.sample) - 1] = '\0';
				}
			}
		}
	}

	FREE_FILE(data);
}

int M72SoundChannel(const char* sample)
{
	if (Q_stristr(sample, "travel"))
		return CHAN_VOICE;
	if (Q_stristr(sample, "fire"))
		return CHAN_WEAPON;
	return CHAN_ITEM;
}
}

LINK_ENTITY_TO_CLASS(weapon_m72, CM72);

class CSpentM72 : public CBaseEntity
{
	DECLARE_CLASS(CSpentM72, CBaseEntity);
public:
	void Spawn() override
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->solid = SOLID_TRIGGER;
		SetTouch(NULL); // visual-only spent tube; it cannot be picked up
		SET_MODEL(edict(), "models/weapon/m72/w_law.mdl");
		UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 8));
	}
};

LINK_ENTITY_TO_CLASS(spent_m72, CSpentM72);

CM72::CM72()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CM72WeaponContext>(std::move(layer));
}

void CM72::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(M72_CLASSNAME));
	Precache();
	SET_MODEL(edict(), "models/weapon/m72/w_law-closed.mdl");
	FallInit();
}

void CM72::Precache()
{
	PRECACHE_MODEL("models/weapon/m72/v_law.mdl");
	PRECACHE_MODEL("models/weapon/m72/p_law.mdl");
	PRECACHE_MODEL("models/weapon/m72/w_law-closed.mdl");
	PRECACHE_MODEL("models/weapon/m72/w_law.mdl");
	PRECACHE_MODEL("models/weapon/m72/lawrocket.mdl");
	PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND("weapons/rocket1.wav");
	PRECACHE_SOUND("weapons/M72/law_fire.wav");
	LoadM72SoundEvents();
	for (int sequence = 0; sequence < M72_SEQUENCE_COUNT; ++sequence)
		for (int eventNumber = 0; eventNumber < g_M72SoundEventCount[sequence]; ++eventNumber)
			PRECACHE_SOUND(g_M72SoundEvents[sequence][eventNumber].sample);
	UTIL_PrecacheOther("m72_rocket");
}

int CM72::AddToPlayer(CBasePlayer* player)
{
	return CBasePlayerWeapon::AddToPlayer(player);
}

void CM72::PlayViewModelSounds(int sequence)
{
	if (!m_pPlayer || sequence < 0 || sequence >= M72_SEQUENCE_COUNT || !g_M72SoundEventCount[sequence])
		return;

	m_iViewModelSoundSequence = sequence;
	m_iNextViewModelSound = 0;
	m_flViewModelSoundStart = gpGlobals->time;
	ViewModelSoundThink();
}

void CM72::ViewModelSoundThink()
{
	if (!m_pPlayer || m_iViewModelSoundSequence < 0 || m_iViewModelSoundSequence >= M72_SEQUENCE_COUNT)
	{
		SetThink(NULL);
		return;
	}

	const int count = g_M72SoundEventCount[m_iViewModelSoundSequence];
	while (m_iNextViewModelSound < count)
	{
		const M72SoundEvent& soundEvent = g_M72SoundEvents[m_iViewModelSoundSequence][m_iNextViewModelSound];
		const float eventTime = m_flViewModelSoundStart + soundEvent.delay;
		if (eventTime > gpGlobals->time)
		{
			SetThink(&CM72::ViewModelSoundThink);
			pev->nextthink = eventTime;
			return;
		}

		EMIT_SOUND(m_pPlayer->edict(), M72SoundChannel(soundEvent.sample), soundEvent.sample, 1.0f, ATTN_NORM);
		++m_iNextViewModelSound;
	}

	SetThink(NULL);
	pev->nextthink = 0;
}

void CM72::RetireSpentLauncher()
{
	if (!m_pPlayer)
		return;

	CBasePlayer* player = m_pPlayer;
	CBaseEntity* spent = CBaseEntity::Create("spent_m72", player->GetAbsOrigin() + Vector(0, 0, 12), Vector(0, player->pev->v_angle.y, 0), player->edict());
	if (spent)
	{
		UTIL_MakeVectors(Vector(0, player->pev->v_angle.y, 0));
		spent->SetAbsVelocity(gpGlobals->v_forward * 80 + Vector(0, 0, 40));
	}

	g_pGameRules->GetNextBestWeapon(player, this);
	player->RemoveWeapon(WEAPON_M72);
	SetThink(&CM72::DestroyItem);
	pev->nextthink = gpGlobals->time + 0.1f;
}
