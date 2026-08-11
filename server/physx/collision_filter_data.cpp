/*
collision_filter_data.cpp - part of PhysX physics engine implementation
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

#include "collision_filter_data.h"

using namespace physx;

enum ColllisionFlags : PxU32
{
	ConveyorActor = (1 << 0),
	CharacterActor = (1 << 1),
	DroppedWeaponActor = (1 << 2),
	VehicleActor = (1 << 3),
};

CollisionFilterData::CollisionFilterData() :
	m_conveyorFlag(false), m_character(false), m_droppedWeapon(false), m_vehicle(false)
{
}

CollisionFilterData::CollisionFilterData(const physx::PxFilterData &data)
{
	m_conveyorFlag = data.word0 & ColllisionFlags::ConveyorActor;
	m_character = data.word0 & ColllisionFlags::CharacterActor;
	m_droppedWeapon = data.word0 & ColllisionFlags::DroppedWeaponActor;
	m_vehicle = data.word0 & ColllisionFlags::VehicleActor;
}

bool CollisionFilterData::HasConveyorFlag() const
{
	return m_conveyorFlag;
}

void CollisionFilterData::SetConveyorFlag(bool state)
{
	m_conveyorFlag = state;
}

bool CollisionFilterData::IsCharacter() const { return m_character; }
void CollisionFilterData::SetCharacter(bool state) { m_character = state; }
bool CollisionFilterData::IsDroppedWeapon() const { return m_droppedWeapon; }
void CollisionFilterData::SetDroppedWeapon(bool state) { m_droppedWeapon = state; }
bool CollisionFilterData::IsVehicle() const { return m_vehicle; }
void CollisionFilterData::SetVehicle(bool state) { m_vehicle = state; }

PxFilterData CollisionFilterData::ToNativeType() const
{
	PxFilterData filterData{};
	filterData.word0 |= m_conveyorFlag ? ColllisionFlags::ConveyorActor : 0;
	filterData.word0 |= m_character ? ColllisionFlags::CharacterActor : 0;
	filterData.word0 |= m_droppedWeapon ? ColllisionFlags::DroppedWeaponActor : 0;
	filterData.word0 |= m_vehicle ? ColllisionFlags::VehicleActor : 0;
	return filterData;
}
