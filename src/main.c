/*	Battle City implemented in C
 *	using neslib
 */	
#include "../lib/neslib.h"
#include "globals.h" 
#include "util.h"
#include "levels.h"

static unsigned char gameover_str_x;
static unsigned char gameover_str_y;
static unsigned char gameover_scroll_type;
static unsigned char gameover_str_timer;
static unsigned char staff_string_ram[16];
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
	oam_hide_rest();
	construction_flag = 0;
	
	load_spr_pal();
	set_bg_pal_number(3);
	memfill(SCREEN_BUFFER, 0, 1024);
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
	temp = skip_leading_zeros(hiscore_1p_string);
	draw_str(&hiscore_1p_string[temp], 3 + temp, 3, 0x30);
	
	draw_str(hi_string, 0xB, 3, 0);
	temp = skip_leading_zeros(hiscore_string);
	draw_str(&hiscore_string[temp], 0xD + temp, 3, 0x30);
	
	if (cursor_pos != 0){// If cursor at RESET wasn't at 1 player position, need to draw 2nd player data. TODO: isColdBoot better?
		draw_str(two_string, 0x15, 3, 0);	
		temp = skip_leading_zeros(hiscore_2p_string);
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
void command_explode(unsigned char id){
	
}

void command_get_random_status(unsigned char id){
	
}
void command_move(unsigned char id){
    //k - direction
    //temp - direction tile index
    //i - probe x, j - probe y
    //x - block x, y - block y
    unsigned char tmp1, tmp2;

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
    track_pos[id] ^= 4;  // blocked — animate tracks only
    // TODO: enemy direction randomisation goes here (id >= 2)	
}

// Called from handle_tank_status_command() via jump table entry 8 (command nibble $8).
// Triggered by handle_input() when no direction button is pressed:
//   tank_status[id] = (tank_status[id] & 0x0F) | 0x80 | 0x08
// The two bits 2-3 in tank_status act as a 2-bit coast counter (starts at $8 = 0b1000,
// decremented by 4 each call, stops at 0). This gives ~2 player-rate frames of coasting.
void command_drift(unsigned char id){
    // Ice sliding — players only (enemies skip directly to coast logic)
    if (id < 2 &&
        (player_ice_status[id] & 0x80) &&       // on ice (bit 7 set)
        (player_ice_status[id] & 0x7F) != 0) {  // timer not zero
        --player_ice_status[id];                // count down slide timer
        track_pos[id] ^= 4;                     // animate tracks while sliding
        command_move(id);                       // keep moving in current direction
        return;
    }
    // coast to stop Each call subtracts 4, stepping: $88 → $84 → $80.
    tank_status[id] -= 4;
    // When bits 2-3 reach zero, tank is fully stopped — reset to $80 (command 8,
    // dir=0) so next handle_input() call can set a new direction freely.
    if ((tank_status[id] & 0xC) == 0) {
        tank_status[id] |= 0x80;
    }	
}

void command_aim_hq(unsigned char id){
	
}
void command_aim_player2(unsigned char id){
	
}
void command_aim_player1(unsigned char id){
	
}
void command_set_respawn(unsigned char id){
    ++tank_status[id];
    if ((tank_status[id] & 0xF) == 0xE)
        tank_status[id] = TANK_CMD_RESPAWN_E; // 0xE0
}
void load_new_tank(unsigned char id){
    static const unsigned char respawn_status[] = {
        0xA0, 0xA0,              // players face up
        0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2  // enemies face down
    };
    tank_status[id] = respawn_status[id];
    if (id < 2){
        invincible_timer[id] = 3;    // spawn shield
        tank_type[id] = player_type[id];
    } else {
        // TODO:enemy type loading from EnemyType_ROMArray — skip for now
    }
    track_pos[id] = 0;
}
void command_load_tank(unsigned char id){
    ++tank_status[id];
    if ((tank_status[id] & 0xF) == 0xE)
        load_new_tank(id);   // transition to live status
}

void handle_tank_status_command(unsigned char id){
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
	
}
void render_small_explode1(unsigned char id){
	
}
void render_small_explode2(unsigned char id){
	
}
void render_big_explode(unsigned char id){
	
}

void add_sprite_to_buffer(unsigned char x, unsigned char y) {
    //temp - tile under tank
    //cnt - sprite pal
    cnt = pal;
    temp = SCREEN_BUFFER[(y >> 3) * 32
                       + ((unsigned char)(x + 3) >> 3)] & 0x7F;
    //hide if tank under forest
    if (temp == TILE_FOREST)
        cnt |= OAM_BEHIND;
    oam_spr(x, y - 8, current_spr_tile_index, cnt);
}

// Draw 16x16 sprite as two 8x16 sprites side-by-side
void draw_sprite_16x16(unsigned char x, unsigned char y) {
    add_sprite_to_buffer(x - 8, y);
    current_spr_tile_index += 2;    
    add_sprite_to_buffer(x, y);
}
void render_respawn(unsigned char id){
    cnt = tank_status[id] & 0x0F;  // 0..13
    temp = (cnt >= 7) ? (cnt - 7) : (7 - cnt);  // |frame - 7|
    current_spr_tile_index = ((temp << 1) & 0xFC) + 0xA1;
    pal = 3;
    draw_sprite_16x16(tank_x[id], tank_y[id]);
}



void render_normal_tank(unsigned char tank_id) {
	static const unsigned char tank_type_pal[8] = {
        2, 0, 0, 1, 2, 1, 2, 2
    };
    
    if (tank_id < 2) {// Player tank
        if (player_blink_timer[tank_id]) {
            // Blink effect: hide sprite every 8 frames
            if (FRAME_CNT & 8) {
                return;  // Skip drawing this frame
            }
        }
        pal = tank_id;  // Player 1 = palette 0, Player 2 = palette 1
    }
    else {// Enemy tank         
        if (IS_BONUS_TANK(tank_id)) {
            // Bonus tank: alternate between palettes 2 and 3
            pal = ((FRAME_CNT >> 3) & 1) + 2;
        }
        else {
            // Normal enemy: cycle through 8 palettes based on tank type
            i = ((FRAME_CNT << 2) + tank_type[tank_id]) & 7;
            pal = tank_type_pal[i];
        }
    }   
    current_spr_tile_index = GET_TANK_BASE_TILE(tank_id);
	current_spr_tile_index += GET_TANK_DIR(tank_id) << 3;
	current_spr_tile_index += track_pos[tank_id];     
    draw_sprite_16x16(tank_x[tank_id], tank_y[tank_id]);
}


void render_tank(unsigned char tank_id) {
    typedef void (*tank_handler_func)(unsigned char);
    
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
    
    i = GET_TANK_CMD(tank_id);
    tank_render_jump_table[i](tank_id);
}


// Draw one 8x16 bullet sprite. Direction is in bullet_status bits 0-1.
static void draw_bullet_sprite() {
    // i - bullet id
    // cnt - direction
    cnt = bullet_status[i] & 3;                    // direction
    current_spr_tile_index = 0xB1 + (cnt << 1);    // base bullet tile + direction offset
    pal = 2;                                       // sprite palette 2
    oam_spr(bullet_x[i] - 5, bullet_y[i] - 8, current_spr_tile_index, pal);
}
static void draw_ricochet_sprite() {
    // i  - bullet id
    // cnt - frame index
    cnt = (3 - (GET_BULLET_CMD(i) >> 4)) * 4;
    current_spr_tile_index = 0xF1 + cnt;
    pal = 3;
    draw_sprite_16x16(bullet_x[i], bullet_y[i]);
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
	tank_status[0] = TANK_CMD_NORMAL | TANK_DIR_RIGHT;
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
		oam_hide_rest();
        
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
    temp = stage_mode == 0 ? stage_number : 35;
    --temp;
    temp <<= 2;
    memcpy(enemy_count, &enemy_amounts[temp], sizeof(enemy_count));
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
    tank_status[id] = TANK_CMD_RESPAWN_F;
    //Draw empty space under tank. In case if level was created from Construction and respawn point is occupied by tsa block
    write_tsa_to_vram((tank_x[id] >> 3) - 1, (tank_y[id] >> 3) - 1, 0x0F);
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
        tank_status[cnt] = (tank_status[cnt] & 0x0F) | TANK_CMD_NORMAL | 8;
    }
}


