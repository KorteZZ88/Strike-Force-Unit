/*
game_event_manager.cpp - class that responsible for registering game events handlers
Copyright (C) 2024 SNMetamorph

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "game_event_manager.h"
#include "hud.h"
#include "utils.h"
#include "event_api.h"
#include "event_args.h"
#include "glock_fire_event.h"
#include "beretta_fire_event.h"
#include "p229_fire_event.h"
#include "fiveseven_fire_event.h"
#include "usp_fire_event.h"
#include "colt1911_fire_event.h"
#include "crossbow_fire_event.h"
#include "python_fire_event.h"
#include "ragingbull_fire_event.h"
#include "deagle_fire_event.h"
#include "mp5_fire_event.h"
#include "mp5a3_fire_event.h"
#include "shotgun_fire_event.h"
#include "m3_fire_event.h"
#include "crowbar_swing_event.h"
#include "tripmine_deploy_event.h"
#include "snark_throw_event.h"
#include "hornetgun_fire_event.h"
#include "rpg_fire_event.h"
#include "egon_stop_event.h"
#include "egon_fire_event.h"
#include "gauss_fire_event.h"
#include "gauss_spin_event.h"
#include "m24_fire_event.h"
#include "m4_fire_event.h"
#include "ak47_fire_event.h"
#include "m60_fire_event.h"

CGameEventManager::CGameEventManager()
{
	RegisterGlockEvents();
	gEngfuncs.pfnHookEvent("events/beretta.sc", [](event_args_s *args) { CBerettaFireEvent event(args); event.Execute(); });
	gEngfuncs.pfnHookEvent("events/p229.sc", [](event_args_s *args) { CP229FireEvent event(args); event.Execute(); });
	gEngfuncs.pfnHookEvent("events/fiveseven.sc", [](event_args_s *args) { CFiveSevenFireEvent event(args); event.Execute(); });
	RegisterUSPEvents();
	RegisterCrossbowEvents();
	RegisterRBullEvents();
	RegisterSFUWeaponEvents();
	RegisterDeagleEvents();
	RegisterMP5Events();
	RegisterShotgunEvents();
	RegisterCrowbarEvents();
	RegisterTripmineEvents();
	RegisterSnarkEvents();
	RegisterHornetgunEvents();
	RegisterRPGEvents();
	RegisterEgonEvents();
	RegisterGaussEvents();
	RegisterM24Events();
	RegisterM4Events();
	RegisterAK47Events();
	gEngfuncs.pfnHookEvent("events/m60.sc", [](event_args_s *args) { CM60FireEvent event(args); event.Execute(); });
}

void CGameEventManager::RegisterAK47Events()
{
	gEngfuncs.pfnHookEvent("events/ak47.sc", [](event_args_s *args) { CAK47FireEvent event(args); event.Execute(); });
}

void CGameEventManager::RegisterUSPEvents()
{
	gEngfuncs.pfnHookEvent("events/usp.sc", [](event_args_s *args) { CUSPFireEvent event(args); event.Execute(); });
	gEngfuncs.pfnHookEvent("events/1911.sc", [](event_args_s *args) { CColt1911FireEvent event(args); event.Execute(); });
}

void CGameEventManager::RegisterM4Events()
{
	gEngfuncs.pfnHookEvent("events/m4.sc", [](event_args_s *args) {
		CM4FireEvent event(args);
		event.Execute(false);
	});
	gEngfuncs.pfnHookEvent("events/m42.sc", [](event_args_s *args) {
		CM4FireEvent event(args);
		event.Execute(true);
	});
}

void CGameEventManager::RegisterM24Events()
{
	gEngfuncs.pfnHookEvent("events/m24.sc", [](event_args_s *args) {
		CM24FireEvent event(args);
		event.Execute(false);
	});
}

void CGameEventManager::RegisterGlockEvents()
{
	gEngfuncs.pfnHookEvent("events/glock1.sc", [](event_args_s *args) {
		CGlockFireEvent event(args);
		event.Execute();
	});
	gEngfuncs.pfnHookEvent("events/glock2.sc", [](event_args_s *args) {
		CGlockFireEvent event(args);
		event.Execute();
	});
	gEngfuncs.pfnHookEvent("events/glock18.sc", [](event_args_s *args) {
		CGlockFireEvent event(args);
		event.Execute();
	});
}

void CGameEventManager::RegisterCrossbowEvents()
{
	gEngfuncs.pfnHookEvent("events/crossbow1.sc", [](event_args_s *args) {
		CCrossbowFireEvent event(args);
		event.Execute();
	});
	gEngfuncs.pfnHookEvent("events/crossbow2.sc", [](event_args_s *args) {
		CCrossbowFireEvent event(args);
		event.Execute();
	});
}

void CGameEventManager::RegisterRBullEvents()
{
	gEngfuncs.pfnHookEvent("events/python.sc", [](event_args_s *args) {
		CPythonFireEvent event(args);
		event.Execute();
	});
}

void CGameEventManager::RegisterSFUWeaponEvents()
{
	gEngfuncs.pfnHookEvent("events/ragingbull.sc", [](event_args_s *args) { CRagingBullFireEvent event(args); event.Execute(); });
	gEngfuncs.pfnHookEvent("events/mp5a3.sc", [](event_args_s *args) { CMP5A3FireEvent event(args); event.Execute(); });
	gEngfuncs.pfnHookEvent("events/m3.sc", [](event_args_s *args) { CM3FireEvent event(args); event.Execute(true); });
}

void CGameEventManager::RegisterDeagleEvents()
{
	gEngfuncs.pfnHookEvent("events/deagle.sc", [](event_args_s *args) {
		CDeagleFireEvent event(args);
		event.Execute();
	});
}

void CGameEventManager::RegisterMP5Events()
{
	gEngfuncs.pfnHookEvent("events/mp5.sc", [](event_args_s *args) {
		CMP5FireEvent event(args);
		event.Execute(false);
	});
	gEngfuncs.pfnHookEvent("events/mp52.sc", [](event_args_s *args) {
		CMP5FireEvent event(args);
		event.Execute(true);
	});
}

void CGameEventManager::RegisterShotgunEvents()
{
	gEngfuncs.pfnHookEvent("events/shotgun1.sc", [](event_args_s *args) {
		CShotgunFireEvent event(args);
		event.Execute(true);
	});
	gEngfuncs.pfnHookEvent("events/shotgun2.sc", [](event_args_s *args) {
		CShotgunFireEvent event(args);
		event.Execute(false);
	});
}

void CGameEventManager::RegisterCrowbarEvents()
{
	gEngfuncs.pfnHookEvent("events/crowbar.sc", [](event_args_s *args) {
		CCrowbarSwingEvent event(args);
		event.Execute();
	});
}

void CGameEventManager::RegisterTripmineEvents()
{
	gEngfuncs.pfnHookEvent("events/tripfire.sc", [](event_args_s *args) {
		CTripmineDeployEvent event(args);
		event.Execute();
	});
}

void CGameEventManager::RegisterSnarkEvents()
{
	gEngfuncs.pfnHookEvent("events/snarkfire.sc", [](event_args_s *args) {
		CSnarkThrowEvent event(args);
		event.Execute();
	});
}

void CGameEventManager::RegisterHornetgunEvents()
{
	gEngfuncs.pfnHookEvent("events/firehornet.sc", [](event_args_s *args) {
		CHornetgunFireEvent event(args);
		event.Execute();
	});
}

void CGameEventManager::RegisterRPGEvents()
{
	gEngfuncs.pfnHookEvent("events/rpg.sc", [](event_args_s *args) {
		CRpgFireEvent event(args);
		event.Execute();
	});
}

void CGameEventManager::RegisterEgonEvents()
{
	gEngfuncs.pfnHookEvent("events/egon_fire.sc", [](event_args_s *args) {
		CEgonFireEvent event(args);
		event.Execute();
	});
	gEngfuncs.pfnHookEvent("events/egon_stop.sc", [](event_args_s *args) {
		CEgonStopEvent event(args);
		event.Execute();
	});
}

void CGameEventManager::RegisterGaussEvents()
{
	gEngfuncs.pfnHookEvent("events/gauss.sc", [](event_args_s *args) {
		CGaussFireEvent event(args);
		event.Execute();
	});
	gEngfuncs.pfnHookEvent("events/gaussspin.sc", [](event_args_s *args) {
		CGaussSpinEvent event(args);
		event.Execute();
	});
}
