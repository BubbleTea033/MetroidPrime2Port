// F1 overlay: the Enhancements page, optional changes to how the game plays or looks that are
// not cheats (they are saved). State and helpers shared with the other parts are in
// debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_overlay.h"
#include "port_prime1_tweaks.h"

#include "MetroidPrime/Player/CGameOptions.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Tweaks/CTweakBall.hpp"

#include <imgui.h>

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <initializer_list>
#include <limits>
#include <mutex>
#include <utility>
#include <optional>
#include <string>

namespace PortDebug {
namespace {

// The Prime 1 disc picked in the file dialog, handed from its callback (any thread) to the next
// frame; empty when cancelled.
std::mutex sPrime1DiscMutex;
std::optional< std::string > sPrime1DiscPick;
bool sPrime1DiscDialogOpen = false;

void OpenPrime1DiscDialog() {
  int windowCount = 0;
  SDL_Window** windows = SDL_GetWindows(&windowCount);
  SDL_Window* window = windows != nullptr && windowCount > 0 ? windows[0] : nullptr;
  SDL_free(windows);
  static const SDL_DialogFileFilter filters[] = {
      {"GameCube disc images", "iso;gcm;ciso;gcz;rvz;wia;nfs"},
      {"All files", "*"},
  };
  const SDL_DialogFileCallback done = [](void*, const char* const* files, int) {
    std::lock_guard lock(sPrime1DiscMutex);
    sPrime1DiscPick = files != nullptr && files[0] != nullptr ? std::string(files[0]) : std::string();
    sPrime1DiscDialogOpen = false;
  };
  sPrime1DiscDialogOpen = true;
  SDL_ShowOpenFileDialog(done, nullptr, window, filters, 2, nullptr, false);
}

// Prime 1 boost ball's disc: the pick, what was read from it, and its values beside Echoes'.
void DrawPrime1Disc() {
  {
    std::lock_guard lock(sPrime1DiscMutex);
    if (sPrime1DiscPick.has_value()) {
      if (!sPrime1DiscPick->empty()) {
        PortOverlay::SetPrime1DiscPath(*sPrime1DiscPick);
        PortPrime1Tweaks::Reload();
      }
      sPrime1DiscPick.reset();
    }
  }
  ImGui::BeginDisabled(sPrime1DiscDialogOpen);
  if (ImGui::Button("Choose Prime 1 disc...")) {
    OpenPrime1DiscDialog();
  }
  ImGui::EndDisabled();
  ImGui::SetItemTooltip("Your own Metroid Prime 1 GameCube disc. Prime 1 boost ball reads its\n"
                        "Boost Ball values (charge times, boost speeds) from it; the port\n"
                        "ships none of them. Without one, Echoes' values are used.");
  ImGui::SameLine();
  ImGui::TextDisabled("%s", PortPrime1Tweaks::Status().c_str());
  PortPrime1Tweaks::BoostBall prime1;
  if (gpTweakBall.get() == nullptr || !PortPrime1Tweaks::Values(prime1) ||
      !ImGui::TreeNode("Boost values")) {
    return;
  }
  PortPrime1Tweaks::BoostBall echoes;
  {
    const PortPrime1Tweaks::SuppressOverride suppress;
    echoes.drainTime = gpTweakBall->GetBoostBallDrainTime();
    echoes.minChargeTime = gpTweakBall->GetBoostBallMinChargeTime();
    echoes.maxChargeTime = gpTweakBall->GetBoostBallMaxChargeTime();
    for (int i = 0; i < 3; ++i) {
      echoes.chargeTime[i] = gpTweakBall->GetBoostBallChargeTimeTable(i);
      echoes.incrementalSpeed[i] = gpTweakBall->GetBoostBallIncrementalSpeedTable(i);
    }
  }
  if (ImGui::BeginTable("prime1boost", 3, ImGuiTableFlags_SizingFixedFit)) {
    ImGui::TableSetupColumn("");
    ImGui::TableSetupColumn("Echoes");
    ImGui::TableSetupColumn("Prime 1");
    ImGui::TableHeadersRow();
    auto row = [](const char* name, float e, float p) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(name);
      ImGui::TableNextColumn();
      ImGui::Text("%.3f", e);
      ImGui::TableNextColumn();
      ImGui::Text("%.3f", p);
    };
    row("Drain time (s)", echoes.drainTime, prime1.drainTime);
    row("Min charge time (s)", echoes.minChargeTime, prime1.minChargeTime);
    row("Charge time 1 (s)", echoes.chargeTime[0], prime1.chargeTime[0]);
    row("Charge time 2 (s)", echoes.chargeTime[1], prime1.chargeTime[1]);
    row("Max charge time (s)", echoes.maxChargeTime, prime1.maxChargeTime);
    row("Boost speed 1", echoes.incrementalSpeed[0], prime1.incrementalSpeed[0]);
    row("Boost speed 2", echoes.incrementalSpeed[1], prime1.incrementalSpeed[1]);
    row("Boost speed 3", echoes.incrementalSpeed[2], prime1.incrementalSpeed[2]);
    ImGui::EndTable();
  }
  // Rolling and the boost camera, as the tweak files hold them (degrees; gravity as stored), for
  // comparing the feel around a boost. Prime 1's from its Ball body by CTweakBall's read order.
  // Its boost camera floats 87-91 are distance, elevation, yaw speed, dampen angle and turn rate,
  // Echoes' order: MP1's decomp names them otherwise, but CBallCamera uses them so (the "boost
  // elevation" is compared with the ball-to-camera distance, the "distance" passed as yaw speed).
  const CTweakBall& tweak = *gpTweakBall;
  const float toDegrees = 180.f / 3.14159265f;
  const auto compare = [&](const char* id, const char* title,
                           std::initializer_list< std::pair< const char*, std::pair< float, int > > > rows) {
    if (!ImGui::TreeNode(title)) {
      return;
    }
    if (ImGui::BeginTable(id, 3, ImGuiTableFlags_SizingFixedFit)) {
      ImGui::TableSetupColumn("");
      ImGui::TableSetupColumn("Echoes");
      ImGui::TableSetupColumn("Prime 1");
      ImGui::TableHeadersRow();
      for (const auto& [name, values] : rows) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(name);
        ImGui::TableNextColumn();
        if (values.first == values.first) {
          ImGui::Text("%.3f", values.first);
        } else {
          ImGui::TextUnformatted("-"); // Echoes has no accessor for it
        }
        ImGui::TableNextColumn();
        ImGui::Text("%.3f", prime1.raw[values.second]);
      }
      ImGui::EndTable();
    }
    ImGui::TreePop();
  };
  compare("prime1roll", "Rolling",
          {{"Acceleration (ground)", {tweak.GetMaxBallTranslationAcceleration(0), 0}},
           {"Acceleration (air)", {tweak.GetMaxBallTranslationAcceleration(1), 1}},
           {"Friction (ground)", {tweak.GetBallTranslationFriction(0), 8}},
           {"Friction (air)", {tweak.GetBallTranslationFriction(1), 9}},
           {"Top speed (ground)", {tweak.GetBallTranslationMaxSpeed(0), 16}},
           {"Top speed (air)", {tweak.GetBallTranslationMaxSpeed(1), 17}},
           {"Braking (ground)", {tweak.GetBallForwardBrakingAcceleration(0), 28}},
           {"Braking (air)", {tweak.GetBallForwardBrakingAcceleration(1), 29}},
           {"Gravity", {-tweak.GetBallGravity(), 36}},
           {"Water gravity", {-tweak.GetBallWaterGravity(), 37}},
           {"Tireness", {tweak.GetTireness(), 42}}});
  compare("prime1cam", "Ball camera",
          {{"Distance at low speed", {tweak.GetBallCameraMinSpeedDistance(), 52}},
           {"Distance at high speed", {tweak.GetBallCameraMaxSpeedDistance(), 53}},
           {"Boost distance", {tweak.GetBallCameraBoostDistance(), 87}},
           {"Boost elevation", {std::numeric_limits< float >::quiet_NaN(), 88}},
           {"Boost yaw speed", {tweak.GetBallCameraBoostYawSpeed(), 89}},
           {"Boost dampen angle (deg)", {tweak.GetBallCameraBoostDampenAngle() * toDegrees, 90}},
           {"Boost turn rate (deg/s)", {tweak.GetBallCameraBoostAnglePerSecond() * toDegrees, 91}},
           {"Boost spring constant", {tweak.GetBallCameraBoostSpringConstant(), 95}},
           {"Boost spring max", {tweak.GetBallCameraBoostSpringMax(), 96}},
           {"Boost spring tardis", {tweak.GetBallCameraBoostSpringTardis(), 97}}});
  ImGui::TreePop();
}

} // namespace

