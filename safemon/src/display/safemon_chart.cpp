#include <iostream>
#include <csignal>
#include <GLES3/gl3.h>
#include "config.h"
#include "drm_helper.h"
#include "egl_helper.h"
#include "waterfall_data.h"
#include "waterfall_chart.h"
#include "camera.h"
#include "text_renderer.h"
#include "axis_gizmo.h"
#include "evdev_keyboard.h"
#include "evdev_mouse.h"
#include "input_manager.h"

static volatile bool g_running = true;
static void signal_handler(int) { g_running = false; }

// Tracks which movement keys are currently held down.
// Updated from input events, applied every frame.
struct KeyState {
    bool w      = false;
    bool a      = false;
    bool s      = false;
    bool d      = false;
    bool q      = false;
    bool e      = false;
};

// Process all pending events - update key state and handle one-shot actions.
static void process_events(InputManager& input,
                           KeyState& keys,
                           Safemon::Camera& camera,
                           const glm::vec3& orbit_center)
{
    std::vector<InputEvent> events = input.poll();

    for (const auto& ev : events) {
        switch (ev.type) {

            case InputEventType::Key: {
                bool down = (ev.key.action == KeyAction::Pressed ||
                             ev.key.action == KeyAction::Repeated);
                switch (ev.key.code) {
                    case KeyCode::W:      keys.w = down; break;
                    case KeyCode::A:      keys.a = down; break;
                    case KeyCode::S:      keys.s = down; break;
                    case KeyCode::D:      keys.d = down; break;
                    case KeyCode::Q:      keys.q = down; break;
                    case KeyCode::E:      keys.e = down; break;
                    // One-shot actions on press only
                    case KeyCode::R:
                        if (ev.key.action == KeyAction::Pressed)
                            camera.SpeedUp();
                        break;
                    case KeyCode::Tab:
                        if (ev.key.action == KeyAction::Pressed)
                            camera.SpeedDown();
                        break;
                    case KeyCode::Escape:
                        if (ev.key.action == KeyAction::Pressed)
                            g_running = false;
                        break;
                    default: break;
                }
                break;
            }

            case InputEventType::MouseMove:
                camera.HandleMouseOrbit(
                    static_cast<float>(ev.mouse_move.dx) * 0.3f,
                    static_cast<float>(ev.mouse_move.dy) * 0.3f,
                    orbit_center);
                break;

            case InputEventType::MouseScroll:
                camera.HandleWheel(static_cast<float>(ev.mouse_scroll.delta));
                break;

            case InputEventType::MouseButton:
                break;
        }
    }
}

// Apply held keys to the camera every frame.
static void apply_movement(const KeyState& keys, Safemon::Camera& camera)
{
    if (keys.w) camera.HandleKeyW();
    if (keys.s) camera.HandleKeyS();
    if (keys.a) camera.HandleKeyA();
    if (keys.d) camera.HandleKeyD();
    if (keys.q) camera.HandleKeyQ();
    if (keys.e) camera.HandleKeyE();
}

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Config is loaded on all platforms - needed for input device paths
    SafemonConfig cfg = load_config("/etc/safemon/safemon.conf");

#ifndef PLATFORM_JETSON
    DrmState drm;
    if (!drm_open(drm, cfg)) return 1;
    uint32_t W = drm.mode.hdisplay;
    uint32_t H = drm.mode.vdisplay;
#else
    DrmState drm;
    uint32_t W = 1920;
    uint32_t H = 1080;
#endif

    EglContext egl;
    if (!egl_init(egl, drm.fd, W, H)) return 1;

#ifdef PLATFORM_JETSON
    W = egl.width;
    H = egl.height;
