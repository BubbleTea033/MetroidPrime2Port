// The newer-release toast in the game's text (include/port_update_toast.h).
#include "port_update_toast.h"

#include "port_build_info.h"
#include "port_menu_hint.h"
#include "port_shader_toast.h"
#include "port_update_check.h"

#include <algorithm>
#include <atomic>
#include <chrono>

namespace PortUpdateToast {
namespace {
using Clock = std::chrono::steady_clock;
constexpr float kShowSecs = 12.f;
constexpr float kFadeSecs = 0.4f;
constexpr const char* kDim = "&main-color=#8A8A8AFF;";

std::atomic< bool > sDone{false};
// Not a unique_ptr (see port_game_ui.h: one deleted while the program exits hangs it); freed once
// it is done.
PortGameUi::Text* sText = nullptr;
bool sStarted = false;
Clock::time_point sShownAt;
} // namespace

void Draw() {
  if (sDone.load() || PortUpdateCheck::Status() != PortUpdateCheck::kStatus_Available) {
    delete sText;
    sText = nullptr;
    return;
  }
  const Clock::time_point now = Clock::now();
  if (!sStarted) {
    sStarted = true;
    sShownAt = now;
  }
  const float age = std::chrono::duration< float >(now - sShownAt).count();
  if (age > kShowSecs) {
    Dismiss();
    delete sText;
    sText = nullptr;
    return;
  }
  if (sText == nullptr) {
    const std::string text = "Version " + PortUpdateCheck::Latest().version + " is out (this is " +
                             MP_BUILD_VERSION + ")\n&push;" + kDim +
                             "Click or tap here to open its release page&pop;";
    sText = new PortGameUi::Text(PortGameUi::TextSpec{.text = text, .scale = 0.55f});
  }
  // Below the shader compile's count while that is up, and below the launch hint where the two
  // would meet (a narrow screen).
  float top = std::max(0.025f, PortShaderToast::Bottom() + 0.01f);
  const PortGameUi::ScreenRect hint = PortMenuHint::LastRect(), self = sText->LastRect();
  if (hint.valid && self.valid && hint.x1 + 0.01f > self.x0) {
    top = std::max(top, hint.y1 + 0.01f);
  }
  sText->SetPlacement({PortGameUi::Anchor::Top, 0.f, top});
  sText->SetAlpha(std::clamp((kShowSecs - age) / kFadeSecs, 0.f, 1.f));
  sText->Draw();
}

bool Showing() { return sText != nullptr && !sDone.load(); }

PortGameUi::ScreenRect LastRect() { return sText != nullptr ? sText->LastRect() : PortGameUi::ScreenRect{}; }

void Dismiss() { sDone.store(true); }
} // namespace PortUpdateToast
