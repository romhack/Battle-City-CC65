/*	Battle City implemented in C
 *	using neslib
 */	
#include "globals.h" 
#include "../game.h"
#include "util.h"
#include "sound.h"
#include "levels.h"

static unsigned char gameover_str_x;
static unsigned char gameover_str_y;
static unsigned char gameover_scroll_dir;
static unsigned char gameover_str_timer;

static unsigned char staff_string_ram[16];
// number_string layout: indices 0-7, digits 0-9, 0xFF = sentinel/end
// Score strings (hiscore_1p_string, hiscore_2p_string): same 8-byte BCD format
// Scores indexed: player 0 → hiscore_1p_string, player 1 → hiscore_2p_string
static unsigned char hiscore_1p_string[8];
static unsigned char hiscore_2p_string[8];
static unsigned char hiscore_string[8];

static unsigned char killcount_1p[4];
static unsigned char killcount_2p[4];


// Bullets and tanks coordinate increments
// 0=up,1=left,2=down,3=right
static const unsigned char dx[4] = { 0, 0xFF,  0,  1 };
static const unsigned char dy[4] = { 0xFF,  0,  1,  0 };
// *8 increment: replacing multiplication by LUT
static const unsigned char dx8[4] = { 0, 0xF8,  0,  8 };
static const unsigned char dy8[4] = { 0xF8,  0,  8,  0 };

// abs(dx[k]) and abs(dy[k]), precomputed - dx/dy only ever hold 0 or ±1
static const unsigned char abs_dx[4]      = { 0, 1, 0, 1 };
static const unsigned char abs_dy[4]      = { 1, 0, 1, 0 };
static const unsigned char abs_dx_quad[4] = { 0, 4, 0, 4 };   // abs_dx[k] << 2
static const unsigned char abs_dy_quad[4] = { 4, 0, 4, 0 };   // abs_dy[k] << 2


static const unsigned char PlayerBulletIds[] = {0, 1, 8, 9};




static const unsigned char staff_string[16] = "RYOUITI OOKUBO  ";
unsigned char is_cold_boot(void) {
    i = 15;
    
    do {
        if (staff_string_ram[i] != staff_string[i]) {
            return 1;
        }
        --i;
    } while (i != 0xFF); 
    
    return 0;
}


// Sprite helper functions to optimize passing through stack arguments via fixed zp variables
#define add_sprite(x, y, tile, pal) \
    (SPR_X = (x), SPR_Y = (y), CHR_BYTE = (tile), add_sprite_(pal))

#define add_sprite_16x16(x, y, tile, pal) \
    (SPR_X = (x), SPR_Y = (y), CHR_BYTE = (tile), add_sprite_16x16_(pal))

#define PALETTE_SPR_ADDR 0x3F10
#define PALETTE_SIZE     16

void load_spr_pal(void) {
    static const unsigned char spr_pal[PALETTE_SIZE] = {
        0x0F, 0x18, 0x27, 0x38,
        0x0F, 0x0A, 0x1B, 0x3B,
        0x0F, 0x0C, 0x10, 0x20,
        0x0F, 0x04, 0x16, 0x20
    };
    
	vram_adr(PALETTE_SPR_ADDR);
    vram_write(spr_pal, PALETTE_SIZE);
}



void set_ppu(void) {
    frame_wait();
    oam_size(1);          // Set 8x16 sprites (bit 5)
    bank_bg(1);           // Background CHR at $1000 (bit 4)
}

void reset_screen(void){
	construction_flag = 0;
	
	load_spr_pal();
	set_bg_pal_number(3);
	memfill(SCREEN_BUFFER, 0, SCREEN_WIDTH * SCREEN_COLS);
    SPR_PRIORITY = 0x20;//OAM_BEHIND for forest overlap
	oam_set(0);
	init_8_char_string(hiscore_1p_string);
	init_8_char_string(hiscore_2p_string);
	if (is_cold_boot()){
		memcpy(staff_string_ram, staff_string, 16);
		init_8_char_string(hiscore_string);
		hiscore_string[2] = 2;//Write 20000 to HiScore
		cursor_pos = 0;
	}
	vram_adr(NAMETABLE_A);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);
	vram_adr(NAMETABLE_C);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);

}

void draw_brick_str(const unsigned char *str, unsigned char x, unsigned char y) {
    //i - string index
    i = 0;

    while (1) {
        temp = str[i];        
        if (temp == 0xFF) {
            break;
        }
        
        ++i;        

        draw_brick_char(temp, x, y);        
        x += 0x20;
    }
}
//Copy string to screen buffer
void draw_str(const unsigned char *str, unsigned char x, unsigned char y, unsigned char char_index_base) {
	dst = SCREEN_BUFFER + COORD_TO_NT_OFFSET(x, y);
    i = 0;
    
    while (str[i] != 0xFF) {
        *dst = str[i] + char_index_base;
        ++dst;
        ++i;
    }	
}


void draw_title_screen(void){
	static const unsigned char battle_string[] = "BATTLE\xFF";
	static const unsigned char city_string[] = "CITY\xFF";
	static const unsigned char one_string[] = {0x5E, 0x6B, 0xFF};//I-
	static const unsigned char two_string[] = {0x5F, 0x6B, 0xFF};//II-
	static const unsigned char hi_string[] =  {0x48, 0x49, 0x6B, 0xFF};//HI-
	static const unsigned char one_player_string[] = "1 PLAYER\xFF";
	static const unsigned char two_players_string[] = "2 PLAYERS\xFF";
	static const unsigned char construction_string[] = "CONSTRUCTION\xFF";
	static const unsigned char namcot_string[] = {0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0xFF};//red NAMCOT
	static const unsigned char year_string[] = "@ 1980 1985 NAMCO LTDi\xFF";
	static const unsigned char rights_string[] = "ALL RIGHTS RESERVED\xFF";
	
	draw_brick_str(battle_string, 0x1A, 0x2E);
	draw_brick_str(city_string, 0x3C, 0x56);
	draw_str(one_string, 2, 3, 0);
	temp = skip_leading_zeros(hiscore_1p_string, 0);
	draw_str(&hiscore_1p_string[temp], 3 + temp, 3, TILE_DIGIT_BASE);
	
	draw_str(hi_string, 0xB, 3, 0);
	temp = skip_leading_zeros(hiscore_string, 0);
	draw_str(&hiscore_string[temp], 0xD + temp, 3, TILE_DIGIT_BASE);
	
	if (cursor_pos != 0){// If cursor at RESET wasn't at 1 player position, need to draw 2nd player data.
		draw_str(two_string, 0x15, 3, 0);	
		temp = skip_leading_zeros(hiscore_2p_string, 0);
		draw_str(&hiscore_2p_string[temp], 0x14 + temp, 3, TILE_DIGIT_BASE);
	}
	draw_str(one_player_string, 0xB, 0x11, 0);
	draw_str(two_players_string, 0xB, 0x13, 0);
	draw_str(construction_string, 0xB, 0x15, 0);
	draw_str(namcot_string, 0xB, 0x17, 0);
	draw_str(year_string, 0x4, 0x19, 0);
	draw_str(rights_string, 0x6, 0x1B, 0);
	
	vram_adr(NAMETABLE_C); 
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);
}

GameState title_intro_loop(void) {  
	static const unsigned char SCROLL_MAX = 0xF0;  
    i = 0;
    do {
        scroll(0, i);
        frame_wait();
        pad_poll(0);
       
        if (PAD_STATET[0] & (PAD_SELECT | PAD_START)) {
            break;
        }
        ++i;
    } while (i < SCROLL_MAX );    
    scroll(0, SCROLL_MAX );
	return STATE_TITLE_MENU;
}

static void make_gray_frame(void) {
    memfill(SCREEN_BUFFER, TILE_GREY, SCREEN_WIDTH * SCREEN_COLS);
    memfill(SCREEN_BUFFER + ATTR_OFFSET, 0, 64); // all tiles on pal 0
    for (i = 0; i < PLAYFIELD_SIZE; ++i) {
        dst = SCREEN_BUFFER + COORD_TO_NT_OFFSET(FRAME_OFFSET, FRAME_OFFSET + i);
        memfill(dst, TILE_BLACK, PLAYFIELD_SIZE);
    }
}

// Wait ~1 second (64 frames), like Wait_1Second in the ROM
static void wait_1_second(void) {
    FRAME_CNT = 0;
    do {
        frame_wait();
    } while (FRAME_CNT & 0x3F);
}

// Queue a string (no terminator) at tile x,y of nametable A
#define SECRET_STR(str, x, y) \
    add_vram_cpy_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET((x), (y)), (str), sizeof(str) - 1)

// Falling green drop (Draw_Drop in the ROM)
static void draw_drop(void) {
    static unsigned char drop_x, drop_y, drop_frame, drop_tile, hold;

    drop_x = 0x78;
    drop_y = 0x1E;

    // 7 animation frames of the respawn "star", 4 video frames each
    for (drop_frame = 0; drop_frame != 7; ++drop_frame) {
        // tile = (3 - |3 - frame|) * 4 + 0xA1
        drop_tile = (drop_frame >= 3) ? (drop_frame - 3) : (3 - drop_frame);
        drop_tile = ((3 - drop_tile) << 2) + TILE_RESPAWN_BASE;
        for (hold = 0; hold != 4; ++hold) {
            frame_wait();
            add_sprite_16x16(drop_x, drop_y, drop_tile, 3);
        }
    }

    // The unused "tear drop" sprite falls until it leaves the screen
    do {
        frame_wait();
        ++drop_y;
        add_sprite_16x16(drop_x, drop_y, TILE_DROP, 1);
    } while (drop_y != 0xF8);
}

void show_secret_msg(void) {
    static const unsigned char msg_this_program_was[] = "THIS PROGRAM WAS";
    static const unsigned char msg_written_by[]       = "WRITTEN BY";
    static const unsigned char msg_open_reach[]       = "OPENkREACH"; // 'k' glyph is the hyphen
    static const unsigned char msg_who_loves[]        = "WHO LOVES NORIKO";
    static const unsigned char msg_dot[]              = { TILE_DOT, 0xFF };
    static unsigned char dot_x;

    ppu_off();
    scroll(0, 0);                       // show nametable A (title art lives in C)
    oam_clear();                        // no title tank on this screen
    memfill(SCREEN_BUFFER, 0, SCREEN_WIDTH * SCREEN_COLS);
    vram_adr(NAMETABLE_A);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);
    set_ppu();
    ppu_mask(MASK_SHOW_ALL);

    wait_1_second();
    wait_1_second();

    SECRET_STR(msg_this_program_was, 8, 0x08); wait_1_second();
    SECRET_STR(msg_written_by,       8, 0x0A); wait_1_second();
    SECRET_STR(msg_open_reach,       8, 0x0C); wait_1_second();
    SECRET_STR(msg_who_loves,        8, 0x0E); wait_1_second();

    // Five dots, one per second, then the drop falls
    for (dot_x = 8; dot_x <= 0x0C; ++dot_x) {
        SECRET_STR(msg_dot, dot_x, 0x10);
        if (dot_x != 0x0C) {
            wait_1_second();
        }
    }
    draw_drop();

    // Leave a fresh gray frame behind, as the ROM does
    ppu_off();
    oam_clear();
    make_gray_frame();
    vram_adr(NAMETABLE_A);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);
    set_ppu();
    ppu_mask(MASK_SHOW_ALL);
}


void skip_handle(unsigned char id){
	
}

void initialize_gameover (void){
    gameover_str_timer = GAMEOVER_TIMER_INIT;
    gameover_str_y = 0xD8;
    FRAME_CNT = 0;
}

static void initialize_tank(unsigned char id){
    static const unsigned char player_respawn_x[] = {0x58, 0x98};
    static const unsigned char player_respawn_y[] = {0xD8, 0xD8};
    static const unsigned char enemy_respawn_x[] = {0x18, 0x78, 0xD8};
    static const unsigned char enemy_respawn_y[] = {0x18, 0x18, 0x18};

    tank_type[id] = 0;
    if (id < 2){//player tank
        tank_x[id] = player_respawn_x[id];
        tank_y[id] = player_respawn_y[id];
        player_blink_timer[id] = 0;
    }
    else{//enemy tank
        ++enemy_respawn_place_index;
        if (enemy_respawn_place_index >= 3){
            enemy_respawn_place_index = 0;
        }
        tank_x[id] = enemy_respawn_x[enemy_respawn_place_index];
        tank_y[id] = enemy_respawn_y[enemy_respawn_place_index];
        if (enemy_reinforce_count == 3 || enemy_reinforce_count == 10 || enemy_reinforce_count == 17){
            tank_type[id] = TANK_BONUS_FLAG;
            bonus_x = 0; //hide unpicked bonus if bonus tank appears
        }
    }
    tank_status[id] = TANK_CMD_RESPAWN;
    //Draw empty space under tank. In case if level was created from Construction and respawn point is occupied by tsa block
    write_tsa_to_vram((tank_x[id] >> 3) - 1, (tank_y[id] >> 3) - 1, 0x0F);
}

void command_explode(unsigned char id){
    //temp - temporary
    tank_status[id]--;
    if ((tank_status[id] & 0x0F) != 0) 
        return;//animation frame decremented, no additional actions needed

    temp = tank_status[id] - 0x10;
    if (temp != 0) {    // if temp != 0, it's just an animation frame transition. Handle it and return early.
        if (temp != 0x10) {
            temp |= 3; 
        } else {
            temp |= 6; 
        }
        tank_status[id] = temp;
        return;
    }
    //here  explosion animation finished
    tank_status[id] = 0;
    if (id >=2){//no additional processing for enemies needed
        enemy_counter--;
        return;
    }
    player_lives[id]--;
    if (player_lives[id] != 0){//player still have lives, respawn him
        initialize_tank(id);
        return;
    }
    if (hq_status != HQ_STATUS_ALIVE)
        return; //HQ is not alive, do nothing
    if (id == 1){//player 2 has no lives
        if (player_lives[0] == 0)
            return;
        //here player 2 has no lives, but player 1 has lives, draw gameover for player 2 only
        gameover_scroll_dir = 1;
        gameover_str_x = 0xC0;
    }
    else{//player 1 has no lives
        if (player_lives[1] == 0)
            return;
        //here player 1 has no lives, but player 2 has lives, draw gameover for player 1 only    
        gameover_scroll_dir = 3;
        gameover_str_x = 0x20;
    }
    initialize_gameover();	
}


unsigned char get_random(void) {
    //temp - temporary byte
    temp = random_lo;
    random_lo = random_lo << 3;   
    random_lo = random_lo - temp;
    random_lo += SECONDS_CNT;
    ++random_hi;
    temp = ((unsigned char*)0)[random_hi];
    random_lo += temp;
    return random_lo;
}

void get_random_aim(unsigned char id) {
    //cnt - threshold

    cnt = respawn_delay >> 2;
    if (cnt < SECONDS_CNT) {
        // long time with no progress - tank gets impatient, rushes the HQ
        tank_status[id] = (tank_status[id] & 0x0F) | TANK_CMD_TARGET_HQ;
        return;
    }

    cnt >>= 1;
    if (cnt >= SECONDS_CNT) {
        // still fresh - just wander
        tank_status[id] = (get_random() & 0x03) | TANK_CMD_MOVE;
        return;
    }

    // middle ground: go after a player. Even-id tanks prefer player 1,
    // odd-id tanks prefer player 2 - but fall back to whichever player
    // still has a tank in play.
    if (tank_status[0] == 0) {
        SET_TANK_CMD(id, TANK_CMD_TARGET_P2);// P1 gone
    } else if ((id & 1) == 0) {
        SET_TANK_CMD(id, TANK_CMD_TARGET_P1);  // even id
    } else if (tank_status[1] == 0) {
        SET_TANK_CMD(id, TANK_CMD_TARGET_P1); // P2 gone
    } else {
        SET_TANK_CMD(id, TANK_CMD_TARGET_P2);  // odd id, P2 alive
    }
}

