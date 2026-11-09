// A short note in the game's text (include/port_note_toast.h).
#include "port_note_toast.h"

#include "port_game_ui.h"
#include "port_shader_toast.h"

#include <algorithm>
#include <chrono>
#include <mutex>

namespace PortNoteToast {
namespace {
using Clock = std::chrono::steady_clock;
constexpr float kShowSecs = 2.5f;
constexpr float kFadeSecs = 0.4f;

std::mutex sMutex;
std::string sNext;
bool sNew = false;
// Not a unique_ptr (see port_game_ui.h: one deleted while the program exits hangs it); freed once
// the note is done.
PortGameUi::Text* sText = nullptr;
// Timed from the first frame that draws the note.
Clock::time_point sShownAt;
} // namespace

void Show(const std::string& text) {
  std::lock_guard< std::mutex > lock(sMutex);
  sNext = text;
  sNew = true;
}

void Draw() {
  const Clock::time_point now = Clock::now();
  {
    std::lock_guard< std::mutex > lock(sMutex);
    if (sNew) {
      sNew = false;
      sShownAt = now;
      if (sNext.empty()) {
        delete sText;
        sText = nullptr;
      } else if (sText == nullptr) {
        sText = new PortGameUi::Text(PortGameUi::TextSpec{.text = sNext, .scale = 0.55f});
      } else {
        sText->SetText(sNext);
      }
    }
  }
  if (sText == nullptr) {
    return;
  }
  const float age = std::chrono::duration< float >(now - sShownAt).count();
  if (age > kShowSecs) {
    delete sText;
    sText = nullptr;
    return;
  }
  // Below the shader compile's count while that is up.
  sText->SetPlacement({PortGameUi::Anchor::Top, 0.f, PortShaderToast::Showing() ? 0.075f : 0.025f});
  sText->SetAlpha(std::clamp((kShowSecs - age) / kFadeSecs, 0.f, 1.f));
  sText->Draw();
}
} // namespace PortNoteToast
