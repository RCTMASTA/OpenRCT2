/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LargeSceneryPlaceAction.h"

#include "../OpenRCT2.h"
#include "../management/Finance.h"
#include "../object/ObjectLimits.h"
#include "../ride/Ride.h"
#include "../world/Banner.h"
#include "../world/MapAnimation.h"
#include "../world/Surface.h"

LargeSceneryPlaceActionResult::LargeSceneryPlaceActionResult()
    : GameActions::Result(GameActions::Status::Ok, STR_CANT_POSITION_THIS_HERE)
{
}

LargeSceneryPlaceActionResult::LargeSceneryPlaceActionResult(GameActions::Status error)
    : GameActions::Result(error, STR_CANT_POSITION_THIS_HERE)
{
}

LargeSceneryPlaceActionResult::LargeSceneryPlaceActionResult(GameActions::Status error, rct_string_id message)
    : GameActions::Result(error, STR_CANT_POSITION_THIS_HERE, message)
{
}

LargeSceneryPlaceActionResult::LargeSceneryPlaceActionResult(GameActions::Status error, rct_string_id message, uint8_t* args)
    : GameActions::Result(error, STR_CANT_POSITION_THIS_HERE, message, args)
{
}

LargeSceneryPlaceAction::LargeSceneryPlaceAction(
    const CoordsXYZD& loc, ObjectEntryIndex sceneryType, uint8_t primaryColour, uint8_t secondaryColour)
    : _loc(loc)
    , _sceneryType(sceneryType)
    , _primaryColour(primaryColour)
    , _secondaryColour(secondaryColour)
{
    auto* sceneryEntry = get_large_scenery_entry(_sceneryType);
    if (sceneryEntry != nullptr)
    {
        if (sceneryEntry->scrolling_mode != SCROLLING_MODE_NONE)
        {
            _bannerId = create_new_banner(0);
        }
    }
}

void LargeSceneryPlaceAction::AcceptParameters(GameActionParameterVisitor& visitor)
{
    visitor.Visit(_loc);
    visitor.Visit("object", _sceneryType);
    visitor.Visit("primaryColour", _primaryColour);
    visitor.Visit("secondaryColour", _secondaryColour);
    auto* sceneryEntry = get_large_scenery_entry(_sceneryType);
    if (sceneryEntry != nullptr)
    {
        if (sceneryEntry->scrolling_mode != SCROLLING_MODE_NONE)
        {
            _bannerId = create_new_banner(0);
        }
    }
}

uint16_t LargeSceneryPlaceAction::GetActionFlags() const
{
    return GameAction::GetActionFlags();
}

void LargeSceneryPlaceAction::Serialise(DataSerialiser& stream)
{
    GameAction::Serialise(stream);

    stream << DS_TAG(_loc) << DS_TAG(_sceneryType) << DS_TAG(_primaryColour) << DS_TAG(_secondaryColour) << DS_TAG(_bannerId);
}