void command_get_random_status(unsigned char id){
    //temp - temporary byte

    if ((get_random() & 1) == 0) {
        // 50%: give up on the current direction nudge, pick a fresh AI target
        get_random_aim(id);
        return;
    }
    // 50%: tweak current direction by +1 or -1 (mod 4), stay in "normal move"
    if (get_random() & 1) {
        temp = tank_status[id] + 1;
    } else {
        temp = tank_status[id] - 1;
    }
    
    tank_status[id] = (temp & 0x03) | TANK_CMD_MOVE;	
}
void command_move(unsigned char id){
    //k - direction
    //temp - direction tile index
    //i - probe x, j - probe y
    //x - block x, y - block y
    //temp16 - pointer in screen buffer
    unsigned char tmp1, tmp2;

    // Pre-move RNG check: 1/16 chance for aligned enemies to retarget
    if (id >= 2) {
        if ((tank_x[id] & 7) == 0 && (tank_y[id] & 7) == 0) {
            if ((get_random() & RANDOM_AIM_MASK) == 0) {
                get_random_aim(id);
                return;
            }
        }
    }

    k = tank_status[id] & 0x03;
    tmp1   = (unsigned char)(dx8[k]);  // x_inc*8, wraps signed→byte
    tmp2   = (unsigned char)(dy8[k]);  // y_inc*8
    x = tank_x[id] + (unsigned char)dx[k]; // 1-pixel step
    y = tank_y[id] + (unsigned char)dy[k];

    // Corner 1
    i = x + tmp1 + tmp2;
    if (i >= x) i--;  // Compare_Block_X
    j = y + tmp1 + tmp2;
    if (j >= y) j--;  // Compare_Block_Y
    temp16 = ((unsigned int)(j >> 3) << 5) + (i >> 3);  
    temp = SCREEN_BUFFER[temp16];

    if (temp < 0x80 && (temp == 0 || temp >= TILE_BRICK_THRESHOLD)) {
        // Corner 2
        i = x + tmp1 - tmp2;
        if (i >= x) i--;
        j = y + tmp2 - tmp1;
        if (j >= y) j--;
        temp16 = ((unsigned int)(j >> 3) << 5) + (i >> 3);
        temp = SCREEN_BUFFER[temp16];

        if (temp < 0x80 && (temp == 0 || temp >= TILE_BRICK_THRESHOLD)) {
            tank_x[id] = x;
            tank_y[id] = y;
            track_pos[id] ^= 4;
            return;
        }
    }
    // 3. Collision failed
    if (id < 2) {
        // Players: just animate tracks and return
        track_pos[id] ^= 4;
        return;
    }
    

    if (get_random() & 3) {
        // 75% chance: Enter "coast" state (Command 8). 
        // Keeps current direction, pauses movement for ~2 frames.
        SET_TANK_CMD(id, 0x88);
        track_pos[id] ^= 4; 
        return;
    }

    // 25% chance (get_random() & 3 == 0)
    if ((tank_x[id] & 7) == 0 && (tank_y[id] & 7) == 0) {
        // On a tile boundary: Escalate to Command 9 (Get Random Status/Aim)
        SET_TANK_CMD(id, TANK_CMD_RANDOM);
    }
    // Not on an 8x8 tile boundary: Immediate 180-degree reverse
    tank_status[id] ^= 2; 
}

// Called from handle_tank_status_command() via jump table entry 8 (command nibble $8).
// Triggered by handle_input() when no direction button is pressed:
//   tank_status[id] = (tank_status[id] & 0x0F) | 0x80 | 0x08
// The two bits 2-3 in tank_status act as a 2-bit coast counter (starts at $8 = 0b1000,
// decremented by 4 each call, stops at 0). This gives ~2 player-rate frames of coasting.
void command_drift(unsigned char id) {
    // Ice sliding — players only
    if (id < 2) {
        if ((player_ice_status[id] & 0x80) && (player_ice_status[id] & 0x7F) != 0) {
            player_ice_status[id]--;
            track_pos[id] ^= 4;
            command_move(id);
            return;
        }
    }

    // Coast counter steps: $x8 → $x4 → $x0, then transition to command_move
    tank_status[id] -= COAST_COOLDOWN;
    if ((tank_status[id] & 0x0C) != 0) return;

    // Bits 3-2 reached zero — restore command A (move), keep direction in low nibble
    SET_TANK_CMD(id, TANK_CMD_MOVE);
}

// Picks the new direction/status for a tank based on its position
// relative to its current AI aim target (ai_x_aim/ai_y_aim).
unsigned char get_ai_status(unsigned char id) {
    static const unsigned char ai_status[18] = {
        0xA0, 0xA0, 0xA0, 0xA1, 0xA0, 0xA3, 0xA2, 0xA2, 0xA2,
        0xA1, 0xA0, 0xA3, 0xA1, 0xA0, 0xA3, 0xA1, 0xA2, 0xA3
    };
    //cnt - id
    //x, y  - temporary coordinates
    //k - index in ai table

    cnt = id;  
    x = tank_x[cnt];
    if (ai_x_aim == x) {
        x = 1;
    } else if (ai_x_aim > x) {
        x = 2;
    } else {
        x = 0;
    }
    y = tank_y[cnt];
    if (ai_y_aim == y) {
        y = 1;
    } else if (ai_y_aim > y) {
        y = 2;
    } else {
        y = 0;
    }

    k = y*3 + x;

    if (cnt < 2) {
        // player: alternate table half on a time-based coin flip
        if (((cnt << 1) ^ SECONDS_CNT) & 2) {
            k += 9;
        }
    } else {
        // enemy: alternate using the RNG instead
        if (get_random() & 1) {
            k += 9;
        }
    }
    return ai_status[k];
}

void command_aim_hq(unsigned char id) {
    ai_x_aim = HQ_X;
    ai_y_aim = HQ_Y;
    tank_status[id] = get_ai_status(id);
}
void command_aim_player1(unsigned char id){
    ai_x_aim = tank_x[0];
    ai_y_aim = tank_y[0];
    tank_status[id] = get_ai_status(id);
}
void command_aim_player2(unsigned char id){
    ai_x_aim = tank_x[1];
    ai_y_aim = tank_y[1];
    tank_status[id] = get_ai_status(id);
}

void command_set_respawn(unsigned char id){
    ++tank_status[id];
    if ((tank_status[id] & 0xF) == 0xE)
        tank_status[id] = TANK_CMD_LOAD_TANK; // 0xE0
}

void load_new_tank(unsigned char id){
    //cnt - index of enemy type
    //temp - new tank type value
    static const unsigned char respawn_status[] = {
        0xA0, 0xA0,                         // players face up
        0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2  // enemies face down
    };
    //Enemy types for each level (4 per level, 35 levels + demo)
    static const unsigned char enemy_types[] = {        
        0x80, 0xA0, 0xC0, 0xE0,// Level  1        
        0xE0, 0xA0, 0xC0, 0x80,// Level  2
        0x80, 0xA0, 0xC0, 0xE0,// Level  3
        0xC0, 0xA0, 0x80, 0xE0,// Level  4
        0xC0, 0xE0, 0x80, 0xA0,// Level  5
        0xC0, 0xA0, 0x80, 0xE0,// Level  6
        0x80, 0xA0, 0xC0, 0x80,// Level  7
        0xC0, 0xE0, 0xA0, 0x80,// Level  8
        0x80, 0xA0, 0xC0, 0xE0,// Level  9
        0x80, 0xA0, 0xC0, 0xE0,// Level  10
        0xA0, 0xE0, 0xC0, 0xA0,// Level  11
        0xC0, 0xA0, 0x80, 0xE0,// Level  12
        0xC0, 0xA0, 0x80, 0xE0,// Level  13
        0xC0, 0xA0, 0x80, 0xE0,// Level  14
        0x80, 0xC0, 0xA0, 0xE0,// Level  15
        0x80, 0xC0, 0xA0, 0xE0,// Level  16
        0xE0, 0xA0, 0xC0, 0x80,// Level  17
        0xE0, 0x80, 0xC0, 0xA0,// Level  18
        0xA0, 0xE0, 0x80, 0xC0,// Level  19
        0xA0, 0x80, 0xC0, 0xE0,// Level  20
        0xC0, 0xA0, 0x80, 0xE0,// Level  21
        0xA0, 0x80, 0xC0, 0xE0,// Level  22
        0xE0, 0x80, 0xC0, 0xA0,// Level  23
        0xC0, 0xE0, 0xA0, 0x80,// Level  24
        0xC0, 0xA0, 0x80, 0xE0,// Level  25
        0xA0, 0xE0, 0x80, 0xC0,// Level  26
        0xC0, 0xE0, 0xA0, 0x80,// Level  27
        0xA0, 0xE0, 0x80, 0xC0,// Level  28
        0xC0, 0xA0, 0x80, 0xE0,// Level  29
        0x80, 0xA0, 0xC0, 0xE0,// Level  30
        0xC0, 0xA0, 0xE0, 0xC0,// Level  31
        0xE0, 0x80, 0xC0, 0xA0,// Level  32
        0xA0, 0xE0, 0xC0, 0xA0,// Level  33
        0xC0, 0xA0, 0x80, 0xE0,// Level  34
        0xC0, 0xA0, 0x80, 0xE0 // Level 35 and Demo Level
    };

    tank_status[id] = respawn_status[id];
    //Player
    if (id < 2){
        invincible_timer[id] = INVINCIBLE_TIME;    // spawn shield
        tank_type[id] = player_type[id];
    //Enemy
    } else {
        //find a type that still has remaining count
        while (enemy_count[enemy_type_number] == 0) {
            ++enemy_type_number;
            if (enemy_type_number >= 4) 
                break;
        }
        --enemy_count[enemy_type_number];

        // Determine stage number for enemy set
        cnt = (stage_mode != STAGE_MODE_NORMAL) ? MAX_STAGE : stage_number;
        cnt = ((cnt - 1) << 2) + enemy_type_number;
        temp = enemy_types[cnt];

        // Strongest tank ($E0) gets armour level 3
        if (temp == TANK_TYPE_HEAVY) {
            temp |= TANK_TYPE_HEAVY_ARMOR;
        }
        // OR with existing tank_type (may have bonus flag from initialize_tank)
        temp |= tank_type[id];
        // Strongest bonus tank will have low armor - balancing tweak
        if (temp == 0xE7) {
            temp = TANK_TYPE_HEAVY_BONUS_FIX;
        }
        tank_type[id] = temp;
    }
    track_pos[id] = 0;
}
void command_load_tank(unsigned char id){
    ++tank_status[id];
    if ((tank_status[id] & 0xF) == 0xE)
        load_new_tank(id);   // transition to live status
}

void handle_tank_status_command(unsigned char id){
    //all zp variables should be available now for any of commands
    static const void * const jumptable[] = {
        &&L_skip,          // 0
        &&L_explode,       // 1
        &&L_explode,       // 2
        &&L_explode,       // 3
        &&L_explode,       // 4
        &&L_explode,       // 5
        &&L_explode,       // 6
        &&L_explode,       // 7
        &&L_drift,         // 8
        &&L_random_status, // 9
        &&L_move,          // 10
        &&L_aim_hq,        // 11
        &&L_aim_p2,        // 12
        &&L_aim_p1,        // 13
        &&L_load_tank,     // 14
        &&L_set_respawn    // 15
    };

    cnt = GET_TANK_CMD(id);
    goto *jumptable[cnt];

L_skip:
    return;
L_explode:
    command_explode(id);
    return;
L_drift:
    command_drift(id);
    return;
L_random_status:
    command_get_random_status(id);
    return;
L_move:
    command_move(id);
    return;
L_aim_hq:
    command_aim_hq(id);
    return;
L_aim_p2:
    command_aim_player2(id);
    return;
L_aim_p1:
    command_aim_player1(id);
    return;
L_load_tank:
    command_load_tank(id);
    return;
L_set_respawn:
    command_set_respawn(id);
    return;
}

void render_kill_points(unsigned char id){
    SPR_PRIORITY = 0;
    if(tank_type[id] == 0){//player kill don't draw points, just ricochet
        add_sprite_16x16(tank_x[id], tank_y[id], 0xF1, 3);
    }
    else{
        current_spr_tile_index = ((tank_type[id] >> 3) & 0xFC) + TILE_KILL_PTS_BASE;
        add_sprite_16x16(tank_x[id], tank_y[id], current_spr_tile_index, 3);
    }
    SPR_PRIORITY = 0x20;	
}

void render_small_explode1(unsigned char id){
    SPR_PRIORITY = 0;
    add_sprite_16x16(tank_x[id], tank_y[id], TILE_RICOCHET_BASE + 8, 3);
    SPR_PRIORITY = 0x20;	
}
void render_small_explode2(unsigned char id){
    //i, j - temporary
    i = tank_status[id] >> 4;
    j = (7 - i) << 2;
    current_spr_tile_index = TILE_RICOCHET_BASE + j;
    SPR_PRIORITY = 0;
    add_sprite_16x16(tank_x[id], tank_y[id], current_spr_tile_index, 3);	
    SPR_PRIORITY = 0x20;
}

static void set_sprite_index(unsigned char quadrant, unsigned char tank_idx) {
    //i - base, j - phase
    i = (quadrant << 2) + TILE_BIG_EXPLODE_BASE;
    j = (tank_status[tank_idx] & 0xF0) - 0x30;
    j ^= 0x10; 
    current_spr_tile_index = i + j;
}


void render_big_explode(unsigned char id){
    //x, y - tank coordinates
    SPR_PRIORITY = 0;
    x = tank_x[id];
    y = tank_y[id];

    set_sprite_index(0, id);
    add_sprite_16x16(x - 8, y - 8, current_spr_tile_index, 3);	

    set_sprite_index(1, id);
    add_sprite_16x16(x + 8, y - 8, current_spr_tile_index, 3);	

    set_sprite_index(2, id);
    add_sprite_16x16(x - 8, y + 8, current_spr_tile_index, 3);	

    set_sprite_index(3, id);
    add_sprite_16x16(x + 8, y + 8, current_spr_tile_index, 3);    
    SPR_PRIORITY = 0x20;
}

void render_respawn(unsigned char id){
    //cnt - command
    //temp - temporary
    cnt = tank_status[id] & 0x0F;  // 0..13
    temp = (cnt >= 7) ? (cnt - 7) : (7 - cnt);  // |frame - 7|
    current_spr_tile_index = ((temp << 1) & 0xFC) + TILE_RESPAWN_BASE;
    add_sprite_16x16(tank_x[id], tank_y[id], current_spr_tile_index, 3);
}

void render_normal_tank(unsigned char tank_id) {
    //i - tank id
    //cnt - pal index
	static const unsigned char tank_type_pal[8] = {
        2, 0, 0, 1, 2, 1, 2, 2
    };
    i = tank_id;
    
    if (i < 2) {// Player tank
        if (player_blink_timer[i]) {
            // Blink effect: hide sprite every 8 frames
            if (FRAME_CNT & 8) {
                return;  // Skip drawing this frame
            }
        }
        pal = i;  // Player 1 = palette 0, Player 2 = palette 1
    }
    else {// Enemy tank         
        if (IS_BONUS_TANK(i)) {
            // Bonus tank: alternate between palettes 2 and 3
            pal = ((FRAME_CNT >> 3) & 1) + 2;
        }
        else {
            // Normal enemy: cycle through 8 palettes based on tank type
            cnt = ((FRAME_CNT << 2) + tank_type[i]) & 7;
            pal = tank_type_pal[cnt];
        }
    }   
    current_spr_tile_index = GET_TANK_BASE_TILE(i);
	current_spr_tile_index += GET_TANK_DIR(i) << 3;
	current_spr_tile_index += track_pos[i];     
    add_sprite_16x16(tank_x[i], tank_y[i], current_spr_tile_index, pal);
}


