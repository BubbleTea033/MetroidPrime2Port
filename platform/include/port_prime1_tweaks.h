// Metroid Prime 1's Boost Ball tuning, read from the player's own Prime 1 disc (Prime 1 boost
// ball, PortOverlay::Prime1BoostBall). The port ships no game data: the values are read from the
// disc the player points it at (`prime1_disc`), never compiled in.
//
// Prime 1 keeps its tweaks in Tweaks.pak, a version 3.5 PAK; the ball's is the CTWK resource named
// "Ball". Its body is 114 big-endian floats read in order by CTweakBall(CInputStream&) (GM8E01_00),
// the Boost Ball's at floats 105-113. The parsing here is pure (bytes in, values out) so the tests
// can build the files; port_prime1_tweaks.cpp does the disc I/O and zlib.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace PortPrime1Tweaks {

struct BoostBall {
  float drainTime = 0.f;
  float minChargeTime = 0.f;
  float maxChargeTime = 0.f;
  float chargeTime[3] = {}; // [2] is maxChargeTime, as Prime 1's CTweakBall sets it
  float incrementalSpeed[3] = {};
  // The whole Ball body as read, CTweakBall(CInputStream&)'s order (diagnostics: F1 Boost values).
  float raw[114] = {};
};

struct PakEntry {
  uint32_t offset = 0;
  uint32_t size = 0;
  bool compressed = false;
};

// CTweakBall(CInputStream&) reads this many floats; the resource is padded to 32 bytes in the PAK.
constexpr size_t kBallFloats = 114;
constexpr size_t kBallBytes = kBallFloats * 4;

inline uint32_t ReadBE32(const uint8_t* p) {
  return (uint32_t{p[0]} << 24) | (uint32_t{p[1]} << 16) | (uint32_t{p[2]} << 8) | uint32_t{p[3]};
}

inline float ReadBEFloat(const uint8_t* p) {
  const uint32_t bits = ReadBE32(p);
  float value;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

// Finds the resource of `type` (a FourCC) named `name` in a version 3.5 PAK.
inline bool FindPakResource(const uint8_t* pak, size_t size, const char type[4], const char* name,
                            PakEntry& out, std::string& error) {
  size_t at = 0;
  auto need = [&](size_t bytes) { return at <= size && size - at >= bytes; };
  if (!need(12) || ReadBE32(pak) != 0x00030005u) {
    error = "not a Metroid Prime 1 PAK (version 3.5)";
    return false;
  }
  at = 8;
  const uint32_t nameCount = ReadBE32(pak + at);
  at += 4;
  bool named = false;
  uint32_t id = 0;
  for (uint32_t i = 0; i < nameCount; ++i) {
    if (!need(12)) {
      error = "PAK name table is truncated";
      return false;
    }
    const uint8_t* entry = pak + at;
    const uint32_t nameLength = ReadBE32(entry + 8);
    at += 12;
    if (!need(nameLength)) {
      error = "PAK name table is truncated";
      return false;
    }
    if (!named && std::memcmp(entry, type, 4) == 0 && nameLength == std::strlen(name) &&
        std::memcmp(pak + at, name, nameLength) == 0) {
      named = true;
      id = ReadBE32(entry + 4);
    }
    at += nameLength;
  }
  if (!named) {
    error = std::string("PAK has no resource named ") + name;
    return false;
  }
  if (!need(4)) {
    error = "PAK resource table is truncated";
    return false;
  }
  const uint32_t resourceCount = ReadBE32(pak + at);
  at += 4;
  for (uint32_t i = 0; i < resourceCount; ++i) {
    if (!need(20)) {
      error = "PAK resource table is truncated";
      return false;
    }
    const uint8_t* entry = pak + at;
    at += 20;
    if (std::memcmp(entry + 4, type, 4) == 0 && ReadBE32(entry + 8) == id) {
      out.compressed = ReadBE32(entry) != 0;
      out.size = ReadBE32(entry + 12);
      out.offset = ReadBE32(entry + 16);
      if (out.offset > size || size - out.offset < out.size) {
        error = std::string("PAK resource ") + name + " lies outside the file";
        return false;
      }
      return true;
    }
  }
  error = std::string("PAK names ") + name + " but holds no such resource";
  return false;
}

// Reads the Boost Ball's values from an uncompressed Prime 1 CTWK "Ball" body. A body that is
// not CTweakBall's 114 floats (plus the PAK's padding) is refused rather than misread.
inline bool ParseBall(const uint8_t* data, size_t size, BoostBall& out, std::string& error) {
  if (size < kBallBytes || size >= kBallBytes + 32) {
    error = "the Ball tweak is " + std::to_string(size) + " bytes, not Prime 1's " +
            std::to_string(kBallBytes);
    return false;
  }
  auto at = [&](size_t index) { return ReadBEFloat(data + index * 4); };
  for (size_t i = 0; i < kBallFloats; ++i) {
    out.raw[i] = at(i);
  }
  out.drainTime = at(105);
  out.minChargeTime = at(106);
  out.chargeTime[0] = at(108);
  out.chargeTime[1] = at(109);
  out.maxChargeTime = at(110);
  out.chargeTime[2] = out.maxChargeTime;
  out.incrementalSpeed[0] = at(111);
  out.incrementalSpeed[1] = at(112);
  out.incrementalSpeed[2] = at(113);
  return true;
}

// Loads the values from a Prime 1 disc image (GameCube, GM8*). On success `source` names the
// disc ("GM8E01 rev 0"); on failure `error` says why. Implemented in port_prime1_tweaks.cpp.
bool LoadFromDisc(const std::string& path, BoostBall& out, std::string& source, std::string& error);

// The values from the `prime1_disc` setting, loaded once per path; false when none is set or the
// disc cannot be read (Status says why).
bool Values(BoostBall& out);
// "GM8E01 rev 0", "no Prime 1 disc set" or the reason the disc could not be read.
std::string Status();
// Reads the disc again on the next use, even at the same path (a disc picked again).
void Reload();

// While one is alive, CTweakBall returns Echoes' own Boost Ball values even with Prime 1's
// loaded, so the console's `prime1boost` can print the two side by side. Game thread only.
inline bool& OverrideSuppressed() {
  static bool suppressed = false;
  return suppressed;
}
struct SuppressOverride {
  SuppressOverride() { OverrideSuppressed() = true; }
  ~SuppressOverride() { OverrideSuppressed() = false; }
  SuppressOverride(const SuppressOverride&) = delete;
  SuppressOverride& operator=(const SuppressOverride&) = delete;
};

} // namespace PortPrime1Tweaks
