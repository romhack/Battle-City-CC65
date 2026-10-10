#ifndef GLOBALS_H
#define GLOBALS_H

#include "../lib/neslib.h"

/*============================================================
 *  SINGLE DEFINITION TRICK
 *  globals.c defines GLOBALS_DEFINE before including this file
 *  Everyone else just includes this file and gets extern
 *============================================================*/
#ifdef GLOBALS_DEFINE
    #define EXTERN
#else
    #define EXTERN extern
#endif

/*============================================================
 *  SCREEN / TILE DEFINES
 *============================================================*/
#define SCREEN_BUFFER       ((unsigned char*)0x0400)
#define SCREEN_WIDTH        0x20
#define SCREEN_COLS         32
#define PLAYFIELD_SIZE  	26
#define FRAME_OFFSET    	2//Offset of stage frame from screen start
// Calculate offset within screen buffer. Coordinates in tiles (pixels/8)
#define COORD_TO_NT_OFFSET(x, y) (((y) << 5) + (x))
//neslib hides left column for scrolling, battle city - not, define custom mask
#define MASK_SHOW_ALL (MASK_BG | MASK_SPR | MASK_EDGE_BG | MASK_EDGE_SPR)
#define TILE_BLACK            0x00
#define TILE_ARMOR            0x10
#define TILE_GREY             0x11
#define TILE_PLAYER_LIVE      0x14
#define TILE_ICE              0x21
#define TILE_FOREST           0x22
#define TILE_BRICK            0x01   /* generic brick (tiles 01–0F) */
#define TILE_BRICK_THRESHOLD  0x20   /* tiles >= 0x20 are passable */
#define TILE_WATER_THRESHOLD  0x12   /* tiles >= 0x12 are water/forest/ice (bullets pass) */
#define TILE_HQ_BASE          0xC8   /* first of 4 HQ eagle tiles C8–CB */
#define TILE_HQ_MASK          0xFC   /* (tile & 0xFC) == 0xC8 → HQ */
#define TILE_EMPTY_BLOCK      0x00   /* empty nametable tile */
#define TILE_PTS_ARROW_LEFT   0x5B
#define TILE_PTS_ARROW_RIGHT  0x5D
#define TILE_PTS_LINE         0x5C
#define TILE_REINFORCE_ICON   0x6A
#define TILE_IP_ICON          0x58
#define TILE_IIP_ICON         0x5A
#define TILE_IP_SUFFIX        0x13   /* second tile of I/IP icon */
#define TILE_FLAG_UPPER       0x6C
#define TILE_FLAG_UPPER_2     0xFC
#define TILE_FLAG_LOWER       0x6D
#define TILE_FLAG_LOWER_2     0xFD
#define TILE_GREY_DIGIT_BASE  0x6E   /* digit glyphs start here */
#define TILE_DIGIT_BASE       0x30   /* score/number digit base */
#define TILE_DROP             0x9D
#define TILE_DOT              0x69
#define TILE_500_PTS          0x3B
/* --- Sprite tile bases --- */
#define TILE_BULLET_BASE      0xB1   /* 8×16 bullet sprites (4 dirs × 2 tiles) */
#define TILE_RICOCHET_BASE    0xF1   /* ricochet / small-explosion frames */
#define TILE_RESPAWN_BASE     0xA1   /* respawn ring animation base */
#define TILE_KILL_PTS_BASE    0xA9   /* kill-points overlay base */
#define TILE_BIG_EXPLODE_BASE 0xD1   /* 32×32 explosion quadrant base */
#define TILE_BONUS_BASE       0x81   /* bonus item sprite base (4 tiles each) */
#define TILE_INVINC_BASE      0x29   /* spawn-shield aura base */
#define TILE_GAMEOVER_L       0x79   /* "GAME" half */
#define TILE_GAMEOVER_R       0x7D   /* "OVER" half */
#define TILE_PAUSE_P          0x17
#define TILE_PAUSE_A          0x19
#define TILE_PAUSE_U          0x1B
#define TILE_PAUSE_S          0x1D
#define TILE_PAUSE_E          0x1F

/* --- HQ position (pixel centre of eagle) --- */
#define HQ_X        0x78
#define HQ_Y        0xD8
#define HQ_STATUS_ALIVE 0x80

