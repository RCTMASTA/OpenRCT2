/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "GameAction.h"

// Clang format is broken for small game actions
// clang-format off
DEFINE_GAME_ACTION(PauseToggleAction, GameCommand::TogglePause, GameActions::Result)
{
public:
    PauseToggleAction() = default;

    uint16_t GetActionFlags() const override;

    GameActions::Result::Ptr Query() const override;
    GameActions::Result::Ptr Execute() const override;
};
// clang-format on
