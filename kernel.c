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

typedef struct registers {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern void gdt_flush(uint32_t);
extern void idt_flush(uint32_t);

extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

extern void irq0(); extern void irq1(); extern void irq2(); extern void irq3();
extern void irq4(); extern void irq5(); extern void irq6(); extern void irq7();
extern void irq8(); extern void irq9(); extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

#define COLOR_BG          0xFF0A1128
#define COLOR_PANEL       0xFF141E36
#define COLOR_HEADER      0xFF0B132B
#define COLOR_ACCENT      0xFF00D2FF
#define COLOR_BORDER      0xFF1E56A0
#define COLOR_BTN         0xFF1A2744
#define COLOR_BTN_BORDER  0xFF2D4B7D
#define COLOR_BTN_HOVER   0xFF263963
#define COLOR_BTN_ACTIVE  0xFF00A3FF
#define COLOR_PIN_ON      0xFF00F0FF
#define COLOR_PIN_OFF     0xFF0D1527
#define COLOR_WHITE       0xFFFFFFFF
#define COLOR_TEXT_MUTED  0xFF8A99AD
#define COLOR_TERM_BG     0xFF050814
#define COLOR_SUCCESS     0xFF00E676

static struct gdt_entry gdt[3];
static struct gdt_ptr   gp;
static struct idt_entry idt[256];
static struct idt_ptr   idtp;

static uint32_t* fb_front;
static uint32_t  back_buffer[1024 * 768];
static uint32_t  fb_pitch;
static uint32_t  fb_w = 1024;
static uint32_t  fb_h = 768;

static volatile int32_t mouse_x = 512;
static volatile int32_t mouse_y = 384;
static volatile uint8_t mouse_buttons = 0;
static volatile uint8_t prev_mouse_buttons = 0;

static uint8_t  gpio_state[16] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0};
static uint8_t  analyzer_running = 1;
static uint32_t system_ticks = 0;
static uint32_t tx_counter = 1042;
static uint8_t  log_cursor_blink = 0;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

static void init_gdt(void) {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base = (uint32_t)&gdt;
    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    gdt_flush((uint32_t)&gp);
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

static void init_pic(void) {
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0xF8);
    outb(0xA1, 0xEF);
}

static void init_idt(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0x08, 0);
    }

    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    init_pic();

    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);

    idt_flush((uint32_t)&idtp);
}