/* --- Screen hide position (push sprite off bottom) --- */
#define SPRITE_OFFSCREEN_Y  0xF0

/* --- Construction mode borders --- */
#define CONSTRUCT_X_MIN  0x18
#define CONSTRUCT_X_MAX  0xD8
#define CONSTRUCT_Y_MIN  0x18
#define CONSTRUCT_Y_MAX  0xD8

/* --- Title menu --- */
#define TITLE_TANK_X       0x48
#define TITLE_MENU_Y_BASE  0x8B   /* top of first menu item */

/* --- Timers (in frames or seconds) --- */
#define FRAMES_IN_SECOND       63
#define BLINK_TIMER_INIT        0xC8  /* friendly-fire grace period */
#define ICE_SLIDE_INIT          0x9C  /* full ice-slide value (0x80|0x1C) */
#define ICE_SLIDE_DURATION      0x1C  /* lower-5-bit slide frames */
#define BONUS_SCORE_FRAMES      0x32  /* "500" display duration */
#define GAMEOVER_TIMER_INIT     0x0D  /* per-player gameover string */
#define GAMEOVER_TIMER_CENTER   0x11  /* level-end centred gameover */
#define HQ_ARMOR_TIME           20    /* shovel bonus armour seconds */
#define INVINCIBLE_TIME         3     /* spawn-shield seconds */
#define ENEMY_FREEZE_TIME       10    /* watch-bonus freeze seconds */
#define ENEMY_FIRE_MASK         0x1F  /* 1/32 chance per frame */
#define RANDOM_AIM_MASK         0x0F  /* 1/16 retarget chance */
#define RANDOM_DIR_MASK         0x03  /* 2-bit random */
#define COAST_COOLDOWN          4     /* drift sub-step (status -= 4) */
#define ICE_DEMO_LIMIT_Y        0xC8  /* demo: stop driving down */

#define SCORE_TYPE0   0x10   /* 100 pts */
#define SCORE_TYPE1   0x20   /* 200 pts */
#define SCORE_TYPE2   0x30   /* 300 pts */
#define SCORE_TYPE3   0x40   /* 400 pts */
#define SCORE_BONUS   0x50   /* 500 pts (bonus pickup) */

#define PTS_TANK_TYPE0  0x80
#define PTS_TANK_TYPE1  0xA0
#define PTS_TANK_TYPE2  0xC0
#define PTS_TANK_TYPE3  0xE0
#define PTS_TANK_X      0x81

#define STAGE_MIN           1
#define STAGE_MAX           35

#define NAMETABLE_A       0x2000
#define NAMETABLE_C       0x2800
#define ATTR_OFFSET       0x3C0   /* attribute table within SCREEN_BUFFER */
#define ATTR_SIZE         64

#define TANK_STATUS_EXPLODE_INIT  0x73  /* written to tank_status to start explosion */
#define TANK_STATUS_ALIVE_MIN     0x80  /* lowest "active" status */
#define TANK_STATUS_ALIVE_MAX     0xE0  /* >= this = spawning, not active */
#define TANK_TYPE_MASK            0xF0
#define TANK_TYPE_HEAVY           0xE0
#define TANK_TYPE_HEAVY_ARMOR     0x03
#define TANK_TYPE_HEAVY_BONUS_FIX 0xE4
#define BULLET_STATUS_FLY_MASK    0xF0
#define OCCUPIED_BIT              0x80
#define BONUS_TABLE_SIZE          8
#define MAX_ENEMIES               20
#define DEMO_STAGE_NUMBER         36
#define DEMO_DISPLAY_STAGE        30
#define HARD_MODE_STAGE           36
#define FINAL_STAGE               71
#define MAX_STAGE                 35

/*============================================================
 *  TANK STATUS (upper 4 bits = command)
 *============================================================*/
