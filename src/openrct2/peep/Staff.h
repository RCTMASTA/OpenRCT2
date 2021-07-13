/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#ifndef _STAFF_H_
#define _STAFF_H_

#include "../common.h"
#include "Peep.h"

#define STAFF_MAX_COUNT 200
// The number of elements in the gStaffPatrolAreas array per staff member. Every bit in the array represents a 4x4 square.
// Right now, it's a 32-bit array like in RCT2. 32 * 128 = 4096 bits, which is also the number of 4x4 squares on a 256x256 map.
#define STAFF_PATROL_AREA_SIZE 128

enum class StaffMode : uint8_t
{
    None,
    Walk,
    Patrol = 3
};

enum STAFF_ORDERS
{
    STAFF_ORDERS_SWEEPING = (1 << 0),
    STAFF_ORDERS_WATER_FLOWERS = (1 << 1),
    STAFF_ORDERS_EMPTY_BINS = (1 << 2),
    STAFF_ORDERS_MOWING = (1 << 3),
    STAFF_ORDERS_INSPECT_RIDES = (1 << 0),
    STAFF_ORDERS_FIX_RIDES = (1 << 1)
};

enum class EntertainerCostume : uint8_t
{
    Panda,
    Tiger,
    Elephant,
    Roman,
    Gorilla,
    Snowman,
    Knight,
    Astronaut,
    Bandit,
    Sheriff,
    Pirate,

    Count
};

extern const rct_string_id StaffCostumeNames[static_cast<uint8_t>(EntertainerCostume::Count)];

extern uint32_t gStaffPatrolAreas[(STAFF_MAX_COUNT + static_cast<uint8_t>(StaffType::Count)) * STAFF_PATROL_AREA_SIZE];
extern StaffMode gStaffModes[STAFF_MAX_COUNT + static_cast<uint8_t>(StaffType::Count)];
extern uint16_t gStaffDrawPatrolAreas;
extern colour_t gStaffHandymanColour;
extern colour_t gStaffMechanicColour;
extern colour_t gStaffSecurityColour;

void staff_reset_modes();
void staff_set_name(uint16_t spriteIndex, const char* name);
bool staff_hire_new_member(StaffType staffType, EntertainerCostume entertainerType);
void staff_update_greyed_patrol_areas();
bool staff_is_patrol_area_set_for_type(StaffType type, const CoordsXY& coords);
void staff_set_patrol_area(int32_t staffIndex, const CoordsXY& coords, bool value);
void staff_toggle_patrol_area(int32_t staffIndex, const CoordsXY& coords);
colour_t staff_get_colour(StaffType staffType);
bool staff_set_colour(StaffType staffType, colour_t value);
uint32_t staff_get_available_entertainer_costumes();
int32_t staff_get_available_entertainer_costume_list(EntertainerCostume* costumeList);

money32 GetStaffWage(StaffType type);
PeepSpriteType EntertainerCostumeToSprite(EntertainerCostume entertainerType);

#endif
