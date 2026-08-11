/*
contact_modify_callback.cpp - part of PhysX physics engine implementation
Copyright (C) 2023 SNMetamorph

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "contact_modify_callback.h"
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "entities/func_car.h"

using namespace physx;

void ContactModifyCallback::onContactModify(PxContactModifyPair* const pairs, PxU32 count)
{
	for (PxU32 i = 0; i < count; i++)
	{
		PxContactModifyPair &pair = pairs[i];
		const PxActor *a1 = pair.actor[0];
		const PxActor *a2 = pair.actor[1];
		edict_t *e1 = (edict_t*)a1->userData;
		edict_t *e2 = (edict_t*)a2->userData;

		if (!e1 || !e2)
			return;

		if (FBitSet(e1->v.flags, FL_CONVEYOR) || FBitSet(e2->v.flags, FL_CONVEYOR))
		{
			edict_t *conveyorEntity = FBitSet(e1->v.flags, FL_CONVEYOR) ? e1 : e2;
			Vector conveyorSpeed = conveyorEntity->v.movedir * conveyorEntity->v.speed;
			for (PxU32 j = 0; j < pair.contacts.size(); j++) {
				pair.contacts.setTargetVelocity(j, conveyorSpeed);
			}
		}

		const bool firstVehicle = !Q_strnicmp(STRING(e1->v.classname), "car_", 4);
		const bool secondVehicle = !Q_strnicmp(STRING(e2->v.classname), "car_", 4);
		const bool firstCharacter = e1->v.flags & (FL_CLIENT | FL_MONSTER);
		const bool secondCharacter = e2->v.flags & (FL_CLIENT | FL_MONSTER);
		if ((firstVehicle && secondCharacter) || (secondVehicle && firstCharacter))
		{
			// The vehicle keeps a short multiplayer-safe EHANDLE list of occupants
			// that have just exited; only those contact points are suppressed.
			CFuncCar *car = static_cast<CFuncCar *>(CBaseEntity::Instance(firstVehicle ? e1 : e2));
			CBaseEntity *character = CBaseEntity::Instance(firstVehicle ? e2 : e1);
			if (car && car->ShouldIgnoreExitCollision(character))
			{
				for (PxU32 j = 0; j < pair.contacts.size(); ++j)
					pair.contacts.ignore(j);
				continue;
			}
			// Keep a full blocking contact for the character, but make the vehicle
			// immovable for this pair only. This prevents player pushing without
			// weakening the collision or allowing penetration into the chassis.
			if (firstVehicle)
			{
				pair.contacts.setInvMassScale0(0.0f);
				pair.contacts.setInvInertiaScale0(0.0f);
			}
			else
			{
				pair.contacts.setInvMassScale1(0.0f);
				pair.contacts.setInvInertiaScale1(0.0f);
			}
		}
	}
}