GameActions::Result::Ptr LargeSceneryPlaceAction::Query() const
{
    auto res = std::make_unique<LargeSceneryPlaceActionResult>();
    res->ErrorTitle = STR_CANT_POSITION_THIS_HERE;
    res->Expenditure = ExpenditureType::Landscaping;
    int16_t surfaceHeight = tile_element_height(_loc);
    res->Position.x = _loc.x + 16;
    res->Position.y = _loc.y + 16;
    res->Position.z = surfaceHeight;
    res->GroundFlags = 0;

    money32 supportsCost = 0;

    if (_primaryColour > TILE_ELEMENT_COLOUR_MASK || _secondaryColour > TILE_ELEMENT_COLOUR_MASK)
    {
        log_error(
            "Invalid game command for scenery placement, primaryColour = %u, secondaryColour = %u", _primaryColour,
            _secondaryColour);
        return std::make_unique<LargeSceneryPlaceActionResult>(GameActions::Status::InvalidParameters);
    }

    if (_sceneryType >= MAX_LARGE_SCENERY_OBJECTS)
    {
        log_error("Invalid game command for scenery placement, sceneryType = %u", _sceneryType);
        return std::make_unique<LargeSceneryPlaceActionResult>(GameActions::Status::InvalidParameters);
    }

    auto* sceneryEntry = get_large_scenery_entry(_sceneryType);
    if (sceneryEntry == nullptr)
    {
        log_error("Invalid game command for scenery placement, sceneryType = %u", _sceneryType);
        return std::make_unique<LargeSceneryPlaceActionResult>(GameActions::Status::InvalidParameters);
    }

    uint32_t totalNumTiles = GetTotalNumTiles(sceneryEntry->tiles);
    int16_t maxHeight = GetMaxSurfaceHeight(sceneryEntry->tiles);

    if (_loc.z != 0)
    {
        maxHeight = _loc.z;
    }

    res->Position.z = maxHeight;

    if (sceneryEntry->scrolling_mode != SCROLLING_MODE_NONE)
    {
        if (_bannerId == BANNER_INDEX_NULL)
        {
            log_error("Banner Index not specified.");
            return MakeResult(GameActions::Status::InvalidParameters, STR_TOO_MANY_BANNERS_IN_GAME);
        }

        auto banner = GetBanner(_bannerId);
        if (!banner->IsNull())
        {
            log_error("No free banners available");
            return std::make_unique<LargeSceneryPlaceActionResult>(GameActions::Status::NoFreeElements);
        }
    }

    uint8_t tileNum = 0;
    for (rct_large_scenery_tile* tile = sceneryEntry->tiles; tile->x_offset != -1; tile++, tileNum++)
    {
        auto curTile = CoordsXY{ tile->x_offset, tile->y_offset }.Rotate(_loc.direction);

        curTile.x += _loc.x;
        curTile.y += _loc.y;

        int32_t zLow = tile->z_offset + maxHeight;
        int32_t zHigh = tile->z_clearance + zLow;

        QuarterTile quarterTile = QuarterTile{ static_cast<uint8_t>(tile->flags >> 12), 0 }.Rotate(_loc.direction);
        const auto isTree = (sceneryEntry->flags & LARGE_SCENERY_FLAG_IS_TREE) != 0;
        auto canBuild = MapCanConstructWithClearAt(
            { curTile, zLow, zHigh }, &map_place_scenery_clear_func, quarterTile, GetFlags(), CREATE_CROSSING_MODE_NONE,
            isTree);
        if (canBuild->Error != GameActions::Status::Ok)
        {
            canBuild->ErrorTitle = STR_CANT_POSITION_THIS_HERE;
            return canBuild;
        }

        supportsCost += canBuild->Cost;

        int32_t tempSceneryGroundFlags = canBuild->GroundFlags & (ELEMENT_IS_ABOVE_GROUND | ELEMENT_IS_UNDERGROUND);
        if (!gCheatsDisableClearanceChecks)
        {
            if ((canBuild->GroundFlags & ELEMENT_IS_UNDERWATER) || (canBuild->GroundFlags & ELEMENT_IS_UNDERGROUND))
            {
                return std::make_unique<LargeSceneryPlaceActionResult>(
                    GameActions::Status::Disallowed, STR_CANT_BUILD_THIS_UNDERWATER);
            }
            if (res->GroundFlags && !(res->GroundFlags & tempSceneryGroundFlags))
            {
                return std::make_unique<LargeSceneryPlaceActionResult>(
                    GameActions::Status::Disallowed, STR_CANT_BUILD_PARTLY_ABOVE_AND_PARTLY_BELOW_GROUND);
            }
        }

        res->GroundFlags = tempSceneryGroundFlags;

        if (!LocationValid(curTile) || curTile.x >= gMapSizeUnits || curTile.y >= gMapSizeUnits)
        {
            return std::make_unique<LargeSceneryPlaceActionResult>(GameActions::Status::Disallowed, STR_OFF_EDGE_OF_MAP);
        }

        if (!(gScreenFlags & SCREEN_FLAGS_SCENARIO_EDITOR) && !map_is_location_owned({ curTile, zLow }) && !gCheatsSandboxMode)
        {
            return std::make_unique<LargeSceneryPlaceActionResult>(GameActions::Status::Disallowed, STR_LAND_NOT_OWNED_BY_PARK);
        }
    }

    if (!CheckMapCapacity(sceneryEntry->tiles, totalNumTiles))
    {
        log_error("No free map elements available");
        return std::make_unique<LargeSceneryPlaceActionResult>(GameActions::Status::NoFreeElements);
    }

    // Force ride construction to recheck area
    _currentTrackSelectionFlags |= TRACK_SELECTION_FLAG_RECHECK;

    res->Cost = (sceneryEntry->price * 10) + supportsCost;
    return res;
}

