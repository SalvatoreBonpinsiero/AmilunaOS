#include <stdint.h>

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
} __attribute__((packed)) multiboot_info_t;

#define COLOR_BG          0xFF0A1128
#define COLOR_PANEL       0xFF1C2541
#define COLOR_HEADER      0xFF0B132B
#define COLOR_ACCENT      0xFF00D2FF
#define COLOR_BORDER      0xFF1E56A0
#define COLOR_BTN         0xFF16203B
#define COLOR_BTN_BORDER  0xFF2A4374
#define COLOR_BTN_ACTIVE  0xFF00D2FF
#define COLOR_PIN_ON      0xFF00F0FF
#define COLOR_PIN_OFF     0xFF0F172A
#define COLOR_WHITE       0xFFFFFFFF
#define COLOR_TERM_BG     0xFF050814

static uint32_t* fb;
static uint32_t fb_pitch;
static uint32_t fb_w;
static uint32_t fb_h;

static const uint8_t font8x8_basic[128][8] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['-'] = {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00},
    ['/'] = {0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00},
    ['['] = {0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},
    [']'] = {0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00},
    ['0'] = {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00},
    ['1'] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00},
    ['2'] = {0x3C, 0x66, 0x06, 0x0C, 0x30, 0x60, 0x7E, 0x00},
    ['3'] = {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00},
    ['4'] = {0x0C, 0x1C, 0x34, 0x64, 0x7E, 0x04, 0x04, 0x00},
    ['5'] = {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00},
    ['6'] = {0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00},
    ['7'] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00},
    ['8'] = {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00},
    ['9'] = {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00},
    [':'] = {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00},
    ['A'] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
    ['B'] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00},
    ['C'] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00},
    ['D'] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00},
    ['E'] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00},
    ['F'] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00},
    ['G'] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3E, 0x00},
    ['H'] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
    ['I'] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['J'] = {0x0E, 0x06, 0x06, 0x06, 0x06, 0x66, 0x3C, 0x00},
    ['K'] = {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00},
    ['L'] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00},
    ['M'] = {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00},
    ['N'] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00},
    ['O'] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['P'] = {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00},
    ['Q'] = {0x3C, 0x66, 0x66, 0x66, 0x6A, 0x6C, 0x36, 0x00},
    ['R'] = {0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x00},
    ['S'] = {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00},
    ['T'] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    ['U'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['V'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00},
    ['W'] = {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},
    ['X'] = {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00},
    ['Y'] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00},
    ['Z'] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00},
    ['a'] = {0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00},
    ['b'] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00},
    ['c'] = {0x00, 0x00, 0x3C, 0x60, 0x60, 0x66, 0x3C, 0x00},
    ['d'] = {0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00},
    ['e'] = {0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00},
    ['f'] = {0x0E, 0x18, 0x7C, 0x18, 0x18, 0x18, 0x18, 0x00},
    ['g'] = {0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x7C},
    ['h'] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00},
    ['i'] = {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['l'] = {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['m'] = {0x00, 0x00, 0x76, 0x69, 0x69, 0x69, 0x69, 0x00},
    ['n'] = {0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00},
    ['o'] = {0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['p'] = {0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60},
    ['r'] = {0x00, 0x00, 0x6C, 0x76, 0x60, 0x60, 0x60, 0x00},
    ['s'] = {0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00},
    ['t'] = {0x18, 0x18, 0x7E, 0x18, 0x18, 0x18, 0x0E, 0x00},
    ['u'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00},
    ['v'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00},
    ['w'] = {0x00, 0x00, 0x63, 0x6B, 0x6B, 0x7F, 0x36, 0x00},
    ['x'] = {0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00},
    ['y'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C},
    ['z'] = {0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00}
};

static void putpixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < (int)fb_w && y >= 0 && y < (int)fb_h) {
        fb[y * (fb_pitch / 4) + x] = color;
    }
}

static void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int j = y; j < y + h; ++j) {
        for (int i = x; i < x + w; ++i) {
            putpixel(i, j, color);
        }
    }
}

