/*	Battle City implemented in C
 *	using neslib
 */	
#include "globals.h" 
#include "../game.h"
#include "util.h"
#include "levels.h"

//todo: move gamover to specified function of gameover handle
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

// Bullets and tanks coordinate increments
// 0=up,1=left,2=down,3=right
static const unsigned char dx[4] = { 0, 0xFF,  0,  1 };
static const unsigned char dy[4] = { 0xFF,  0,  1,  0 };




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
    ppu_wait_nmi(); 
    oam_size(1);          // Set 8x16 sprites (bit 5)
    bank_bg(1);           // Background CHR at $1000 (bit 4)
}

void reset_screen(void){
	construction_flag = 0;
	
	load_spr_pal();
	set_bg_pal_number(3);
	memfill(SCREEN_BUFFER, 0, 1024);
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
	vram_adr(0x2000);
    vram_write(SCREEN_BUFFER, 1024);
	vram_adr(0x2800);
    vram_write(SCREEN_BUFFER, 1024);

}

void draw_brick_str(const unsigned char *str, unsigned char x, unsigned char y) {
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
	draw_str(&hiscore_1p_string[temp], 3 + temp, 3, 0x30);
	
	draw_str(hi_string, 0xB, 3, 0);
	temp = skip_leading_zeros(hiscore_string, 0);
	draw_str(&hiscore_string[temp], 0xD + temp, 3, 0x30);
	
	if (cursor_pos != 0){// If cursor at RESET wasn't at 1 player position, need to draw 2nd player data. TODO: isColdBoot better?
		draw_str(two_string, 0x15, 3, 0);	
		temp = skip_leading_zeros(hiscore_2p_string, 0);
		draw_str(&hiscore_2p_string[temp], 0x14 + temp, 3, 0x30);
	}
	draw_str(one_player_string, 0xB, 0x11, 0);
	draw_str(two_players_string, 0xB, 0x13, 0);
	draw_str(construction_string, 0xB, 0x15, 0);
	draw_str(namcot_string, 0xB, 0x17, 0);
	draw_str(year_string, 0x4, 0x19, 0);
	draw_str(rights_string, 0x6, 0x1B, 0);
	
	vram_adr(0x2800); 
    vram_write(SCREEN_BUFFER, 1024);
}

GameState title_intro_loop(void) {  
	static const unsigned char SCROLL_MAX = 0xF0;  
    i = 0;
    do {
        scroll(0, i);
        ppu_wait_nmi();
        pad_poll(0);
       
        if (PAD_STATE[0] & (PAD_SELECT | PAD_START)) {
            break;
        }
        ++i;
    } while (i < SCROLL_MAX );    
    scroll(0, SCROLL_MAX );
	return STATE_TITLE_MENU;
}

void show_secret_msg(void){//TODO:перевести на state
	
}


void skip_handle(unsigned char id){
	
}