#endif

    std::cout << "[safemon-chart] Running. Ctrl+C or Esc to exit.\n";
    std::cout << "[safemon-chart] Keys: WASD = move, QE = up/down, "
                 "R = faster, Tab = slower, mouse = orbit, scroll = zoom\n";

    // Input - device paths from config
    InputManager input(
        std::make_unique<EvdevKeyboard>(cfg.keyboard_device),
        std::make_unique<EvdevMouse>(cfg.mouse_device)
    );
    if (!input.open()) {
        std::cerr << "[safemon-chart] Warning: no input devices available\n";
    }

    // Load data
    Safemon::WaterfallData data;
    if (!data.Load("/etc/safemon/fault_data.json")) return 1;

    Safemon::Camera camera;
    float chart_width = static_cast<float>(data.buckets_per_day) / 100.0f;
    float chart_depth = static_cast<float>(data.days) * 1.0f;

    float cx = chart_width * 0.5f;
    float cz = chart_depth * 0.5f;

    camera.LookAt(
        glm::vec3(cx + 40.0f, 15.0f, cz + 80.0f),
        glm::vec3(cx + 20.0f,  0.0f, cz)
    );

    // Boost default speed - 0.03f is too slow for this chart scale
    while (camera.GetSpeed() < 0.3f) camera.SpeedUp();

    const glm::vec3 orbit_center(cx, 0.0f, cz);

    Safemon::WaterfallChart chart;
    chart.Init(data);

    Safemon::TextRenderer textRenderer;
    textRenderer.Init("/etc/safemon/JetBrainsMono-Regular.ttf");
    textRenderer.AddSize("title", 28.0f);
    textRenderer.AddSize("small", 16.0f);

    Safemon::AxisGizmo gizmo;
    gizmo.Init();

    std::cout << "[gl] Vendor:   " << glGetString(GL_VENDOR)   << "\n";
    std::cout << "[gl] Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "[gl] Version:  " << glGetString(GL_VERSION)  << "\n";

    glViewport(0, 0, W, H);
    glEnable(GL_DEPTH_TEST);

    KeyState keys;

#ifndef PLATFORM_JETSON
    gbm_bo*  prev_bo    = nullptr;
    uint32_t prev_fb_id = 0;
#endif

    while (g_running) {
        // 1. Collect events and update key state
        process_events(input, keys, camera, orbit_center);

        // 2. Apply held keys every frame
        apply_movement(keys, camera);

        // 3. Render
        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        chart.Render(camera.GetViewMatrix(),
                     camera.GetProjectionMatrix(
                         static_cast<float>(W) / static_cast<float>(H)));

        textRenderer.Begin(W, H);
        textRenderer.DrawText(20.0f, 30.0f, "SAFEMON CHART",
                              glm::vec3(1.0f, 1.0f, 1.0f), "title");
        textRenderer.End();

        gizmo.Render(camera, textRenderer, W, H);

        textRenderer.Begin(W, H);
        textRenderer.DrawText(200.0f, H - 130.0f, "X: Time of Day",
                              glm::vec3(1.0f, 0.25f, 0.25f), "small");
        textRenderer.DrawText(200.0f, H - 100.0f, "Y: Fault Severity",
                              glm::vec3(0.3f, 1.0f, 0.4f), "small");
        textRenderer.DrawText(200.0f, H -  70.0f, "Z: Day (0 = Today)",
                              glm::vec3(0.3f, 0.6f, 1.0f), "small");
        textRenderer.End();

        eglSwapBuffers(egl.dpy, egl.surf);

#ifndef PLATFORM_JETSON
        gbm_bo* bo = gbm_surface_lock_front_buffer(egl.gbm_surf);
        if (!bo) break;

        uint32_t fb_id  = 0;
        uint32_t stride = gbm_bo_get_stride(bo);
        uint32_t handle = gbm_bo_get_handle(bo).u32;
        uint32_t handles[4] = { handle, 0, 0, 0 };
        uint32_t strides[4] = { stride, 0, 0, 0 };
        uint32_t offsets[4] = { 0, 0, 0, 0 };
        drmModeAddFB2(drm.fd, W, H, GBM_FORMAT_XRGB8888,
                      handles, strides, offsets, &fb_id, 0);
        drmModeSetCrtc(drm.fd, drm.crtc_id, fb_id, 0, 0,
                       &drm.conn->connector_id, 1, &drm.mode);

        if (prev_bo) {
            drmModeRmFB(drm.fd, prev_fb_id);
            gbm_surface_release_buffer(egl.gbm_surf, prev_bo);
        }
        prev_bo    = bo;
        prev_fb_id = fb_id;
#endif
    }

    input.close();
#ifndef PLATFORM_JETSON
    if (prev_bo) {
        drmModeRmFB(drm.fd, prev_fb_id);
        gbm_surface_release_buffer(egl.gbm_surf, prev_bo);
    }
#endif
    gizmo.Shutdown();
    textRenderer.Shutdown();
    chart.Shutdown();
    egl_cleanup(egl);
    drm_close(drm);
    return 0;
}