#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "viewmodel_sounds.h"
#include "user_messages.h"

#include <string>
#include <vector>

namespace
{
struct SoundEvent { float delay; std::string sample; };
struct ModelSounds { std::string model; std::vector<std::vector<SoundEvent>> sequences; };
std::vector<ModelSounds> g_models;
struct ScheduledSound
{
	EHANDLE player;
	EHANDLE weapon;
	int model = -1;
	int sequence = -1;
	int nextEvent = 0;
	float started = 0;
};
ScheduledSound g_scheduled[33];

int MdlInt(const byte* p, int o) { int v; memcpy(&v, p + o, sizeof(v)); return v; }
float MdlFloat(const byte* p, int o) { float v; memcpy(&v, p + o, sizeof(v)); return v; }

int FindModel(const char* name)
{
	for (size_t i = 0; i < g_models.size(); ++i)
		if (!Q_stricmp(g_models[i].model.c_str(), name)) return (int)i;
	return -1;
}

const char* NormalizeModelSound(const char* modelName, const char* sample)
{
	if (!Q_stricmp(modelName, "models/weapon/FiveSeven/v_fiveseven.mdl"))
	{
		if (!Q_stricmp(sample, "weapons/five7_clipout.wav")) return "weapons/FiveSeven/57_clipout.wav";
		if (!Q_stricmp(sample, "weapons/five7_clipin.wav")) return "weapons/FiveSeven/57_clipin.wav";
		if (!Q_stricmp(sample, "weapons/five7_release.wav")) return "weapons/FiveSeven/57_release.wav";
		if (!Q_stricmp(sample, "weapons/five7_deploy.wav")) return "weapons/FiveSeven/57_deploy.wav";
	}
	else if (!Q_stricmp(modelName, "models/weapon/UMP/v_ump45.mdl"))
	{
		if (!Q_stricmp(sample, "weapons/ump45_clipout.wav")) return "weapons/UMP/ump45-clipout.wav";
		if (!Q_stricmp(sample, "weapons/ump45_clipin.wav")) return "weapons/UMP/ump45-clipin.wav";
		if (!Q_stricmp(sample, "weapons/ump45_boltslap.wav")) return "weapons/UMP/ump45-boltslap.wav";
	}
	else if (!Q_stricmp(modelName, "models/weapon/SG552/v_sg552.mdl"))
	{
		// The supplied model references a bolt-up sample that is not present in
		// its sound pack; reuse its bolt-pull so the networked event stays valid.
		if (!Q_stricmp(sample, "weapons/SG552/sg552_boltup.wav")) return "weapons/SG552/sg552_boltpull.wav";
	}
	else if (!Q_stricmp(modelName, "models/weapon/AUG/v_aug_mirror.mdl"))
	{
		if (!Q_strnicmp(sample, "weapons/", 8))
			return UTIL_VarArgs("weapons/AUG/%s", sample + 8);
	}
	return sample;
}

}

void PrecacheViewModelSounds(const char* modelName)
{
	if (!modelName) return;
	const int existing = FindModel(modelName);
	if (existing >= 0)
	{
		for (const auto& sequence : g_models[existing].sequences)
			for (const auto& event : sequence)
				PRECACHE_SOUND(event.sample.c_str());
		return;
	}
	int length = 0;
	byte* data = LOAD_FILE((char*)modelName, &length);
	if (!data) return;

	ModelSounds parsed;
	parsed.model = modelName;
	if (length >= 244 && MdlInt(data, 0) == ('I' | ('D' << 8) | ('S' << 16) | ('T' << 24)) && MdlInt(data, 4) == 10)
	{
		const int count = MdlInt(data, 164), base = MdlInt(data, 168);
		if (count >= 0 && base >= 0 && base <= length && count <= (length - base) / 176)
		{
			parsed.sequences.resize(count);
			for (int s = 0; s < count; ++s)
			{
				const int d = base + s * 176, eventCount = MdlInt(data, d + 48), eventBase = MdlInt(data, d + 52);
				const float fps = MdlFloat(data, d + 32);
				if (fps <= 0 || eventCount < 0 || eventBase < 0 || eventBase > length || eventCount > (length - eventBase) / 76) continue;
				for (int e = 0; e < eventCount; ++e)
				{
					const int o = eventBase + e * 76;
					if (MdlInt(data, o + 4) != 5004) continue;
					char sample[65]; memcpy(sample, data + o + 12, 64); sample[64] = 0;
					if (!sample[0]) continue;
					const char* normalizedSample = NormalizeModelSound(modelName, sample);
					parsed.sequences[s].push_back({ MdlInt(data, o) / fps, normalizedSample });
					PRECACHE_SOUND(normalizedSample);
				}
			}
		}
	}
	FREE_FILE(data);
	g_models.push_back(std::move(parsed));
}

void PlayViewModelSounds(CBasePlayerWeapon* weapon, int sequence)
{
	if (!weapon || !weapon->m_pPlayer) return;
	CBasePlayer* player = weapon->m_pPlayer;
	const char* modelName = STRING(player->pev->viewmodel);
	const int model = FindModel(modelName);
	const int playerIndex = ENTINDEX(player->edict());
	g_scheduled[playerIndex] = ScheduledSound();
	if (model < 0 || sequence < 0 || sequence >= (int)g_models[model].sequences.size() || g_models[model].sequences[sequence].empty()) return;

	ScheduledSound& scheduled = g_scheduled[playerIndex];
	scheduled.player = player;
	scheduled.weapon = weapon;
	scheduled.model = model;
	scheduled.sequence = sequence;
	scheduled.nextEvent = 0;
	scheduled.started = gpGlobals->time;
}

void CancelViewModelSounds(CBasePlayerWeapon* weapon)
{
	if (weapon && weapon->m_pPlayer)
	{
		const int index = ENTINDEX(weapon->m_pPlayer->edict());
		g_scheduled[index] = ScheduledSound();
	}
}

void UpdateViewModelSounds()
{
	for (int index = 1; index <= gpGlobals->maxClients && index < 33; ++index)
	{
		ScheduledSound& scheduled = g_scheduled[index];
		if (scheduled.model < 0) continue;
		CBasePlayer* player = static_cast<CBasePlayer*>(scheduled.player.GetPointer());
		CBasePlayerWeapon* weapon = static_cast<CBasePlayerWeapon*>(scheduled.weapon.GetPointer());
		if (!player || !weapon || player->m_pActiveItem != weapon || scheduled.model >= (int)g_models.size() ||
			scheduled.sequence < 0 || scheduled.sequence >= (int)g_models[scheduled.model].sequences.size())
		{
			scheduled = ScheduledSound();
			continue;
		}

		const auto& events = g_models[scheduled.model].sequences[scheduled.sequence];
		while (scheduled.nextEvent < (int)events.size() && scheduled.started + events[scheduled.nextEvent].delay <= gpGlobals->time)
		{
			MESSAGE_BEGIN(MSG_ALL, gmsgVModelSound);
				WRITE_BYTE(index);
				WRITE_COORD(player->pev->origin.x);
				WRITE_COORD(player->pev->origin.y);
				WRITE_COORD(player->pev->origin.z);
				WRITE_STRING(events[scheduled.nextEvent].sample.c_str());
			MESSAGE_END();
			++scheduled.nextEvent;
		}
		if (scheduled.nextEvent >= (int)events.size()) scheduled = ScheduledSound();
	}
}
