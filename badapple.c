#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <nes.h>

extern const uint8_t video_bin[];
extern const uint8_t video_bin_end[];

#define VIDEO_WIDTH_TILES 16
#define VIDEO_HEIGHT_TILES 12

#define SCREEN_WIDTH_TILES 32
#define SCREEN_HEIGHT_TILES 30

#define VIDEO_X ((SCREEN_WIDTH_TILES - VIDEO_WIDTH_TILES) / 2)
#define VIDEO_Y ((SCREEN_HEIGHT_TILES - VIDEO_HEIGHT_TILES) / 2)

#define VIDEO_BYTES_PER_ROW ((VIDEO_WIDTH_TILES + 7) / 8)
#define VIDEO_BYTES_PER_FRAME \
    (VIDEO_BYTES_PER_ROW * VIDEO_HEIGHT_TILES)

#define PPU_RESET_LATCH() ((void)PPU.status)

static uint8_t prev_frame[VIDEO_BYTES_PER_FRAME];

static void ppu_set_addr(uint16_t addr)
{
    PPU_RESET_LATCH();
    PPU.vram.address = (uint8_t)(addr >> 8);
    PPU.vram.address = (uint8_t)(addr & 0xFF);
}

static void load_chr_ram(void)
{
    uint8_t i;

    ppu_set_addr(0x0000);
    for (i = 0; i < 16; i++)
    {
        PPU.vram.data = 0x00;
    }

    ppu_set_addr(0x0010);
    for (i = 0; i < 16; i++)
    {
        PPU.vram.data = 0xFF;
    }
}

static void load_palette(void)
{
    ppu_set_addr(0x3F00);

    PPU.vram.data = 0x0F;
    PPU.vram.data = 0x0F;
    PPU.vram.data = 0x0F;
    PPU.vram.data = 0x30;
}

static void clear_nametable(void)
{
    uint8_t row;
    uint8_t col;

    for (row = 0; row < VIDEO_HEIGHT_TILES; ++row)
    {

        ppu_set_addr(
            0x2000 +
            (uint16_t)(VIDEO_Y + row) * 32 +
            VIDEO_X);

        for (col = 0; col < VIDEO_WIDTH_TILES; ++col)
        {
            PPU.vram.data = 0;
        }
    }
}

static void run_raw_frames(void)
{
    const uint8_t *vptr = video_bin;
    const uint8_t *vend = video_bin_end;

    const size_t frame_size =
        VIDEO_BYTES_PER_FRAME;

    size_t total_bytes;
    size_t frames;
    size_t f;

    const uint8_t *frame_start;
    const uint8_t *brow;

    uint8_t current_byte;
    uint8_t prev_byte;
    uint8_t diff;

    uint8_t bit;
    uint8_t tile;

    uint16_t addr;

    int row;
    int col;
    int byte_index;
    int bit_index;

    int delay_vblanks;

    if (vptr >= vend)
        return;

    total_bytes = (size_t)(vend - vptr);
    frames = total_bytes / frame_size;

    memset(prev_frame, 0xFF,
           sizeof(prev_frame));

    for (f = 0; f < frames; ++f)
    {

        frame_start =
            vptr + f * frame_size;

        waitvsync();

        PPU.mask = 0x00;

        for (row = 0;
             row < VIDEO_HEIGHT_TILES;
             ++row)
        {
            brow =
                frame_start +
                row * VIDEO_BYTES_PER_ROW;

            for (byte_index = 0;
                 byte_index <
                 VIDEO_BYTES_PER_ROW;
                 ++byte_index)
            {
                current_byte =
                    brow[byte_index];

                prev_byte =
                    prev_frame[row *
                                   VIDEO_BYTES_PER_ROW +
                               byte_index];

                diff =
                    current_byte ^
                    prev_byte;

                if (!diff)
                    continue;

                for (bit_index = 0;
                     bit_index < 8;
                     ++bit_index)
                {
                    if (!(diff &
                          (1 << (7 - bit_index))))
                    {
                        continue;
                    }

                    col =
                        byte_index * 8 +
                        bit_index;

                    if (col >=
                        VIDEO_WIDTH_TILES)
                    {
                        continue;
                    }

                    bit =
                        (current_byte >>
                         (7 - bit_index)) &
                        1;

                    tile =
                        bit ? 1 : 0;

                    addr =
                        0x2000 +
                        (uint16_t)(VIDEO_Y + row) * 32 +
                        VIDEO_X +
                        col;

                    ppu_set_addr(addr);
                    PPU.vram.data = tile;
                }

                prev_frame[row *
                               VIDEO_BYTES_PER_ROW +
                           byte_index] = current_byte;
            }
        }

        PPU_RESET_LATCH();
        PPU.scroll = 0x00;
        PPU.scroll = 0x00;

        PPU.control = 0x00;
        PPU.mask = 0x0A;

        for (delay_vblanks = 0;
             delay_vblanks < 7;
             ++delay_vblanks)
        {
            waitvsync();
        }
    }
}

void main(void)
{
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

    PPU_RESET_LATCH();
    PPU.scroll = 0x00;
    PPU.scroll = 0x00;

    PPU.mask = 0x0A;

    run_raw_frames();

    while (1)
    {
        waitvsync();
    }
}