static void draw_rect_outline(int x, int y, int w, int h, uint32_t color) {
    for (int i = x; i < x + w; ++i) {
        putpixel(i, y, color);
        putpixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; ++j) {
        putpixel(x, j, color);
        putpixel(x + w - 1, j, color);
    }
}

static void draw_char(int x, int y, char c, uint32_t color) {
    uint8_t uc = (uint8_t)c;
    if (uc > 127) return;
    const uint8_t* glyph = font8x8_basic[uc];
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if ((glyph[row] >> (7 - col)) & 1) {
                putpixel(x + col, y + row, color);
            }
        }
    }
}

static void draw_text(int x, int y, const char* str, uint32_t color) {
    while (*str) {
        draw_char(x, y, *str, color);
        x += 8;
        str++;
    }
}

static void draw_button(int x, int y, int w, int h, const char* label, uint8_t active) {
    draw_rect(x + 2, y + 2, w, h, 0xFF050814);
    draw_rect(x, y, w, h, active ? COLOR_BTN_ACTIVE : COLOR_BTN);
    draw_rect_outline(x, y, w, h, active ? COLOR_WHITE : COLOR_BTN_BORDER);

    int len = 0;
    const char* p = label;
    while (*p++) len++;

    int tx = x + (w - (len * 8)) / 2;
    int ty = y + (h - 8) / 2;
    draw_text(tx, ty, label, active ? COLOR_HEADER : COLOR_WHITE);
}

static void draw_window(int x, int y, int w, int h, const char* title) {
    draw_rect(x + 4, y + 4, w, h, 0xFF050814);
    draw_rect(x, y, w, h, COLOR_PANEL);
    draw_rect_outline(x, y, w, h, COLOR_BORDER);
    draw_rect(x, y, w, 26, COLOR_HEADER);
    draw_rect_outline(x, y, w, 26, COLOR_BORDER);
    draw_rect(x + 8, y + 7, 12, 12, COLOR_ACCENT);
    draw_text(x + 28, y + 9, title, COLOR_WHITE);
}

static void draw_gpio_matrix(int x, int y) {
    draw_window(x, y, 440, 270, "GPIO MATRIX CONTROLLER");

    for (int i = 0; i < 16; ++i) {
        int px = x + 20 + (i % 8) * 50;
        int py = y + 42 + (i / 8) * 85;

        uint8_t is_on = (i == 1 || i == 4 || i == 7 || i == 11 || i == 14);
        draw_rect(px, py, 42, 42, is_on ? COLOR_PIN_ON : COLOR_PIN_OFF);
        draw_rect_outline(px, py, 42, 42, COLOR_BORDER);

        if (is_on) {
            draw_rect(px + 14, py + 14, 14, 14, COLOR_WHITE);
        }

        char pnum[4];
        pnum[0] = 'P';
        if (i < 10) {
            pnum[1] = '0' + i;
            pnum[2] = '\0';
        } else {
            pnum[1] = '1';
            pnum[2] = '0' + (i - 10);
            pnum[3] = '\0';
        }
        draw_text(px + 8, py + 48, pnum, COLOR_ACCENT);
    }

    draw_button(x + 20, y + 225, 95, 28, "ALL ON", 0);
    draw_button(x + 125, y + 225, 95, 28, "ALL OFF", 0);
    draw_button(x + 230, y + 225, 95, 28, "PULL-UP", 1);
    draw_button(x + 335, y + 225, 85, 28, "RESET", 0);
}