void render_tank(unsigned char tank_id) {
    //cnt - cmd index
    //i - tank id
    //zp variables will be screwed by called functions, but it's allowed - variables be not used twice 
    static const void * const jumptable[] = {
        &&L_skip,        // 0
        &&L_kill_pts,    // 1
        &&L_explode1,    // 2
        &&L_big_expl,    // 3
        &&L_big_expl,    // 4
        &&L_explode2,    // 5
        &&L_explode2,    // 6
        &&L_explode2,    // 7
        &&L_normal,      // 8
        &&L_normal,      // 9
        &&L_normal,      // A
        &&L_normal,      // B
        &&L_normal,      // C
        &&L_normal,      // D
        &&L_respawn,     // E
        &&L_respawn,     // F
    };

    i = tank_id;
    cnt = GET_TANK_CMD(i);
    goto *jumptable[cnt];

L_skip:     return;
L_kill_pts: render_kill_points(i);    return;
L_explode1: render_small_explode1(i); return;
L_big_expl: render_big_explode(i);    return;
L_explode2: render_small_explode2(i); return;
L_normal:   render_normal_tank(i);    return;
L_respawn:  render_respawn(i);        return;
}


// Draw one 8x16 bullet sprite. Direction is in bullet_status bits 0-1.
static void draw_bullet_sprite() {
    // i - bullet id
    // cnt - direction
    cnt = bullet_status[i] & 3;                    // direction
    current_spr_tile_index = TILE_BULLET_BASE + (cnt << 1);    // base bullet tile + direction offset
    pal = 2;                                       // sprite palette 2
    add_sprite(bullet_x[i] - 5, bullet_y[i], current_spr_tile_index, pal);
}
static void draw_ricochet_sprite() {
    // i  - bullet id
    // cnt - frame index
    cnt = (3 - (GET_BULLET_CMD(i) >> 4)) * 4;
    current_spr_tile_index = TILE_RICOCHET_BASE + cnt;
    add_sprite_16x16(bullet_x[i], bullet_y[i], current_spr_tile_index, 3);
}

static void render_bullets(void) {
    //i - bullet id
    for (i = 9; i != 0xFF; --i) {
        switch (GET_BULLET_CMD(i)) {
            case BULLET_CMD_RICOCHET_1:
            case BULLET_CMD_RICOCHET_2:
            case BULLET_CMD_RICOCHET_3:
                draw_ricochet_sprite();
                break;
            case BULLET_CMD_FLY:
                draw_bullet_sprite();
                break;
            default:
                break;
        }
    }
}
static void render_pause(void) {
    if (pause_flag == 0) return;
    
    // Blink effect: hide every 16 frames
    if ((FRAME_CNT & 0x10) == 0) return; 
    
    pal = 3;
    SPR_PRIORITY = 0; // In front of background
    
    // Draw "PAUSE" using 5 individual 8x16 sprites
    // Tiles: P=0x17, A=0x19, U=0x1B, S=0x1D, E=0x1F
    add_sprite(0x64, 0x80, TILE_PAUSE_P, pal);
    add_sprite(0x6C, 0x80, TILE_PAUSE_A, pal);
    add_sprite(0x74, 0x80, TILE_PAUSE_U, pal);
    add_sprite(0x7C, 0x80, TILE_PAUSE_S, pal);
    add_sprite(0x84, 0x80, TILE_PAUSE_E, pal);
    
    SPR_PRIORITY = 0x20;
}


static void null_status(void) {
    i = 8;
    do {
        --i;
        tank_status[i] = 0;
    } while (i);
    player_ice_status[0] = 0;
    player_ice_status[1] = 0;
}

GameState title_menu_loop(void) {

	//From top screen border to first menu item vertically $88 pixels
	#define CURSOR_POS_TO_SPR_COORD ((cursor_pos << 4) + TITLE_MENU_Y_BASE)
    static unsigned char pad_count;
	
    set_bg_pal_number(3);
    scroll(0, 240);       
    null_status();
	tank_x[0] = TITLE_TANK_X; //x on title wont change
	tank_y[0] = CURSOR_POS_TO_SPR_COORD;
	tank_status[0] = TANK_CMD_DRIFT | TANK_DIR_RIGHT;
	tank_type[0] = TANK_TYPE_0;
	track_pos[0] = 0;
	player_blink_timer[0] = 0;
	player_blink_timer[1] = 0;
	pad_count = 0;
    SECONDS_CNT = 0;
    
    while (1) {
        frame_wait();
        pad_poll(0);
        pad_poll(1);
        // Every 4 frames toggle track animation
        if ((FRAME_CNT & 3) == 0) {
            track_pos[0] ^= 4;
        }        
        render_tank(0);       
        // Check for SELECT button (cycle cursor)
        if (PAD_STATET[0] & PAD_SELECT) {
            ++cursor_pos;
            SECONDS_CNT = 0;
        }
        
        // Secret code, step 1: Player 1 holds DOWN, Player 2 presses A (8 times -> 0x80)
        if ((PAD_STATE[0] & PAD_DOWN) && (PAD_STATET[1] & PAD_A)) {
            pad_count += 0x10;
        }
        
        // Secret code, step 2: Player 1 holds RIGHT, Player 2 presses B (12 times -> 0x74)
        if ((PAD_STATE[0] & PAD_RIGHT) && (PAD_STATET[1] & PAD_B)) {
            --pad_count;
        }
        
        // Wrap cursor position (0-2)
        if (cursor_pos >= 3) {
            cursor_pos = 0;
        }        
        tank_y[0] = CURSOR_POS_TO_SPR_COORD;
        
        // Check for demo-roll start (10 seconds idle)
        if (SECONDS_CNT == 10) {
            if (!construction_flag) {
                return STATE_DEMOPLAY;  
            }
        }
        
        // Check for START button
        if (PAD_STATET[0]& PAD_START) {
            // Check for secret message unlock
            if (construction_flag == 7 && pad_count == 0x74) {
                show_secret_msg();
            }            

            set_bg_pal_number(0);            
            switch (cursor_pos) {
                case 0:
					max_tanks_count = 5;
                    return STATE_STAGE_SELECT;
                case 1:
                    max_tanks_count = 7;
                    return STATE_STAGE_SELECT;
                case 2:
                    return STATE_CONSTRUCTION;
            }            
        }
    }
}

//Draw curtain closing effect (converging from top and bottom)
static void draw_converging_shutters(void) {
    
    for (i = 0; i < 0x10; ++i) {
        frame_wait();
        
        // Fill from top
        dst = (unsigned char*)(NAMETABLE_A + COORD_TO_NT_OFFSET(0, i));
        add_vram_fill_cmd(dst, TILE_GREY, SCREEN_WIDTH);
        
        // Fill from bottom
        temp = 0x1D - i;
        dst = (unsigned char*)(NAMETABLE_A + COORD_TO_NT_OFFSET(0, temp));
        add_vram_fill_cmd(dst, TILE_GREY, SCREEN_WIDTH);
    }   

}

//Draw stage number on screen
static void draw_stage_display(void) {
    static const unsigned char stage_text[] = {
		0x23, 0x24, 0x25, 0x26, 0x27, 0x11, 0x11  // "STAGE__"
	};

    add_vram_cpy_cmd(
        NAMETABLE_A + COORD_TO_NT_OFFSET(0xC, 0xE), 
        stage_text, sizeof(stage_text)
        );

    set_number_string(stage_number);
    if (stage_number > 9){
        add_vram_cpy_based_cmd(
            NAMETABLE_A + COORD_TO_NT_OFFSET(0xC + 6, 0xE),
            &number_string[5], 2, TILE_GREY_DIGIT_BASE);
    }
    else{
        add_vram_cpy_based_cmd(
            NAMETABLE_A + COORD_TO_NT_OFFSET(0xC + 7, 0xE),
            &number_string[6], 1, TILE_GREY_DIGIT_BASE);
    }
}

// Handle stage number changes
static void change_stage(signed char delta) {
    FRAME_CNT = 0;
    stage_number += delta;
    
    // Clamp to valid range
    if (stage_number < STAGE_MIN) {
        stage_number = STAGE_MIN;
    }
    if (stage_number > STAGE_MAX) {
        stage_number = STAGE_MAX;
    }
}

// Initialize game variables for stage select
static void init_stage_select_vars(void) {
    init_8_char_string(hiscore_1p_string);
    init_8_char_string(hiscore_2p_string);
    
    player_type[0] = 0;
    player_type[1] = 0;
    player_add_life_flag[0] = 0;
    player_add_life_flag[1] = 0;
    
    skip_stage_select_flag = 0;
    pause_flag = 0;
    
    player_lives[0] = 3;
    player_lives[1] = (cursor_pos == 0) ? 0 : 3; 
    
    enemy_respawn_place_index = 3;
    stage_number = 1;
    stage_mode = STAGE_MODE_NORMAL;
}

// Handle input for stage selection. True, if start pressed, False otherwise
static unsigned char handle_stage_input(void) {
    pad_poll(0);
    if (PAD_STATET[0] & PAD_START) {
        return 1;  // Exit loop
    }
    
    // A button: increment (press or hold)
    if (PAD_STATET[0] & PAD_A) {
        change_stage(1);
        return 0;
    }
    
    if ((pad_state(0) & PAD_A) && ((FRAME_CNT & 7) == 0)) {
        change_stage(1);
        return 0;
    }
    
    // B button: decrement (press or hold)
    if (PAD_STATET[0] & PAD_B) {
        change_stage(-1);
        return 0;
    }
    
    if ((pad_state(0) & PAD_B) && ((FRAME_CNT & 7) == 0)) {
        change_stage(-1);
        return 0;
    }
    
    return 0;  
}

GameState stage_select_loop(void) {
    // If skip_stage_select_flag is 1, we are continuing to the next level, preserving all variables
    if (skip_stage_select_flag == 0) {
        init_stage_select_vars();
    }

    oam_clear();
    frame_wait();
    vram_adr(NAMETABLE_A);
    scroll(0, 0);
    pause_flag = 0;
    set_bg_pal_number(4); 
       
    draw_converging_shutters();

    while (1) {
        frame_wait();
        draw_stage_display();
        
        if (skip_stage_select_flag) {
            return STATE_STAGE_START;
        }
        
        if (handle_stage_input()) {
            return STATE_STAGE_START; 
        }
    }
}



//Draw curtain open effect copying level from screen buffer to VRAM. (diverging from middle towards top and bottom)
static void draw_diverging_shutters(void) {
    static unsigned char top_row, bot_row;
    
    for (temp = 0xF; temp != 0xFF; --temp) {
        frame_wait();
        
        // top row (counting down from middle to top)
        top_row = temp;
        add_vram_cpy_cmd(
            NAMETABLE_A + COORD_TO_NT_OFFSET(FRAME_OFFSET, top_row),
            SCREEN_BUFFER + COORD_TO_NT_OFFSET(FRAME_OFFSET, top_row),
            26
        );
        
        // mirrored bottom row
        bot_row = 0x1D - temp;
        add_vram_cpy_cmd(
            NAMETABLE_A + COORD_TO_NT_OFFSET(FRAME_OFFSET, bot_row),
            SCREEN_BUFFER + COORD_TO_NT_OFFSET(FRAME_OFFSET, bot_row),
            26
        );
    }
}


static void load_reinforcements_icons(void){
    static const unsigned char reinforcement_line[] = {TILE_REINFORCE_ICON, TILE_REINFORCE_ICON};
    for (i = 0; i <= 18; i += 2) {
        add_vram_cpy_cmd(
            NAMETABLE_A + COORD_TO_NT_OFFSET((i & 1) + 29, (i >> 1) + 3),
            reinforcement_line, sizeof(reinforcement_line));
    }
}

static void load_Ip_icons(void){
    static const unsigned char Ip_line[] = {TILE_IP_ICON, TILE_IP_SUFFIX};
    static const unsigned char IIp_line[] = {TILE_IIP_ICON, TILE_IP_SUFFIX};
    add_vram_cpy_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x11), Ip_line, sizeof(Ip_line));
    if (stage_mode == 2 || cursor_pos > 0){
        add_vram_cpy_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x14), IIp_line, sizeof(Ip_line));
    }
}

static void set_enemy_count(){
    //Number of enemies (4 types, no more than 20) per level
    static const unsigned char enemy_amounts[] = {
        0x12, 0x02, 0x00, 0x00,
        0x02, 0x04, 0x00, 0x0E,
        0x0E, 0x04, 0x00, 0x02,
        0x0A, 0x05, 0x02, 0x03,
        0x05, 0x02, 0x08, 0x05,
        0x07, 0x02, 0x09, 0x02,
        0x03, 0x04, 0x06, 0x07,
        0x07, 0x02, 0x04, 0x07,
        0x06, 0x04, 0x07, 0x03,
        0x0C, 0x02, 0x04, 0x02,
        0x05, 0x06, 0x04, 0x05,
        0x08, 0x06, 0x00, 0x06,
        0x08, 0x08, 0x00, 0x04,
        0x0A, 0x04, 0x00, 0x06,
        0x02, 0x00, 0x0A, 0x08,
        0x10, 0x00, 0x02, 0x02,
        0x02, 0x02, 0x08, 0x08,
        0x04, 0x02, 0x06, 0x08,
        0x04, 0x08, 0x04, 0x04,
        0x08, 0x02, 0x02, 0x08,
        0x08, 0x02, 0x06, 0x04,
        0x08, 0x06, 0x02, 0x04,
        0x06, 0x00, 0x04, 0x0A,
        0x04, 0x02, 0x04, 0x0A,
        0x02, 0x08, 0x00, 0x0A,
        0x06, 0x06, 0x04, 0x04,
        0x02, 0x08, 0x08, 0x02,
        0x02, 0x01, 0x0F, 0x02,
        0x0A, 0x04, 0x00, 0x06,
        0x04, 0x08, 0x04, 0x04,
        0x03, 0x08, 0x06, 0x03,
        0x08, 0x06, 0x02, 0x04,
        0x04, 0x08, 0x04, 0x04,
        0x04, 0x0A, 0x00, 0x06,
        0x04, 0x06, 0x00, 0x0A
    };
    temp = stage_mode == STAGE_MODE_NORMAL ? stage_number : MAX_STAGE;
    --temp;
    temp <<= 2;
    memcpy(enemy_count, &enemy_amounts[temp], sizeof(enemy_count));
}

static unsigned char hq_armor_blink_state = 0xFF;  // sentinel: forces first redraw
static void reset_stage_vars(){
    for(i = 0; i < sizeof(bullet_status); ++i){//hide bullets
        bullet_status[i] = 0;
    }
    for(i = 0; i < sizeof(tank_status); ++i){
        tank_status[i] = 0;
    }
    for(i = 0; i < sizeof(player_ice_status); ++i){
        player_ice_status[i] = 0;
    }
    gameover_str_timer = 0;
    gameover_str_y = SPRITE_OFFSCREEN_Y;

    if (player_lives[0] != 0){
        initialize_tank(0);
    }
    if (player_lives[1] != 0){
        initialize_tank(1);
    }

    enemy_reinforce_count = MAX_ENEMIES;
    enemy_counter = MAX_ENEMIES;

    enemy_type_number = 0;
    SECONDS_CNT = 0;
    construction_flag = 0;
    hq_armor_timer = 0;
    hq_armor_blink_state = 0xFF;
    player_blink_timer[0] = 0;
    player_blink_timer[1] = 0;
    invincible_timer[0] = 0;
    invincible_timer[1] = 0;
    respawn_timer = 0;
    bonus_x = 0;
    enemy_freeze_timer = 0;
    enemy_respawn_place_index = 0;
    skip_stage_select_flag = 1;


    for(i = 0; i < sizeof(killcount_1p); ++i){
        killcount_1p[i] = 0;
        killcount_2p[i] = 0;
    }
    temp = stage_mode == 1 ? MAX_STAGE : stage_number;
    cnt = cursor_pos > 0? 20 : 0;
    respawn_delay = 190 - (temp << 2) - cnt;
    frame_wait(); 
    load_reinforcements_icons();
    load_Ip_icons();
    load_level_flag();
    set_enemy_count();
    hq_status = HQ_STATUS_ALIVE;
    sound_play_background(SND_ENGINE);
}

