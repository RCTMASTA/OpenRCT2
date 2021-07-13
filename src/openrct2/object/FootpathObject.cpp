/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "FootpathObject.h"

#include "../core/IStream.hpp"
#include "../core/Json.hpp"
#include "../drawing/Drawing.h"
#include "../localisation/Language.h"
#include "../world/Footpath.h"

void FootpathObject::ReadLegacy(IReadObjectContext* context, OpenRCT2::IStream* stream)
{
    stream->Seek(10, OpenRCT2::STREAM_SEEK_CURRENT);
    _legacyType.support_type = static_cast<RailingEntrySupportType>(stream->ReadValue<uint8_t>());
    _legacyType.flags = stream->ReadValue<uint8_t>();
    _legacyType.scrolling_mode = stream->ReadValue<uint8_t>();
    stream->Seek(1, OpenRCT2::STREAM_SEEK_CURRENT);

    GetStringTable().Read(context, stream, ObjectStringID::NAME);
    GetImageTable().Read(context, stream);

    // Validate properties
    if (_legacyType.support_type >= RailingEntrySupportType::Count)
    {
        context->LogError(ObjectError::InvalidProperty, "RailingEntrySupportType not supported.");
    }
}

void FootpathObject::Load()
{
    GetStringTable().Sort();
    _legacyType.string_idx = language_allocate_object_string(GetName());
    _legacyType.image = gfx_object_allocate_images(GetImageTable().GetImages(), GetImageTable().GetCount());
    _legacyType.bridge_image = _legacyType.image + 109;

    _pathSurfaceEntry.string_idx = _legacyType.string_idx;
    _pathSurfaceEntry.image = _legacyType.image;
    _pathSurfaceEntry.preview = _legacyType.image + 71;
    _pathSurfaceEntry.flags = _legacyType.flags;

    _queueEntry.string_idx = _legacyType.string_idx;
    _queueEntry.image = _legacyType.image + 51;
    _queueEntry.preview = _legacyType.image + 72;
    _queueEntry.flags = _legacyType.flags | FOOTPATH_ENTRY_FLAG_IS_QUEUE;

    _pathRailingsEntry.string_idx = _legacyType.string_idx;
    _pathRailingsEntry.bridge_image = _legacyType.bridge_image;
    _pathRailingsEntry.preview = _legacyType.image + 71;
    _pathRailingsEntry.flags = _legacyType.flags;
    _pathRailingsEntry.scrolling_mode = _legacyType.scrolling_mode;
    _pathRailingsEntry.support_type = _legacyType.support_type;
    _pathRailingsEntry.railings_image = _legacyType.image + 73;
}

void FootpathObject::Unload()
{
    language_free_object_string(_legacyType.string_idx);
    gfx_object_free_images(_legacyType.image, GetImageTable().GetCount());

    _legacyType.string_idx = 0;
    _legacyType.image = 0;
}

void FootpathObject::DrawPreview(rct_drawpixelinfo* dpi, int32_t width, int32_t height) const
{
    auto screenCoords = ScreenCoordsXY{ width / 2, height / 2 };
    gfx_draw_sprite(dpi, _pathSurfaceEntry.preview, screenCoords - ScreenCoordsXY{ 49, 17 }, 0);
    gfx_draw_sprite(dpi, _queueEntry.preview, screenCoords + ScreenCoordsXY{ 4, -17 }, 0);
}

static RailingEntrySupportType ParseSupportType(const std::string& s)
{
    if (s == "pole")
        return RailingEntrySupportType::Pole;
    else /* if (s == "box") */
        return RailingEntrySupportType::Box;
}

void FootpathObject::ReadJson(IReadObjectContext* context, json_t& root)
{
    Guard::Assert(root.is_object(), "FootpathObject::ReadJson expects parameter root to be object");

    auto properties = root["properties"];

    if (properties.is_object())
    {
        _legacyType.support_type = ParseSupportType(Json::GetString(properties["supportType"]));
        _legacyType.scrolling_mode = Json::GetNumber<uint8_t>(properties["scrollingMode"]);

        _legacyType.flags = Json::GetFlags<uint8_t>(
            properties,
            {
                { "hasSupportImages", RAILING_ENTRY_FLAG_HAS_SUPPORT_BASE_SPRITE },
                { "hasElevatedPathImages", RAILING_ENTRY_FLAG_DRAW_PATH_OVER_SUPPORTS },
                { "editorOnly", FOOTPATH_ENTRY_FLAG_SHOW_ONLY_IN_SCENARIO_EDITOR },
            });
    }

    PopulateTablesFromJson(context, root);
}
