#pragma once

// "Compiling shaders 12 / 340" in the game's text (port_game_ui.h), top centre, while the
// startup shader compile runs (the pipeline cache queued before the first frame): after half a
// second, until that queue has drained; shaders met in game later compile without it. Video >
// "Show shader compilation" (PortOverlay::ShowShaderCompilation). From MP1's overlay toast.
namespace PortShaderToast {
// Draws it over whatever the game drew, on the game thread after the IOWin draw.
void Draw();
// The count is up (the note toast, port_note_toast.h, goes below it).
bool Showing();
} // namespace PortShaderToast
