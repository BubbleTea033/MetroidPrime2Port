#pragma once

#include "port_game_ui.h"

// The cinematic skip's "Skip cutscene?" prompt (PortOverlay::SkipConfirm*) as a game box
// (port_game_ui.h): the scan box's line, no fill, at the bottom of the screen over the letterbox, while the
// skip is armed. Enhancements > Pace > Game-style skip prompt (PortOverlay::SkipPromptBox); off, the
// overlay draws its own "Press [A] to skip".
namespace PortSkipPrompt {
// Draws the box while the skip is armed (CInGameGuiManager::Draw, over a playing cutscene).
void Draw();
// Where it last drew, for a tap or click on it (the overlay's).
PortGameUi::ScreenRect LastRect();
} // namespace PortSkipPrompt
