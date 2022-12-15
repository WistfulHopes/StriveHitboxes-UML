#pragma once

#include "struct_util.h"
#include "CoreUObject_classes.hpp"
#include "bbscript.h"

class UWorld : public UE4::UObject {
public:
	FIELD(0x130, UE4::AGameState*, GameState);
};

class AREDGameState_Battle : public UE4::AGameState {
public:
	static UE4::UClass *StaticClass();

	FIELD(0xBA0, class asw_engine*, Engine);
	FIELD(0xBA8, class asw_scene*, Scene);
};

class player_block {
	char pad[0x160];
public:
	FIELD(0x8, class asw_entity*, entity);
};

static_assert(sizeof(player_block) == 0x160);

// Used by the shared GG/BB/DBFZ engine code
class asw_engine {
public:
	static constexpr auto COORD_SCALE = .458f;

	static asw_engine *get();

	ARRAY_FIELD(0x0, player_block[2], players);
	FIELD(0x8A0, int, entity_count);
	ARRAY_FIELD(0xC10, class asw_entity* [110], entities);
};


class asw_scene {
public:
	static asw_scene *get();

	// "delta" is the difference between input and output position
	// position gets written in place
	// position/angle can be null
	void camera_transform(UE4::FVector *delta, UE4::FVector *position, UE4::FVector *angle) const;
	void camera_transform(UE4::FVector *position, UE4::FVector *angle) const;
};

class hitbox {
public:
	enum class box_type : int {
		hurt = 0,
		hit = 1,
		grab = 2 // Not used by the game
	};

	box_type type;
	float x, y;
	float w, h;
};

enum class direction : int {
	right = 0,
	left = 1
};

class event_handler {
	char pad[0x58];

public:
	FIELD(0x0, bbscript::code_pointer, script);
	FIELD(0x28, int, trigger_value);
};

static_assert(sizeof(event_handler) == 0x58);

class asw_entity {
public:	FIELD(0x18, bool, is_player);
	  FIELD(0x44, unsigned char, player_index);
	  FIELD(0x70, hitbox*, hitboxes);
	  FIELD(0x104, int, hurtbox_count);
	  FIELD(0x108, int, hitbox_count);
	  //   _____    ____    _    _   _   _   _______   ______   _____  
	  //  / ____|  / __ \  | |  | | | \ | | |__   __| |  ____| |  __ \ 
	  // | |      | |  | | | |  | | |  \| |    | |    | |__    | |__) |
	  // | |      | |  | | | |  | | | . ` |    | |    |  __|   |  _  / 
	  // | |____  | |__| | | |__| | | |\  |    | |    | |____  | | \ \ 
	  //  \_____|  \____/   \____/  |_| \_|    |_|    |______| |_|  \_\ 
	  BIT_FIELD(0x1A0, 0x4000000, cinematic_counter);
	  FIELD(0x1B8, int, state_frames);
	  FIELD(0x2B8, asw_entity*, opponent);
	  FIELD(0x2D0, asw_entity*, parent);
	  FIELD(0x310, asw_entity*, attached);
	  BIT_FIELD(0x388, 1, airborne);
	  BIT_FIELD(0x388, 256, counterhit);
	  BIT_FIELD(0x38C, 16, strike_invuln);
	  BIT_FIELD(0x38C, 32, throw_invuln);
	  BIT_FIELD(0x38C, 64, wakeup);
	  FIELD(0x39C, direction, facing);
	  FIELD(0x3A0, int, pos_x);
	  FIELD(0x3A4, int, pos_y);
	  FIELD(0x3A8, int, pos_z);
	  FIELD(0x3AC, int, angle_x);
	  FIELD(0x3B0, int, angle_y);
	  FIELD(0x3B4, int, angle_z);
	  FIELD(0x3BC, int, scale_x);
	  FIELD(0x3C0, int, scale_y);
	  FIELD(0x3C4, int, scale_z);
	  FIELD(0x4C0, int, vel_x);
	  FIELD(0x4C4, int, vel_y);
	  FIELD(0x4C8, int, gravity);
	  FIELD(0x4F4, int, pushbox_front_offset);
	  FIELD(0x734, int, throw_box_top);
	  FIELD(0x73C, int, throw_box_bottom);
	  FIELD(0x740, int, throw_range);
	  FIELD(0x1144, int, backdash_invuln);
	  // bbscript
	  FIELD(0x11A8, bbscript::event_bitmask, event_handler_bitmask);
	  FIELD(0x11F8, char*, bbs_file);
	  FIELD(0x1200, char*, script_base);
	  FIELD(0x1208, char*, next_script_cmd);
	  FIELD(0x1210, char*, first_script_cmd);
	  FIELD(0x1244, int, sprite_frames);
	  FIELD(0x1238, int, sprite_duration);
	  FIELD(0x1324, int, sprite_changes);
	  ARRAY_FIELD(0x1330, event_handler[(size_t)bbscript::event_type::MAX], event_handlers);
	  ARRAY_FIELD(0x3718, char[32], state_name);

	  bool is_active() const;
	  bool is_pushbox_active() const;
	  bool is_strike_invuln() const;
	  bool is_throw_invuln() const;
	  int get_pos_x() const;
	  int get_pos_y() const;
	  int pushbox_width() const;
	  int pushbox_height() const;
	  int pushbox_bottom() const;
	  void get_pushbox(int* left, int* top, int* right, int* bottom) const;
};