// input: raw joypad byte — returns 0=up 1=left 2=down 3=right 0xFF=none
unsigned char button_to_direction(unsigned char pad) {
    if (pad & PAD_UP)    return 0;
    if (pad & PAD_LEFT)  return 1;
    if (pad & PAD_DOWN)  return 2;
    if (pad & PAD_RIGHT) return 3;
    return 0xFF;
}

static void handle_input(){
    //cnt - tank id
    //i - new direction
    //j - current direction

    if ((FRAME_CNT & 1) == 0 && (FRAME_CNT & 3) != 0)
        return;

    for (cnt = 0; cnt < 2; ++cnt) {

        // skip exploding or spawning tanks
        if (tank_status[cnt] < TANK_STATUS_ALIVE_MIN || tank_status[cnt] >= TANK_STATUS_ALIVE_MAX)
            continue;

        // blink timer (friendly fire grace period) — tick it down, suppress input
        if (player_blink_timer[cnt] != 0) {
            --player_blink_timer[cnt];
            goto no_input;
        }

        // if on ice and still sliding, skip normal input, continue drift in previous direction
        if ((player_ice_status[cnt] & 0x80) && (player_ice_status[cnt] & 0x10))
            goto no_input;

        // read joypad direction
        i = button_to_direction(PAD_STATE[cnt]);

        if (i == 0xFF)  // no direction pressed
            goto no_input;

        // direction change: if perpendicular to current, snap to 8px grid
        j = tank_status[cnt] & 3;
        if (i != j && (i ^ j) != 2) {
            // not same, not opposite → perpendicular: snap coords
            tank_x[cnt] = (tank_x[cnt] + 4) & 0xF8;
            tank_y[cnt] = (tank_y[cnt] + 4) & 0xF8;
        }

        // ice: if on ice and slide timer expired, restart it
        if (player_ice_status[cnt] & 0x80) {
            if ((player_ice_status[cnt] & 0x1F) == 0) {
                player_ice_status[cnt] = ICE_SLIDE_INIT;  // 28 frames of sliding
                sound_play(SND_ICE);
            }
        }
        // apply new direction, command stays $A 
        tank_status[cnt] = 0xA0 | i;
        continue;

    no_input:
        // no button: set command $8 (command_drift) which counts down to stop ice sliding
        tank_status[cnt] = (tank_status[cnt] & 0x0F) | TANK_CMD_DRIFT | 8;
    }
}

static void handle_all_tanks_commands(){
    //specifically don't take zp variable, because further commands can have complex functions
    unsigned char id;

    if (enemy_freeze_timer != 0 && (FRAME_CNT & FRAMES_IN_SECOND) == 0) {
        --enemy_freeze_timer;
    }
    for (id = 7; id != 0xFF; --id) {
        if (id < 2) {
            if ((FRAME_CNT & 1) == 0 && (FRAME_CNT & 3) != 0)
                continue;//Process statuses on specific frames: 0, 4, 8, 12
        } else {
            if (enemy_freeze_timer != 0) {
                if (tank_status[id] >= TANK_STATUS_ALIVE_MIN && tank_status[id] < TANK_STATUS_ALIVE_MAX)
                    continue;//Dont process dead enemies
            }
            if (GET_TANK_BASE_TILE(id) != 0xA0) {//Enemy #2 status processed 2 times more often, so it moves faster
                if (((id ^ FRAME_CNT) & 1) == 0)
                    continue;
            }
        }
        handle_tank_status_command(id);
    }
}
static unsigned char prev_tank_x[8], prev_tank_y[8];
//Set occupied flag to nametable tile under each tank for collision detection
//Note: this function looks strange, because it is optimized in C for shorter assembly output and speed
static void mark_tiles_under_tanks(void) {
    for (cnt = 7; cnt != 0xFF; --cnt) {
        if (tank_status[cnt] < TANK_STATUS_ALIVE_MIN || tank_status[cnt] >= TANK_STATUS_ALIVE_MAX)
            continue;

        if (tank_x[cnt] != prev_tank_x[cnt] || tank_y[cnt] != prev_tank_y[cnt]) {
            // Moved since last frame (or first frame active) - recompute everything
            x = ((unsigned char)(tank_x[cnt] - 8)) / 8;
            y = ((unsigned char)(tank_y[cnt] - 8)) / 8;
            j = (y & 0x07) * 0x20;
            i = (y >> 3) | 4;
            temp16 = ((i << 8) | (j | x));

            nt_addr_lo[cnt] = (unsigned char)temp16;
            nt_addr_hi[cnt] = (unsigned char)(temp16 >> 8) & 3;   // wipes stale flag bits too

            if (cnt < 2) {
                i = (((MetaTileEntry*)temp16)->right_bottom) & 0x7F;
                j = player_ice_status[cnt];
                if (i == TILE_ICE) j |= OCCUPIED_BIT; else j &= ~OCCUPIED_BIT;
                player_ice_status[cnt] = j;
            }

            if ((tank_x[cnt] & 7) == 0) nt_addr_hi[cnt] |= 0x80;  // left-neighbour flag
            if ((tank_y[cnt] & 7) == 0) nt_addr_hi[cnt] |= 0x40;  // top-neighbour flag

            prev_tank_x[cnt] = tank_x[cnt];
            prev_tank_y[cnt] = tank_y[cnt];
        } else {
            temp16 = (((unsigned int)(nt_addr_hi[cnt] & 3) | 4) << 8) | nt_addr_lo[cnt];
        }

        // This part always runs - the tile must be (re-)flagged every frame
        i = ((MetaTileEntry*)temp16)->right_bottom;
        i |= OCCUPIED_BIT;
        ((MetaTileEntry*)temp16)->right_bottom = i;

        if (nt_addr_hi[cnt] & 0x80) {
            i = ((MetaTileEntry*)temp16)->left_bottom;
            i |= OCCUPIED_BIT;
            ((MetaTileEntry*)temp16)->left_bottom = i;
        }
        if (nt_addr_hi[cnt] & 0x40) {
            i = ((MetaTileEntry*)temp16)->right_top;
            i |= OCCUPIED_BIT;
            ((MetaTileEntry*)temp16)->right_top = i;
        }
    }
}
//Clear occupied flag of nametable tile under each tank
static void clear_tiles_under_tanks(void) {
    //cnt - tank id
    //temp - temporary hi byte
    
    static union { //structure for optimized addressing
        unsigned char *p;
        unsigned char b[2];
    } u;

    for (cnt = 7; cnt != 0xFF; --cnt) {
        if (tank_status[cnt] < TANK_STATUS_ALIVE_MIN || tank_status[cnt] >= TANK_STATUS_ALIVE_MAX) continue;
        
        temp = nt_addr_hi[cnt];
        
        // Reconstruct pointer from saved addresses
        u.b[0] = nt_addr_lo[cnt];
        u.b[1] = (temp & 0x03) | 0x04;

        // Clear primary tile
        u.p[0x21] &= ~OCCUPIED_BIT;

        // Clear left neighbour
        if (temp & 0x80) {
            u.p[0x20] &= ~OCCUPIED_BIT;
        }
        
        // Clear top neighbour
        if (temp & 0x40) {
            u.p[0x01] &= ~OCCUPIED_BIT;
        }
    }
}

void draw_player_lives(void) {
    // P1 icon + lives count
    add_vram_fill_cmd((unsigned char*)(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x12)), TILE_PLAYER_LIVE, 1);
    set_number_string(player_lives[0] > 0 ? player_lives[0] - 1 : 0);
    if (player_lives[0] > 10) {
        add_vram_cpy_based_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x12), &number_string[5], 2, TILE_GREY_DIGIT_BASE);
    } else {
        add_vram_cpy_based_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1E, 0x12), &number_string[6], 1, TILE_GREY_DIGIT_BASE);
    }

    if (stage_mode != 2 && cursor_pos == 0) return;   // 1-player: skip P2

    // P2 icon + lives count
    add_vram_fill_cmd((unsigned char*)(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x15)), TILE_PLAYER_LIVE, 1);
    set_number_string(player_lives[1] > 0 ? player_lives[1] - 1 : 0);
    if (player_lives[1] > 10) {
        add_vram_cpy_based_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x15), &number_string[5], 2, TILE_GREY_DIGIT_BASE);
    } else {
        add_vram_cpy_based_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1E, 0x15), &number_string[6], 1, TILE_GREY_DIGIT_BASE);
    }
}

// Alternates BkgPal_Number between 1 and 2 every 32 frames.
// NMI handler loads the corresponding background palette each frame.
void swap_water_pal_colors(void) {
    unsigned char cnt = FRAME_CNT & 0x3F;
    if (cnt == 0)
        set_bg_pal_number(2);
    else if (cnt == 0x20)
        set_bg_pal_number(1);
}

// Draws force field aura around players and counts down their invincibility timer.
// Timer decrements once per second (every 64 frames).
// Aura animates every 2 frames by cycling through 2 tile pairs starting at $29.
void handle_invincibility(void) {
    //i - player tank id
    for (i = 1; i != 0xFF; --i) {
        if (invincible_timer[i] == 0)
            continue;

        // decrement timer once per second
        if ((FRAME_CNT & FRAMES_IN_SECOND) == 0)
            --invincible_timer[i];

        // draw aura: tile $29/$2B alternating every 2 frames
        current_spr_tile_index = ((FRAME_CNT & 2) << 1) + TILE_INVINC_BASE;
        add_sprite_16x16(tank_x[i], tank_y[i], current_spr_tile_index, 2);
    }
}

static void make_shot() {
    //i - tank id
    //temp - tank status and type
    if (bullet_status[i] != 0) 
        return;

    // Sound for player shots only
    if (i < 2) {
        sound_play(SND_SHOOT);
    }

    // Direction is lower 2 bits of tank status, OR'd with 0x40 = flying
    temp = tank_status[i] & 3;
    bullet_status[i] = 0x40 | temp;

    // Position bullet 8 pixels in front of the tank
    bullet_x[i] = tank_x[i] + (dx8[temp]);
    bullet_y[i] = tank_y[i] + (dy8[temp]);

    // Reset properties
    bullet_property[i] = 0;

    // Set speed / armor based on tank type (upper 4 bits)
    temp = tank_type[i] & TANK_TYPE_MASK;
    
    // Types 0, 4, 5, 7 ($00, $80, $A0, $E0) all fire normal bullets.
    if (temp == TANK_TYPE_0 ) {
        return; 
    }

    // bonus/fast enemy — must be before the $80 test
    if (temp == TANK_TYPE_6) {               
        bullet_property[i] = 0x01;
        return;
    }
    
    // Type 3 ($60) fires fast, armor-piercing bullets (property 3)
    if (temp == TANK_TYPE_3) {
        bullet_property[i] = 0x03;
        return;
    }

    // $80, $90, $A0, $B0, $D0, $E0, $F0 -> property 0
    if (temp & 0x80) {
        return;                       
    }
    
    // Types 1, 2, 6 ($20, $40, $C0) fire fast bullets (property 1)
    // This acts as the fall-through for any remaining types.
    bullet_property[i] = 0x01;
}

static void make_player_shot(void) {
    //i - player id
    for (i = 1; i != 0xFF; --i) {
        if (tank_status[i] < TANK_STATUS_ALIVE_MIN || tank_status[i] >= TANK_STATUS_ALIVE_MAX) 
            continue;
        if (!(PAD_STATET[i] & (PAD_A | PAD_B))) continue;

        // TANK_TYPE_2 and TANK_TYPE_3: double-shot capable (upper 2 bits == 01)
        // Allow a second simultaneous bullet using extra slot (i+8) if main slot busy
        if ((GET_TANK_TYPE_BITS(i) == TANK_TYPE_2 || GET_TANK_TYPE_BITS(i) == TANK_TYPE_3) && bullet_status[i] != 0) {
            if (bullet_status[i + 8] != 0) 
                continue;  // both slots occupied, can't fire
            // Move active bullet to extra slot, freeing main slot for new shot
            bullet_status[i + 8]    = bullet_status[i];
            bullet_x[i + 8]         = bullet_x[i];
            bullet_y[i + 8]         = bullet_y[i];
            bullet_property[i + 8]  = bullet_property[i];
            bullet_status[i]        = 0;
        }
        make_shot();   
    }
}
static void make_enemy_shot(void) {
    // Enemies cannot shoot if frozen by the watch bonus
    if (enemy_freeze_timer != 0) 
        return;
    // Iterate through enemy tank slots (7 down to 2)
    for (i = 7; i >= 2; --i) {
        // Skip if tank is exploding (< 0x80) or spawning (>= 0xE0)
        if (tank_status[i] < TANK_STATUS_ALIVE_MIN || tank_status[i] >= TANK_STATUS_ALIVE_MAX)
            continue;        
        // 1/32 chance to fire a bullet each frame
        if ((get_random() & ENEMY_FIRE_MASK) != 0)
            continue;        
        make_shot();
    }
}

static void bullet_move() {
    //i - bullet id
    //temp - direction
    // x, y - new values of bullet coordinates
    temp = bullet_status[i] & 3;
    if (bullet_property[i] & 0x01) {  // fast bullet twice faster
        x = bullet_x[i] + dx[temp] * 4;
        y = bullet_y[i] + dy[temp] * 4;
    }
    else{
        x = bullet_x[i] + dx[temp] * 2;
        y = bullet_y[i] + dy[temp] * 2;
    }
    bullet_x[i] = x;
    bullet_y[i] = y;
}

//move bullet or handle ricochet
static void handle_all_bullets_statuses(void) {
    // i - bullet id
    // cnt - frame index
    for (i = 9; i != 0xFF; --i) {
        switch (GET_BULLET_CMD(i)) {
            case BULLET_CMD_FLY:
                bullet_move();
                break;
            case BULLET_CMD_RICOCHET_3:
            case BULLET_CMD_RICOCHET_2:
            case BULLET_CMD_RICOCHET_1:
                bullet_status[i]--;
                if ((bullet_status[i] & 0x0F) != 0) break;
                cnt = GET_BULLET_CMD(i) - BULLET_CMD_RICOCHET_3;
                bullet_status[i] = (cnt == 0) ? BULLET_CMD_IDLE : (cnt | BULLET_FRAME_TIMER);
                break;
            default:
                break;                                                
        }        
    }
}

static void draw_naked_hq(void) {
    static const unsigned char naked_hq_line1[] = { 0xC8, 0xCA };
    static const unsigned char naked_hq_line2[] = { 0xC9, 0xCB };

    buffer_and_vram_cpy(0x0E, 0x1A, naked_hq_line1, sizeof(naked_hq_line1));
    buffer_and_vram_cpy(0x0E, 0x1B, naked_hq_line2, sizeof(naked_hq_line2));

    temp = SCREEN_BUFFER[0x3F3] & 0x3F; // reset HQ tile to palette 0
    buffer_and_vram_fill(0x3F3, temp, 1);
}

//Draw HQ in different statuses. Simultaneously writes to screen buffer (to handle collisions) and to VRAM (for visual representation)
void draw_normal_hq(void){
    static const unsigned char hq_line_1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static const unsigned char hq_line_2[] = {0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x00};
    static const unsigned char hq_line_3[] = {0x00, 0x0F, 0xC8, 0xCA, 0x0F, 0x00};
    static const unsigned char hq_line_4[] = {0x00, 0x0F, 0xC9, 0xCB, 0x0F, 0x00};

    buffer_and_vram_cpy(0x0C, 0x18, hq_line_1, sizeof(hq_line_1));
    buffer_and_vram_cpy(0x0C, 0x19, hq_line_2, sizeof(hq_line_2));
    buffer_and_vram_cpy(0x0C, 0x1A, hq_line_3, sizeof(hq_line_3));
    buffer_and_vram_cpy(0x0C, 0x1B, hq_line_4, sizeof(hq_line_4));
    //HQ attributes
    buffer_and_vram_fill(0x3F3, 0, 1);
    temp = SCREEN_BUFFER[0x3F4] & 0xCC;
    buffer_and_vram_fill(0x3F4, temp, 1);
}

