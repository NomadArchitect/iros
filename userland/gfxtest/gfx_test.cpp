#include <di/container/string/string_view.h>
#include <di/math/abs_diff.h>
#include <di/types/integers.h>
#include <dius/print.h>
#include <diusgfx/bitmap.h>
#include <string.h>
#include <sys/mman.h>
#include <syscall.h>
#include <unistd.h>
#include <wayland-client.h>

#include "xdg-shell.h"

// NOTE: this test code is heavily inspired from this tutorial:
// https://bugaevc.gitbooks.io/writing-wayland-clients/content/
constexpr usize width = 1253;
constexpr usize height = 1374;
constexpr usize stride = width * 4;
constexpr usize size = stride * height;

static wl_compositor* compositor;
static wl_shm* shm;
static xdg_wm_base* shell;

void draw_rect(gfx::BitMap data, usize sx, usize sy, usize w, usize h, gfx::ARGBPixel color) {
    for (auto y = sy; y < sy + h && y < data.height(); y++) {
        for (auto x = sx; x < sx + w && x < data.width(); x++) {
            data.argb_pixels()[y, x] = color;
        }
    }
}

void draw_circle(gfx::BitMap data, usize cx, usize cy, usize r, gfx::ARGBPixel color) {
    auto sx = r > cx ? 0 : cx - r;
    auto sy = r > cy ? 0 : cy - r;

    for (auto y = sy; y <= cy + r && y < data.height(); y++) {
        for (auto x = sx; x <= cx + r && x < data.width(); x++) {
            auto dx = di::abs_diff(x, cx);
            auto dy = di::abs_diff(y, cy);
            if (dx * dx + dy * dy < r * r) {
                data.argb_pixels()[y, x] = color;
            }
        }
    }
}

void draw(gfx::BitMap data) {
    // Clear
    draw_rect(data, 0, 0, width, height, gfx::ARGBPixel(0, 0, 0, 255));

    // Moving rect.
    static usize x = 0;
    draw_rect(data, x++, 100, 200, 200, gfx::ARGBPixel(255, 0, 0, 255));
    x %= width;

    // Moving circle.
    static usize z = 100;
    draw_circle(data, z++, 500, 100, gfx::ARGBPixel(0, 255, 0, 255));
    z %= width;
}

int main() {
    auto* display = wl_display_connect(nullptr);
    if (!display) {
        dius::eprintln("Failed to connect to Wayland display"_sv);
        return 1;
    }

    auto* registry = wl_display_get_registry(display);
    if (!registry) {
        dius::eprintln("Failed to get Wayland registry: {}"_sv, wl_display_get_error(display));
        return 1;
    }

    auto registry_listener = wl_registry_listener {
        .global =
            [](void*, wl_registry* registry, uint32_t name, char const* interface_s, uint32_t version) {
                auto interface = di::TransparentStringView { interface_s, strlen(interface_s) };
                dius::println("Global: name={}, interface={}, version={}"_sv, name, interface, version);

                if (interface == "wl_compositor"_tsv) {
                    compositor =
                        static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 4));
                } else if (interface == "wl_shm"_tsv) {
                    shm = static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
                } else if (interface == "xdg_wm_base"_tsv) {
                    shell = static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
                }
            },
        .global_remove =
            [](void*, wl_registry*, uint32_t name) {
                dius::println("Global remove: name={}"_sv, name);
            },
    };

    wl_registry_add_listener(registry, &registry_listener, nullptr);

    wl_display_roundtrip(display);
    if (!compositor) {
        dius::eprintln("Compositor not available"_sv);
        return 1;
    }

    if (!shm) {
        dius::eprintln("Shared memory not available"_sv);
        return 1;
    }

    if (!shell) {
        dius::eprintln("Shell not available"_sv);
        return 1;
    }

    auto fd = (int) syscall(SYS_memfd_create, "buffer", 0);
    if (ftruncate(fd, size * 2) < 0) {
        dius::eprintln("Failed to alloc shared memory"_sv);
        return 1;
    }

    auto* data = (byte*) mmap(nullptr, size * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    auto bitmap1 = gfx::BitMap({ data, size }, width, height);
    auto bitmap2 = gfx::BitMap({ data + size, size }, width, height);
    draw(bitmap1);

    wl_shm_pool* pool = wl_shm_create_pool(shm, fd, size * 2);
    wl_buffer* buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_buffer* buffer2 = wl_shm_pool_create_buffer(pool, size, width, height, stride, WL_SHM_FORMAT_ARGB8888);

    wl_surface* surface = wl_compositor_create_surface(compositor);

    auto xdg_surface_listen = xdg_surface_listener {
        .configure =
            [](void*, xdg_surface* xdg_surface, uint32_t serial) {
                xdg_surface_ack_configure(xdg_surface, serial);
            },
    };

    xdg_surface* xdg_surface = xdg_wm_base_get_xdg_surface(shell, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listen, nullptr);

    static auto should_exit = false;
    auto xdg_toplevel_listen = xdg_toplevel_listener {
        .configure =
            [](void*, xdg_toplevel*, int32_t width, int32_t height, wl_array*) {
                dius::println("Configure: width={}, height={}"_sv, width, height);
            },
        .close =
            [](void*, xdg_toplevel*) {
                dius::println("Close"_sv);
                should_exit = true;
            },
        .configure_bounds = nullptr,
        .wm_capabilities = nullptr,
    };

    xdg_toplevel* toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(toplevel, &xdg_toplevel_listen, nullptr);

    xdg_toplevel_set_title(toplevel, "Hello, world!");
    wl_surface_commit(surface);

    wl_display_roundtrip(display);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);

    auto use2 = true;
    while (!should_exit) {
        wl_display_dispatch(display);

        if (use2) {
            draw(bitmap2);
            wl_surface_attach(surface, buffer2, 0, 0);
        } else {
            draw(bitmap1);
            wl_surface_attach(surface, buffer, 0, 0);
        }
        wl_surface_damage_buffer(surface, 0, 0, width, height);
        wl_surface_commit(surface);
        usleep(16000);
        use2 = !use2;
    }

    wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return 0;
}