#define TANK_CMD_NONE       0x00 // Empty slot / No tank
#define TANK_CMD_EXPLODE_1  0x10
#define TANK_CMD_EXPLODE_2  0x20
#define TANK_CMD_EXPLODE_3  0x30
#define TANK_CMD_EXPLODE_4  0x40
#define TANK_CMD_EXPLODE_5  0x50
#define TANK_CMD_EXPLODE_6  0x60
#define TANK_CMD_EXPLODE    0x70 // Explosion animation (Lower nibble is the frame countdown, e.g., 0x73 starts it)
#define TANK_CMD_DRIFT      0x80 // Coast / Stop / Ice Slide (Command_Misc_Status)
#define TANK_CMD_RANDOM     0x90 // AI: Pick random direction/target
#define TANK_CMD_MOVE       0xA0 // Move forward until obstacle (The actual "Normal" move)
#define TANK_CMD_TARGET_HQ  0xB0
#define TANK_CMD_TARGET_P2  0xC0
#define TANK_CMD_TARGET_P1  0xD0
#define TANK_CMD_LOAD_TANK  0xE0 // Initialize tank type, armor, and properties
#define TANK_CMD_RESPAWN    0xF0 // Respawn animation (flashing ring)

#define TANK_DIR_UP         0x00
#define TANK_DIR_LEFT       0x01
#define TANK_DIR_DOWN       0x02
#define TANK_DIR_RIGHT      0x03

#define GET_TANK_CMD(id)            ((tank_status[id] & 0xF0) >> 4)
#define GET_TANK_DIR(id)            (tank_status[id] & 0x03)
#define GET_RESPAWN_FRAME(id)       ((tank_status[id] & 0x06) >> 1)
#define SET_TANK_CMD(id, cmd)       (tank_status[id] = (tank_status[id] & 0x0F) | (cmd))
#define SET_TANK_DIR(id, dir)       (tank_status[id] = (tank_status[id] & 0xFC) | (dir))
#define SET_TANK_STATUS(id, cmd, dir) (tank_status[id] = (cmd) | (dir))

/*============================================================
 *  TANK TYPE
 *============================================================*/
#define TANK_ARMOR_0        0x00
#define TANK_ARMOR_1        0x01
#define TANK_ARMOR_2        0x02
#define TANK_ARMOR_3        0x03
#define TANK_BONUS_FLAG     0x04
#define TANK_TYPE_0         0x00
#define TANK_TYPE_1         0x20
#define TANK_TYPE_2         0x40
#define TANK_TYPE_3         0x60 //Last type, armor piercing bullets
#define TANK_TYPE_4         0x80
#define TANK_TYPE_5         0xA0
#define TANK_TYPE_6         0xC0 //Fast bullets
#define TANK_TYPE_7         0xE0

#define GET_TANK_ARMOR(id)      (tank_type[id] & 0x03)
#define IS_BONUS_TANK(id)       (tank_type[id] & TANK_BONUS_FLAG)
#define GET_TANK_TYPE_BITS(id)  (tank_type[id] & 0xE0)
#define GET_TANK_BASE_TILE(id)  (tank_type[id] & 0xF0)

/*============================================================
 *  BULLET STATUS (upper 4 bits = command)
 *============================================================*/
#define BULLET_CMD_IDLE         0x00
#define BULLET_CMD_RICOCHET_3   0x10  // ricochet frame 3 (first)
#define BULLET_CMD_RICOCHET_2   0x20  // ricochet frame 2
#define BULLET_CMD_RICOCHET_1   0x30  // ricochet frame 1 (last)
#define BULLET_CMD_FLY          0x40  // bullet in flight

#define BULLET_RICOCHET_INIT    0x33  // start ricochet: frame 3, timer = 3
#define BULLET_FRAME_TIMER      0x03  // initial per-frame countdown

#define GET_BULLET_CMD(id)      (bullet_status[id] & 0xF0)

/*============================================================
 *  ENUMS
 *============================================================*/
typedef enum {
    STATE_TITLE_INTRO,
    STATE_TITLE_MENU,
    STATE_STAGE_SELECT,
    STATE_STAGE_START,
    STATE_GAMEPLAY,
    STATE_GAME_OVER,
    STATE_CONSTRUCTION,
    STATE_DEMOPLAY
} GameState;

typedef enum {
    STAGE_MODE_NORMAL,
    STAGE_MODE_HARD,
    STAGE_MODE_DEMO
} StageMode;

/*============================================================
 *  STRUCTS
 *============================================================*/
