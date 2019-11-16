#include <fcntl.h>
#include <graphics/font.h>
#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <liim/pointers.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

uint8_t a[][5] = {
    { 0x00, 0x00, 0xFF, 0x00, 0x00, },
    { 0x00, 0xFF, 0x00, 0xFF, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF  }
};

uint8_t b[][5] = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0x00  }
};

uint8_t c[][5] = {
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF  }
};

uint8_t d[][5] = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0x00  }
};

uint8_t e[][5] = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  }
};

uint8_t f[][5] = {
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00  }
};

uint8_t g[][5] = {
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0x00, 0x00, },
    { 0xFF, 0x00, 0x00, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0x00, 0xFF, 0xFF, 0xFF, 0x00  }
};

uint8_t h[][5] = {
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF  }
};

uint8_t i[][5] = {
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF, },
    { 0xFF, 0x00, 0x00, 0x00, 0xFF  }
};

int main()
{
    int fb = open("/dev/fb0", O_RDWR);
    if (fb == -1) {
        perror("open");
        return 1;
    }

    screen_res sz = { 1024, 768 };
    if (ioctl(fb, SSRES, &sz) == -1) {
        perror("ioctl");
        return 1;
    }

    uint32_t *raw_pixels = static_cast<uint32_t*>(mmap(NULL, sz.x * sz.y * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0));
    if (raw_pixels == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    memset(raw_pixels, 0, sz.y * sz.x * sizeof(uint32_t));

    auto pixels = PixelBuffer::wrap(raw_pixels, sz.x, sz.y);
    auto renderer = Renderer(pixels);
    Font font;

    renderer.set_color(Color(255, 0, 0));
    renderer.fill_rect(200, 200, 50, 50);

    renderer.set_color(Color(255, 255, 255));
    char a[128];
    memset(a, 0, 128);
    for (int i = 0; i < 128; i++) {
        a[i] = i + 1;
    }

    renderer.render_text(50, 50, a);

    renderer.render_text(150, 150, "Hello World!");

    return 0;
}