void draw_brick_char(unsigned char ascii_char, unsigned char x, unsigned char y);
void set_bg_pal_number(unsigned char pal_id);
void reset_vram_cmd_queue(void);
void add_vram_fill_cmd(unsigned char* ppu_addr, unsigned char value, unsigned char length);
void add_vram_cpy_cmd(unsigned int ppu_addr, const unsigned char *src, unsigned char length);
void add_vram_cpy_based_cmd(unsigned int ppu_addr, const unsigned char *src, unsigned char length, unsigned char char_base);
void oam_spr_step(unsigned char x, unsigned char y, unsigned char chrnum, unsigned char attr);
void add_sprite_(unsigned char pal);
void add_sprite_16x16_(unsigned char pal);
