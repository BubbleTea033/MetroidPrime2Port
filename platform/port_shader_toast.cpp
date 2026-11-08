// The startup shader compile's count as a game box (include/port_shader_toast.h).
#include "port_shader_toast.h"

#include "port_game_ui.h"
#include "port_overlay.h"

#include <aurora/gfx.h>

#include <chrono>
#include <cstdio>

namespace PortShaderToast {
namespace {
// Never destroyed (see port_game_ui.h: one deleted while the program exits hangs it). Text, as the
// launch hint is: it only draws once its shaders are, and the font's are among the first compiled.
PortGameUi::Text* sText = nullptr;
bool sStartupDone = false;
bool sQueued = false;
std::chrono::steady_clock::time_point sQueuedSince;
uint32_t sBase = 0; // createdPipelines when the compile was first seen
} // namespace

void Draw() {
  const AuroraStats* stats = aurora_get_stats();
  if (stats != nullptr && stats->queuedPipelines == 0) {
    sStartupDone = true;
  }
  if (!PortOverlay::ShowShaderCompilation() || stats == nullptr || sStartupDone) {
    return;
  }
  const auto now = std::chrono::steady_clock::now();
  if (!sQueued) {
    sQueued = true;
    sQueuedSince = now;
    sBase = stats->createdPipelines;
  }
  if (now - sQueuedSince < std::chrono::milliseconds(500)) {
    return;
  }
  const uint32_t done = stats->createdPipelines >= sBase ? stats->createdPipelines - sBase : 0;
  const uint32_t total = done + stats->queuedPipelines;
  char text[64];
  std::snprintf(text, sizeof(text), "Compiling shaders %u / %u", done, total);
  if (sText == nullptr) {
    sText = new PortGameUi::Text(PortGameUi::TextSpec{
        .text = text,
        .scale = 0.55f,
        .placement = {PortGameUi::Anchor::Top, 0.f, 0.025f},
    });
    sText->SetAlpha(1.f);
  }
  sText->SetText(text);
  sText->Draw();
}
} // namespace PortShaderToast