GameActions::Result::Ptr LargeSceneryPlaceAction::Execute() const
{
    auto res = std::make_unique<LargeSceneryPlaceActionResult>();
    res->ErrorTitle = STR_CANT_POSITION_THIS_HERE;
    res->Expenditure = ExpenditureType::Landscaping;

    int16_t surfaceHeight = tile_element_height(_loc);
    res->Position.x = _loc.x + 16;
    res->Position.y = _loc.y + 16;
    res->Position.z = surfaceHeight;
    res->GroundFlags = 0;

    money32 supportsCost = 0;

    auto* sceneryEntry = get_large_scenery_entry(_sceneryType);
    if (sceneryEntry == nullptr)
    {
        log_error("Invalid game command for scenery placement, sceneryType = %u", _sceneryType);
        return std::make_unique<LargeSceneryPlaceActionResult>(GameActions::Status::InvalidParameters);
    }

    if (sceneryEntry->tiles == nullptr)
    {
        log_error("Invalid large scenery object, sceneryType = %u", _sceneryType);
        return std::make_unique<LargeSceneryPlaceActionResult>(GameActions::Status::InvalidParameters);
    }

    int16_t maxHeight = GetMaxSurfaceHeight(sceneryEntry->tiles);

    if (_loc.z != 0)
    {
        maxHeight = _loc.z;
    }

    res->Position.z = maxHeight;

    uint8_t tileNum = 0;
    for (rct_large_scenery_tile* tile = sceneryEntry->tiles; tile->x_offset != -1; tile++, tileNum++)
    {
        auto curTile = CoordsXY{ tile->x_offset, tile->y_offset }.Rotate(_loc.direction);

        curTile.x += _loc.x;
        curTile.y += _loc.y;

        int32_t zLow = tile->z_offset + maxHeight;
        int32_t zHigh = tile->z_clearance + zLow;

        QuarterTile quarterTile = QuarterTile{ static_cast<uint8_t>(tile->flags >> 12), 0 }.Rotate(_loc.direction);
        const auto isTree = (sceneryEntry->flags & LARGE_SCENERY_FLAG_IS_TREE) != 0;
        auto canBuild = MapCanConstructWithClearAt(
            { curTile, zLow, zHigh }, &map_place_scenery_clear_func, quarterTile, GetFlags(), CREATE_CROSSING_MODE_NONE,
            isTree);
        if (canBuild->Error != GameActions::Status::Ok)
        {
            canBuild->ErrorTitle = STR_CANT_POSITION_THIS_HERE;
            return canBuild;
        }

        supportsCost += canBuild->Cost;
        res->GroundFlags = canBuild->GroundFlags & (ELEMENT_IS_ABOVE_GROUND | ELEMENT_IS_UNDERGROUND);

        if (!(GetFlags() & GAME_COMMAND_FLAG_GHOST))
        {
            footpath_remove_litter({ curTile, zLow });
            if (!gCheatsDisableClearanceChecks)
            {
                wall_remove_at({ curTile, zLow, zHigh });
            }
        }

        auto* newSceneryElement = TileElementInsert<LargeSceneryElement>(
            CoordsXYZ{ curTile.x, curTile.y, zLow }, quarterTile.GetBaseQuarterOccupied());
        Guard::Assert(newSceneryElement != nullptr);
        newSceneryElement->SetClearanceZ(zHigh);

        SetNewLargeSceneryElement(*newSceneryElement, tileNum);
        map_animation_create(MAP_ANIMATION_TYPE_LARGE_SCENERY, { curTile, zLow });

        if (tileNum == 0)
        {
            res->tileElement = newSceneryElement->as<TileElement>();
        }
        map_invalidate_tile_full(curTile);
    }

    // Allocate banner after all tiles to ensure banner id doesn't need to be freed.
    if (sceneryEntry->scrolling_mode != SCROLLING_MODE_NONE)
    {
        if (_bannerId == BANNER_INDEX_NULL)
        {
            log_error("No free banners available");
            return MakeResult(GameActions::Status::NoFreeElements, STR_TOO_MANY_BANNERS_IN_GAME);
        }

        auto banner = GetBanner(_bannerId);
        if (!banner->IsNull())
        {
            log_error("No free banners available");
            return std::make_unique<LargeSceneryPlaceActionResult>(GameActions::Status::NoFreeElements);
        }

        banner->text = {};
        banner->colour = 2;
        banner->text_colour = 2;
        banner->flags = BANNER_FLAG_IS_LARGE_SCENERY;
        banner->type = 0;
        banner->position = TileCoordsXY(_loc);

        ride_id_t rideIndex = banner_get_closest_ride_index({ _loc, maxHeight });
        if (rideIndex != RIDE_ID_NULL)
        {
            banner->ride_index = rideIndex;
            banner->flags |= BANNER_FLAG_LINKED_TO_RIDE;
        }
    }

    // Force ride construction to recheck area
    _currentTrackSelectionFlags |= TRACK_SELECTION_FLAG_RECHECK;

    res->Cost = (sceneryEntry->price * 10) + supportsCost;
    return res;
}