//4x4 tiles quadrant struct for read/write compilation optimizations
typedef struct {
    unsigned char left_top;         // offset 0x00
    unsigned char right_top;        // offset 0x01
    unsigned char pad[0x1E];    
    unsigned char left_bottom;      // offset 0x20
    unsigned char right_bottom;     // offset 0x21
} MetaTileEntry;


/*============================================================
 *  ASM ZERO PAGE VARIABLES
 *============================================================*/
extern unsigned char FRAME_CNT;
#pragma zpsym("FRAME_CNT")
extern unsigned char SECONDS_CNT;
#pragma zpsym("SECONDS_CNT")
extern unsigned char SPR_PRIORITY;
#pragma zpsym("SPR_PRIORITY")
extern unsigned char PAD_STATE[2];
#pragma zpsym("PAD_STATE")
extern unsigned char PAD_STATET[2];
#pragma zpsym("PAD_STATET")
extern unsigned char SPR_X;
#pragma zpsym("SPR_X")
extern unsigned char SPR_Y;
#pragma zpsym("SPR_Y")
extern unsigned char CHR_BYTE;
#pragma zpsym("CHR_BYTE")

/*============================================================
 *  ZERO PAGE VARIABLES
 *============================================================*/
EXTERN unsigned char i, j, k, x, y, temp, cnt;
#pragma zpsym("i")
#pragma zpsym("j")
#pragma zpsym("k")
#pragma zpsym("x")
#pragma zpsym("y")
#pragma zpsym("temp")
#pragma zpsym("cnt")

EXTERN unsigned int temp16;
#pragma zpsym("temp16")

EXTERN unsigned char *src, *dst;
#pragma zpsym("src")
#pragma zpsym("dst")

EXTERN unsigned char random_lo, random_hi;
#pragma zpsym("random_lo")
#pragma zpsym("random_hi")

EXTERN GameState current_state;
#pragma zpsym("current_state")

EXTERN unsigned char pause_flag, cursor_pos, construction_flag;
#pragma zpsym("pause_flag")
#pragma zpsym("cursor_pos")
#pragma zpsym("construction_flag")

EXTERN unsigned char current_spr_tile_index, pal, players_count, max_tanks_count;
#pragma zpsym("current_spr_tile_index")
#pragma zpsym("pal")
#pragma zpsym("players_count")
#pragma zpsym("max_tanks_count")

EXTERN unsigned char skip_stage_select_flag;
#pragma zpsym("skip_stage_select_flag")

EXTERN unsigned char enemy_respawn_place_index;
#pragma zpsym("enemy_respawn_place_index")

EXTERN unsigned char player_lives[2];
#pragma zpsym("player_lives")

EXTERN unsigned char stage_number;
#pragma zpsym("stage_number")

EXTERN unsigned char stage_mode;
#pragma zpsym("stage_mode")

EXTERN unsigned char number_string[8];
#pragma zpsym("number_string")

EXTERN unsigned char hq_armor_timer;
#pragma zpsym("hq_armor_timer")
//0 - hq destroyed, 0x80 - hq alive, variable has explosion countdown in it
EXTERN unsigned char hq_status;
#pragma zpsym("hq_status")

EXTERN unsigned char enemy_reinforce_count;
#pragma zpsym("enemy_reinforce_count")

EXTERN unsigned char enemy_counter;
#pragma zpsym("enemy_counter")

EXTERN unsigned char enemy_type_number;
#pragma zpsym("enemy_type_number")

EXTERN unsigned char enemy_count[4];
#pragma zpsym("enemy_count")

EXTERN unsigned char respawn_timer;
#pragma zpsym("respawn_timer")

EXTERN unsigned char respawn_delay;
#pragma zpsym("respawn_delay")

EXTERN unsigned char invincible_timer[2];
#pragma zpsym("invincible_timer")

EXTERN unsigned char bonus_x, bonus_y, bonus_number, bonus_timer;
#pragma zpsym("bonus_x")
#pragma zpsym("bonus_y")
#pragma zpsym("bonus_number")
#pragma zpsym("bonus_timer")

EXTERN unsigned char ai_x_aim, ai_y_aim;
#pragma zpsym("ai_x_aim")
#pragma zpsym("ai_y_aim")

