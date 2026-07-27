#include "weapon_ak47.h"
#include "user_messages.h"
#include "weapons/ak47.h"
#include "server_weapon_layer_impl.h"

namespace
{
constexpr int AK47_SEQUENCE_COUNT = 6;
constexpr int AK47_MAX_SOUND_EVENTS = 8;

struct AK47SoundEvent
{
	float delay;
	char sample[64];
};

AK47SoundEvent g_AK47SoundEvents[AK47_SEQUENCE_COUNT][AK47_MAX_SOUND_EVENTS];
int g_AK47SoundEventCount[AK47_SEQUENCE_COUNT] = {};
bool g_AK47SoundEventsLoaded = false;

int ReadAK47MdlInt(const byte* data, int offset)
{
	int value;
	memcpy(&value, data + offset, sizeof(value));
	return value;
}

float ReadAK47MdlFloat(const byte* data, int offset)
{
	float value;
	memcpy(&value, data + offset, sizeof(value));
	return value;
}

void LoadAK47SoundEvents()
{
	if (g_AK47SoundEventsLoaded)
		return;

	g_AK47SoundEventsLoaded = true;
	int length = 0;
	byte* data = LOAD_FILE("models/weapon/AK-47/v_ak47.mdl", &length);
	if (!data)
		return;

	if (length >= 244 && ReadAK47MdlInt(data, 0) == ('I' | ('D' << 8) | ('S' << 16) | ('T' << 24)) && ReadAK47MdlInt(data, 4) == 10)
	{
		const int numSequences = ReadAK47MdlInt(data, 164);
		const int sequenceIndex = ReadAK47MdlInt(data, 168);
		if (numSequences >= 0 && sequenceIndex >= 0 && sequenceIndex <= length && numSequences <= (length - sequenceIndex) / 176)
		{
			for (int sequence = 0; sequence < numSequences && sequence < AK47_SEQUENCE_COUNT; ++sequence)
			{
				const int descriptor = sequenceIndex + sequence * 176;
				const float fps = ReadAK47MdlFloat(data, descriptor + 32);
				const int numEvents = ReadAK47MdlInt(data, descriptor + 48);
				const int eventIndex = ReadAK47MdlInt(data, descriptor + 52);
				if (fps <= 0.0f || numEvents < 0 || eventIndex < 0 || eventIndex > length || numEvents > (length - eventIndex) / 76)
					continue;

				for (int eventNumber = 0; eventNumber < numEvents && g_AK47SoundEventCount[sequence] < AK47_MAX_SOUND_EVENTS; ++eventNumber)
				{
					const int eventOffset = eventIndex + eventNumber * 76;
					if (ReadAK47MdlInt(data, eventOffset + 4) != 5004)
						continue;

					AK47SoundEvent& soundEvent = g_AK47SoundEvents[sequence][g_AK47SoundEventCount[sequence]++];
					soundEvent.delay = ReadAK47MdlInt(data, eventOffset) / fps;
					memcpy(soundEvent.sample, data + eventOffset + 12, sizeof(soundEvent.sample));
					soundEvent.sample[sizeof(soundEvent.sample) - 1] = '\0';
				}
			}
		}
	}

	FREE_FILE(data);
}
}

LINK_ENTITY_TO_CLASS(weapon_ak47, CAK47);
CAK47::CAK47() { auto layer = std::make_unique<CServerWeaponLayerImpl>(this); m_pWeaponContext = std::make_unique<CAK47WeaponContext>(std::move(layer)); }
void CAK47::Spawn() { pev->classname = MAKE_STRING(CLASSNAME_STR(AK47_CLASSNAME)); Precache(); SET_MODEL(ENT(pev), "models/weapon/AK-47/w_ak47.mdl"); FallInit(); }
void CAK47::Precache()
{
	PRECACHE_MODEL("models/weapon/AK-47/v_ak47.mdl"); PRECACHE_MODEL("models/weapon/AK-47/w_ak47.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl"); PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/AK-47/ak47-1.wav"); PRECACHE_SOUND("weapons/AK-47/ak47_magout.wav");
	PRECACHE_SOUND("weapons/AK-47/ak47_magin.wav"); PRECACHE_SOUND("weapons/AK-47/ak47_boltpull.wav");
	LoadAK47SoundEvents();
	for (int sequence = 0; sequence < AK47_SEQUENCE_COUNT; ++sequence)
		for (int eventNumber = 0; eventNumber < g_AK47SoundEventCount[sequence]; ++eventNumber)
			PRECACHE_SOUND(g_AK47SoundEvents[sequence][eventNumber].sample);
}
int CAK47::AddToPlayer(CBasePlayer *player)
{
	if (!CBasePlayerWeapon::AddToPlayer(player)) return FALSE;
	return TRUE;
}

void CAK47::PlayViewModelSounds(int sequence)
{
	if (!m_pPlayer || sequence < 0 || sequence >= AK47_SEQUENCE_COUNT || !g_AK47SoundEventCount[sequence])
		return;

	m_iViewModelSoundSequence = sequence;
	m_iNextViewModelSound = 0;
	m_flViewModelSoundStart = gpGlobals->time;
	ViewModelSoundThink();
}

void CAK47::CancelViewModelSounds()
{
	m_iViewModelSoundSequence = -1;
	m_iNextViewModelSound = 0;
	m_flViewModelSoundStart = 0.0f;
	SetThink(NULL);
	pev->nextthink = 0;
}

void CAK47::ViewModelSoundThink()
{
	if (!m_pPlayer || m_iViewModelSoundSequence < 0 || m_iViewModelSoundSequence >= AK47_SEQUENCE_COUNT)
	{
		SetThink(NULL);
		return;
	}

	const int count = g_AK47SoundEventCount[m_iViewModelSoundSequence];
	while (m_iNextViewModelSound < count)
	{
		const AK47SoundEvent& soundEvent = g_AK47SoundEvents[m_iViewModelSoundSequence][m_iNextViewModelSound];
		const float eventTime = m_flViewModelSoundStart + soundEvent.delay;
		if (eventTime > gpGlobals->time)
		{
			SetThink(&CAK47::ViewModelSoundThink);
			pev->nextthink = eventTime;
			return;
		}

		EMIT_SOUND(m_pPlayer->edict(), CHAN_ITEM, soundEvent.sample, 1.0f, ATTN_NORM);
		++m_iNextViewModelSound;
	}

	SetThink(NULL);
	pev->nextthink = 0;
}