void DrawEnhancementsTab() {
  // Original experience plays the game as it shipped: every enhancement acts as retail.
  ImGui::BeginDisabled(PortOverlay::OriginalExperience());
  ImGui::SeparatorText("Dark Visor");
  Changed(ImGui::Checkbox("Full-screen Dark Visor", &sDarkVisorFull));
  Changed(ImGui::SliderInt("Dark Visor static", &sDarkVisorStatic, 0, PortOverlay::kDarkVisorStaticMax, "%d%%"));

  ImGui::SeparatorText("Ammo");
  Changed(ImGui::Checkbox("Infinite Light and Dark Ammo", &sInfiniteBeamAmmo));

  ImGui::SeparatorText("Music");
  // Read when the next track starts: a room change or a portal hop.
  Changed(ImGui::Checkbox("Light world music in Dark Aether", &sDarkWorldLightMusic));

  ImGui::SeparatorText("Pace");
  // The game reads this every frame in its cinematic flow state.
  Changed(ImGui::Checkbox("Skippable cutscenes", &sSkipCinematics));
  Changed(ImGui::Checkbox("Game-style skip prompt", &sSkipPromptBox));
  ItemHelp("The first press while a cutscene plays asks \"Skip cutscene?\" in a box drawn like the "
           "game's own, at the bottom of the screen; a second press skips. Off: a plain "
           "\"Press [A] to skip\" label.");
  // Read when a pickup is touched and when a map station's message box comes up.
  int seamless = sSeamlessPickups;
  if (ImGui::Combo("Seamless item pickups", &seamless, "Off\0" "All\0" "Repeats only\0")) {
    sSeamlessPickups = std::clamp(seamless, static_cast< int >(PortOverlay::kSeamlessPickups_Off),
                                  static_cast< int >(PortOverlay::kSeamlessPickups_Repeats));
    Changed(true);
  }
  ItemHelp("No control lock, message box or item cutscene: the item's name shows as a HUD memo. "
           "All: every pickup and map station. Repeats only: pickups of an item you already have, "
           "such as a second Missile Expansion or Energy Tank, so each kind's first box still "
           "shows.");
  // Off walks at retail speed; a custom action bound to this switch makes the slider a sprint.
  CheckboxTip("Custom walk speed", &sWalkSpeedEnabled,
                  "Off: retail speed. Bind a custom action to this setting (Toggle or While\n"
                  "held) in Controls to sprint at the speed below.");
  ImGui::BeginDisabled(!sWalkSpeedEnabled);
  int walkSpeed = sWalkSpeed;
  if (ImGui::SliderInt("Walk speed", &walkSpeed, PortOverlay::kWalkSpeedMin,
                       PortOverlay::kWalkSpeedMax, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
    sWalkSpeed = walkSpeed;
    Changed(true);
  }
  ImGui::EndDisabled();
  int scanSpeed = sScanSpeed;
  if (ImGui::SliderInt("Scan speed", &scanSpeed, PortOverlay::kScanSpeedMin,
                       PortOverlay::kScanSpeedMax, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
    sScanSpeed = scanSpeed;
    Changed(true);
  }
  int beamSwitchSpeed = sBeamSwitchSpeed;
  if (ImGui::SliderInt("Beam switch speed", &beamSwitchSpeed, PortOverlay::kBeamSwitchSpeedMin,
                       PortOverlay::kBeamSwitchSpeedMax, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
    sBeamSwitchSpeed = beamSwitchSpeed;
    Changed(true);
  }
  ItemHelp("How fast the cannon closes, morphs and opens on a beam change; 100% is retail. The "
           "rest of the gun keeps its speed.");
  // Elevator and portal share a row: same choices, short labels.
  const float rideWidth = ImGui::GetFontSize() * 6.f;
  int elevatorRide = sElevatorRide;
  ImGui::SetNextItemWidth(rideWidth);
  if (ImGui::Combo("Elevator ride cutscene", &elevatorRide, "Original\0" "Fast\0" "Skip\0")) {
    sElevatorRide = std::clamp(elevatorRide, static_cast< int >(PortOverlay::kElevatorRide_Original),
                               static_cast< int >(PortOverlay::kElevatorRide_Skip));
    Changed(true);
  }
  ImGui::SameLine();
  int portalTransition = sPortalTransition;
  ImGui::SetNextItemWidth(rideWidth);
  if (ImGui::Combo("Portal transition", &portalTransition, "Original\0" "Fast\0" "Skip\0")) {
    sPortalTransition = std::clamp(portalTransition,
                                   static_cast< int >(PortOverlay::kPortalTransition_Original),
                                   static_cast< int >(PortOverlay::kPortalTransition_Skip));
    Changed(true);
  }
  int rotatingRooms = sRotatingRooms;
  ImGui::SetNextItemWidth(rideWidth);
  if (ImGui::Combo("Energy controller access cutscene", &rotatingRooms, "Original\0" "Fast\0")) {
    sRotatingRooms = std::clamp(rotatingRooms, static_cast< int >(PortOverlay::kRotatingRooms_Original),
                                static_cast< int >(PortOverlay::kRotatingRooms_Fast));
    Changed(true);
  }
  int travelCutscenes = sTravelCutscenes;
  ImGui::SetNextItemWidth(rideWidth);
  if (ImGui::Combo("Elevator and light beam cutscenes", &travelCutscenes, "Original\0" "Fast\0")) {
    sTravelCutscenes = std::clamp(travelCutscenes,
                                  static_cast< int >(PortOverlay::kTravelCutscenes_Original),
                                  static_cast< int >(PortOverlay::kTravelCutscenes_Fast));
    Changed(true);
  }
  ItemHelp("Fast plays the cutscenes in the room as you leave or arrive by elevator or light beam "
           "teleporter at double speed. The ride between the worlds is Elevator ride's.");

  ImGui::SeparatorText("Morph Ball");
  Changed(ImGui::Checkbox("Fast Morph", &sFastMorph));
  ImGui::SameLine();
  Changed(ImGui::Checkbox("Spring Ball", &sSpringBall));

  ImGui::SeparatorText("Prime 1 movement");
  // Single player only: CPlayerDynamics and CMorphBall ignore both in multiplayer.
  CheckboxTip("Prime 1 air physics", &sPrime1AirPhysics,
              "Samus moves through the air as in Metroid Prime 1: no air friction once the jump\n"
              "button is released, and only her forward speed is capped, so L-jumps, R-jumps and\n"
              "scan dashes carry their speed. With mouse, twin-stick or touch aim, turning the\n"
              "view in the air does what holding R does. Single player only.");
  CheckboxTip("Prime 1 boost ball", &sPrime1BoostBall,
              "The Boost Ball as in Metroid Prime 1: the next boost can charge as soon as one\n"
              "ends, and a boost keeps its full strength when the ball is already rolling fast,\n"
              "so boosts chain and stack. Single player only.");
  ImGui::BeginDisabled(!sPrime1BoostBall);
  int boostFalloff = sPrime1BoostFalloff;
  if (ImGui::SliderInt("Boost speed falloff", &boostFalloff, PortOverlay::kPrime1BoostFalloffMin,
                       PortOverlay::kPrime1BoostFalloffMax, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
    sPrime1BoostFalloff = boostFalloff;
    Changed(true);
  }
  ItemHelp("How much weaker a boost gets when the ball is already moving: 0% is Prime 1 (full "
           "strength at any speed), 100% is Echoes. A boost from a standstill is the same at "
           "every value.");
  ImGui::EndDisabled();
  DrawPrime1Disc();
  ImGui::EndDisabled();

  ImGui::SeparatorText("Hints");
  // The game's own option, the same switch as the pause menu's Hint System and saved with the other
  // game options, so Original experience leaves it alone.
  ImGui::BeginDisabled(gpGameState == nullptr);
  bool hints = gpGameState != nullptr && gpGameState->GameOptions().GetIsHintSystemEnabled();
  if (ImGui::Checkbox("Hints", &hints) && gpGameState != nullptr) {
    gpGameState->GameOptions().SetIsHintSystemEnabled(hints);
  }
  ImGui::EndDisabled();
  ImGui::BeginDisabled(PortOverlay::OriginalExperience());
  int hintDelay = sHintDelay;
  if (ImGui::SliderInt("Hint delay", &hintDelay, PortOverlay::kHintDelayMin,
                       PortOverlay::kHintDelayMax, hintDelay == 0 ? "Game default" : "%d min",
                       ImGuiSliderFlags_AlwaysClamp)) {
    sHintDelay = hintDelay;
    Changed(true);
  }
  ImGui::EndDisabled();
  ImGui::BeginDisabled(gpGameState == nullptr ||
                       !gpGameState->GameOptions().GetIsHintSystemEnabled());
  if (ImGui::Button("Show current hint")) {
    PortOverlay::RequestShowHint();
  }
  ImGui::EndDisabled();
}

} // namespace PortDebug