//Coordinates of exact center  of tank's bounding box
EXTERN unsigned char tank_x[8];
#pragma zpsym("tank_x")
EXTERN unsigned char tank_y[8];
#pragma zpsym("tank_y")

/*
 * tank_status: Main state machine variable for each tank (players and enemies).
 * 
 * Format: [Command (4 bits)][Sub-state/Timer (2 bits)][Direction (2 bits)]
 *
 * Upper Nibble (Command Index):
 *  0x0_ : No tank / Empty slot
 *  0x1_ - 0x7_ : Explosion animation sequence (counts down frames)
 *  0x8_ : Drift / Coast / Stop (bits 2-3 act as a countdown timer)
 *  0x9_ : Get Random AI Status (AI decision making)
 *  0xA_ : Move (normal movement until obstacle)
 *  0xB_ : AI Target = HQ
 *  0xC_ : AI Target = Player 2
 *  0xD_ : AI Target = Player 1
 *  0xE_ : Load Tank (initializes type and properties)
 *  0xF_ : Set Respawn (respawn animation)
 *
 * Lower Nibble (Sub-state & Direction):
 *  - Bits 0-1 (0x03): Movement Direction (0=Up, 1=Left, 2=Down, 3=Right)
 *  - Bits 2-3 (0x0C): Animation frame index or state timer 
 *                     (e.g., drift timer steps down by 4: 8 -> 4 -> 0)
 *
 * Examples:
 *  0xA2 : Moving Down
 *  0x88 : Drifting/Coasting (timer = 8)
 *  0x73 : Explosion animation frame
 *  0xF0 : Respawn animation start
 */
EXTERN unsigned char tank_status[8];
#pragma zpsym("tank_status")

//tank_type bits:
//0,1 - armor level
//2 - bonus tank flag
//3,4 - unused
//5,6,7 - tank type (possibly 8 types)
EXTERN unsigned char tank_type[8];
#pragma zpsym("tank_type")

EXTERN unsigned char track_pos[8];
#pragma zpsym("track_pos")
//Life can be obtained only once per game if player earns 200,000 points ; if not zero - player already received extra life
EXTERN unsigned char player_add_life_flag[2];
#pragma zpsym("player_add_life_flag")

EXTERN unsigned char player_blink_timer[2];
#pragma zpsym("player_blink_timer")

EXTERN unsigned char enemy_freeze_timer;
#pragma zpsym("enemy_freeze_timer")

EXTERN unsigned char player_type[2];
#pragma zpsym("player_type")

EXTERN unsigned char player_ice_status[2];//High bit = flag that tank is on ice
                                          //Lower 5 bits = sliding timer on ice (0 = not sliding)
#pragma zpsym("player_ice_status")

EXTERN unsigned char bullet_status[10];
#pragma zpsym("bullet_status")

EXTERN unsigned char bullet_x[10];
#pragma zpsym("bullet_x")

EXTERN unsigned char bullet_y[10];
#pragma zpsym("bullet_y")

//Bullet property. Format:
//Bit    0: set = bullet twice as fast
//       1: set = armor-piercing bullet 
EXTERN unsigned char bullet_property[10];
#pragma zpsym("bullet_property")

//Bullet status. Format:
//Lower 2 bits (#0,1):  frame counter per animation frame (up to 3)
//Bits #4,5:            ricochet animation frame counter (up to 3)
//Bit #6:               set = bullet in flight, direction attributes processed (lower 2 bits):
//  0 = up
//  1 = left
//  2 = down
//  3 = right
EXTERN unsigned char bullet_status[10];
#pragma zpsym("bullet_status")

EXTERN unsigned char nt_addr_lo[8], nt_addr_hi[8];   
#pragma zpsym("nt_addr_lo")
#pragma zpsym("nt_addr_hi")

//Construction mode variables 
EXTERN unsigned char tsa_block_number, joypad_delay, bkg_occurence_flag;
#pragma zpsym("tsa_block_number")   // TSA_BlockNumber: currently selected wall-block type (0-13)
#pragma zpsym("joypad_delay")       // Joypad_Delay: D-pad auto-repeat timer
#pragma zpsym("bkg_occurence_flag") // BkgOccurence_Flag: "already placed a block at this tile" flag

EXTERN unsigned char snd_move_active;
#pragma zpsym("snd_move_active")




#endif