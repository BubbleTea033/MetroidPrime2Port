# Nintendo Switch (NX) feasibility spike

Question: can Aurora's renderer (Dawn/WebGPU) run on a homebrew Switch, so the port can target it?
This builds Dawn's GL/GLES backend with devkitA64 against devkitPro's Mesa (nouveau) and runs a
WGSL triangle as an `.nro`. Nothing here is wired into the game build yet.

    tools/nx-spike/build.sh [work dir]          # toolchain (no sudo), Dawn, patches, dawn_nx.nro
    tools/nx-spike/run_ryujinx.sh <nro> 120     # local Ryujinx build, headless, guest log printed
    # build-app/gl_control.nro: the same scene in plain GLES (ninja -C <work>/build-app gl_control_nro)

## Result (2026-11-08, Ryujinx 475615f, Mesa 20.1 nouveau, libnx 4.12, gcc 16.1)

- Dawn (pinned fork) cross-compiles for Switch with three small patches (`patches/`): abseil's
  newlib gaps (no `link.h`, `tm_gmtoff`, signals), Khronos `eglplatform.h` typedefs for `__SWITCH__`,
  and Dawn itself: an EGL `SwitchNWindow` swapchain case and optional context robustness. Upstream
  Dawn already knows the Switch platform and the `SurfaceSourceSwitchNWindow` surface.
- No `dlopen`: the app hands Dawn its `eglGetProcAddress` (`RequestAdapterOptionsGetGLProc`).
- Mesa's Switch EGL is 1.4 with no fence sync; `app/egl_sync_shim.cpp` provides
  `EGL_KHR_fence_sync` on GL fences (as the fork's EGLOnWGL does), so Dawn needs no queue change.
- GL adapters are Compatibility-level only (`featureLevel = Compatibility`, as Aurora's GL path).
- Ryujinx: adapter `NV120`, `OpenGL ES 3.2 Mesa 20.1.0-rc3`; surface configures (RGBA8Unorm) and
  presents at a steady 60 fps (Fifo) for 6000+ frames. Application mode, 3.2 GiB.
- Not yet verified: pixels. Ryujinx cannot present on Xvfb (no DRI3), and readback under
  emulation is wrong (left quarter holds the clear colour, rest zero, no triangle; once a
  `GL_OUT_OF_MEMORY` on the readback texture). `gl_control.nro` (plain GLES 3 on Mesa, no Dawn,
  `glReadPixels` direct and via PBO) reads back exactly the same, so this is Ryujinx with Mesa,
  not Dawn. Pixels need real hardware.

## Next steps toward a port

1. Pixel check on hardware (copy the `.nro` to `sdmc:/switch/`, launch over a title for full memory;
   log goes to `sdmc:/dawn-nx.log`, frame to `sdmc:/dawn-nx.ppm`).
2. Aurora: `AuroraDawnProvider` Switch branch (vendor build, GL+GLES on), a libnx window/input/audio
   layer in place of SDL3 (devkitPro ships SDL2 only), `SurfaceSourceSwitchNWindow` surface.
3. Game: devkitA64 build of the port (static, newlib), disc image from SD, MusyX on audren.
4. Performance: Dawn GL on nouveau vs. a deko3d backend, decided by numbers from step 1-3.
