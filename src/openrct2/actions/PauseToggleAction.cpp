/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "PauseToggleAction.h"

uint16_t PauseToggleAction::GetActionFlags() const
{
    return GameAction::GetActionFlags() | GameActions::Flags::AllowWhilePaused;
}

GameActions::Result::Ptr PauseToggleAction::Query() const
{
    return std::make_unique<GameActions::Result>();
}

GameActions::Result::Ptr PauseToggleAction::Execute() const
{
    pause_toggle();
    return std::make_unique<GameActions::Result>();
}