void draw_armoured_hq(void) {
    //temp - attribute byte
    static const unsigned char ArmourHQ_Line1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const unsigned char ArmourHQ_Line2[] = { 0x00, 0x10, 0x10, 0x10, 0x10, 0x00 };
    static const unsigned char ArmourHQ_Line3[] = { 0x00, 0x10, 0xC8, 0xCA, 0x10, 0x00 };
    static const unsigned char ArmourHQ_Line4[] = { 0x00, 0x10, 0xC9, 0xCB, 0x10, 0x00 };

    buffer_and_vram_cpy(0x0C, 0x18, ArmourHQ_Line1, sizeof(ArmourHQ_Line1));
    buffer_and_vram_cpy(0x0C, 0x19, ArmourHQ_Line2, sizeof(ArmourHQ_Line2));
    buffer_and_vram_cpy(0x0C, 0x1A, ArmourHQ_Line3, sizeof(ArmourHQ_Line3));
    buffer_and_vram_cpy(0x0C, 0x1B, ArmourHQ_Line4, sizeof(ArmourHQ_Line4));

    // Attribute byte at PPU 0x23F3: set all four 2×2 sub-blocks to palette 3
    // (0x3F = 0b00_11_11_11 — all three lower pairs = 3)
    buffer_and_vram_fill(0x3F3, 0x3F, 1);
    temp = (SCREEN_BUFFER[0x3F4] & 0xCC) | 0x33;
    buffer_and_vram_fill(0x3F4, temp, 1);
}

static void draw_destroyed_hq() {
    static const unsigned char DestroyedHQ_TSA_Line1[] = { 0xCC, 0xCE};
    static const unsigned char DestroyedHQ_TSA_Line2[] = { 0xCD, 0xCF};

    buffer_and_vram_cpy(0x0E, 0x1A, DestroyedHQ_TSA_Line1, sizeof(DestroyedHQ_TSA_Line1));
    buffer_and_vram_cpy(0x0E, 0x1B, DestroyedHQ_TSA_Line2, sizeof(DestroyedHQ_TSA_Line2));
}

// Draw a 16×16 explosion sprite centred on the HQ eagle position.
static void draw_hq_small_explode(unsigned char tile) {
    add_sprite_16x16(HQ_X, HQ_Y, tile, 3);   // HQ eagle screen centre
}

// Draw a 32×32 explosion sprite (four 16×16 quadrants) over the HQ.
static void draw_hq_big_explode(unsigned char spr_base) {
    add_sprite_16x16(HQ_X - 8, HQ_Y - 8, TILE_BIG_EXPLODE_BASE + spr_base, 3); // top-left
    add_sprite_16x16(HQ_X + 8, HQ_Y - 8, TILE_BIG_EXPLODE_BASE + 4 + spr_base, 3); // top-right
    add_sprite_16x16(HQ_X - 8, HQ_Y + 8, TILE_BIG_EXPLODE_BASE + 8 + spr_base, 3); // bottom-left
    add_sprite_16x16(HQ_X + 8, HQ_Y + 8, TILE_BIG_EXPLODE_BASE + 12 + spr_base, 3); // bottom-right
}

// Armour timer (hq_armor_timer):
//   Updated every 16 frames; decremented once per second (every 64 frames).
//   While timer >= 4 the armoured tiles are assumed already in VRAM – no
//   per-frame redraw is needed.
//   When timer drops below 4 the HQ blinks at ~2 Hz (FRAME_CNT bit 4),
//   toggling between armoured and normal appearance every 16 frames.
//   When timer reaches 0 the normal HQ tiles are restored once.
static void handle_hq_armor(void) {
    static unsigned char blink_state;

    if (hq_armor_timer == 0)       return;
    // Decrement once per second
    if ((FRAME_CNT & FRAMES_IN_SECOND) == 0) {
        --hq_armor_timer;
        if (hq_armor_timer == 0) {
            draw_normal_hq();   // armour expired — restore normal tiles
            return;
        }
    }
    // Blink during the last 3 seconds (timer 1–3): toggle every 16 frames
    if (hq_armor_timer < 4) {
        blink_state = (FRAME_CNT & 0x10) != 0;
        if (blink_state != hq_armor_blink_state) {
            hq_armor_blink_state = blink_state;
            if (blink_state) draw_armoured_hq();
            else draw_normal_hq();
        }
    }
    // timer >= 4: armoured tiles already correct in VRAM, nothing to redraw
}
// Advances the HQ explosion animation by one step and draws the sprite.
// hq_status:
//   Set to 0x27 by bullet_bg_impact() when the eagle is destroyed.
//   Counts down to 0, producing a 6-step animation:
//     steps 1-3  – small 16×16 sprite (tiles 0xF1, 0xF5, 0xF9)
//     steps 4-5  – large 32×32 sprite (tile bases 0xD1+0, 0xD1+0x10)
//     step  0    – no sprite (animation finished)
static void handle_hq_explode(void) {
    if (hq_status == 0 || hq_status & HQ_STATUS_ALIVE) return;//hq exploded or intact

    pal = 3;
    --hq_status;
    temp = hq_status >> 2;
    temp = (temp >= 5) ? (temp - 5) : (5 - temp);  // first fold  around 5
    temp = (temp >= 5) ? (temp - 5) : (5 - temp);  // second fold around 5

    switch (temp) {
        case 0:                           return;  // animation finished
        case 1: draw_hq_small_explode(TILE_RICOCHET_BASE); break;
        case 2: draw_hq_small_explode(TILE_RICOCHET_BASE + 4); break;
        case 3: draw_hq_small_explode(TILE_RICOCHET_BASE + 8); break;
        case 4: draw_hq_big_explode(0x00);   break;
        case 5: draw_hq_big_explode(0x10);   break;
        default:                          return;
    }
}

//Checks if bullet collides with tiles and starts ricochet/explosions if do
//i - bullet id, x, y are inited with probe coordinates before calling this function
static unsigned char bullet_bg_impact(unsigned char *ricochet_played) {
    //i - bullet id
    //x - sprite x
    //y - sprite y
    //cnt - mask
    //temp - tile
    //src - tile pointer 
    //temp16 - offset
    unsigned char tile_row, tile_col; //local variables for optimization
    unsigned char lo, hi;
    static const unsigned char QuadLut[4] = {1, 2, 4, 8};
    

    // define quadrant
    cnt = QuadLut[((y & 4) >> 1) | ((x & 4) >> 2)];
    // mask now holds 1,2,4,8 (top‑left, top‑right, bottom‑left, bottom‑right)

    // Get pointer to the tile in the screen buffer
    tile_row = y >> 3;
    tile_col = x >> 3;
    lo = (tile_row & 0x07) * 0x20;   
    lo |= tile_col;                  
    hi = tile_row >> 3;              
    temp16 = ((unsigned int)hi << 8) | lo;  

    src = &SCREEN_BUFFER[temp16];
    temp = *src;

    // Returns non‑zero if the quadrant is still solid.
    if (((cnt | 0xF0) & temp) == 0)
        return 0;                     // quadrant already empty → pass through

    // 4 HQ eagle tiles (C8‑CB) – explode and ricochet
    if ((temp & TILE_HQ_MASK) == TILE_HQ_BASE) {
        if (hq_status != 0) {
            hq_status = 0x27;       // initial explosion frame
            sound_play(SND_HQ_EXPLODE);
            sound_play(SND_PLAYER_EXPLODE);
            draw_destroyed_hq();
            bullet_status[i] = BULLET_RICOCHET_INIT;
        }
        return 0;
    }

    // Water, forest, ice (tile >= $12) – pass through
    if (temp >= TILE_WATER_THRESHOLD)
        return 0;

    // Solid obstacle – always set ricochet animation
    bullet_status[i] = BULLET_RICOCHET_INIT;

    // Screen border ($11) – no tile destruction
    if (temp == TILE_GREY) {
        if (i < 2 && !*ricochet_played) {
            sound_play(SND_ARMOUR_RICOCHET_WALL);
            *ricochet_played = 1;
        }
        return 0;
    }

    // Armour‑piercing bullet (property bit 1) – destroys the whole tile, even for bricks
    if (bullet_property[i] & 0x02) {
        buffer_and_vram_fill(temp16, 0, 1);
        if (i < 2 && !*ricochet_played) {
            sound_play(SND_BRICK_RICOCHET);
            *ricochet_played = 1;
        }
        return 0;
    }
    // armour ($10) without armor-piercing, no destruction
    if (temp == TILE_ARMOR) {
        if (i < 2 && !*ricochet_played) {
            sound_play(SND_ARMOUR_RICOCHET_WALL);
            *ricochet_played = 1;
        }
        return 0;
    }
    // Normal bullet hitting ordinary brick – create a hole in the correct quadrant
    temp &= ~cnt;
    buffer_and_vram_fill(temp16, temp, 1);
    if (i < 2 && !*ricochet_played) {
        sound_play(SND_BRICK_RICOCHET);
        *ricochet_played = 1;
    }
    return 1;
}


static void handle_bullet_bg_collisions(void) {
    //i - bullet id
    //k - direction
    //x - probe x
    //y - probe y
    static unsigned char temp_x, temp_y;
    static unsigned char temp_x_quad, temp_y_quad;
    unsigned char ricochet_played;

    for (i = 9; i != 0xFF; --i) {

        if (GET_BULLET_CMD(i) != BULLET_CMD_FLY)
            continue;

        if (bullet_property[i] == 0) {     // slow bullet: skip every other frame
            if (((i ^ FRAME_CNT) & 1) == 0)
                continue;
        }

        ricochet_played = 0;

        k = bullet_status[i] & 0x03;

        // Direct table lookup - no sign test, no negation, no shift
        temp_x      = abs_dx[k];
        temp_x_quad = abs_dx_quad[k];
        temp_y      = abs_dy[k];
        temp_y_quad = abs_dy_quad[k];

        // Probe 1: leading edge
        x = bullet_x[i];
        y = bullet_y[i];
        if (bullet_bg_impact(&ricochet_played)) {
            // Probe 1b: same tile, opposite quadrant (ptr unchanged, new mask)
            x += temp_y_quad;
            y += temp_x_quad;
            bullet_bg_impact(&ricochet_played);
        }
        // Probe 2: trailing edge (offset by 1 pixel perpendicular)
        x = bullet_x[i] - temp_y;
        y = bullet_y[i] - temp_x;
        if (bullet_bg_impact(&ricochet_played)) {
            // Probe 2b: same tile, opposite quadrant (ptr unchanged, new mask)
            x -= temp_y_quad;
            y -= temp_x_quad;
            bullet_bg_impact(&ricochet_played);
        }
    }
}

// Encodes a score value into number_string for use by add_score.
// Nibble encoding: high nibble = hundreds digit, low nibble = tens digit.
// Special case: value 0 means 1000 points (digit 1 at index 3).
void num_to_num_string(unsigned char value) {

    init_8_char_string(number_string);
    if (value == 0) {
        number_string[3] = 1;   // 1000 points
        return;
    }
    number_string[5] = value & 0x0F;         // ones/tens digit
    number_string[4] = (value >> 4) & 0x0F; // hundreds digit
}

// Adds number_string into the player's score string, digit by digit with carry.
void add_score(unsigned char player_id) {
    //dst - pointer to filling hiscore string
    //temp - carry
    //cnt - char index
    //x - sum
    dst = (player_id == 0) ? hiscore_1p_string : hiscore_2p_string;
    temp = 0;
    
    // The score is stored as individual decimal digits (0-9) in an 8-byte array.
    // The lowest digit (ones place) is at index 6. 
    // We add from right to left (index 6 down to 0) to handle carries properly.
    for (cnt = 7; cnt-- != 0; ) {
        x = number_string[cnt] + dst[cnt] + temp;
        if (x >= 10) {
            x -= 10;
            temp = 1;
        } else {
            temp = 0;
        }
        dst[cnt] = x;
    }
}
// Awards an extra life to any player whose score has reached 200,000.
// Checks both players every call; player_add_life_flag prevents duplicates.
void add_life() {
    if (hq_status != HQ_STATUS_ALIVE) return;  // HQ destroyed — no bonus lives

    // Player 1
    if (player_add_life_flag[0] == 0 && hiscore_1p_string[2] >= 2) {
        player_lives[0]++;
        player_add_life_flag[0] = 1;
        sound_play(SND_ANCILLARY_LIFE1);
        sound_play(SND_ANCILLARY_LIFE2);
        return;
    }

    // Player 2 (only in 2-player mode)
    if (cursor_pos == 0) return;
    if (player_add_life_flag[1] != 0) return;
    if (hiscore_2p_string[2] < 2) return;
    player_lives[1]++;
    player_add_life_flag[1] = 1;
    sound_play(SND_ANCILLARY_LIFE1);
    sound_play(SND_ANCILLARY_LIFE2);
}


// Math trick to replace ABS_DIFF - optimization for top hotspot places in frame loop
#define OUT_OF_RANGE_10(a, b) ((unsigned char)(((a) - (b)) + 9) > 18)
#define OUT_OF_RANGE_6(a, b) ((unsigned char)(((a) - (b)) + 5) > 10)
#define OUT_OF_RANGE_12(a, b) ((unsigned char)(((a) - (b)) + 11) > 22)

static void render_bonus(void) {
    if (bonus_x == 0) return; // No bonus on screen
    
    if (bonus_timer != 0) {
        // Bonus was taken, draw the 500 points score and count down the timer
        --bonus_timer;
        if (bonus_timer == 0) {
            bonus_x = 0; // Clear bonus completely from memory after score disappears
            return;
        }
        // Draw 500 points sprite
        current_spr_tile_index = TILE_500_PTS; // Tile index for '500'
        pal = 2;
        SPR_PRIORITY = 0; // In front of background
        add_sprite_16x16(bonus_x, bonus_y, current_spr_tile_index, pal);
        SPR_PRIORITY = 0x20;
    } else {
        // Bonus not yet taken, draw the blinking bonus item
        if ((FRAME_CNT & 8) == 0) return; // Blink effect: hide sprite every 8 frames
        
        // 4 tiles per bonus icon, base offset 0x81
        current_spr_tile_index = (bonus_number << 2) + TILE_BONUS_BASE; 
        pal = 2;
        SPR_PRIORITY = 0;
        add_sprite_16x16(bonus_x, bonus_y, current_spr_tile_index, pal);
        SPR_PRIORITY = 0x20;
    }
}

static void bonus_helmet() {
    invincible_timer[i] = 10;
}

static void bonus_watch() {
    enemy_freeze_timer = ENEMY_FREEZE_TIME;
}

static void bonus_shovel() {
    if (hq_status & HQ_STATUS_ALIVE) {  // HQ still standing (bit 7 set = intact)
        draw_armoured_hq();
        hq_armor_timer = HQ_ARMOR_TIME;
    }
}

static void bonus_star() {
    if (player_type[i] == TANK_TYPE_3) return;  // already max type
    player_type[i] += 0x20;
    tank_type[i] = player_type[i];
}

static void bonus_grenade() {
    //cnt - iterator
    sound_play(SND_ENEMY_EXPLODE);
    for (cnt = 7; cnt >= 2; --cnt) {
        if (tank_status[cnt] >= TANK_STATUS_ALIVE_MIN && tank_status[cnt] < TANK_STATUS_ALIVE_MAX) {
            tank_status[cnt] = TANK_STATUS_EXPLODE_INIT;
            tank_type[cnt] = 0;
        }
    }
}

static void bonus_life() {
    player_lives[i]++;
    sound_play(SND_ANCILLARY_LIFE1);
    sound_play(SND_ANCILLARY_LIFE2);
}

static void bonus_pistol() {
    // Not used — has an icon but does nothing
}

