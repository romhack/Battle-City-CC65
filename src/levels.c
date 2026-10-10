#include "../lib/neslib.h"
#include "globals.h" 
#include "../game.h"
#include "util.h"     
#include "levels.h"

/* ---- defines local to this file ---- */
#define LEVEL_COLS      13
#define LEVEL_ROWS      13
#define LEVEL_BYTES     91

/* ---- data ---- */
static const unsigned char tsa_data[64] = {
    0x00, 0x0F, 0x00, 0x0F,  /* 0: brick TL half */
    0x00, 0x00, 0x0F, 0x0F,  /* 1: brick TR half */
    0x0F, 0x00, 0x0F, 0x00,  /* 2: brick BL half */
    0x0F, 0x0F, 0x00, 0x00,  /* 3: brick BR half */
    0x0F, 0x0F, 0x0F, 0x0F,  /* 4: full brick */
    0x20, 0x10, 0x20, 0x10,  /* 5: steel A */
    0x20, 0x20, 0x10, 0x10,  /* 6: steel B */
    0x10, 0x20, 0x10, 0x20,  /* 7: steel C */
    0x10, 0x10, 0x20, 0x20,  /* 8: steel D */
    0x10, 0x10, 0x10, 0x10,  /* 9: full steel */
    0x12, 0x12, 0x12, 0x12,  /* A: water */
    0x22, 0x22, 0x22, 0x22,  /* B: forest */
    0x21, 0x21, 0x21, 0x21,  /* C: ice */
    0x00, 0x00, 0x00, 0x00,  /* D: empty */
    0x00, 0x00, 0x00, 0x00,  /* E: empty */
    0x00, 0x00, 0x00, 0x00   /* F: empty */
};


/* Stage layouts, read from the ROM at build time.  See README.md. */
#define LEVEL_COUNT     36
extern const unsigned char levels[LEVEL_COUNT * LEVEL_BYTES];



//copy tsa block to dst
static void draw_tsa(unsigned char idx) {
    memcpy(dst, tsa_data + (idx << 2), 2);
    memcpy(dst + SCREEN_COLS, (tsa_data + 2) + (idx << 2), 2);
}
static void write_tsa_attrib(unsigned char x, unsigned char y, unsigned char tsa_id)
{
    static const unsigned char tsa_pal[16] = {
    0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 1, 2, 3, 0, 0, 0};
    static const unsigned char attr_mask[4] = {0xFC,0xF3,0xCF,0x3F};
    unsigned char pal;

    pal = tsa_pal[tsa_id];
    k = attr_mask[(y & 2) | ((x & 2) >> 1)];
    i = ((y >> 2) << 3) + (x >> 2);
    pal |= pal << 2;
    pal |= pal << 4;
    SCREEN_BUFFER[0x3C0 + i] =  (SCREEN_BUFFER[0x3C0 + i] & k) | (pal & ~k);
}

//write tsa to screen buffer and copy it to vram via cpy command
void write_tsa_to_vram(unsigned char x, unsigned char y, unsigned char tsa_id){
   dst = SCREEN_BUFFER + COORD_TO_NT_OFFSET(x, y);
   draw_tsa(tsa_id);
   add_vram_cpy_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(x, y), dst, 2);
   add_vram_cpy_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(x, y+1), dst + SCREEN_COLS, 2);

   write_tsa_attrib(x, y, tsa_id);
   i = ((y << 1) & 0xF8) + (x >> 2);
   add_vram_cpy_cmd(NAMETABLE_A + 0x3C0 + i, SCREEN_BUFFER + 0x3C0 + i , 1);
}

//Copy stage attributes from screen buffer to commands. 
//As attribute could have multiple FFs (command terminators in command buffer format), 
//each byte of attribute is copied as separate command by one frame
void copy_stage_attributes_to_vram(void) {
    for (i = 0; i < 0x40; ++i) {
        frame_wait();
        add_vram_cpy_cmd(NAMETABLE_A + 0x3C0 + i, 
                          SCREEN_BUFFER + 0x3C0 + i, 1);
    }
}

//Load stage data to screen buffer. Number to be set in global stage_number
void load_stage(void) {
    unsigned char tile_x, tile_y;
    i = stage_number;
    // Hard mode (stages 36–70): wrap layout back to 1–35.
    // Demo mode also uses stage_number=36 but must load the demo level.
    if (stage_mode != STAGE_MODE_DEMO && i > MAX_STAGE) {
        i -= MAX_STAGE;   // 36→1, 37→2, …, 70→35
    }
    src = (unsigned char *)levels;
    while (--i) {
        src += LEVEL_BYTES;
    }

    dst = SCREEN_BUFFER + COORD_TO_NT_OFFSET(FRAME_OFFSET, FRAME_OFFSET);
    tile_y = FRAME_OFFSET;
    for (j = 0; j < LEVEL_ROWS; ++j) {
        frame_wait();
        tile_x = FRAME_OFFSET;
        for (cnt = 0; cnt < LEVEL_COLS; cnt += 2) {
            temp = *src++;
            //left tsa
            draw_tsa(temp >> 4);
            dst += 2;
            tile_x = FRAME_OFFSET + (cnt<<1);
            write_tsa_attrib(tile_x, tile_y, temp >> 4);
            //right tsa
            if (cnt < LEVEL_COLS - 1) {
                draw_tsa(temp & 0x0F);
                dst += 2;
                write_tsa_attrib (tile_x + 2, tile_y, temp & 0x0F);                
            }
            tile_x += 4;
        }
        tile_y += 2;
        dst += (SCREEN_COLS - LEVEL_COLS) * 2;
    }
    //Copy unpacked attributes to VRAM, so during diverging shutters stage will be shown in correct attributes
    copy_stage_attributes_to_vram();    
}

void load_level_flag(){
    static const unsigned char flag_line1[] = {0x6C, 0xFC};
    static const unsigned char flag_line2[] = {0x6D, 0xFD};
    add_vram_cpy_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x17), flag_line1, sizeof(flag_line1));
    add_vram_cpy_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x18), flag_line2, sizeof(flag_line2));
    set_number_string(stage_number);
    if (stage_number > 9){
        add_vram_cpy_based_cmd(
            NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x19),
            &number_string[5], 2, 0x6E);
    }
    else{
        add_vram_cpy_based_cmd(
            NAMETABLE_A + COORD_TO_NT_OFFSET(0x1E, 0x19),
            &number_string[6], 1, 0x6E);
    }    
}

