#include "arcsys.h"

#include "sigscan.h"

UE4::UClass* Class;

using asw_scene_camera_transform_t = void(*)(const asw_scene*, UE4::FVector*, UE4::FVector*, UE4::FVector*);
const auto asw_scene_camera_transform = (asw_scene_camera_transform_t)(
	sigscan::get().scan("\x4D\x85\xC0\x74\x15\xF2\x41\x0F", "xxxxxxxx") - 0x56);

using asw_entity_is_active_t = bool(*)(const asw_entity*, int);
const auto asw_entity_is_active = (asw_entity_is_active_t)(
	sigscan::get().scan("\x0F\x85\x00\x00\x00\x00\x8B\x81\xA4\x01", "xx????xxxx") - 0x1C);

using asw_entity_is_pushbox_active_t = bool(*)(const asw_entity*);
const auto asw_entity_is_pushbox_active = (asw_entity_is_pushbox_active_t)(
	sigscan::get().scan("\xF7\x80\xCC\x5D\x00\x00\x00\x00\x02\x00", "xx????xxxx") - 0x1A);

using asw_entity_get_pos_x_t = int(*)(const asw_entity*);
const auto asw_entity_get_pos_x = (asw_entity_get_pos_x_t)(
	sigscan::get().scan("\xEB\x00\x8B\xBB\xA0\x03\x00\x00", "x?xxxxxx") - 0x104);

using asw_entity_get_pos_y_t = int(*)(const asw_entity*);
const auto asw_entity_get_pos_y = (asw_entity_get_pos_y_t)(
	sigscan::get().scan("\x3D\x00\x08\x04\x00\x75\x18", "xxxxxxx") - 0x3D);

using asw_entity_pushbox_width_t = int(*)(const asw_entity*);
const auto asw_entity_pushbox_width = (asw_entity_pushbox_width_t)(
	sigscan::get().scan("\xF6\x81\x88\x03\x00\x00\x01\x75\x40\xE8\xB9\xFC\xFF\xFF", "xxxxxxxxxxxxxx") - 0x19);

using asw_entity_pushbox_height_t = int(*)(const asw_entity*);
const auto asw_entity_pushbox_height = (asw_entity_pushbox_height_t)(
	sigscan::get().scan("\xF6\x81\x88\x03\x00\x00\x01\x75\x40\xE8\x99\xFD\xFF\xFF", "xxxxxxxxxxxxxx") - 0x19);

using asw_entity_pushbox_bottom_t = int(*)(const asw_entity*);
const auto asw_entity_pushbox_bottom = (asw_entity_pushbox_bottom_t)(
	sigscan::get().scan("\xF6\x81\x88\x03\x00\x00\x01\x75\x09\xE8\x06\xFD\xFF\xFF", "xxxxxxxxxxxxxx") - 0x1C);

using asw_entity_get_pushbox_t = void(*)(const asw_entity*, int*, int*, int*, int*);
const auto asw_entity_get_pushbox = (asw_entity_get_pushbox_t)(
	sigscan::get().scan("\x99\x48\x8B\xCB\x2B\xC2\xD1\xF8\x44", "xxxxxxxxx") - 0x5B);

asw_engine *asw_engine::get()
{
	if (Class == nullptr)
		Class = UE4::UObject::FindClass("Class RED.REDGameState_Battle");
	if (*UE4::UWorld::GWorld == nullptr)
		return nullptr;

	const auto *GameState = ((UWorld*)(*UE4::UWorld::GWorld))->GameState;

	if (!GameState->IsA(Class))
		return nullptr;

	return ((AREDGameState_Battle*)GameState)->Engine;
}

asw_scene *asw_scene::get()
{
	if (Class == nullptr)
		Class = UE4::UObject::FindClass("Class RED.REDGameState_Battle");
	if (*UE4::UWorld::GWorld == nullptr)
		return nullptr;

	const auto *GameState = ((UWorld*)(*UE4::UWorld::GWorld))->GameState;

	if (!GameState->IsA(Class))
		return nullptr;

	return ((AREDGameState_Battle*)GameState)->Scene;
}

void asw_scene::camera_transform(UE4::FVector* delta, UE4::FVector* position, UE4::FVector* angle) const
{
	asw_scene_camera_transform(this, delta, position, angle);
}

void asw_scene::camera_transform(UE4::FVector* position, UE4::FVector* angle) const
{
	UE4::FVector delta;
	asw_scene_camera_transform(this, &delta, position, angle);
}


bool asw_entity::is_active() const
{
	// Otherwise returns false during COUNTER
	return cinematic_counter || asw_entity_is_active(this, !is_player);
}

bool asw_entity::is_pushbox_active() const
{
	return asw_entity_is_pushbox_active(this);
}

bool asw_entity::is_strike_invuln() const
{
	return strike_invuln || wakeup || backdash_invuln > 0;
}

bool asw_entity::is_throw_invuln() const
{
	return throw_invuln || wakeup || backdash_invuln > 0;
}

int asw_entity::get_pos_x() const
{
	return asw_entity_get_pos_x(this);
}

int asw_entity::get_pos_y() const
{
	return asw_entity_get_pos_y(this);
}

int asw_entity::pushbox_width() const
{
	return asw_entity_pushbox_width(this);
}

int asw_entity::pushbox_height() const
{
	return asw_entity_pushbox_height(this);
}

int asw_entity::pushbox_bottom() const
{
	return asw_entity_pushbox_bottom(this);
}

void asw_entity::get_pushbox(int* left, int* top, int* right, int* bottom) const
{
	asw_entity_get_pushbox(this, left, top, right, bottom);
}