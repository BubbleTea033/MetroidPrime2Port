#pragma once

// A short note ("Save state slot 3 (empty)", a save state's result, a custom action's note) in the
// game's text (port_game_ui.h), top centre, for two and a half seconds, fading out at the end, as
// the shader compile's count is (port_shader_toast.h). PortDebug::ShowNote shows one; a new note
// replaces the one up.
#include <string>

namespace PortNoteToast {
// Any thread.
void Show(const std::string& text);
// Draws it over whatever the game drew, on the game thread after the IOWin draw.
void Draw();
} // namespace PortNoteToast