static void mouse_wait(uint8_t a_type) {
    uint32_t time_out = 100000;
    if (a_type == 0) {
        while (time_out--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (time_out--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

static void mouse_write(uint8_t a_write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, a_write);
}

static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

static void init_mouse(void) {
    uint8_t status;
    mouse_wait(1);
    outb(0x64, 0xA8);

    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = (inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    mouse_write(0xF6);
    mouse_read();

    mouse_write(0xF4);
    mouse_read();
}

static void init_timer(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void isr_handler(registers_t* r) {
    (void)r;
}

static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

void irq_handler(registers_t* r) {
    if (r->int_no == 32) {
        system_ticks++;
        if ((system_ticks % 30) == 0) {
            log_cursor_blink = !log_cursor_blink;
        }
        if (analyzer_running && (system_ticks % 6) == 0) {
            tx_counter++;
        }
    } else if (r->int_no == 44) {
        uint8_t status = inb(0x64);
        if (status & 0x01) {
            int8_t mouse_in = inb(0x60);
            if (mouse_cycle == 0) {
                if (mouse_in & 0x08) {
                    mouse_byte[0] = mouse_in;
                    mouse_cycle++;
                }
            } else if (mouse_cycle == 1) {
                mouse_byte[1] = mouse_in;
                mouse_cycle++;
            } else if (mouse_cycle == 2) {
                mouse_byte[2] = mouse_in;
                mouse_cycle = 0;

                mouse_buttons = mouse_byte[0] & 0x07;

                int32_t dx = mouse_byte[1];
                int32_t dy = mouse_byte[2];

                if (mouse_byte[0] & 0x10) dx |= 0xFFFFFF00;
                if (mouse_byte[0] & 0x20) dy |= 0xFFFFFF00;

                mouse_x += dx;
                mouse_y -= dy;

                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x >= (int32_t)fb_w) mouse_x = fb_w - 1;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y >= (int32_t)fb_h) mouse_y = fb_h - 1;
            }
        }
    }

    if (r->int_no >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

static const uint8_t font8x8[128][8] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['!'] = {0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00},
    ['#'] = {0x24, 0x7E, 0x24, 0x24, 0x7E, 0x24, 0x00, 0x00},
    ['%'] = {0x00, 0x62, 0x64, 0x08, 0x10, 0x26, 0x46, 0x00},
    ['('] = {0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00},
    [')'] = {0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00},
    ['+'] = {0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00},
    [','] = {0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00},
    ['-'] = {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00},
    ['.'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
    ['/'] = {0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00},
    [':'] = {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00},
    ['='] = {0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00},
    ['['] = {0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},
    [']'] = {0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00},
    ['~'] = {0x00, 0x32, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00},
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
    ['k'] = {0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00},
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

static void buf_pixel(int x, int y, uint32_t c) {
    if (x >= 0 && x < (int)fb_w && y >= 0 && y < (int)fb_h) {
        back_buffer[y * fb_w + x] = c;
    }
}

static void buf_rect(int x, int y, int w, int h, uint32_t c) {
    for (int j = y; j < y + h; ++j) {
        for (int i = x; i < x + w; ++i) {
            buf_pixel(i, j, c);
        }
    }
}

static void buf_rect_outline(int x, int y, int w, int h, uint32_t c) {
    for (int i = x; i < x + w; ++i) {
        buf_pixel(i, y, c);
        buf_pixel(i, y + h - 1, c);
    }
    for (int j = y; j < y + h; ++j) {
        buf_pixel(x, j, c);
        buf_pixel(x + w - 1, j, c);
    }
}

static void buf_char(int x, int y, char c, uint32_t col) {
    uint8_t uc = (uint8_t)c;
    if (uc > 127) return;
    const uint8_t* glyph = font8x8[uc];
    for (int row = 0; row < 8; ++row) {
        for (int col_i = 0; col_i < 8; ++col_i) {
            if ((glyph[row] >> (7 - col_i)) & 1) {
                buf_pixel(x + col_i, y + row, col);
            }
        }
    }
}

static void buf_text(int x, int y, const char* s, uint32_t col) {
    while (*s) {
        buf_char(x, y, *s, col);
        x += 8;
        s++;
    }
}

static void int_to_str(uint32_t val, char* buf) {
    char temp[12];
    int i = 0;
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (val > 0) {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

static uint8_t point_in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return (px >= rx && px < rx + rw && py >= ry && py < ry + rh);
}

static uint8_t render_button(int x, int y, int w, int h, const char* label, uint8_t is_active, uint8_t click_event) {
    uint8_t hovered = point_in_rect(mouse_x, mouse_y, x, y, w, h);
    uint32_t bg = is_active ? COLOR_BTN_ACTIVE : (hovered ? COLOR_BTN_HOVER : COLOR_BTN);
    uint32_t border = hovered ? COLOR_ACCENT : COLOR_BTN_BORDER;

    buf_rect(x + 2, y + 2, w, h, 0xFF04060E);
    buf_rect(x, y, w, h, bg);
    buf_rect_outline(x, y, w, h, border);

    int len = 0;
    const char* p = label;
    while (*p++) len++;

    int tx = x + (w - (len * 8)) / 2;
    int ty = y + (h - 8) / 2;
    buf_text(tx, ty, label, is_active ? COLOR_HEADER : (hovered ? COLOR_ACCENT : COLOR_WHITE));

    return (hovered && click_event);
}

static void draw_window(int x, int y, int w, int h, const char* title) {
    buf_rect(x + 4, y + 4, w, h, 0xFF04060E);
    buf_rect(x, y, w, h, COLOR_PANEL);
    buf_rect_outline(x, y, w, h, COLOR_BORDER);
    buf_rect(x, y, w, 28, COLOR_HEADER);
    buf_rect_outline(x, y, w, 28, COLOR_BORDER);
    buf_rect(x + 8, y + 8, 12, 12, COLOR_ACCENT);
    buf_text(x + 28, y + 10, title, COLOR_WHITE);
}

static const uint8_t cursor_mask[16][12] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,1,1,1,1,0,0},
    {1,2,2,1,2,2,1,0,0,0,0,0},
    {1,2,1,0,1,2,2,1,0,0,0,0},
    {1,1,0,0,1,2,2,1,0,0,0,0},
    {1,0,0,0,0,1,2,2,1,0,0,0},
    {0,0,0,0,0,1,2,2,1,0,0,0},
    {0,0,0,0,0,0,1,1,0,0,0,0}
};

static void draw_cursor(int x, int y) {
    for (int j = 0; j < 16; ++j) {
        for (int i = 0; i < 12; ++i) {
            uint8_t p = cursor_mask[j][i];
            if (p == 1) {
                buf_pixel(x + i, y + j, 0xFF000000);
            } else if (p == 2) {
                buf_pixel(x + i, y + j, COLOR_ACCENT);
            }
        }
    }
}

static void render_gui(uint8_t click_now) {
    buf_rect(0, 0, fb_w, fb_h, COLOR_BG);

    buf_rect(0, 0, fb_w, 32, COLOR_HEADER);
    buf_rect_outline(0, 0, fb_w, 32, COLOR_BORDER);
    buf_rect(10, 9, 14, 14, COLOR_ACCENT);
    buf_text(32, 12, "AMILUNA OS - REALTIME EMBEDDED SYSTEM", COLOR_WHITE);
    buf_text(fb_w - 290, 12, "SALVATORE BONPENSIERO", COLOR_ACCENT);

    draw_window(24, 52, 440, 280, "VIRTUAL GPIO CONTROLLER (CLICK PINS)");

    for (int i = 0; i < 16; ++i) {
        int px = 24 + 20 + (i % 8) * 50;
        int py = 52 + 45 + (i / 8) * 90;

        uint8_t hovered = point_in_rect(mouse_x, mouse_y, px, py, 42, 42);
        if (hovered && click_now) {
            gpio_state[i] = !gpio_state[i];
        }

        uint32_t pin_col = gpio_state[i] ? COLOR_PIN_ON : COLOR_PIN_OFF;
        buf_rect(px, py, 42, 42, pin_col);
        buf_rect_outline(px, py, 42, 42, hovered ? COLOR_WHITE : COLOR_BORDER);

        if (gpio_state[i]) {
            buf_rect(px + 14, py + 14, 14, 14, COLOR_WHITE);
        }

        char pstr[5];
        pstr[0] = 'P';
        if (i < 10) {
            pstr[1] = '0' + i;
            pstr[2] = '\0';
        } else {
            pstr[1] = '1';
            pstr[2] = '0' + (i - 10);
            pstr[3] = '\0';
        }
        buf_text(px + 8, py + 48, pstr, gpio_state[i] ? COLOR_ACCENT : COLOR_TEXT_MUTED);
    }

    if (render_button(44, 285, 95, 28, "ALL ON", 0, click_now)) {
        for (int i = 0; i < 16; i++) gpio_state[i] = 1;
    }
    if (render_button(149, 285, 95, 28, "ALL OFF", 0, click_now)) {
        for (int i = 0; i < 16; i++) gpio_state[i] = 0;
    }
    if (render_button(254, 285, 95, 28, "INVERT", 0, click_now)) {
        for (int i = 0; i < 16; i++) gpio_state[i] = !gpio_state[i];
    }
    if (render_button(359, 285, 85, 28, "RESET", 0, click_now)) {
        for (int i = 0; i < 16; i++) gpio_state[i] = (i % 3 == 0);
    }

    draw_window(480, 52, 520, 280, "BLUETRACE REAL-TIME LOGIC ANALYZER");

    int gx = 496;
    int gy = 92;
    int gw = 488;
    int gh = 150;

    buf_rect(gx, gy, gw, gh, COLOR_TERM_BG);
    buf_rect_outline(gx, gy, gw, gh, COLOR_BORDER);

    const char* channels[3] = {"CH0:UART-TX", "CH1:SPI-SCK", "CH2:I2C-SDA"};
    uint32_t sig_colors[3] = {COLOR_ACCENT, 0xFF38EF7D, 0xFFFF7675};

    for (int ch = 0; ch < 3; ++ch) {
        int cy = gy + 14 + ch * 46;
        buf_text(gx + 8, cy, channels[ch], sig_colors[ch]);

        int wave_y = cy + 12;
        for (int i = 110; i < gw - 8; i += 6) {
            buf_pixel(gx + i, wave_y + 16, 0xFF142036);
        }

        uint8_t is_high = 0;
        int speed_shift = analyzer_running ? ((system_ticks * (ch + 1)) % 32) : 0;
        int step = 16 + ch * 12;

        for (int i = 110; i < gw - 9; ++i) {
            int cur_x = i + speed_shift;
            if ((cur_x % step) == 0) {
                is_high = !is_high;
                for (int v = 0; v <= 16; ++v) {
                    buf_pixel(gx + i, wave_y + v, sig_colors[ch]);
                }
            }
            buf_pixel(gx + i, wave_y + (is_high ? 0 : 16), sig_colors[ch]);
        }
    }

    if (render_button(496, 285, 110, 28, "START RUN", analyzer_running, click_now)) {
        analyzer_running = 1;
    }
    if (render_button(616, 285, 110, 28, "PAUSE", !analyzer_running, click_now)) {
        analyzer_running = 0;
    }
    if (render_button(736, 285, 110, 28, "CLR TRIG", 0, click_now)) {
        tx_counter = 0;
    }
    if (render_button(856, 285, 110, 28, "DUMP PCAP", 0, click_now)) {
        tx_counter += 100;
    }

    draw_window(24, 350, 976, 390, "SERIAL TELEMETRY & EMULATION BUS [/dev/vbus0]");

    int tx = 40;
    int ty = 390;
    int tw = 944;
    int th = 334;

    buf_rect(tx, ty, tw, th, COLOR_TERM_BG);
    buf_rect_outline(tx, ty, tw, th, COLOR_BORDER);

    buf_text(tx + 14, ty + 14, "[BOOT] Amiluna RT-Kernel 2.0.0 (x86-32 Protected Mode) initialized.", COLOR_SUCCESS);
    buf_text(tx + 14, ty + 32, "[GDT/IDT] Global Descriptor and Interrupt Descriptor Tables loaded.", COLOR_WHITE);
    buf_text(tx + 14, ty + 50, "[IRQ] Dual 8259 PIC remapped to vectors 0x20..0x2F.", COLOR_WHITE);
    buf_text(tx + 14, ty + 68, "[DRV] PS/2 Auxiliary Mouse Controller online (IRQ12 streaming active).", COLOR_ACCENT);
    buf_text(tx + 14, ty + 86, "[DRV] 8253 PIT Timer synchronized at 100 Hz (IRQ0 tick count).", COLOR_WHITE);

    char tick_str[32] = "[TIME] Uptime System Ticks: ";
    char num_buf[16];
    int_to_str(system_ticks, num_buf);
    int p = 0;
    while (tick_str[p]) p++;
    int q = 0;
    while (num_buf[q]) tick_str[p++] = num_buf[q++];
    tick_str[p] = '\0';
    buf_text(tx + 14, ty + 104, tick_str, COLOR_TEXT_MUTED);

    char tx_str[48] = "[UART] Frames Transmitted: ";
    int_to_str(tx_counter, num_buf);
    p = 0;
    while (tx_str[p]) p++;
    q = 0;
    while (num_buf[q]) tx_str[p++] = num_buf[q++];
    tx_str[p] = '\0';
    buf_text(tx + 14, ty + 122, tx_str, COLOR_ACCENT);

    char mouse_pos_str[48] = "[INPUT] Pointer X: ";
    int_to_str(mouse_x, num_buf);
    p = 0;
    while (mouse_pos_str[p]) p++;
    q = 0;
    while (num_buf[q]) mouse_pos_str[p++] = num_buf[q++];
    mouse_pos_str[p++] = ' '; mouse_pos_str[p++] = 'Y'; mouse_pos_str[p++] = ':'; mouse_pos_str[p++] = ' ';
    int_to_str(mouse_y, num_buf);
    q = 0;
    while (num_buf[q]) mouse_pos_str[p++] = num_buf[q++];
    mouse_pos_str[p] = '\0';
    buf_text(tx + 14, ty + 140, mouse_pos_str, COLOR_WHITE);

    buf_text(tx + 14, ty + 168, "amiluna-root@host:~# vbus-monitor --stream --interactive", COLOR_ACCENT);

    if (log_cursor_blink) {
        buf_rect(tx + 14 + 56 * 8, ty + 168, 8, 10, COLOR_WHITE);
    }

    draw_cursor(mouse_x, mouse_y);

    uint32_t* src = back_buffer;
    uint32_t* dst = fb_front;
    int total_pixels = fb_w * fb_h;
    while (total_pixels--) {
        *dst++ = *src++;
    }
}

void kmain(uint32_t magic, multiboot_info_t* mbi) {
    if (magic != 0x2BADB002 || !(mbi->flags & (1 << 12))) {
        return;
    }

    fb_front = (uint32_t*)(uintptr_t)mbi->framebuffer_addr;
    fb_pitch = mbi->framebuffer_pitch;
    fb_w = mbi->framebuffer_width;
    fb_h = mbi->framebuffer_height;

    init_gdt();
    init_idt();
    init_timer(100);
    init_mouse();

    __asm__ __volatile__("sti");

    while (1) {
        uint8_t click_now = (mouse_buttons & 1) && !(prev_mouse_buttons & 1);
        prev_mouse_buttons = mouse_buttons;

        render_gui(click_now);
    }
}