static void handle_all_tanks_commands(){
    unsigned char id;
    for (id = 7; id != 0xFF; --id) {

        if (enemy_freeze_timer != 0 && (FRAME_CNT & 63) == 0) {
            --enemy_freeze_timer;
        }

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
static void mark_tiles_under_tanks(void) {
    //cnt - tank id
    //temp16 - screen buffer base
    for (cnt = 7; cnt != 0xFF; --cnt) {
        // skip exploding or spawning
        if (tank_status[cnt] < 0x80 || tank_status[cnt] >= 0xE0)
            continue;

        temp16 = (unsigned int)((tank_y[cnt] - 8) >> 3) * 32
                           + ((tank_x[cnt] - 8) >> 3);

        nt_addr_lo[cnt] = (unsigned char)temp16;
        nt_addr_hi[cnt] = (unsigned char)(temp16 >> 8) & 3;

        // ice detection for players only. Set or clear ice sliding flag
        if (cnt < 2) {
            if ((SCREEN_BUFFER[temp16 + 0x21] & 0x7F) == TILE_ICE)
                player_ice_status[cnt] |= 0x80;
            else
                player_ice_status[cnt] &= 0x7F;
        }

        // always mark primary tile (top-left of sprite = base + $21)
        SCREEN_BUFFER[temp16 + 0x21] |= 0x80;

        // tile-aligned on X: also mark left neighbour ($20)
        if ((tank_x[cnt] & 7) == 0) {
            nt_addr_hi[cnt] |= 0x80;
            SCREEN_BUFFER[temp16 + 0x20] |= 0x80;
        }

        // tile-aligned on Y: also mark top neighbour ($01)
        if ((tank_y[cnt] & 7) == 0) {
            nt_addr_hi[cnt] |= 0x40;
            SCREEN_BUFFER[temp16 + 0x01] |= 0x80;
        }
    }
}

//Clear occupied flag of nametable tile under each tank
static void clear_tiles_under_tanks(void) {
    //cnt - tank id
    //temp16 - screen buffer base
    for (cnt = 7; cnt != 0xFF; --cnt) {
        if (tank_status[cnt] < 0x80 || tank_status[cnt] >= 0xE0)
            continue;

        temp16 = ((unsigned int)(nt_addr_hi[cnt] & 3) << 8)
                             | nt_addr_lo[cnt];

        SCREEN_BUFFER[temp16 + 0x21] &= 0x7F;

        if (nt_addr_hi[cnt] & 0x80)
            SCREEN_BUFFER[temp16 + 0x20] &= 0x7F;

        if (nt_addr_hi[cnt] & 0x40)
            SCREEN_BUFFER[temp16 + 0x01] &= 0x7F;
    }
}

void draw_player_lives(void) {
    //cnt - player tank id
    static const unsigned char player_lives_icon[] = {0x14, 0xFF};
    static const unsigned char lives_tile_y[2] = {0x12, 0x15};

     // P1 life icon at (0x1D, 0x12)
    add_vram_fill_cmd((unsigned char*)NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x12), TILE_PLAYER_LIVE, 1);
    // in 2-player mode draw P2 icon too
    if (stage_mode == 2 || cursor_pos != 0) {
        add_vram_fill_cmd((unsigned char*)NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, 0x15), TILE_PLAYER_LIVE, 1);
    }
    // draw lives digit
    for (cnt = 0; cnt <= 1; ++cnt) {
 
        if (cnt == 1 && stage_mode != 2 && cursor_pos == 0)
            break;  // 1-player: skip P2
        set_number_string(player_lives[cnt] > 0 ? player_lives[cnt] - 1 : 0);
        k = lives_tile_y[cnt]; 
        if (player_lives[cnt] > 10) {
            add_vram_cpy_based_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1D, k),
                                   &number_string[5], 2, TILE_GREY_DIGIT_BASE);
        } else {
            add_vram_cpy_based_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x1E, k),
                                   &number_string[6], 1, TILE_GREY_DIGIT_BASE);
        }
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
void invinsible_timer_handle(void) {
    //i - player tank id
    for (i = 1; i != 0xFF; --i) {
        if (invincible_timer[i] == 0)
            continue;

        // decrement timer once per second
        if ((FRAME_CNT & 63) == 0)
            --invincible_timer[i];

        // draw aura: tile $29/$2B alternating every 2 frames
        current_spr_tile_index = ((FRAME_CNT & 2) << 1) + 0x29;
        pal = 2;
        draw_sprite_16x16(tank_x[i], tank_y[i]);
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
    if (temp == TANK_TYPE_0) return;          // normal tank
    if (temp == TANK_TYPE_6) {
        bullet_property[i] = 0x01;  // fast
    } else if (temp == TANK_TYPE_3) {
        bullet_property[i] = 0x03;  // armor-piercing
    } else if (temp & 0x80) {
        //TODO: implement bonus tank double-shot
        //bonus_pistol_shot(i);  
    }
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

static void draw_destroyed_hq() {
    static const unsigned char DestroyedHQ_TSA_Line1[] = { 0xCC, 0xCE};
    static const unsigned char DestroyedHQ_TSA_Line2[] = { 0xCD, 0xCF};
    add_vram_cpy_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x0E, 0x1A), DestroyedHQ_TSA_Line1, sizeof(DestroyedHQ_TSA_Line1));
    add_vram_cpy_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(0x0E, 0x1B), DestroyedHQ_TSA_Line2, sizeof(DestroyedHQ_TSA_Line2));
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

    // HQ tiles (C8‑CB) – explode and ricochet
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
        *src = 0;
        add_vram_fill_cmd((unsigned char *)(NAMETABLE_A + temp16), 0, 1);
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
    *src = temp;
    add_vram_fill_cmd((unsigned char *)(NAMETABLE_A + temp16), temp, 1);
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


//Main operations processed during gameplay frame (not in pause)
static void handle_gameplay(){

    mark_tiles_under_tanks();
    handle_input();
    handle_all_tanks_commands();
    clear_tiles_under_tanks();
    handle_all_bullets_statuses();
    invinsible_timer_handle();
    make_player_shot();
    handle_bullet_bg_collisions();

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
    load_normal_hq();
    draw_diverging_shutters();

    set_bg_pal_number(0);
    ppu_wait_nmi();//Copy palette and also process commands buffer (may not fit into single NMI)
    reset_stage_vars();	
	return STATE_GAMEPLAY;	
}


GameState gameplay_loop(void) {
    unsigned char id;
    while(1){
        ppu_wait_nmi();
        pad_poll(0);
        pad_poll(1);
        handle_gameplay();
        for(id = 0; id < 8; ++id){
            render_tank(id);    // draw spawn ring / tank
        }
        render_bullets();
        oam_hide_rest();
    }
	return STATE_GAME_OVER;	
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


	
	