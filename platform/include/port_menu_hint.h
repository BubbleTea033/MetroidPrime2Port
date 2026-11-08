#pragma once

// The launch hint "Press F1 for settings" as the game's own text (port_game_ui.h), in the top left
// corner for 8 seconds after it first shows, fading out over the last one; gone for good once the
// settings open. Video > "Show F1 hint at launch" (PortOverlay::ShowMenuHint).
namespace PortMenuHint {
// Draws it over whatever the game drew (the boot screens, the title screen or the game), on the
// game thread after the IOWin draw.
void Draw();
} // namespace PortMenuHint