static void draw_logic_analyzer(int x, int y, int w, int h) {
    draw_window(x, y, w, h, "BLUETRACE LOGIC ANALYZER");

    int gx = x + 16;
    int gy = y + 40;
    int gw = w - 32;
    int gh = 150;

    draw_rect(gx, gy, gw, gh, COLOR_TERM_BG);
    draw_rect_outline(gx, gy, gw, gh, COLOR_BORDER);

    const char* channels[3] = {"CH0:UART-TX", "CH1:SPI-SCK", "CH2:I2C-SDA"};
    for (int ch = 0; ch < 3; ++ch) {
        int cy = gy + 12 + ch * 45;
        draw_text(gx + 8, cy, channels[ch], COLOR_ACCENT);

        int sig_y = cy + 12;
        for (int i = 110; i < gw - 8; i += 6) {
            putpixel(gx + i, sig_y + 16, 0xFF142240);
        }

        int high = 0;
        int step = (ch + 1) * 20;
        for (int i = 110; i < gw - 9; ++i) {
            if ((i % step) == 0) {
                high = !high;
                for (int v = 0; v <= 16; ++v) {
                    putpixel(gx + i, sig_y + v, COLOR_ACCENT);
                }
            }
            putpixel(gx + i, sig_y + (high ? 0 : 16), COLOR_ACCENT);
        }
    }

    draw_button(x + 16, y + 205, 100, 28, "START", 1);
    draw_button(x + 124, y + 205, 100, 28, "STOP", 0);
    draw_button(x + 232, y + 205, 100, 28, "TRIGGER", 0);
    draw_button(x + 340, y + 205, 100, 28, "ZOOM +", 0);
    draw_button(x + 448, y + 205, 100, 28, "EXPORT", 0);
}

static void draw_terminal(int x, int y, int w, int h) {
    draw_window(x, y, w, h, "SERIAL TELEMETRY & EMULATION BUS [/dev/vbus0]");

    int tx = x + 16;
    int ty = y + 38;
    int tw = w - 32;
    int th = h - 54;

    draw_rect(tx, ty, tw, th, COLOR_TERM_BG);
    draw_rect_outline(tx, ty, tw, th, COLOR_BORDER);

    draw_text(tx + 12, ty + 12, "[INIT] Amiluna RT-Kernel 1.0.0-PROT loaded.", 0xFF68D391);
    draw_text(tx + 12, ty + 28, "[VBUS] Peripheral bridge mapped to I/O space 0x03F8.", COLOR_WHITE);
    draw_text(tx + 12, ty + 44, "[UART] Baudrate set to 115200 bps (8N1).", COLOR_WHITE);
    draw_text(tx + 12, ty + 60, "[SPI0] Master controller ready at 10.0 MHz.", COLOR_WHITE);
    draw_text(tx + 12, ty + 76, "[I2C0] Virtual wire slave ACK simulation: ENABLED.", COLOR_ACCENT);
    draw_text(tx + 12, ty + 92, "[WARN] Pin P04 frequency spike: 24.2 MHz.", 0xFFF6E05E);
    draw_text(tx + 12, ty + 116, "amiluna-root@host:~# vbus-probe --target=stm32f4", COLOR_ACCENT);

    draw_rect(tx + 12 + 48 * 8, ty + 116, 8, 10, COLOR_WHITE);
}

void kmain(uint32_t magic, multiboot_info_t* mbi) {
    if (magic != 0x2BADB002 || !(mbi->flags & (1 << 12))) {
        return;
    }

    fb = (uint32_t*)(uintptr_t)mbi->framebuffer_addr;
    fb_pitch = mbi->framebuffer_pitch;
    fb_w = mbi->framebuffer_width;
    fb_h = mbi->framebuffer_height;

    draw_rect(0, 0, fb_w, fb_h, COLOR_BG);

    draw_rect(0, 0, fb_w, 30, COLOR_HEADER);
    draw_rect_outline(0, 0, fb_w, 30, COLOR_BORDER);
    draw_rect(10, 8, 14, 14, COLOR_ACCENT);
    draw_text(32, 11, "AMILUNA OS - EMBEDDED DEV WORKSTATION", COLOR_WHITE);
    draw_text(fb_w - 290, 11, "SALVATORE BONPENSIERO", COLOR_ACCENT);

    draw_gpio_matrix(24, 48);
    draw_logic_analyzer(480, 48, 520, 270);
    draw_terminal(24, 336, 976, 400);

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
