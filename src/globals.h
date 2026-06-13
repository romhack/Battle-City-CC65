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
#define TILE_BLACK          0x00
#define TILE_ARMOR          0x10
#define TILE_GREY           0x11
#define TILE_PLAYER_LIVE    0x14
#define TILE_ICE            0x21
#define TILE_FOREST         0x22

#define STAGE_MIN           1
#define STAGE_MAX           35

/*============================================================
 *  TANK STATUS (upper 4 bits = command)
 *============================================================*/
#define TANK_CMD_NONE       0x00
#define TANK_CMD_EXPLODE_1  0x10
#define TANK_CMD_EXPLODE_2  0x20
#define TANK_CMD_EXPLODE_3  0x30
#define TANK_CMD_EXPLODE_4  0x40
#define TANK_CMD_EXPLODE_5  0x50
#define TANK_CMD_EXPLODE_6  0x60
#define TANK_CMD_EXPLODE_7  0x70
#define TANK_CMD_NORMAL     0x80
#define TANK_CMD_RANDOM     0x90
#define TANK_CMD_MOVE_OBST  0xA0
#define TANK_CMD_TARGET_HQ  0xB0
#define TANK_CMD_TARGET_P2  0xC0
#define TANK_CMD_TARGET_P1  0xD0
#define TANK_CMD_RESPAWN_E  0xE0
#define TANK_CMD_RESPAWN_F  0xF0

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
extern unsigned char PAD_STATE[2];
#pragma zpsym("PAD_STATE")
extern unsigned char PAD_STATET[2];
#pragma zpsym("PAD_STATET")

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

EXTERN unsigned char bonus_x, bonus_y, bonus_number;
#pragma zpsym("bonus_x")
#pragma zpsym("bonus_y")
#pragma zpsym("bonus_number")

//Coordinates of exact center  of tank's bounding box
EXTERN unsigned char tank_x[8];
#pragma zpsym("tank_x")
EXTERN unsigned char tank_y[8];
#pragma zpsym("tank_y")

EXTERN unsigned char tank_status[8];
#pragma zpsym("tank_status")

EXTERN unsigned char tank_type[8];
#pragma zpsym("tank_type")

EXTERN unsigned char track_pos[8];
#pragma zpsym("track_pos")

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

EXTERN unsigned char killcount_1p[4];
#pragma zpsym("killcount_1p")

EXTERN unsigned char killcount_2p[4];
#pragma zpsym("killcount_2p")

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

#endif