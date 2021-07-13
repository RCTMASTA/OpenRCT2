/*****************************************************************************
 * Copyright (c) 2014-2021 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/
#include "EntityTweener.h"

#include "../peep/Peep.h"
#include "../ride/Vehicle.h"
#include "EntityList.h"
#include "Sprite.h"

#include <cmath>

void EntityTweener::PopulateEntities()
{
    for (auto ent : EntityList<Guest>())
    {
        Entities.push_back(ent);
        PrePos.emplace_back(ent->x, ent->y, ent->z);
    }
    for (auto ent : EntityList<Staff>())
    {
        Entities.push_back(ent);
        PrePos.emplace_back(ent->x, ent->y, ent->z);
    }
    for (auto ent : EntityList<Vehicle>())
    {
        Entities.push_back(ent);
        PrePos.emplace_back(ent->x, ent->y, ent->z);
    }
}

void EntityTweener::PreTick()
{
    Restore();
    Reset();
    PopulateEntities();
}

void EntityTweener::PostTick()
{
    for (auto* ent : Entities)
    {
        if (ent == nullptr)
        {
            // Sprite was removed, add a dummy position to keep the index aligned.
            PostPos.emplace_back(0, 0, 0);
        }
        else
        {
            PostPos.emplace_back(ent->x, ent->y, ent->z);
        }
    }
}

void EntityTweener::RemoveEntity(SpriteBase* entity)
{
    if (!entity->Is<Peep>() && !entity->Is<Vehicle>())
    {
        // Only peeps and vehicles are tweened, bail if type is incorrect.
        return;
    }

    auto it = std::find(Entities.begin(), Entities.end(), entity);
    if (it != Entities.end())
        *it = nullptr;
}

void EntityTweener::Tween(float alpha)
{
    const float inv = (1.0f - alpha);
    for (size_t i = 0; i < Entities.size(); ++i)
    {
        auto* ent = Entities[i];
        if (ent == nullptr)
            continue;

        auto& posA = PrePos[i];
        auto& posB = PostPos[i];

        if (posA == posB)
            continue;

        sprite_set_coordinates(
            { static_cast<int32_t>(std::round(posB.x * alpha + posA.x * inv)),
              static_cast<int32_t>(std::round(posB.y * alpha + posA.y * inv)),
              static_cast<int32_t>(std::round(posB.z * alpha + posA.z * inv)) },
            ent);
        ent->Invalidate();
    }
}

void EntityTweener::Restore()
{
    for (size_t i = 0; i < Entities.size(); ++i)
    {
        auto* ent = Entities[i];
        if (ent == nullptr)
            continue;

        sprite_set_coordinates(PostPos[i], ent);
        ent->Invalidate();
    }
}

void EntityTweener::Reset()
{
    Entities.clear();
    PrePos.clear();
    PostPos.clear();
}

static EntityTweener tweener;

EntityTweener& EntityTweener::Get()
{
    return tweener;
}