//Handle bonus pickup effect
void handle_bonus(void){
    //i is set as tank index
    //all other zp variables should be available now for any of commands
    typedef void (*bonus_handler_func)(void);
    static const bonus_handler_func bonus_jump_table[] = {
        bonus_helmet,
        bonus_watch,
        bonus_shovel,
        bonus_star,
        bonus_grenade,
        bonus_life,
        bonus_pistol,
    };
    bonus_jump_table[bonus_number]();
}

void handle_bonus_collision(void) {
    //i - tank index. Will be also used as tank index by all handling functions

    if (bonus_x == 0) return;        // no active bonus on screen
    if (bonus_timer != 0) return;    // points display still running

    // Check players 1 then 0 (loop counts down from 1)
    for (i = 1; i != 0xFF; --i) {
        if (tank_status[i] < TANK_STATUS_ALIVE_MIN) continue;         // not active
        if (tank_status[i] >= TANK_STATUS_ALIVE_MAX) continue;        // exploding/respawning
        
        if (OUT_OF_RANGE_12(tank_x[i], bonus_x)) continue;
        if (OUT_OF_RANGE_12(tank_y[i], bonus_y)) continue;

        // Collision confirmed
        bonus_timer = BONUS_SCORE_FRAMES;  // show score for 50 frames

        if (bonus_number & 0x80) return;  // bit 7 = newly spawned, not yet active

        if (stage_mode != STAGE_MODE_DEMO) {
            num_to_num_string(SCORE_BONUS);
            add_score(i);
            add_life();
            sound_play(SND_BONUS_TAKEN);
        }
        handle_bonus();
        return;
    }
}


// Multiply_Bonus_Coord maps RNG (0..3) to screen coordinates: 48, 96, 144, 192
// Formula: (val + 1) * 48
static unsigned char multiply_bonus_coord(unsigned char n) {
    //i - temp accumulator
    i = n;
    i = i << 1;
    i += n;     // n*3
    i = i << 1; // n*6
    i += 6;
    i = i << 3; // (n*6+6)*8
    return i;
}

static void bonus_appear_handle(void) {
    static const unsigned char BonusNumberTable[8] = { 0, 1, 2, 3, 4, 5, 4, 3 };

    sound_play(SND_BONUS_APPEARS);
    do {
        bonus_x = multiply_bonus_coord(get_random() & 3);
        bonus_y = multiply_bonus_coord(get_random() & 3);
        //bonus cannot appear under player
        //make fake bonus and check if one of players taken it, if so, get new coordinates
        bonus_number = 0xFF;
        bonus_timer = 0;
        handle_bonus_collision();
    } while (bonus_timer != 0);

    bonus_number = BonusNumberTable[get_random() & 7];
    bonus_timer = 0;
}

// Process bullet-tank collisions (called once per frame)
static void handle_bullet_tank_collisions(void) {
    // i - tank id
    // j - bullet id
    // cnt - tank type index
    // k - temporary storage for i
    // temp - current status
    static const unsigned char EnemyKillScore[] = { SCORE_TYPE0, SCORE_TYPE1, SCORE_TYPE2, SCORE_TYPE3 };
    //Function is heavily used in gameplay loop, it needs optimization variables:
    static unsigned char current_x, current_y, idx;

    // ---------- Phase 1: Enemy bullets vs Players ----------
    // t = 1 (player 2), then 0 (player 1)
    for (i = 1; i != 0xFF; --i) {
        temp = tank_status[i];
        if (temp < TANK_STATUS_ALIVE_MIN || temp >= TANK_STATUS_ALIVE_MAX)
            continue;                     // tank not active
        current_x = tank_x[i];
        current_y = tank_y[i];
        // Enemy bullets are IDs 2..7
        for (j = 7; j >= 2; --j) {
            if (GET_BULLET_CMD(j) != BULLET_CMD_FLY)
                continue;                 

            // Collision if both axes differ by less than 10 pixels
            if (OUT_OF_RANGE_10(bullet_x[j], current_x)) continue;
            if (OUT_OF_RANGE_10(bullet_y[j], current_y)) continue;

            // Hit → start ricochet animation
            bullet_status[j] = BULLET_RICOCHET_INIT;

            // Spawn shield? Bullet disappears, no damage
            if (invincible_timer[i] != 0) {
                bullet_status[j] = 0;
                continue;
            }

            // Player destroyed
            tank_status[i] = TANK_STATUS_EXPLODE_INIT;
            sound_play(SND_PLAYER_EXPLODE);
            player_type[i] = 0;           // reset to basic tank
            tank_type[i] = 0;
            goto next_player;             // skip remaining bullets for this tank
        }
        next_player:;
    }

    // ---------- Phase 2: Player bullets vs Enemies ----------
    // Enemies are IDs 7 down to 2
    for (i = 7; i >= 2; --i) {
        temp = tank_status[i];
        if (temp < TANK_STATUS_ALIVE_MIN || temp >= TANK_STATUS_ALIVE_MAX)
            continue;
        current_x = tank_x[i];
        current_y = tank_y[i];
        // Player bullets: IDs 0,1,8,9. Original checks (b & 6) == 0.
        for (idx = 0; idx < 4; ++idx) {
            j = PlayerBulletIds[idx];
            if (GET_BULLET_CMD(j) != BULLET_CMD_FLY) continue;

            if (OUT_OF_RANGE_10(bullet_x[j], current_x)) continue;
            if (OUT_OF_RANGE_10(bullet_y[j], current_y)) continue;

            // Collision
            bullet_status[j] = BULLET_RICOCHET_INIT;

            // Bonus tank → spawn item
            if (tank_type[i] & 4) {
                //i will be modified by underlying functions of bonus_appear_handle
                k = i;
                bonus_appear_handle();
                i = k;
            }

            // Tank has armour? Reduce it, tank survives
            k = tank_type[i];
            if ((k & 3) != 0) {
                --k;
                tank_type[i] = k;
                sound_play(SND_ARMOUR_RICOCHET_TANK);
                continue;
            }

            // No armour → destroy enemy
            tank_status[i] = TANK_STATUS_EXPLODE_INIT;
            sound_play(SND_ENEMY_EXPLODE);

            if (stage_mode != STAGE_MODE_DEMO) {
                cnt = (tank_type[i] >> 5) & 7; // 0..7
                if (cnt >= 4) cnt -= 4;                     // map to 0..3 for killcount
                if (j & 1) {
                    ++killcount_2p[cnt];
                } else {
                    ++killcount_1p[cnt];
                }
                num_to_num_string(EnemyKillScore[cnt]);
                add_score(j & 1); //bullets odd for player 1, even for player 2
                add_life();
            }
            break;   // enemy destroyed – don't check other bullets against same tank
        }
    }

    // ---------- Phase 3: Friendly fire (players vs players) ----------
    for (i = 1; i != 0xFF; --i) {
        temp = tank_status[i];
        if (temp < TANK_STATUS_ALIVE_MIN || temp >= TANK_STATUS_ALIVE_MAX)
            continue;
        current_x = tank_x[i];
        current_y = tank_y[i];
        // Bullets from the other player only: original uses (player_id ^ bullet_id) & 1 != 0
        for (idx = 0; idx < 4; ++idx) {
            j = PlayerBulletIds[idx];         // only player bullet IDs
            if (((i ^ j) & 1) == 0) continue;    // skip own bullet
            if (GET_BULLET_CMD(j) != BULLET_CMD_FLY) continue;

            if (OUT_OF_RANGE_10(bullet_x[j], current_x)) continue;
            if (OUT_OF_RANGE_10(bullet_y[j], current_y)) continue;

            // Collision → ricochet
            bullet_status[j] = BULLET_RICOCHET_INIT;

            // Invincible shield? Bullet disappears completely
            if (invincible_timer[i] != 0) {
                bullet_status[j] = 0;
                continue;
            }

            // Friendly fire grace period active? No extra effect
            if (player_blink_timer[i] != 0 || stage_mode == STAGE_MODE_DEMO)
                continue;
            // Set new grace period
            player_blink_timer[i] = BLINK_TIMER_INIT;
        }
    }
}

// Process bullet-vs-bullet collisions (called once per frame)
// Two enemy bullets never check against each other.
static void handle_bullet_bullet_collisions(void) {
    //temp - bullet index
    for (temp = 0; temp < 4; ++temp) {
        i = PlayerBulletIds[temp]; //only player bullets
        if (GET_BULLET_CMD(i) != BULLET_CMD_FLY) continue;   // A not flying

        for (j = 9; j != 0xFF; --j) {
            if ((j & 7) == (i & 7)) continue;               // self (aliased 0/8, 1/9)
            if (GET_BULLET_CMD(j) != BULLET_CMD_FLY) continue; // B not flying

            if (OUT_OF_RANGE_6(bullet_x[j], bullet_x[i])) continue;
            if (OUT_OF_RANGE_6(bullet_y[j], bullet_y[i])) continue;

            // Bullets collided - destroy both
            bullet_status[i] = 0;
            bullet_status[j] = 0;
        }
    }
}

//Process Game Over sprite string appearance
static void gameover_str_handle(void) {
    //temp - old pal

    //Don't show game over on bonus level or if timer is not set
    if (gameover_str_timer == 0 || stage_mode == STAGE_MODE_DEMO) 
        return;
    if ((FRAME_CNT & 0x0F) == 0) {
        --gameover_str_timer;
        if (gameover_str_timer == 0) {
            gameover_str_y = SPRITE_OFFSCREEN_Y;//if finished, hide gamover string
        }
    }
    if (gameover_str_timer >= 10){//scroll until its 10 cycles left
        gameover_str_x += dx[gameover_scroll_dir];
        gameover_str_y += dy[gameover_scroll_dir];
    }
    SPR_PRIORITY = 0;
    add_sprite_16x16(gameover_str_x, gameover_str_y, TILE_GAMEOVER_L, 3);
    add_sprite_16x16(gameover_str_x + 16, gameover_str_y, TILE_GAMEOVER_R, 3);
    SPR_PRIORITY = 0x20;
}

static void handle_enemy_respawn(void) {
    //i - tank id
    //x,y - reinforcement icon coordinates

    if (respawn_timer != 0) {
        --respawn_timer;
        return;
    }
    
    if (enemy_reinforce_count == 0)
        return; // No enemies left in reserve

    // Find an empty enemy slot (from max_tanks_count down to 2)
    for (i = max_tanks_count; i >= 2; --i) {
        if (tank_status[i] == 0)
            break;
    }
    if (i < 2)
        return; // No free slot

    initialize_tank(i);
    respawn_timer = respawn_delay;
    --enemy_reinforce_count;
    x = (enemy_reinforce_count & 1) + 29;
    y = (enemy_reinforce_count >> 1) + 3;
    buffer_and_vram_fill(COORD_TO_NT_OFFSET(x, y), TILE_GREY, 1);
}


// Draws the static frame of the points screen: HI-SCORE, STAGE n,
// I-PLAYER/II-PLAYER + running totals, arrows, PTS labels and the
// TOTAL line.
static void draw_pts_screen_template(void)
{
    //cnt - length
    //k - offset from string start
    /* Original string data */
    static const unsigned char hi_score_label[] = "HIkSCORE"; // 'k' = '-' glyph
    static const unsigned char stage_label[] = "STAGE";
    static const unsigned char p1_label[] = "^kPLAYER"; // '^'=I  'k'=-  -> "I-PLAYER"
    static const unsigned char p2_label[] = "_kPLAYER"; // '_'=II         -> "II-PLAYER"
    static const unsigned char pts_label[] = "PTS";
    static const unsigned char total_label[] = "TOTAL";
    static const unsigned char line_label[] = {
        TILE_PTS_LINE, TILE_PTS_LINE, TILE_PTS_LINE, TILE_PTS_LINE, TILE_PTS_LINE, TILE_PTS_LINE, TILE_PTS_LINE, TILE_PTS_LINE  
    };

    frame_wait();
    scroll(0, 0);
    set_bg_pal_number(3);

    ppu_off();

    /* Null_NT_Buffer */
    memfill(SCREEN_BUFFER, 0, SCREEN_WIDTH * SCREEN_COLS);

    /* Fill_Attrib_Table */
    memfill(SCREEN_BUFFER + ATTR_OFFSET, 0x50, 4);
    memfill(SCREEN_BUFFER + ATTR_OFFSET + 4, 0xA0, 4);
    memfill(SCREEN_BUFFER + ATTR_OFFSET + 8, 0x50, 3);
    memfill(SCREEN_BUFFER + ATTR_OFFSET + 0xD, 0x50, 3);
    memfill(SCREEN_BUFFER + ATTR_OFFSET + 0x10, 0x0A, 3);
    memfill(SCREEN_BUFFER + ATTR_OFFSET + 0x15, 0x0A, 3);

    SCREEN_BUFFER[0x3F0] = 0x05;
    SCREEN_BUFFER[0x3F1] = 0x05;
    SCREEN_BUFFER[0x3F2] = 0x05;
    SCREEN_BUFFER[0x3F5] = 0x05;
    SCREEN_BUFFER[0x3F6] = 0x05;
    SCREEN_BUFFER[0x3F7] = 0x05;

    vram_adr(NAMETABLE_C);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);

    /* Set_PPU */
    set_ppu();
    scroll(0, 240);

    /*
        Draw:
        HIkSCORE
        high score digits
    */
    add_vram_cpy_cmd(NAMETABLE_C+ COORD_TO_NT_OFFSET(8, 3), hi_score_label, sizeof(hi_score_label) - 1);

    k = skip_leading_zeros(hiscore_string, 1);
    cnt = 0;
    while (hiscore_string[k + cnt] != 0xFF) {
        ++cnt;
    }

    add_vram_cpy_based_cmd(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x11 + k, 3), &hiscore_string[k], cnt, TILE_DIGIT_BASE);

    /*
        Draw:
        STAGE
        stage number
    */
    add_vram_cpy_cmd(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x0C, 5), stage_label, sizeof(stage_label) - 1);

    set_number_string(stage_number);

    k = skip_leading_zeros(number_string, 1);

    cnt = 0;
    while (number_string[k + cnt] != 0xFF) {
        ++cnt;
    }

    add_vram_cpy_based_cmd(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x0D + k, 5), &number_string[k], cnt, TILE_DIGIT_BASE);

    frame_wait();
    ppu_mask(MASK_SHOW_ALL);

    /*
        Draw:
        I-PLAYER
        player 1 score
    */
    add_vram_cpy_cmd(NAMETABLE_C+ COORD_TO_NT_OFFSET(3, 7), p1_label, sizeof(p1_label) - 1);

    k = skip_leading_zeros(hiscore_1p_string, 1);
    cnt = 0;
    while (hiscore_1p_string[k + cnt] != 0xFF) {
        ++cnt;
    }

    add_vram_cpy_based_cmd(
        NAMETABLE_C+ COORD_TO_NT_OFFSET(4 + k, 9),
        &hiscore_1p_string[k],
        cnt,
        TILE_DIGIT_BASE
    );

    /* Arrow_Left x4 */
    add_vram_fill_cmd(
        (unsigned char *)(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x0E, 0x0C)),
        TILE_PTS_ARROW_LEFT,
        1
    );
    add_vram_fill_cmd(
        (unsigned char *)(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x0E, 0x0F)),
        TILE_PTS_ARROW_LEFT,
        1
    );
    add_vram_fill_cmd(
        (unsigned char *)(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x0E, 0x12)),
        TILE_PTS_ARROW_LEFT,
        1
    );
    add_vram_fill_cmd(
        (unsigned char *)(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x0E, 0x15)),
        TILE_PTS_ARROW_LEFT,
        1
    );
    //Draw second player, if it was 2p game
    if (cursor_pos != 0) {
        frame_wait();

        /*
            Draw:
            II-PLAYER
            player 2 score
        */
        add_vram_cpy_cmd(
            NAMETABLE_C+ COORD_TO_NT_OFFSET(0x15, 7),
            p2_label,
            sizeof(p2_label) - 1
        );

        k = skip_leading_zeros(hiscore_2p_string, 1);
        cnt = 0;
        while (hiscore_2p_string[k + cnt] != 0xFF) {
            ++cnt;
        }

        add_vram_cpy_based_cmd(
            NAMETABLE_C+ COORD_TO_NT_OFFSET(0x16 + k, 9),
            &hiscore_2p_string[k],
            cnt,
            TILE_DIGIT_BASE
        );

        /* Arrow_Right x4 */
        add_vram_fill_cmd(
            (unsigned char *)(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x11, 0x0C)),
            TILE_PTS_ARROW_RIGHT,
            1
        );
        add_vram_fill_cmd(
            (unsigned char *)(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x11, 0x0F)),
            TILE_PTS_ARROW_RIGHT,
            1
        );
        add_vram_fill_cmd(
            (unsigned char *)(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x11, 0x12)),
            TILE_PTS_ARROW_RIGHT,
            1
        );
        add_vram_fill_cmd(
            (unsigned char *)(NAMETABLE_C+ COORD_TO_NT_OFFSET(0x11, 0x15)),
            TILE_PTS_ARROW_RIGHT,
            1
        );
    }

    frame_wait();

    /* PTS left column */
    add_vram_cpy_cmd(
        NAMETABLE_C+ COORD_TO_NT_OFFSET(8, 0x0C),
        pts_label,
        sizeof(pts_label) - 1
    );
    add_vram_cpy_cmd(
        NAMETABLE_C+ COORD_TO_NT_OFFSET(8, 0x0F),
        pts_label,
        sizeof(pts_label) - 1
    );
    add_vram_cpy_cmd(
        NAMETABLE_C+ COORD_TO_NT_OFFSET(8, 0x12),
        pts_label,
        sizeof(pts_label) - 1
    );
    add_vram_cpy_cmd(
        NAMETABLE_C+ COORD_TO_NT_OFFSET(8, 0x15),
        pts_label,
        sizeof(pts_label) - 1
    );

    /* PTS right column only in 2-player mode */
    if (cursor_pos != 0) {
        frame_wait();

        add_vram_cpy_cmd(
            NAMETABLE_C+ COORD_TO_NT_OFFSET(0x1A, 0x0C),
            pts_label,
            sizeof(pts_label) - 1
        );
        add_vram_cpy_cmd(
            NAMETABLE_C+ COORD_TO_NT_OFFSET(0x1A, 0x0F),
            pts_label,
            sizeof(pts_label) - 1
        );
        add_vram_cpy_cmd(
            NAMETABLE_C+ COORD_TO_NT_OFFSET(0x1A, 0x12),
            pts_label,
            sizeof(pts_label) - 1
        );
        add_vram_cpy_cmd(
            NAMETABLE_C+ COORD_TO_NT_OFFSET(0x1A, 0x15),
            pts_label,
            sizeof(pts_label) - 1
        );
    }

    frame_wait();

    /* Line above TOTAL */
    add_vram_cpy_cmd(
        NAMETABLE_C+ COORD_TO_NT_OFFSET(0x0C, 0x16),
        line_label,
        sizeof(line_label)
    );

    /* TOTAL */
    add_vram_cpy_cmd(
        NAMETABLE_C+ COORD_TO_NT_OFFSET(6, 0x17),
        total_label,
        sizeof(total_label) - 1
    );
}

