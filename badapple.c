#include <stdint.h>
#include <nes.h>

#define CMD_WAIT      0x00
#define CMD_PPU_WRITE 0x01
#define CMD_PPU_OFF   0x02
#define CMD_PPU_ON    0x03
#define CMD_END       0xFF

#define VIDEO_WIDTH_TILES   14
#define VIDEO_HEIGHT_TILES  10

#define SCREEN_WIDTH_TILES  32
#define SCREEN_HEIGHT_TILES 30

#define FRAME_WIDTH_TILES   (VIDEO_WIDTH_TILES -2)
#define FRAME_HEIGHT_TILES  (VIDEO_HEIGHT_TILES)

#define FRAME_X ((SCREEN_WIDTH_TILES - FRAME_WIDTH_TILES) / 2) + 1
#define FRAME_Y ((SCREEN_HEIGHT_TILES - FRAME_HEIGHT_TILES) / 2)

#define VIDEO_X (FRAME_X + 1)
#define VIDEO_Y (FRAME_Y + 1)

#define VIDEO_OFFSET_X 0
#define VIDEO_OFFSET_Y -1

extern const uint8_t badapplevid_bin[];

static const uint8_t* stream_src;
static uint8_t stream_literal_left;
static uint8_t stream_repeat_left;
static uint8_t stream_repeat_value;
static uint8_t ppu_enabled = 0;

#define PPU_RESET_LATCH() ((void)PPU.status)

static void ppu_set_addr(uint16_t addr) {
    PPU_RESET_LATCH();
    PPU.vram.address = (uint8_t)(addr >> 8);
    PPU.vram.address = (uint8_t)(addr & 0xFF);
}

static void load_chr_ram(void) {
    uint8_t i;

    ppu_set_addr(0x0000);
    for (i = 0; i < 16; i++) {
        PPU.vram.data = 0x00;
    }

    ppu_set_addr(0x0010);
    for (i = 0; i < 16; i++) {
        PPU.vram.data = 0xFF;
    }

    ppu_set_addr(0x0020);
    for (i = 0; i < 8; i++) {
        PPU.vram.data = 0xFF;
    }
    for (i = 0; i < 8; i++) {
        PPU.vram.data = 0x00;
    }
}

static void load_palette(void) {
    ppu_set_addr(0x3F00);
    PPU.vram.data = 0x0F;
    PPU.vram.data = 0x10;
    PPU.vram.data = 0x0F;
    PPU.vram.data = 0x30;
}

static void clear_nametable(void) {
    uint16_t i;

    ppu_set_addr(0x2000);

    for (i = 0; i < 960; i++) {
        PPU.vram.data = 0x00;
    }

    for (i = 0; i < 64; i++) {
        PPU.vram.data = 0x00;
    }
}

static void draw_border(void) {
    uint8_t i;
    uint16_t base = 0x2000 + FRAME_Y * 32 + FRAME_X;

    ppu_set_addr(base);
    for (i = 0; i < FRAME_WIDTH_TILES; i++) {
        PPU.vram.data = 1;
    }

    ppu_set_addr(base + (FRAME_HEIGHT_TILES ) * 32);
    for (i = 0; i < FRAME_WIDTH_TILES; i++) {
        PPU.vram.data = 1;
    }

    for (i = 0; i < VIDEO_HEIGHT_TILES; i++) {
        ppu_set_addr(base + (1 + i) * 32);
        PPU.vram.data = 1;

        ppu_set_addr(base + (1 + i) * 32 + FRAME_WIDTH_TILES - 1);
        PPU.vram.data = 1;
    }
}

static uint16_t center_nametable_addr(uint16_t addr) {
    if (addr >= 0x2000 && addr < 0x23C0) {
        uint16_t offset = addr - 0x2000;
        uint8_t x = offset % 32;
        uint8_t y = offset / 32;

        addr = 0x2000 + (y + VIDEO_Y) * 32 + (x + VIDEO_X);
    }

    return addr;
}

static uint8_t readVid(void) {
    int8_t control;

    for (;;) {
        if (stream_repeat_left) {
            stream_repeat_left--;
            return stream_repeat_value;
        }

        if (stream_literal_left) {
            stream_literal_left--;
            return *stream_src++;
        }

        control = (int8_t)(*stream_src++);

        if (control >= 0) {
            stream_literal_left = (uint8_t)control;
            return *stream_src++;
        }

        if (control != -128) {
            stream_repeat_left = (uint8_t)(1 - control);
            stream_repeat_value = *stream_src++;
            stream_repeat_left--;
            return stream_repeat_value;
        }
    }
}

static void run_stream(void) {
    uint8_t cmd;
    uint8_t n;
    uint8_t len;
    uint8_t i;
    uint16_t addr;

    stream_src = badapplevid_bin;
    stream_literal_left = 0;
    stream_repeat_left = 0;
    stream_repeat_value = 0;

    for (;;) {
        cmd = readVid();

        switch (cmd) {
            case CMD_END:
                return;

            case CMD_WAIT:
                n = readVid();
                while (n--) {
                    waitvsync();
                }
                break;

            case CMD_PPU_WRITE:
                if (ppu_enabled) {
                    waitvsync(); 
                    PPU.mask = 0x00;
                    ppu_enabled = 0;
                }
                
                addr = ((uint16_t)readVid() << 8) | readVid();
                len = readVid();

                addr = center_nametable_addr(addr);
                ppu_set_addr(addr);

                for (i = 0; i < len; i++) {
                    PPU.vram.data = readVid();
                }
                break;

            case CMD_PPU_OFF:
                waitvsync(); 
                PPU.mask = 0x00;
                ppu_enabled = 0;
                break;

            case CMD_PPU_ON:
                waitvsync(); 
                PPU_RESET_LATCH();
                PPU.scroll = 0x04;
                PPU.scroll = 0x02;
                PPU.control = 0x00;
                PPU.mask = 0x0A;
                ppu_enabled = 1;
                break;

            default:
                return;
        }
    }
}

void main(void) {
    waitvsync();
    waitvsync();

    PPU.mask = 0x00;
    PPU.control = 0x00;

    PPU_RESET_LATCH();
    PPU.scroll = 0x00;
    PPU.scroll = 0x00;

    load_chr_ram();
    load_palette();
    clear_nametable();
    draw_border();

    PPU_RESET_LATCH();
    PPU.scroll = 0x00;
    PPU.scroll = 0x00;

    run_stream();

    while (1) {
        waitvsync();
    }
}