int16_t LargeSceneryPlaceAction::GetTotalNumTiles(rct_large_scenery_tile* tiles) const
{
    uint32_t totalNumTiles = 0;
    for (rct_large_scenery_tile* tile = tiles; tile->x_offset != -1; tile++)
    {
        totalNumTiles++;
    }
    return totalNumTiles;
}

bool LargeSceneryPlaceAction::CheckMapCapacity(rct_large_scenery_tile* tiles, int16_t numTiles) const
{
    for (rct_large_scenery_tile* tile = tiles; tile->x_offset != -1; tile++)
    {
        auto curTile = CoordsXY{ tile->x_offset, tile->y_offset }.Rotate(_loc.direction);

        curTile.x += _loc.x;
        curTile.y += _loc.y;
        if (!MapCheckCapacityAndReorganise(curTile, numTiles))
        {
            return false;
        }
    }
    return true;
}

int16_t LargeSceneryPlaceAction::GetMaxSurfaceHeight(rct_large_scenery_tile* tiles) const
{
    int16_t maxHeight = -1;
    for (rct_large_scenery_tile* tile = tiles; tile->x_offset != -1; tile++)
    {
        auto curTile = CoordsXY{ tile->x_offset, tile->y_offset }.Rotate(_loc.direction);

        curTile.x += _loc.x;
        curTile.y += _loc.y;

        if (!map_is_location_valid(curTile))
        {
            continue;
        }

        auto* surfaceElement = map_get_surface_element_at(curTile);
        if (surfaceElement == nullptr)
            continue;

        int32_t baseZ = surfaceElement->GetBaseZ();
        int32_t slope = surfaceElement->GetSlope();

        if ((slope & TILE_ELEMENT_SLOPE_ALL_CORNERS_UP) != TILE_ELEMENT_SLOPE_FLAT)
        {
            baseZ += LAND_HEIGHT_STEP;
            if (slope & TILE_ELEMENT_SLOPE_DOUBLE_HEIGHT)
            {
                baseZ += LAND_HEIGHT_STEP;
            }
        }

        if (baseZ > maxHeight)
        {
            maxHeight = baseZ;
        }
    }
    return maxHeight;
}

void LargeSceneryPlaceAction::SetNewLargeSceneryElement(LargeSceneryElement& sceneryElement, uint8_t tileNum) const
{
    sceneryElement.SetDirection(_loc.direction);
    sceneryElement.SetEntryIndex(_sceneryType);
    sceneryElement.SetSequenceIndex(tileNum);
    sceneryElement.SetPrimaryColour(_primaryColour);
    sceneryElement.SetSecondaryColour(_secondaryColour);

    if (_bannerId != BANNER_INDEX_NULL)
    {
        sceneryElement.SetBannerIndex(_bannerId);
    }

    if (GetFlags() & GAME_COMMAND_FLAG_GHOST)
    {
        sceneryElement.SetGhost(true);
    }
}
