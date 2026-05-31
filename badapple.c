#include <stdint.h>
#include <stddef.h>
#include <nes.h>

#define CMD_WAIT      0x00
#define VIDEO_WIDTH_TILES   RAW_VIDEO_WIDTH_PIXELS
#define VIDEO_HEIGHT_TILES  RAW_VIDEO_HEIGHT_PIXELS

#define SCREEN_WIDTH_TILES  32
#define SCREEN_HEIGHT_TILES 30

#define FRAME_WIDTH_TILES   (VIDEO_WIDTH_TILES + 2)
#define FRAME_HEIGHT_TILES  (VIDEO_HEIGHT_TILES + 2)

#define VIDEO_OFFSET_X 0
#define VIDEO_OFFSET_Y 1

#define FRAME_X (((SCREEN_WIDTH_TILES - FRAME_WIDTH_TILES) / 2) + 1 + (VIDEO_OFFSET_X))
#define FRAME_Y ((SCREEN_HEIGHT_TILES - FRAME_HEIGHT_TILES) / 2 + (VIDEO_OFFSET_Y))

#define VIDEO_X (FRAME_X + 1)
#define VIDEO_Y (FRAME_Y + 1)


extern const uint8_t video_bin[];
extern const uint8_t video_bin_end[];

#define RAW_VIDEO_WIDTH_PIXELS 16
#define RAW_VIDEO_HEIGHT_PIXELS 12
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

    ppu_set_addr(base + (FRAME_HEIGHT_TILES - 1) * 32);
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

static void run_raw_frames(void) {
    const uint8_t* vptr = video_bin;
    const uint8_t* vend = video_bin_end;

    const int width = RAW_VIDEO_WIDTH_PIXELS;
    const int height = RAW_VIDEO_HEIGHT_PIXELS;
    const int bytes_per_row = (width + 7) / 8;
    const int frame_size = bytes_per_row * height;
    size_t total_bytes;
    size_t frames;
    size_t f;

    const uint8_t* frame_start;
    const uint8_t* brow;
    int row;
    int col;
    int byte_index;
    int bit_index;
    uint8_t bit;
    uint16_t addr;
    int delay_vblanks;

    if (vptr >= vend) return;

    total_bytes = (size_t)(vend - vptr);
    frames = total_bytes / frame_size;

    for (f = 0; f < frames; ++f) {
        waitvsync();
        PPU.mask = 0x00;

        frame_start = vptr + f * frame_size;

        for (row = 0; row < height; ++row) {
            addr = 0x2000 + row * 32;
            addr = center_nametable_addr(addr);
            ppu_set_addr(addr);

            brow = frame_start + row * bytes_per_row;

            for (col = 0; col < width; ++col) {
                byte_index = col / 8;
                bit_index = 7 - (col & 7);
                bit = (brow[byte_index] >> bit_index) & 1;
                PPU.vram.data = bit ? 1 : 0;
            }
        }

        waitvsync();
        PPU_RESET_LATCH();
        PPU.scroll = 0x04;
        PPU.scroll = 0x02;
        PPU.control = 0x00;
        PPU.mask = 0x0A;

        for (delay_vblanks = 0; delay_vblanks < 6; ++delay_vblanks) {
            waitvsync();
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

    run_raw_frames();

    while (1) {
        waitvsync();
    }
}