static void draw_tank_column(void)
{
    add_sprite_16x16(PTS_TANK_X, 0x64, PTS_TANK_TYPE0, 2);
    add_sprite_16x16(PTS_TANK_X, 0x7C, PTS_TANK_TYPE1, 2);
    add_sprite_16x16(PTS_TANK_X, 0x94, PTS_TANK_TYPE2, 2);
    add_sprite_16x16(PTS_TANK_X, 0xAC, PTS_TANK_TYPE3, 2);
}
static void draw_tank_column_xtimes(unsigned char count)
{
    while (count != 0) {
        frame_wait();
        draw_tank_column();
        --count;
    }
}

static void draw_score_digits(
    const unsigned char *str,
    unsigned char base_x,
    unsigned char y)
{
    //cnt - length
    //k - offset from string start

    k = skip_leading_zeros(str, 1);
    cnt = 0;
    while (str[k + cnt] != 0xFF) {
        ++cnt;
    }

    if (cnt != 0) {
        add_vram_cpy_based_cmd(
            NAMETABLE_C + COORD_TO_NT_OFFSET(base_x + k, y),
            &str[k],
            cnt,
            TILE_DIGIT_BASE
        );
    }
}

static void add_number_to_temp_string(unsigned char *dst)
{
    //cnt - sum
    //k - carry flag
    //x - index of char in string

    k = 0;
    for (x = 6; x != 0xFF; --x) {
        cnt = number_string[x] + dst[x] + k;

        if (cnt >= 10) {
            cnt -= 10;
            k = 1;
        } else {
            k = 0;
        }

        dst[x] = cnt;
    }
}

static void draw_pts_screen(void)
{
    //i - tank id
    //j - row number
    //temp - end count flag
    //y - type index

    static const unsigned char tank_kill_pts[4] = {
        SCORE_TYPE0, SCORE_TYPE1, SCORE_TYPE2, SCORE_TYPE3
    };
    //BONUS! string
    static const unsigned char bonus_label[] = {
        0x42, 0x4F, 0x4E, 0x55, 0x53, 0x15
    };

    static const unsigned char pts_label[] = "PTS";

    static unsigned char temp_1p_pts_string[8];
    static unsigned char temp_2p_pts_string[8];

    unsigned char total_kills_1p;
    unsigned char total_kills_2p;
    unsigned char p1_type_count;
    unsigned char p2_type_count;


    draw_pts_screen_template();
    draw_tank_column_xtimes(0x1E);

    total_kills_1p = 0;
    total_kills_2p = 0;

    for (i = 0; i < 4; ++i) {
        total_kills_1p += killcount_1p[i];
        total_kills_2p += killcount_2p[i];
    }
    y = 0;
    while (1) {
        frame_wait();
        draw_tank_column();

        init_8_char_string(temp_1p_pts_string);
        init_8_char_string(temp_2p_pts_string);

        p1_type_count = 0;
        p2_type_count = 0;

        /*
            Rows used by original:
            type 0 -> $0C
            type 1 -> $0F
            type 2 -> $12
            type 3 -> $15
        */
        j = (unsigned char)((y * 3) + 0x0C);

        do {
            frame_wait();
            draw_tank_column();
            temp = 0;
            num_to_num_string(tank_kill_pts[y]);

            if (killcount_1p[y] != 0) {
                --killcount_1p[y];
                ++p1_type_count;

                add_number_to_temp_string(temp_1p_pts_string);
                temp = 1;

                sound_play(SND_PTS_COUNT1);
                sound_play(SND_PTS_COUNT2);
                add_life();
            }
            if (killcount_2p[y] != 0) {
                --killcount_2p[y];
                ++p2_type_count;
                add_number_to_temp_string(temp_2p_pts_string);
                temp = 1;

                sound_play(SND_PTS_COUNT1);
                sound_play(SND_PTS_COUNT2);
                add_life();
            }
            draw_score_digits(hiscore_1p_string, 4, 9);
            draw_score_digits(temp_1p_pts_string, 0, j);
            set_number_string(p1_type_count);
            draw_score_digits(number_string, 7, j);
            if (cursor_pos != 0) {
                draw_score_digits(hiscore_2p_string, 0x16, 9);
                draw_score_digits(temp_2p_pts_string, 0x12, j);
                set_number_string(p2_type_count);
                draw_score_digits(number_string, 0x0D, j);
            }

            draw_tank_column_xtimes(8);

        } while (temp != 0);

        ++y;

        if (y == 4) {
            break;
        }
        draw_tank_column_xtimes(0x14);
    }

    draw_tank_column_xtimes(0x1E);

    set_number_string(total_kills_1p);
    draw_score_digits(number_string, 7, 0x17);

    if (cursor_pos != 0) {
        set_number_string(total_kills_2p);
        draw_score_digits(number_string, 0x0D, 0x17);
    }

    draw_tank_column_xtimes(0x0F);

    if (cursor_pos != 0 && hq_status != 0) {
        if (total_kills_2p < total_kills_1p && player_lives[0] != 0) {
            /* Num_To_NumString(0) means 1000 points in original. */
            num_to_num_string(0);

            add_score(0);

            /* Redraw updated P1 score. */
            draw_score_digits(hiscore_1p_string, 4, 9);
            draw_score_digits(number_string, 0, 0x1A);
            add_vram_cpy_cmd(
                NAMETABLE_C + COORD_TO_NT_OFFSET(3, 0x19),
                bonus_label,
                sizeof(bonus_label));
            add_vram_cpy_cmd(
                NAMETABLE_C + COORD_TO_NT_OFFSET(8, 0x1A),
                pts_label,
                sizeof(pts_label) - 1);

            sound_play(SND_BONUS_PTS);
            add_life();
        }
        else if (total_kills_1p < total_kills_2p && player_lives[1] != 0) {
            /* Num_To_NumString(0) means 1000 points in original. */
            num_to_num_string(0);
            add_score(1);
            /* Redraw updated P2 score. */
            draw_score_digits(hiscore_2p_string, 0x16, 9);
            draw_score_digits(number_string, 0x13, 0x1A);

            /* Original: BONUS text at ($16,$19), PTS at ($1B,$1A). */
            add_vram_cpy_cmd(
                NAMETABLE_C + COORD_TO_NT_OFFSET(0x16, 0x19),
                bonus_label,
                sizeof(bonus_label));

            add_vram_cpy_cmd(
                NAMETABLE_C + COORD_TO_NT_OFFSET(0x1B, 0x1A),
                pts_label,
                sizeof(pts_label) - 1);

            sound_play(SND_BONUS_PTS);
            add_life();
        }
    }
    draw_tank_column_xtimes(120);//just wait 120 frames after drawing everything on screen
    set_bg_pal_number(0);
}
static void init_gameover_str_center(void)
{
    gameover_str_x = 0x70;
    gameover_str_y = SPRITE_OFFSCREEN_Y;
    gameover_scroll_dir = 0;      /* up */
    gameover_str_timer = 0x11;
    FRAME_CNT = 0;
}

static unsigned char level_end_check(void)
{
    /* HQ destroyed: game over. */
    if (hq_status == 0) {
        init_gameover_str_center();
        return 1;
    }

    /* All enemies destroyed: stage clear. */
    if (enemy_counter == 0)
        return 1;

    /* At least one player still has lives: continue. */
    if ((unsigned char)(player_lives[0] + player_lives[1]) != 0)
        return 0;

    /* Both players dead: game over. */
    init_gameover_str_center();
    return 1;
}

static unsigned char detect_motion(unsigned char id) {
    if (!(PAD_STATE[id] & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT)))
        return 0;                          // no direction held
    if (tank_status[id] == 0)
        return 0;                          // no tank
    return 1;
}

static void play_snd_move(void) {
    if (stage_mode == STAGE_MODE_DEMO) return;  // silent during demo
    if (detect_motion(0) || detect_motion(1)) {
        if (!snd_move_active || !sound_is_playing(SND_MOVE)) {
            if (sound_play(SND_MOVE))
                snd_move_active = 1;
        }
    } else {
        if (snd_move_active) {
            snd_move_active = 0;
            sound_stop(SND_MOVE);
        }
    }
}

//Main operations processed during gameplay frame (not in pause)
static void handle_gameplay(){

    mark_tiles_under_tanks();
    handle_input();
    handle_all_tanks_commands();
    clear_tiles_under_tanks();
    handle_all_bullets_statuses();
    handle_hq_armor();
    handle_hq_explode();
    handle_invincibility();
    make_player_shot();
    make_enemy_shot();
    handle_enemy_respawn();
    handle_bullet_bg_collisions();
    handle_bullet_bullet_collisions();
    handle_bullet_tank_collisions();
    handle_bonus_collision();
    gameover_str_handle();
    play_snd_move();
    draw_player_lives();
    swap_water_pal_colors();    
}


GameState stage_start_loop(void) {

    sound_stop_all();
    sound_play(SND_BATTLE1);
    sound_play(SND_BATTLE2);
    sound_play(SND_BATTLE3);

    if (construction_flag == 0) {
        make_gray_frame();
        load_stage();
        draw_normal_hq();
    } else {
        draw_naked_hq();
    }


    reset_vram_cmd_queue();//hq was written to commands buffer, dont dump it to vram: it should not be shown on top of grey screen
    draw_diverging_shutters();

    set_bg_pal_number(0);
    frame_wait();//Copy palette and also process commands buffer (may not fit into single NMI)
    reset_stage_vars();	
	return STATE_GAMEPLAY;	
}

static void stop_move_engine_sounds(void)
{
    snd_move_active = 0;
    sound_stop(SND_MOVE);
    sound_stop(SND_ENGINE);
}

static void freeze_player_on_hq_destroy(void)
{
    /* Original freezes player input when HQ is not intact. */
    if (hq_status != HQ_STATUS_ALIVE) {
        PAD_STATE[0]  = 0;
        PAD_STATE[1]  = 0;
        PAD_STATET[0] = 0;
        PAD_STATET[1] = 0;
    }
}
static void after_death_battle_run(void)
{
    unsigned char id;//have to emit separate var, as many underlying functions are triggered inside

    SECONDS_CNT = 0;
    FRAME_CNT = 0;

    /* Original:
       - normal level exit waits 2 seconds
       - active GAME OVER string waits longer by starting at 0xFE
    */
    if (gameover_str_timer != 0)
        SECONDS_CNT = 0xFE;

    do {
        frame_wait();

        pad_poll(0);
        pad_poll(1);

        freeze_player_on_hq_destroy();

        /* Gameplay continues during this run, but pause is ignored. */
        handle_gameplay();

        /* Original AfterDeath render order:
           bonus, tanks, bullets, water palette swap. */
        render_bonus();

        for (id = 0; id < 8; ++id)
            render_tank(id);

        render_bullets();

        /* handle_gameplay() already calls this once;
           original AfterDeath calls it again. */
        swap_water_pal_colors();

    } while (SECONDS_CNT != 2);
}



GameState gameplay_loop(void) {
    unsigned char id;
    while(1){
        frame_wait();
        pad_poll(0);
        pad_poll(1);

        if (!pause_flag) {
            handle_gameplay();
        }
        render_bonus();
        render_bullets();
        for(id = 0; id < 8; ++id){
            render_tank(id);    // draw spawn ring / tank
        }
        if (PAD_STATET[0] & PAD_START) {
            pause_flag ^= 1;
            sound_set_paused(pause_flag);
            if (pause_flag) 
                sound_play(SND_PAUSE);
        }
        render_pause();
        if (level_end_check())
            break;
    }
    /* Stop timers/movement sounds like original. */
    SECONDS_CNT = 0;
    FRAME_CNT = 0;
    pause_flag = 0;
    sound_set_paused(0);
    stop_move_engine_sounds();
    after_death_battle_run();
    sound_stop_all();
    draw_pts_screen();

    /* Advance stage like original. */
    ++stage_number;

    if (stage_number == FINAL_STAGE) {
        stage_number = 1;
        stage_mode = STAGE_MODE_NORMAL;
    }

    if (stage_number == HARD_MODE_STAGE) {
        stage_mode = STAGE_MODE_HARD;
    }

    /* Original game-over condition after score screen. */
    if ((unsigned char)(player_lives[0] + player_lives[1]) == 0 ||
        hq_status != HQ_STATUS_ALIVE) {
        return STATE_GAME_OVER;
    }

    /* Next stage.
       skip_stage_select_flag is already set, so stage_select_loop
       should immediately start the next level. */
    return STATE_STAGE_SELECT;
}

//Game over state
static unsigned char score_is_greater(const unsigned char *score, const unsigned char *hi)
{
    //j - string index
    for (j = 0; j < 7; ++j) {
        if (score[j] != hi[j]) {
            return score[j] > hi[j];
        }
    }

    return 0;
}