void initialize_gameover (void){
    gameover_str_timer = 0x0D;
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
    if (hq_status != 0x80)
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
        tank_status[id] = (tank_status[id] & 0x0F) | TANK_CMD_TARGET_P2;  // P1 gone
    } else if ((id & 1) == 0) {
        tank_status[id] = (tank_status[id] & 0x0F) | TANK_CMD_TARGET_P1;  // even id
    } else if (tank_status[1] == 0) {
        tank_status[id] = (tank_status[id] & 0x0F) | TANK_CMD_TARGET_P1;  // P2 gone
    } else {
        tank_status[id] = (tank_status[id] & 0x0F) | TANK_CMD_TARGET_P2;  // odd id, P2 alive
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
    unsigned char tmp1, tmp2;

    // Pre-move RNG check: 1/16 chance for aligned enemies to retarget
    if (id >= 2) {
        if ((tank_x[id] & 7) == 0 && (tank_y[id] & 7) == 0) {
            if ((get_random() & 0x0F) == 0) {
                get_random_aim(id);
                return;
            }
        }
    }

    k = tank_status[id] & 0x03;
    tmp1   = (unsigned char)(dx[k] * 8);  // x_inc*8, wraps signed→byte
    tmp2   = (unsigned char)(dy[k] * 8);  // y_inc*8
    x = tank_x[id] + (unsigned char)dx[k]; // 1-pixel step
    y = tank_y[id] + (unsigned char)dy[k];

    // Corner 1
    i = x + tmp1 + tmp2;
    if (i >= x) i--;  // Compare_Block_X
    j = y + tmp1 + tmp2;
    if (j >= y) j--;  // Compare_Block_Y
    temp = SCREEN_BUFFER[(j >> 3) * 32 + (i >> 3)];

    if (temp < 0x80 && (temp == 0 || temp >= 0x20)) {
        // Corner 2
        i = x + tmp1 - tmp2;
        if (i >= x) i--;
        j = y + tmp2 - tmp1;
        if (j >= y) j--;
        temp = SCREEN_BUFFER[(j >> 3) * 32 + (i >> 3)];

        if (temp < 0x80 && (temp == 0 || temp >= 0x20)) {
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
        tank_status[id] = (tank_status[id] & 0x0F) | 0x88;
        track_pos[id] ^= 4; 
        return;
    }

    // 25% chance (get_random() & 3 == 0)
    if ((tank_x[id] & 7) != 0 || (tank_y[id] & 7) != 0) {
        // Not on an 8x8 tile boundary: Immediate 180-degree reverse
        tank_status[id] ^= 2;
        return;
    }
    // On a tile boundary: Escalate to Command 9 (Get Random Status/Aim)
    tank_status[id] = (tank_status[id] & 0x0F) | 0x90;
    
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
    tank_status[id] -= 4;
    if ((tank_status[id] & 0x0C) != 0) return;

    // Bits 3-2 reached zero — restore command A (move), keep direction in low nibble
    tank_status[id] = (tank_status[id] & 0x0F) | 0xA0;
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
    ai_x_aim = 0x78;
    ai_y_aim = 0xD8;
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
        invincible_timer[id] = 3;    // spawn shield
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
        cnt = (stage_mode != STAGE_MODE_NORMAL) ? 35 : stage_number;
        cnt = ((cnt - 1) << 2) + enemy_type_number;
        temp = enemy_types[cnt];

        // Strongest tank ($E0) gets armour level 3
        if (temp == 0xE0) {
            temp |= 3;
        }
        // OR with existing tank_type (may have bonus flag from initialize_tank)
        temp |= tank_type[id];
        // Strongest bonus tank will have low armor - balancing tweak
        if (temp == 0xE7) {
            temp = 0xE4;
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
    typedef void (*tank_handler_func)(unsigned char);
    static const tank_handler_func tank_command_jump_table[16] = {
        skip_handle,                // 0
        command_explode,            // 1
        command_explode,            // 2
        command_explode,            // 3
        command_explode,            // 4
        command_explode,            // 5
        command_explode,            // 6
        command_explode,            // 7
        command_drift,              // 8
        command_get_random_status,  // 9
        command_move,               // A
        command_aim_hq,             // B
        command_aim_player2,        // C
        command_aim_player1,        // D
        command_load_tank,          // E
        command_set_respawn         // F
    };
    tank_command_jump_table[GET_TANK_CMD(id)](id);
}

void render_kill_points(unsigned char id){
    SPR_PRIORITY = 0;
    if(tank_type[id] == 0){//player kill don't draw points, just ricochet
        add_sprite_16x16(tank_x[id], tank_y[id], 0xF1, 3);
    }
    else{
        current_spr_tile_index = ((tank_type[id] >> 3) & 0xFC) + 0xA9;
        add_sprite_16x16(tank_x[id], tank_y[id], current_spr_tile_index, 3);
    }
    SPR_PRIORITY = 0x20;	
}

void render_small_explode1(unsigned char id){
    SPR_PRIORITY = 0;
    add_sprite_16x16(tank_x[id], tank_y[id], 0xF9, 3);
    SPR_PRIORITY = 0x20;	
}
void render_small_explode2(unsigned char id){
    //i, j - temporary
    i = tank_status[id] >> 4;
    j = (7 - i) << 2;
    current_spr_tile_index = 0xF1 + j;
    SPR_PRIORITY = 0;
    add_sprite_16x16(tank_x[id], tank_y[id], current_spr_tile_index, 3);	
    SPR_PRIORITY = 0x20;
}

static void set_sprite_index(unsigned char quadrant, unsigned char tank_idx) {
    //i - base, j - phase
    i = (quadrant << 2) + 0xD1;
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
    current_spr_tile_index = ((temp << 1) & 0xFC) + 0xA1;
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
    typedef void (*tank_handler_func)(unsigned char);
    //cnt - cmd index
    //i - tank id
    //zp variables will be screwed by called functions, but it's allowed - variables be not used twice   
    static const tank_handler_func tank_render_jump_table[16] = {
        skip_handle,           		// 0
        render_kill_points,      	// 1
        render_small_explode1,      // 2
        render_big_explode,      	// 3
        render_big_explode,      	// 4
        render_small_explode2,      // 5
        render_small_explode2,      // 6
        render_small_explode2,      // 7
        render_normal_tank,         // 8
        render_normal_tank,         // 9
        render_normal_tank,      	// A
        render_normal_tank,      	// B
        render_normal_tank,      	// C
        render_normal_tank,      	// D
        render_respawn,      		// E
        render_respawn      		// F
    };
    i = tank_id;    
    cnt = GET_TANK_CMD(i);
    tank_render_jump_table[cnt](i);
}


// Draw one 8x16 bullet sprite. Direction is in bullet_status bits 0-1.
static void draw_bullet_sprite() {
    // i - bullet id
    // cnt - direction
    cnt = bullet_status[i] & 3;                    // direction
    current_spr_tile_index = 0xB1 + (cnt << 1);    // base bullet tile + direction offset
    pal = 2;                                       // sprite palette 2
    add_sprite(bullet_x[i] - 5, bullet_y[i], current_spr_tile_index, pal);
}
static void draw_ricochet_sprite() {
    // i  - bullet id
    // cnt - frame index
    cnt = (3 - (GET_BULLET_CMD(i) >> 4)) * 4;
    current_spr_tile_index = 0xF1 + cnt;
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
    add_sprite(0x64, 0x80, 0x17, pal); // P
    add_sprite(0x6C, 0x80, 0x19, pal); // A
    add_sprite(0x74, 0x80, 0x1B, pal); // U
    add_sprite(0x7C, 0x80, 0x1D, pal); // S
    add_sprite(0x84, 0x80, 0x1F, pal); // E
    
    SPR_PRIORITY = 0x20;
}


GameState title_menu_loop(void) {

	//From top screen border to first menu item vertically $88 pixels
	#define CURSOR_POS_TO_SPR_COORD ((cursor_pos << 4) + 0x8B)
    static unsigned char pad_count;
	
    i = 8;
    do {
        --i;
        tank_status[i] = 0;
    } while (i);
	player_ice_status[0] = 0;
	player_ice_status[1] = 0;	
	tank_x[0] = 0x48;//x on title wont change
	tank_y[0] = CURSOR_POS_TO_SPR_COORD;
	tank_status[0] = TANK_CMD_DRIFT | TANK_DIR_RIGHT;
	tank_type[0] = TANK_TYPE_0;
	track_pos[0] = 0;
	player_blink_timer[0] = 0;
	player_blink_timer[1] = 0;
	pad_count = 0;
    
    while (1) {
        ppu_wait_nmi();
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
        
        // Secret code: Player 1 holds SELECT, Player 2 presses RIGHT
        if ((PAD_STATET[0] & PAD_SELECT) && (PAD_STATET[1] & PAD_RIGHT)) {
            pad_count += 0x10;
        }
        
        // Secret code: Player 1 holds START, Player 2 presses LEFT
        if ((PAD_STATET[0] & PAD_START) && (PAD_STATET[1] & PAD_LEFT)) {
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
                show_secret_msg();//TODO: перевести на state
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
        ppu_wait_nmi();
        
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
    #define TILE_GREY_DIGIT_BASE 0x6E
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

    #undef TILE_DIGIT_BASE
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
    init_stage_select_vars();

    oam_clear();
    ppu_wait_nmi();
    vram_adr(NAMETABLE_A);
    scroll(0, 0);
    set_bg_pal_number(4);    

    draw_converging_shutters();

    while (1) {
        ppu_wait_nmi();
        draw_stage_display();
        
        if (skip_stage_select_flag) {
            return STATE_GAMEPLAY;
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
        ppu_wait_nmi();
        
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
    static const unsigned char reinforcement_line[] = {0x6A, 0x6A};
    for (i = 0; i <= 18; i += 2) {
        add_vram_cpy_cmd(
            NAMETABLE_A + COORD_TO_NT_OFFSET((i & 1) + 29, (i >> 1) + 3),
            reinforcement_line, sizeof(reinforcement_line));
    }
}

static void load_Ip_icons(void){
    static const unsigned char Ip_line[] = {0x58, 0x13};
    static const unsigned char IIp_line[] = {0x5A, 0x13};
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
    temp = stage_mode == STAGE_MODE_NORMAL ? stage_number : 35;
    --temp;
    temp <<= 2;
    memcpy(enemy_count, &enemy_amounts[temp], sizeof(enemy_count));
}


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
    gameover_str_y = 0xF0;

    if (player_lives[0] != 0){
        initialize_tank(0);
    }
    if (player_lives[1] != 0){
        initialize_tank(1);
    }

    enemy_reinforce_count = 20;
    enemy_counter = 20;

    enemy_type_number = 0;
    SECONDS_CNT = 0;
    construction_flag = 0;
    hq_armor_timer = 0;
    player_blink_timer[0] = 0;
    player_blink_timer[1] = 0;
    invincible_timer[0] = 0;
    invincible_timer[1] = 0;
    respawn_timer = 0;
    bonus_x = 0;
    enemy_freeze_timer = 0;
    enemy_respawn_place_index = 0;
    skip_stage_select_flag = 1;
    hq_status = 0x80;

    for(i = 0; i < sizeof(killcount_1p); ++i){
        killcount_1p[i] = 0;
        killcount_2p[i] = 0;
    }
    temp = stage_mode == 1 ? 35 : stage_number;
    cnt = cursor_pos > 0? 20 : 0;
    respawn_delay = 190 - (temp << 2) - cnt;
    ppu_wait_nmi(); 
    load_reinforcements_icons();
    load_Ip_icons();
    load_level_flag();
    set_enemy_count();
}

// input: raw joypad byte — returns 0=up 1=left 2=down 3=right 0xFF=none
unsigned char button_to_direction(unsigned char pad) {
    static const unsigned char btn_to_dir[16] = {
        0xFF, 3,    1,    3,    2,    3,    2,    3,
        0,    3,    1,    3,    2,    3,    2,    3
    };
    return btn_to_dir[pad & 0x0F];
}

static void handle_input(){
    //cnt - tank id
    //i - new direction
    //j - current direction

    if ((FRAME_CNT & 1) == 0 && (FRAME_CNT & 3) != 0)
        return;

    for (cnt = 0; cnt < 2; ++cnt) {

        // skip exploding or spawning tanks
        if (tank_status[cnt] < 0x80 || tank_status[cnt] >= 0xE0)
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
                player_ice_status[cnt] = 0x80 | 0x1C;  // 28 frames of sliding
                //snd_ice = 1;
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

    if (enemy_freeze_timer != 0 && (FRAME_CNT & 63) == 0) {
        --enemy_freeze_timer;
    }
    for (id = 7; id != 0xFF; --id) {
        if (id < 2) {
            if ((FRAME_CNT & 1) == 0 && (FRAME_CNT & 3) != 0)
                continue;//Process statuses on specific frames: 0, 4, 8, 12
        } else {
            if (enemy_freeze_timer != 0) {
                if (tank_status[id] >= 0x80 && tank_status[id] < 0xE0)
                    continue;//Dont process dead enemies
            }
            if ((tank_type[id] & 0xF0) != 0xA0) {//Enemy #2 status processed 2 times more often, so it moves faster
                if (((id ^ FRAME_CNT) & 1) == 0)
                    continue;
            }
        }
        handle_tank_status_command(id);
    }
}

//Set occupied flag to nametable tile under each tank for collision detection
//Note: this function looks strange, because it is optimized in C for shorter assembly output
static void mark_tiles_under_tanks(void) {
    //cnt - tank id
    //temp16 - screen buffer base
    //x, y - coordinates of probe start
    //i, j - temporary, like hi and low bytes of offset 

    for (cnt = 7; cnt != 0xFF; --cnt) {
        // skip exploding or spawning
        if (tank_status[cnt] < 0x80 || tank_status[cnt] >= 0xE0)
            continue;
        //Optimized calculation of pointer to screen buffer based on coordinates
        //temp16 = 0x400 +((tank_y[cnt] - 8) / 8) * 32+ ((tank_x[cnt] - 8) / 8);   
        x = ((unsigned char)(tank_x[cnt] - 8)) / 8;//coordinates in tiles
        y = ((unsigned char)(tank_y[cnt] - 8)) / 8;
        j = (y & 0x07) * 0x20;//y * screen width tiles will give upper 3 bits of low byte
        i = (y >> 3) | 4; //hight byte of result offset starting from 0x400 
        temp16 = ((i << 8) | (j | x));

        nt_addr_lo[cnt] = (unsigned char)temp16;
        nt_addr_hi[cnt] = (unsigned char)(temp16 >> 8) & 3;

        // ice detection for players only. Set or clear ice sliding flag
        if (cnt < 2) {
            i = (((MetaTileEntry*)temp16)->right_bottom) & 0x7F;
            j = player_ice_status[cnt];
            if (i == TILE_ICE){
                j |= 0x80;
                player_ice_status[cnt] = j;
            }                
            else{
                j &= 0x7F;
                player_ice_status[cnt] = j;
            } 
        }
        // always mark primary tile (bottom-right of sprite = base + $21)
        i = ((MetaTileEntry*)temp16)->right_bottom;
        i |= 0x80;        
        ((MetaTileEntry*)temp16)->right_bottom = i;

        // tile-aligned on X: also mark left neighbour ($20)
        if ((tank_x[cnt] & 7) == 0) {
            i = nt_addr_hi[cnt];
            i |= 0x80;
            nt_addr_hi[cnt] = i;
            i = ((MetaTileEntry*)temp16)->left_bottom;
            i |= 0x80;        
            ((MetaTileEntry*)temp16)->left_bottom = i;
        }
        // tile-aligned on Y: also mark top neighbour ($01)
        if ((tank_y[cnt] & 7) == 0) {
            i = nt_addr_hi[cnt];
            i |= 0x40;
            nt_addr_hi[cnt] = i;
            i = ((MetaTileEntry*)temp16)->right_top;
            i |= 0x80;        
            ((MetaTileEntry*)temp16)->right_top = i;           
        }
    }
}

//Clear occupied flag of nametable tile under each tank
static void clear_tiles_under_tanks(void) {
    //cnt - tank id
    //temp16 - screen buffer base
    //temp - temporary
    for (cnt = 7; cnt != 0xFF; --cnt) {
        if (tank_status[cnt] < 0x80 || tank_status[cnt] >= 0xE0)
            continue;
        
        temp = (nt_addr_hi[cnt] & 3) | 4;
        temp16 = (temp << 8) | nt_addr_lo[cnt];

        temp = ((MetaTileEntry*)temp16)->right_bottom;
        temp &= 0x7F;        
        ((MetaTileEntry*)temp16)->right_bottom = temp;

        if (nt_addr_hi[cnt] & 0x80){
            temp = ((MetaTileEntry*)temp16)->left_bottom;
            temp &= 0x7F;        
            ((MetaTileEntry*)temp16)->left_bottom = temp;
        }
        if (nt_addr_hi[cnt] & 0x40){
            temp = ((MetaTileEntry*)temp16)->right_top;
            temp &= 0x7F;        
            ((MetaTileEntry*)temp16)->right_top = temp; 
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
        if ((FRAME_CNT & 63) == 0)
            --invincible_timer[i];

        // draw aura: tile $29/$2B alternating every 2 frames
        current_spr_tile_index = ((FRAME_CNT & 2) << 1) + 0x29;
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
        // snd_shoot = 1;   // ToDo: uncomment when sound is implemented
    }

    // Direction is lower 2 bits of tank status, OR'd with 0x40 = flying
    temp = tank_status[i] & 3;
    bullet_status[i] = 0x40 | temp;

    // Position bullet 8 pixels in front of the tank
    bullet_x[i] = tank_x[i] + (dx[temp] * 8);
    bullet_y[i] = tank_y[i] + (dy[temp] * 8);

    // Reset properties
    bullet_property[i] = 0;

    // Set speed / armor based on tank type (upper 4 bits)
    temp = tank_type[i] & 0xF0;
    
    // Types 0, 4, 5, 7 ($00, $80, $A0, $E0) all fire normal bullets.
    // We merge the 0x00 check and the 0x80 bit check here.
    if (temp == 0x00 || (temp & 0x80)) {
        return; 
    }
    
    // Type 3 ($60) fires fast, armor-piercing bullets (property 3)
    if (temp == 0x60) {
        bullet_property[i] = 0x03;
        return;
    }
    
    // Types 1, 2, 6 ($20, $40, $C0) fire fast bullets (property 1)
    // This acts as the fall-through for any remaining types.
    bullet_property[i] = 0x01;
}

static void make_player_shot(void) {
    //i - player id
    for (i = 1; i != 0xFF; --i) {
        if (tank_status[i] < 0x80 || tank_status[i] >= 0xE0) 
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
        if (tank_status[i] < 0x80 || tank_status[i] >= 0xE0)
            continue;        
        // 1/32 chance to fire a bullet each frame
        if ((get_random() & 0x1F) != 0)
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
    add_sprite_16x16(0x78, 0xD8, tile, 3);   // HQ eagle screen centre
}

// Draw a 32×32 explosion sprite (four 16×16 quadrants) over the HQ.
static void draw_hq_big_explode(unsigned char spr_base) {
    add_sprite_16x16(0x70, 0xD0, 0xD1 + spr_base, 3); // top-left
    add_sprite_16x16(0x80, 0xD0, 0xD5 + spr_base, 3); // top-right
    add_sprite_16x16(0x70, 0xE0, 0xD9 + spr_base, 3); // bottom-left
    add_sprite_16x16(0x80, 0xE0, 0xDD + spr_base, 3); // bottom-right
}

// Armour timer (hq_armor_timer):
//   Updated every 16 frames; decremented once per second (every 64 frames).
//   While timer >= 4 the armoured tiles are assumed already in VRAM – no
//   per-frame redraw is needed.
//   When timer drops below 4 the HQ blinks at ~2 Hz (FRAME_CNT bit 4),
//   toggling between armoured and normal appearance every 16 frames.
//   When timer reaches 0 the normal HQ tiles are restored once.
static void handle_hq_armor(void) {
    if (hq_armor_timer == 0)       return;
    // Decrement once per second
    if ((FRAME_CNT & 63) == 0) {
        --hq_armor_timer;
        if (hq_armor_timer == 0) {
            draw_normal_hq();   // armour expired — restore normal tiles
            return;
        }
    }
    // Blink during the last 3 seconds (timer 1–3): toggle every 16 frames
    if (hq_armor_timer < 4) {
        if (FRAME_CNT & 0x10)
            draw_armoured_hq();
        else
            draw_normal_hq();
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
    if (hq_status == 0 || hq_status & 0x80) return;//hq exploded or intact

    pal = 3;
    --hq_status;
    temp = hq_status >> 2;
    temp = (temp >= 5) ? (temp - 5) : (5 - temp);  // first fold  around 5
    temp = (temp >= 5) ? (temp - 5) : (5 - temp);  // second fold around 5

    switch (temp) {
        case 0:                           return;  // animation finished
        case 1: draw_hq_small_explode(0xF1); break;
        case 2: draw_hq_small_explode(0xF5); break;
        case 3: draw_hq_small_explode(0xF9); break;
        case 4: draw_hq_big_explode(0x00);   break;
        case 5: draw_hq_big_explode(0x10);   break;
        default:                          return;
    }
}

//Checks if bullet collides with tiles and starts ricochet/explosions if do
//i - bullet id, x, y are inited with probe coordinates before calling this function
static unsigned char bullet_bg_impact() {
    //i - bullet id
    //x - sprite x
    //y - sprite y
    //cnt - mask
    //temp - tile
    //src - tile pointer 
    //temp16 - offset

    cnt = 1;
    if (y & 4) {
        cnt <<= 2;          // bit 2 set → bottom quadrant
    }
    if (x & 4) {
        cnt <<= 1;          // bit 2 set → right quadrant
    }
    // mask now holds 1,2,4,8 (top‑left, top‑right, bottom‑left, bottom‑right)

    // Get pointer to the tile in the screen buffer
    temp16 = COORD_TO_NT_OFFSET(x >> 3, y >> 3);
    src = &SCREEN_BUFFER[temp16];
    temp = *src;

    // Returns non‑zero if the quadrant is still solid.
    if (((cnt | 0xF0) & temp) == 0)
        return 0;                     // quadrant already empty → pass through

    // 4 HQ eagle tiles (C8‑CB) – explode and ricochet
    if ((temp & 0xFC) == 0xC8) {
        if (hq_status != 0) {
            hq_status = 0x27;       // initial explosion frame
            // snd_hq_explode = 1;
            // snd_player_explode = 1;
            draw_destroyed_hq();
            bullet_status[i] = 0x33;
        }
        return 0;
    }

    // Water, forest, ice (tile >= $12) – pass through
    if (temp >= 0x12)
        return 0;

    // Solid obstacle – always set ricochet animation
    bullet_status[i] = 0x33;

    // Screen border ($11) – no tile destruction
    if (temp == TILE_GREY) {
        if (i < 2) {
            // snd_armour_ricochet_wall = 1;
        }
        return 0;
    }

    // Armour‑piercing bullet (property bit 1) – destroys the whole tile, even for bricks
    if (bullet_property[i] & 0x02) {
        buffer_and_vram_fill(temp16, 0, 1);
        if (i < 2) {
            // snd_brick_ricochet = 1;
        }
        return 0;
    }
    // armour ($10) without armor-piercing, no destruction
    if (temp == TILE_ARMOR) {
        if (i < 2) {
            // snd_armour_ricochet_wall = 1;
        }
        return 0;
    }
    // Normal bullet hitting ordinary brick – create a hole in the correct quadrant
    temp &= ~cnt;
    buffer_and_vram_fill(temp16, temp, 1);
    if (i < 2) {
        // snd_brick_ricochet = 1;
    }
    return 1;
}
static void handle_bullet_bg_collisions(void) {
    //i - bullet id
    //k - direction
    //x - probe x
    //y - probe y
    unsigned char temp_x, temp_y;
    unsigned char temp_x_quad, temp_y_quad;
    signed char   inc;

    for (i = 9; i != 0xFF; --i) {

        if ((bullet_status[i] & 0xF0) != 0x40)
            continue;

        if (bullet_property[i] == 0) {     // slow bullet: skip every other frame
            if (((i ^ FRAME_CNT) & 1) == 0)
                continue;
        }

        k = bullet_status[i] & 0x03;

        // abs(Bullet_Coord_X_Increment_2[dir]), then ×4
        inc = dx[k];
        temp_x = (inc < 0) ? (unsigned char)-inc : (unsigned char)inc;
        temp_x_quad = temp_x << 2;

        // abs(Bullet_Coord_Y_Increment_2[dir]), then ×4
        inc = dy[k];
        temp_y = (inc < 0) ? (unsigned char)-inc : (unsigned char)inc;
        temp_y_quad = temp_y << 2;

        // Probe 1: leading edge
        x = bullet_x[i];
        y = bullet_y[i];
        if (bullet_bg_impact()) {
            // Probe 1b: same tile, opposite quadrant (ptr unchanged, new mask)
            x += temp_y_quad;
            y += temp_x_quad;
            bullet_bg_impact();
        }
        // Probe 2: trailing edge (offset by 1 pixel perpendicular)
        x = bullet_x[i] - temp_y;
        y = bullet_y[i] - temp_x;
        if (bullet_bg_impact()) {
            // Probe 2b: same tile, opposite quadrant (ptr unchanged, new mask)
            x -=  temp_y_quad;
            y -=  temp_x_quad;
            bullet_bg_impact();
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
    if (hq_status != 0x80) return;  // HQ destroyed — no bonus lives

    // Player 1
    if (player_add_life_flag[0] == 0 && hiscore_1p_string[2] >= 2) {
        player_lives[0]++;
        player_add_life_flag[0] = 1;
        //snd_ancillary_life1 = 1; //TODO: implement with sound engine
        //snd_ancillary_life2 = 1;
        return;
    }

    // Player 2 (only in 2-player mode)
    if (cursor_pos == 0) return;
    if (player_add_life_flag[1] != 0) return;
    if (hiscore_2p_string[2] < 2) return;
    player_lives[1]++;
    player_add_life_flag[1] = 1;
    //snd_ancillary_life1 = 1;//TODO: implement with sound engine
    //snd_ancillary_life2 = 1;
}



#define ABS_DIFF(a, b) ( \
    temp = (a) - (b), \
    (temp & 0x80) ? (unsigned char)(0 - temp) : temp \
)

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
        current_spr_tile_index = 0x3B; // Tile index for '500'
        pal = 2;
        SPR_PRIORITY = 0; // In front of background
        add_sprite_16x16(bonus_x, bonus_y, current_spr_tile_index, pal);
        SPR_PRIORITY = 0x20;
    } else {
        // Bonus not yet taken, draw the blinking bonus item
        if ((FRAME_CNT & 8) == 0) return; // Blink effect: hide sprite every 8 frames
        
        // 4 tiles per bonus icon, base offset 0x81
        current_spr_tile_index = (bonus_number << 2) + 0x81; 
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
    enemy_freeze_timer = 10;
}

static void bonus_shovel() {
    if (hq_status & 0x80) {  // HQ still standing (bit 7 set = intact)
        draw_armoured_hq();
        hq_armor_timer = 20;
    }
}

static void bonus_star() {
    if (player_type[i] == 0x60) return;  // already max type
    player_type[i] += 0x20;
    tank_type[i] = player_type[i];
}

static void bonus_grenade() {
    //cnt - iterator
    //snd_enemy_explode = 1;//TODO: implement with sound engine
    for (cnt = 7; cnt >= 2; --cnt) {
        if (tank_status[cnt] >= 0x80 && tank_status[cnt] < 0xE0) {
            tank_status[cnt] = 0x73;
            tank_type[cnt] = 0;
        }
    }
}

static void bonus_life() {
    player_lives[i]++;
    //snd_ancillary_life1 = 1;//TODO: implement with sound engine
    //snd_ancillary_life2 = 1;
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
        if (tank_status[i] < 0x80) continue;         // not active
        if (tank_status[i] >= 0xE0) continue;        // exploding/respawning
        
        if (ABS_DIFF(tank_x[i], bonus_x) >= 0x0C) continue;
        if (ABS_DIFF(tank_y[i], bonus_y) >= 0x0C) continue;

        // Collision confirmed
        bonus_timer = 0x32;  // show score for 50 frames

        if (bonus_number & 0x80) return;  // bit 7 = newly spawned, not yet active

        if (stage_mode != STAGE_MODE_DEMO) {
            num_to_num_string(0x50);           // 500 points
            add_score(i);
            add_life();
            //snd_bonus_taken = 1; TODO: implement with sound engine
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

    //snd_bonus_appears = 1;//TODO: implement with sound engine
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
    // temp - temporary for calculating abs_diff
    // k - temporary storage for i
    static const unsigned char EnemyKillScore[] = { 0x10, 0x20, 0x30, 0x40 };

    // ---------- Phase 1: Enemy bullets vs Players ----------
    // t = 1 (player 2), then 0 (player 1)
    for (i = 1; i != 0xFF; --i) {
        if (tank_status[i] < 0x80 || tank_status[i] >= 0xE0)
            continue;                     // tank not active

        // Enemy bullets are IDs 2..7
        for (j = 7; j >= 2; --j) {
            if ((bullet_status[j] & 0xF0) != 0x40)
                continue;                 // not flying

            // Collision if both axes differ by less than 10 pixels
            if (ABS_DIFF(bullet_x[j], tank_x[i]) >= 10) continue;
            if (ABS_DIFF(bullet_y[j], tank_y[i]) >= 10) continue;

            // Hit → start ricochet animation
            bullet_status[j] = 0x33;

            // Spawn shield? Bullet disappears, no damage
            if (invincible_timer[i] != 0) {
                bullet_status[j] = 0;
                continue;
            }

            // Player destroyed
            tank_status[i] = 0x73;        // explosion command
            // snd_player_explode = 1;
            player_type[i] = 0;           // reset to basic tank
            tank_type[i] = 0;
            goto next_player;             // skip remaining bullets for this tank
        }
        next_player:;
    }

    // ---------- Phase 2: Player bullets vs Enemies ----------
    // Enemies are IDs 7 down to 2
    for (i = 7; i >= 2; --i) {
        if (tank_status[i] < 0x80 || tank_status[i] >= 0xE0)
            continue;

        // Player bullets: IDs 0,1,8,9. Original checks (b & 6) == 0.
        for (j = 9; j != 0xFF; --j) {
            if ((j & 6) != 0) continue;
            if ((bullet_status[j] & 0xF0) != 0x40) continue;

            if (ABS_DIFF(bullet_x[j], tank_x[i]) >= 10) continue;
            if (ABS_DIFF(bullet_y[j], tank_y[i]) >= 10) continue;

            // Collision
            bullet_status[j] = 0x33;

            // Bonus tank → spawn item (to be implemented)
            if (tank_type[i] & 4) {
                //i will be modified by underlying functions of bonus_appear_handle
                k = i;
                bonus_appear_handle();
                i = k;
            }

            // Tank has armour? Reduce it, tank survives
            if ((tank_type[i] & 3) != 0) {
                --tank_type[i];
                // snd_armour_ricochet_tank = 1;
                continue;
            }

            // No armour → destroy enemy
            tank_status[i] = 0x73;
            // snd_enemy_explode = 1;//TODO:implement with sound engine

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
        if (tank_status[i] < 0x80 || tank_status[i] >= 0xE0)
            continue;

        // Bullets from the other player only: original uses (player_id ^ bullet_id) & 1 != 0
        for (j = 9; j != 0xFF; --j) {
            if ((j & 6) != 0) continue;          // only player bullet IDs
            if (((i ^ j) & 1) == 0) continue;    // skip own bullet
            if ((bullet_status[j] & 0xF0) != 0x40) continue;

            if (ABS_DIFF(bullet_x[j], tank_x[i]) >= 10) continue;
            if (ABS_DIFF(bullet_y[j], tank_y[i]) >= 10) continue;

            // Collision → ricochet
            bullet_status[j] = 0x33;

            // Invincible shield? Bullet disappears completely
            if (invincible_timer[i] != 0) {
                bullet_status[j] = 0;
                continue;
            }

            // Friendly fire grace period active? No extra effect
            if (player_blink_timer[i] != 0 || stage_mode == STAGE_MODE_DEMO)
                continue;
            // Set new grace period
            player_blink_timer[i] = 0xC8;
        }
    }
}

// Process bullet-vs-bullet collisions (called once per frame)
// Two enemy bullets never check against each other.
static void handle_bullet_bullet_collisions(void) {
    for (i = 9; i != 0xFF; --i) {
        if ((i & 6) != 0) continue;                        // only 0,1,8,9 act as bullet A
        if ((bullet_status[i] & 0xF0) != 0x40) continue;   // A not flying

        for (j = 9; j != 0xFF; --j) {
            if ((j & 7) == (i & 7)) continue;               // self (aliased 0/8, 1/9)
            if ((bullet_status[j] & 0xF0) != 0x40) continue; // B not flying

            if (ABS_DIFF(bullet_x[j], bullet_x[i]) >= 6) continue;
            if (ABS_DIFF(bullet_y[j], bullet_y[i]) >= 6) continue;

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
            gameover_str_y = 0xF0;//if finished, hide gamover string
        }
    }
    if (gameover_str_timer >= 10){//scroll until its 10 cycles left
        gameover_str_x += dx[gameover_scroll_dir];
        gameover_str_y += dy[gameover_scroll_dir];
    }
    SPR_PRIORITY = 0;
    add_sprite_16x16(gameover_str_x, gameover_str_y, 0x79, 3);// left half of "GAME OVER"
    add_sprite_16x16(gameover_str_x + 16, gameover_str_y, 0x7D, 3);// right half
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
        0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C   /* aLine */
    };

    ppu_wait_nmi();
    scroll(0, 0);
    set_bg_pal_number(3);

    ppu_off();

    /* Null_NT_Buffer */
    memfill(SCREEN_BUFFER, 0, 1024);

    /* Fill_Attrib_Table */
    memfill(SCREEN_BUFFER + 0x3C0, 0x50, 4);
    memfill(SCREEN_BUFFER + 0x3C4, 0xA0, 4);
    memfill(SCREEN_BUFFER + 0x3C8, 0x50, 3);
    memfill(SCREEN_BUFFER + 0x3CD, 0x50, 3);
    memfill(SCREEN_BUFFER + 0x3D0, 0x0A, 3);
    memfill(SCREEN_BUFFER + 0x3D5, 0x0A, 3);

    SCREEN_BUFFER[0x3F0] = 0x05;
    SCREEN_BUFFER[0x3F1] = 0x05;
    SCREEN_BUFFER[0x3F2] = 0x05;
    SCREEN_BUFFER[0x3F5] = 0x05;
    SCREEN_BUFFER[0x3F6] = 0x05;
    SCREEN_BUFFER[0x3F7] = 0x05;

    vram_adr(NAMETABLE_B);
    vram_write(SCREEN_BUFFER, 1024);

    /* Set_PPU */
    set_ppu();
    scroll(0, 0);

    /*
        Draw:
        HIkSCORE
        high score digits
    */
    add_vram_cpy_cmd(NAMETABLE_B + COORD_TO_NT_OFFSET(8, 3), hi_score_label, sizeof(hi_score_label) - 1);

    k = skip_leading_zeros(hiscore_string, 1);
    cnt = 0;
    while (hiscore_string[k + cnt] != 0xFF) {
        ++cnt;
    }

    add_vram_cpy_based_cmd(NAMETABLE_B + COORD_TO_NT_OFFSET(0x11 + k, 3), &hiscore_string[k], cnt, 0x30);

    /*
        Draw:
        STAGE
        stage number
    */
    add_vram_cpy_cmd(NAMETABLE_B + COORD_TO_NT_OFFSET(0x0C, 5), stage_label, sizeof(stage_label) - 1);

    set_number_string(stage_number);

    k = skip_leading_zeros(number_string, 1);

    cnt = 0;
    while (number_string[k + cnt] != 0xFF) {
        ++cnt;
    }

    add_vram_cpy_based_cmd(NAMETABLE_B + COORD_TO_NT_OFFSET(0x0D + k, 5), &number_string[k], cnt, 0x30);

    ppu_wait_nmi();
    ppu_mask(MASK_SHOW_ALL);

    /*
        Draw:
        I-PLAYER
        player 1 score
    */
    add_vram_cpy_cmd(NAMETABLE_B + COORD_TO_NT_OFFSET(3, 7), p1_label, sizeof(p1_label) - 1);

    k = skip_leading_zeros(hiscore_1p_string, 1);
    cnt = 0;
    while (hiscore_1p_string[k + cnt] != 0xFF) {
        ++cnt;
    }

    add_vram_cpy_based_cmd(
        NAMETABLE_B + COORD_TO_NT_OFFSET(4 + k, 9),
        &hiscore_1p_string[k],
        cnt,
        0x30
    );

    /* Arrow_Left x4 */
    add_vram_fill_cmd(
        (unsigned char *)(NAMETABLE_B + COORD_TO_NT_OFFSET(0x0E, 0x0C)),
        0x5B,
        1
    );
    add_vram_fill_cmd(
        (unsigned char *)(NAMETABLE_B + COORD_TO_NT_OFFSET(0x0E, 0x0F)),
        0x5B,
        1
    );
    add_vram_fill_cmd(
        (unsigned char *)(NAMETABLE_B + COORD_TO_NT_OFFSET(0x0E, 0x12)),
        0x5B,
        1
    );
    add_vram_fill_cmd(
        (unsigned char *)(NAMETABLE_B + COORD_TO_NT_OFFSET(0x0E, 0x15)),
        0x5B,
        1
    );
    //Draw second player, if it was 2p game
    if (cursor_pos != 0) {
        ppu_wait_nmi();

        /*
            Draw:
            II-PLAYER
            player 2 score
        */
        add_vram_cpy_cmd(
            NAMETABLE_B + COORD_TO_NT_OFFSET(0x15, 7),
            p2_label,
            sizeof(p2_label) - 1
        );

        k = skip_leading_zeros(hiscore_2p_string, 1);
        cnt = 0;
        while (hiscore_2p_string[k + cnt] != 0xFF) {
            ++cnt;
        }

        add_vram_cpy_based_cmd(
            NAMETABLE_B + COORD_TO_NT_OFFSET(0x16 + k, 9),
            &hiscore_2p_string[k],
            cnt,
            0x30
        );

        /* Arrow_Right x4 */
        add_vram_fill_cmd(
            (unsigned char *)(NAMETABLE_B + COORD_TO_NT_OFFSET(0x11, 0x0C)),
            0x5D,
            1
        );
        add_vram_fill_cmd(
            (unsigned char *)(NAMETABLE_B + COORD_TO_NT_OFFSET(0x11, 0x0F)),
            0x5D,
            1
        );
        add_vram_fill_cmd(
            (unsigned char *)(NAMETABLE_B + COORD_TO_NT_OFFSET(0x11, 0x12)),
            0x5D,
            1
        );
        add_vram_fill_cmd(
            (unsigned char *)(NAMETABLE_B + COORD_TO_NT_OFFSET(0x11, 0x15)),
            0x5D,
            1
        );
    }

    ppu_wait_nmi();

    /* PTS left column */
    add_vram_cpy_cmd(
        NAMETABLE_B + COORD_TO_NT_OFFSET(8, 0x0C),
        pts_label,
        sizeof(pts_label) - 1
    );
    add_vram_cpy_cmd(
        NAMETABLE_B + COORD_TO_NT_OFFSET(8, 0x0F),
        pts_label,
        sizeof(pts_label) - 1
    );
    add_vram_cpy_cmd(
        NAMETABLE_B + COORD_TO_NT_OFFSET(8, 0x12),
        pts_label,
        sizeof(pts_label) - 1
    );
    add_vram_cpy_cmd(
        NAMETABLE_B + COORD_TO_NT_OFFSET(8, 0x15),
        pts_label,
        sizeof(pts_label) - 1
    );

    /* PTS right column only in 2-player mode */
    if (cursor_pos != 0) {
        ppu_wait_nmi();

        add_vram_cpy_cmd(
            NAMETABLE_B + COORD_TO_NT_OFFSET(0x1A, 0x0C),
            pts_label,
            sizeof(pts_label) - 1
        );
        add_vram_cpy_cmd(
            NAMETABLE_B + COORD_TO_NT_OFFSET(0x1A, 0x0F),
            pts_label,
            sizeof(pts_label) - 1
        );
        add_vram_cpy_cmd(
            NAMETABLE_B + COORD_TO_NT_OFFSET(0x1A, 0x12),
            pts_label,
            sizeof(pts_label) - 1
        );
        add_vram_cpy_cmd(
            NAMETABLE_B + COORD_TO_NT_OFFSET(0x1A, 0x15),
            pts_label,
            sizeof(pts_label) - 1
        );
    }

    ppu_wait_nmi();

    /* Line above TOTAL */
    add_vram_cpy_cmd(
        NAMETABLE_B + COORD_TO_NT_OFFSET(0x0C, 0x16),
        line_label,
        sizeof(line_label)
    );

    /* TOTAL */
    add_vram_cpy_cmd(
        NAMETABLE_B + COORD_TO_NT_OFFSET(6, 0x17),
        total_label,
        sizeof(total_label) - 1
    );
}

static void draw_tank_column(void)
{
    add_sprite_16x16(0x81, 0x64, 0x80, 2);
    add_sprite_16x16(0x81, 0x7C, 0xA0, 2);
    add_sprite_16x16(0x81, 0x94, 0xC0, 2);
    add_sprite_16x16(0x81, 0xAC, 0xE0, 2);
}
static void draw_tank_column_xtimes(unsigned char count)
{
    while (count != 0) {
        ppu_wait_nmi();
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
            NAMETABLE_B + COORD_TO_NT_OFFSET(base_x + k, y),
            &str[k],
            cnt,
            0x30
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
        0x10, 0x20, 0x30, 0x40
    };

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
        ppu_wait_nmi();
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
            ppu_wait_nmi();
            draw_tank_column();
            temp = 0;
            num_to_num_string(tank_kill_pts[y]);

            if (killcount_1p[y] != 0) {
                --killcount_1p[y];
                ++p1_type_count;

                add_number_to_temp_string(temp_1p_pts_string);
                temp = 1;

                /* TODO: original sets Snd_PtsCount1/Snd_PtsCount2 here. */
                add_life();
            }
            if (killcount_2p[y] != 0) {
                --killcount_2p[y];
                ++p2_type_count;
                add_number_to_temp_string(temp_2p_pts_string);
                temp = 1;

                /* TODO: original sets Snd_PtsCount1/Snd_PtsCount2 here. */
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
                NAMETABLE_B + COORD_TO_NT_OFFSET(3, 0x19),
                bonus_label,
                sizeof(bonus_label));
            add_vram_cpy_cmd(
                NAMETABLE_B + COORD_TO_NT_OFFSET(8, 0x1A),
                pts_label,
                sizeof(pts_label) - 1);

            /* TODO: original sets Snd_BonusPts here. */
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
                NAMETABLE_B + COORD_TO_NT_OFFSET(0x16, 0x19),
                bonus_label,
                sizeof(bonus_label));

            add_vram_cpy_cmd(
                NAMETABLE_B + COORD_TO_NT_OFFSET(0x1B, 0x1A),
                pts_label,
                sizeof(pts_label) - 1);

            /* TODO: original sets Snd_BonusPts here. */
            add_life();
        }
    }
    draw_tank_column_xtimes(0x1E);
    set_bg_pal_number(0);
}
static void init_gameover_str_center(void)
{
    gameover_str_x = 0x70;
    gameover_str_y = 0xF0;
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
    //play_snd_move();//TODO:implement on sound engine
    draw_player_lives();
    swap_water_pal_colors();    
}


GameState stage_start_loop(void) {

	
	memfill(SCREEN_BUFFER, TILE_GREY, 1024);
	memfill(SCREEN_BUFFER + 0x3C0, 0, 64);//all tiles will be on pal 0
	
    for (i = 0; i < PLAYFIELD_SIZE; ++i) {
        dst = SCREEN_BUFFER + COORD_TO_NT_OFFSET(FRAME_OFFSET, FRAME_OFFSET + i);
        memfill(dst, TILE_BLACK, PLAYFIELD_SIZE);//TODO: check, this is not necessary - will be overridden with level data, but maybe not in case of construction
    }
    load_stage();
    draw_normal_hq();
    reset_vram_cmd_queue();//hq was written to commands buffer, dont dump it to vram: it should not be shown on top of grey screen
    draw_diverging_shutters();

    set_bg_pal_number(0);
    ppu_wait_nmi();//Copy palette and also process commands buffer (may not fit into single NMI)
    reset_stage_vars();	
	return STATE_GAMEPLAY;	
}

static void stop_move_engine_sounds(void)
{
    //TODO: implement with sound engine
    /* snd_move = 0; snd_engine = 0; */
}

static void sound_stop(void)
{
    /* TODO: implement with sound engine */
}

static void freeze_player_on_hq_destroy(void)
{
    /* Original freezes player input when HQ is not intact. */
    if (hq_status != 0x80) {
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
        ppu_wait_nmi();

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
        ppu_wait_nmi();
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
            // TODO: pause/unpause sound engine here
        }
        render_pause();
        if (level_end_check())
            break;
    }
	    /* Stop timers/movement sounds like original. */
    SECONDS_CNT = 0;
    FRAME_CNT = 0;
    stop_move_engine_sounds();
    /* Run the original after-death battle loop. */
    if (gameover_str_timer != 0)
        SECONDS_CNT = 0xFE;
    after_death_battle_run();
    sound_stop();
    draw_pts_screen();

    /* Advance stage like original. */
    ++stage_number;

    if (stage_number == 71) {
        stage_number = 1;
        stage_mode = STAGE_MODE_NORMAL;
    }

    if (stage_number == 36) {
        stage_mode = STAGE_MODE_HARD;
    }

    /* Original game-over condition after score screen. */
    if ((unsigned char)(player_lives[0] + player_lives[1]) == 0 ||
        hq_status != 0x80) {
        return STATE_GAME_OVER;
    }

    /* Next stage.
       skip_stage_select_flag is already set, so stage_select_loop
       should immediately start the next level. */
    return STATE_STAGE_SELECT;
}
GameState game_over_loop(void) {
	
	return STATE_TITLE_INTRO;	
}
GameState construction_loop(void) {
	
	return STATE_TITLE_MENU;	
}

GameState demoplay_loop(void) {
	
	return STATE_TITLE_MENU;	
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
	reset_screen();	
	set_ppu();
	draw_title_screen();
	ppu_mask(MASK_SHOW_ALL);
	
	current_state = STATE_TITLE_INTRO;
	while (1){
		current_state = state_handlers[current_state]();
	}
}


	
	