static unsigned char update_hiscore(void)
{
    //i - string index
    //temp - boolean result isRecord
    temp = 0;

    if (score_is_greater(hiscore_1p_string, hiscore_string)) {
        for (i = 0; i < 7; ++i) {
            hiscore_string[i] = hiscore_1p_string[i];
        }
        temp = 1;
    }

    if (score_is_greater(hiscore_2p_string, hiscore_string)) {
        for (i = 0; i < 7; ++i) {
            hiscore_string[i] = hiscore_2p_string[i];
        }
        temp = 1;
    }

    return temp;
}

static void clear_nt(void)
{
    ppu_off();

    memfill(SCREEN_BUFFER, 0, SCREEN_WIDTH * SCREEN_COLS);

    vram_adr(NAMETABLE_A);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);

    set_ppu();
    ppu_mask(MASK_SHOW_ALL);
}

static void draw_brick_gameover(void)
{
    //temp16 - wait frames
    static const unsigned char game_str[] = {
        'G', 'A', 'M', 'E', 0xFF
    };

    static const unsigned char over_str[] = {
        'O', 'V', 'E', 'R', 0xFF
    };

    SECONDS_CNT = 0;

    ppu_off();

    memfill(SCREEN_BUFFER, 0, SCREEN_WIDTH * SCREEN_COLS);

    draw_brick_str(game_str, 0x3C, 0x46);
    draw_brick_str(over_str, 0x3C, 0x78);

    vram_adr(NAMETABLE_A);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);

    set_ppu();
    scroll(0, 0); 
    ppu_mask(MASK_SHOW_ALL);

    sound_play(SND_GAME_OVER1);
    sound_play(SND_GAME_OVER2);
    sound_play(SND_GAME_OVER3);

    while (sound_is_playing(SND_GAME_OVER1)) {
        frame_wait();
        pad_poll(0);

        if (PAD_STATET[0] & (PAD_SELECT | PAD_START)) {
            break;
        }
    }

    /* Original End_Draw_Brick_GameOver clears the name table. */
    ppu_off();

    memfill(SCREEN_BUFFER, 0, SCREEN_WIDTH * SCREEN_COLS);

    vram_adr(NAMETABLE_A);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);

    set_ppu();
    ppu_mask(MASK_SHOW_ALL);

    sound_stop_all();
}

static void draw_record_digit(void)
{
    //x - X coordinate of digit
    //i - index of char in string

    i = 0;
    while (hiscore_string[i] == 0) {
        ++i;
    }

    if (hiscore_string[i] == 0xFF) {
        i = 6;
    }
    x = (unsigned char)(0x10 + (i << 5));

    while (hiscore_string[i] != 0xFF) {
        draw_brick_char(hiscore_string[i] + TILE_DIGIT_BASE, x, 0x64);
        x += 0x20;
        ++i;
    }
}

static void draw_record_hiscore(void)
{
    //temp - counter for palette effect
    static const unsigned char hiscore_str[] = {
        'H', 'I', 'S', 'C', 'O', 'R', 'E', 0xFF
    };

    SECONDS_CNT = 0;

    ppu_off();

    memfill(SCREEN_BUFFER, 0, SCREEN_WIDTH * SCREEN_COLS);

    draw_brick_str(hiscore_str, 0x10, 0x32);
    draw_record_digit();

    vram_adr(NAMETABLE_A);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);

    set_ppu();
    ppu_mask(MASK_SHOW_ALL);

    sound_play(SND_RECORD_PTS1);
    sound_play(SND_RECORD_PTS2);
    sound_play(SND_RECORD_PTS3);
    
    temp = 0;
    while (sound_is_playing(SND_RECORD_PTS1)) {
        frame_wait();
        set_bg_pal_number((unsigned char)(5 + (temp & 3)));
        ++temp;
    }
    set_bg_pal_number(0);
}

GameState game_over_loop(void)
{
    oam_clear();
    pause_flag = 0;

    draw_brick_gameover();

    if (update_hiscore()) {
        draw_record_hiscore();
        clear_nt();
    }

    /* Original jumps to Title_Screen_Loop_Start. */
    set_bg_pal_number(3);

    ppu_off();
    memfill(SCREEN_BUFFER, 0, SCREEN_WIDTH * SCREEN_COLS);

    draw_title_screen();

    set_ppu();
    ppu_mask(MASK_SHOW_ALL);

    construction_flag = 0;
    // Reset flag so the next playthrough starts fresh at the title screen
    skip_stage_select_flag = 0;

    return STATE_TITLE_INTRO;
}

static void check_border_reach(void) {
    if (tank_x[0] > 0xD8) tank_x[0] = CONSTRUCT_X_MAX;
    if (tank_x[0] < 0x18) tank_x[0] = CONSTRUCT_X_MIN;
    if (tank_y[0] > 0xD8) tank_y[0] = CONSTRUCT_Y_MAX;
    if (tank_y[0] < 0x18) tank_y[0] = CONSTRUCT_Y_MIN;
}

static void move_tank_construction(void) {
    if (PAD_STATE[0] & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT)) {
        ++joypad_delay;
        bkg_occurence_flag = 0;   // any direction held resets the "already placed" flag
    } else {
        joypad_delay = 0;
    }

    if (joypad_delay == 0x14) {
        joypad_delay = 0xF;               // repeat every 5 frames from here on
        if (PAD_STATE[0] & PAD_RIGHT)      tank_x[0] += 0x10;
        else if (PAD_STATE[0] & PAD_LEFT)  tank_x[0] -= 0x10;
        else if (PAD_STATE[0] & PAD_DOWN)  tank_y[0] += 0x10;
        else if (PAD_STATE[0] & PAD_UP)    tank_y[0] -= 0x10;
        return;
    }

    if (!(PAD_STATET[0] & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT))) {
        return;
    }
    if (PAD_STATET[0] & PAD_RIGHT)      tank_x[0] += 0x10;
    else if (PAD_STATET[0] & PAD_LEFT)  tank_x[0] -= 0x10;
    else if (PAD_STATET[0] & PAD_DOWN)  tank_y[0] += 0x10;
    else if (PAD_STATET[0] & PAD_UP)    tank_y[0] -= 0x10;
}

static void draw_tsa_on_tank(void) {
    write_tsa_to_vram((tank_x[0] >> 3) & 0xFE, (tank_y[0] >> 3) & 0xFE, tsa_block_number & 0x0F);
}



GameState construction_loop(void) {
    if (construction_flag == 0) {
        // First time in this session: draw a fresh grey frame.
        // Re-entering (Skip_LoadFrame in the ROM) preserves whatever
        // was already built - don't touch the nametable.
        ppu_off();
        make_gray_frame();
        vram_adr(NAMETABLE_A);
        vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);
        set_ppu();
        ppu_mask(MASK_SHOW_ALL);
    }

    null_status();
    tank_x[0] = CONSTRUCT_X_MIN - 8;
    tank_y[0] = CONSTRUCT_Y_MIN;
    tank_status[0] = TANK_CMD_DRIFT | TANK_DIR_UP; // ROM sets Tank_Status=0x84 here (0x83="right" on the title tank) - use whichever TANK_DIR_* matches low nibble 4
    tank_type[0] = TANK_TYPE_0;
    SPR_PRIORITY = 0;          // tank always drawn in front, even over forest, while editing
    track_pos[0] = 0;
    bkg_occurence_flag = 0;
    joypad_delay = 0;
    tsa_block_number = 0;
    scroll(0, 0);
    player_blink_timer[0] = 0;
    player_blink_timer[1] = 0;

    if (construction_flag == 0) {
        draw_normal_hq();
    }

    while (1) {
        frame_wait();
        pad_poll(0);

        move_tank_construction();
        check_border_reach();

        if (FRAME_CNT & 0x10) {
            render_tank(0);
        }

        // Stationary + a fresh A/B press: first press just places the
        // current block; a second press (without moving away) cycles
        // to the next/previous block type instead.
        if (!(PAD_STATE[0] & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT))) {
            if (PAD_STATET[0] & PAD_A) {
                if (bkg_occurence_flag != 0) {
                    ++tsa_block_number;
                    if (tsa_block_number == 14) tsa_block_number = 0;
                }
                bkg_occurence_flag = 1;
            } else if (PAD_STATET[0] & PAD_B) {
                if (bkg_occurence_flag != 0) {
                    tsa_block_number = (tsa_block_number == 0) ? 13 : tsa_block_number - 1;
                }
                bkg_occurence_flag = 1;
            }
        }

        // A/B held (whether just pressed, held while stationary, or
        // held while dragging across tiles) paints the current block.
        if (PAD_STATE[0] & (PAD_A | PAD_B)) {
            draw_tsa_on_tank();
        }

        if (PAD_STATET[0] & PAD_START) {
            break;
        }
    }

    SPR_PRIORITY = 0x20;
    scroll(0, 240);         // 240 = NAMETABLE_C, where the title screen art lives
    ++construction_flag;    // matches INC Construction_Flag (used elsewhere for the secret-code check)
    return STATE_TITLE_MENU;
}


// Demo AI: decides where each player tank should aim, then writes the
// corresponding joypad state into PAD_STATE so handle_input() picks it up.
static void demo_ai(void) {
    // Joypad bytes the demo AI writes for each direction (includes A+B for firing)
    // Index: 0=up, 1=left, 2=down, 3=right
    static const unsigned char demo_tank_direction[4] = {
        PAD_UP    | PAD_A | PAD_B,   // 0 = up
        PAD_LEFT  | PAD_A | PAD_B,   // 1 = left
        PAD_DOWN  | PAD_A | PAD_B,   // 2 = down
        PAD_RIGHT | PAD_A | PAD_B,   // 3 = right
    };
    // temp - enemy id
    // j - player id
    // k - direction
    // cnt - target found

    for (j = 1; j != 0xFF; --j) {
        cnt = 0;

        // Priority 1: grab a live bonus on the field
        if (bonus_x != 0 && bonus_timer == 0) {
            ai_x_aim = bonus_x;
            ai_y_aim = bonus_y;
            cnt = 1;
        }

        // Priority 2: chase the first live enemy found.
        // Original probe order relative to player index: +2, +4, +3
        if (!cnt) {
            static const unsigned char offsets[3] = { 2, 4, 3 };

            for (i = 0; i < 3; ++i) {
                temp = j + offsets[i];
                if (temp >= 8) continue;
                if (tank_status[temp] < TANK_STATUS_ALIVE_MIN)  continue; // dying/exploding
                if (tank_status[temp] >= TANK_STATUS_ALIVE_MAX) continue; // spawning
                ai_x_aim = tank_x[temp];
                ai_y_aim = tank_y[temp];
                cnt = 1;
                break;
            }
        }

        if (cnt) {
            k = get_ai_status(j) & 0x03;
            PAD_STATE[j]  = demo_tank_direction[k];
            PAD_STATET[j] = demo_tank_direction[k];
        } else {
            PAD_STATE[j]  = 0;
            PAD_STATET[j] = 0;
        }

        // Don't let tanks drive off the bottom of the screen
        if (tank_y[j] >= ICE_DEMO_LIMIT_Y) {
            PAD_STATET[j] &= 0x0F; // clear direction bits
        }
    }
}

// Load the demo level into the screen buffer, draw title brick text over it,
// then set up all gameplay variables.
static void setup_demo_level(void) {
    // --- video setup ---
    ppu_off();
    make_gray_frame();

    // Load demo level data (36th entry in the levels array)
    stage_number = DEMO_STAGE_NUMBER;
    stage_mode = STAGE_MODE_DEMO; 
    load_stage();

    {
        static const unsigned char battle_str[] = "BATTLE\xFF";
        static const unsigned char city_str[]   = "CITY\xFF";
        draw_brick_str(battle_str, 0x1A, 0x46);
        draw_brick_str(city_str,   0x3C, 0x78);
    }

    vram_adr(NAMETABLE_A);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);
    set_ppu();
    ppu_mask(MASK_SHOW_ALL);
    scroll(0, 0);

    // --- gameplay variables (subset of reset_stage_vars, no engine sound) ---
    // Init_Level_VARs equivalent
    player_type[0] = 0;
    player_type[1] = 0;
    player_add_life_flag[0] = 0;
    player_add_life_flag[1] = 0;
    skip_stage_select_flag = 0;
    player_lives[0] = 3;
    player_lives[1] = 3;          // demo always has two players
    enemy_respawn_place_index = 3;


    // Display stage "30" in the corner (original shows 30 during demo)
    stage_number = DEMO_DISPLAY_STAGE;

    // SetUp_LevelVARs equivalent (no Snd_Engine)
    for (i = 0; i < sizeof(bullet_status); ++i) bullet_status[i] = 0;
    for (i = 0; i < sizeof(tank_status);   ++i) tank_status[i]   = 0;
    for (i = 0; i < sizeof(player_ice_status); ++i) player_ice_status[i] = 0;

    gameover_str_timer = 0;
    gameover_str_y     = 0xF0;

    initialize_tank(0);
    initialize_tank(1);

    enemy_reinforce_count     = MAX_ENEMIES;
    enemy_counter             = MAX_ENEMIES;
    enemy_type_number         = 0;
    SECONDS_CNT               = 0;
    construction_flag         = 0;
    hq_armor_timer            = 0;
    player_blink_timer[0]     = 0;
    player_blink_timer[1]     = 0;
    invincible_timer[0]       = 0;
    invincible_timer[1]       = 0;
    respawn_timer             = 0;
    bonus_x                   = 0;
    enemy_freeze_timer        = 0;
    enemy_respawn_place_index = 0;
    hq_status                 = HQ_STATUS_ALIVE;
    draw_normal_hq();

    for (i = 0; i < 4; ++i) {
        killcount_1p[i] = 0;
        killcount_2p[i] = 0;
    }

    // Respawn delay: same formula as original (190 - stage*4 - 20 for 2P)
    respawn_delay = 190 - (30 << 2) - 20;

    max_tanks_count = 5;  // original: 1 player + 4 enemies in demo

    frame_wait();
    load_reinforcements_icons();
    load_Ip_icons();
    load_level_flag();
    set_enemy_count();

    // NO sound_play_background(SND_ENGINE) here — demo is silent
}

GameState demoplay_loop(void) {
    //all zp temp variables are used in demo_ai call, don't occupy it here
    unsigned char pressed_exit = 0;
    unsigned char id;   // local counter — do not use global "i" here

    sound_stop_all();
    sound_set_muted(1);
    setup_demo_level();

    while (1) {
        frame_wait();
        pad_poll(0);
        pad_poll(1);
        if (PAD_STATET[0] & (PAD_SELECT | PAD_START)) {
            pressed_exit = 1;
            break;
        }
        demo_ai();
        handle_gameplay();
        render_bonus();
        for (id = 0; id < 8; ++id) {
            render_tank(id);
        }
        render_bullets();
        if (level_end_check())
            break;
    }

    sound_set_muted(0);
    sound_stop_all();
    pause_flag = 0;
    reset_vram_cmd_queue();   // drop any queued demo-frame VRAM writes before either exit

    ppu_off();
    set_bg_pal_number(3);
    memfill(SCREEN_BUFFER, 0, SCREEN_WIDTH * SCREEN_COLS);
    vram_adr(NAMETABLE_A);
    vram_write(SCREEN_BUFFER, SCREEN_WIDTH * SCREEN_COLS);
    set_ppu();
    ppu_mask(MASK_SHOW_ALL); 

    if (pressed_exit) {
        scroll(0, 240);
        return STATE_TITLE_MENU;
    }

    return STATE_TITLE_INTRO;
}




typedef GameState (*state_func)(void);

const state_func state_handlers[] = {
    title_intro_loop,
    title_menu_loop,
    stage_select_loop,
	stage_start_loop,
    gameplay_loop,
    game_over_loop,
    construction_loop,
	demoplay_loop
};
	

void main (void) {	

	
	ppu_off(); 
    sound_init(); 
	reset_screen();	
	set_ppu();
	draw_title_screen();
	ppu_mask(MASK_SHOW_ALL);
	
	current_state = STATE_TITLE_INTRO;
	while (1){
		current_state = state_handlers[current_state]();
	}
}
