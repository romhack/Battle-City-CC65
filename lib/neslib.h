//NES hardware-dependent functions by Shiru (shiru@mail.ru)
//Feel free to do anything you want with this code, consider it Public Domain

// for nesdoug version 1.2, 1/1/2022
// changes, removed sprid from oam functions, oam_spr 11% faster, meta 5% faster

//Versions history:
// 050517 - pad polling code optimized, button bits order reversed
// 280215 - fixed palette glitch caused by the active DMC DMA glitch
// 030914 - minor fixes in the VRAM update system
// 310814 - added vram_flush_update
// 120414 - removed adr argument from vram_write and vram_read,
//          unrle_vram renamed to vram_unrle, with adr argument removed
// 060414 - many fixes and improvements, including sequential VRAM updates
// previous versions were created since mid-2011, there were many updates




//wait actual TV frame, 50hz for PAL, 60hz for NTSC

void __fastcall__ ppu_wait_nmi(void);


//turn off rendering, nmi still enabled when rendering is disabled

void __fastcall__ ppu_off(void);


//set PPU_MASK directly

void __fastcall__ ppu_mask(unsigned char mask);



//clear OAM buffer, all the sprites are hidden
// Note: changed. Now also changes sprid (index to buffer) to zero

void __fastcall__ oam_clear(void);


//set sprite display mode, 0 for 8x8 sprites, 1 for 8x16 sprites

void __fastcall__ oam_size(unsigned char size);




// to manually change the sprid (index to sprite buffer)
// perhaps as part of a sprite shuffling algorithm
// Note: this should be a multiple of 4 (0,4,8,12,etc.)

void __fastcall__ oam_set(unsigned char index);

// returns the sprid (index to the sprite buffer)

unsigned char __fastcall__ oam_get(void);




//poll controller and return flags like PAD_LEFT etc, input is pad number (0 or 1)

unsigned char __fastcall__ pad_poll(unsigned char pad);


//get previous pad state without polling ports

unsigned char __fastcall__ pad_state(unsigned char pad);


//set scroll, including the top bits
//it is always applied at beginning of a TV frame, not at the function call

void __fastcall__ scroll(unsigned int x,unsigned int y);


//select current chr bank for background, 0..1

void __fastcall__ bank_bg(unsigned char n);







//set vram pointer to write operations if you need to write some data to vram

void __fastcall__ vram_adr(unsigned int adr);



//write a block to current address of vram, works only when rendering is turned off

void __fastcall__ vram_write(const unsigned char *src,unsigned int size);





//like a normal memcpy, but does not return anything

void __fastcall__ memcpy(void *dst,void *src,unsigned int len);

//like memset, but does not return anything

void __fastcall__ memfill(void *dst,unsigned char value,unsigned int len);





#define PAD_A			0x80
#define PAD_B			0x40
#define PAD_SELECT		0x20
#define PAD_START		0x10
#define PAD_UP			0x08
#define PAD_DOWN		0x04
#define PAD_LEFT		0x02
#define PAD_RIGHT		0x01

#define OAM_FLIP_V		0x80
#define OAM_FLIP_H		0x40
#define OAM_BEHIND		0x20

#define MAX(x1,x2)		((x1)<(x2)?(x2):(x1))
#define MIN(x1,x2)		((x1)<(x2)?(x1):(x2))

#define MASK_SPR		0x10
#define MASK_BG			0x08
#define MASK_EDGE_SPR	0x04
#define MASK_EDGE_BG	0x02

#define NAMETABLE_A		0x2000
#define NAMETABLE_B		0x2400
#define NAMETABLE_C		0x2800
#define NAMETABLE_D		0x2c00

#define NULL			0
#define TRUE			1
#define FALSE			0

#define NT_UPD_HORZ		0x40
#define NT_UPD_VERT		0x80
#define NT_UPD_EOF		0xff

//macro to calculate nametable address from X,Y in compile time

#define NTADR_A(x,y) 	(NAMETABLE_A|(((y)<<5)|(x)))
#define NTADR_B(x,y) 	(NAMETABLE_B|(((y)<<5)|(x)))
#define NTADR_C(x,y) 	(NAMETABLE_C|(((y)<<5)|(x)))
#define NTADR_D(x,y) 	(NAMETABLE_D|(((y)<<5)|(x)))

//macro to get MSB and LSB

#define MSB(x)			(((x)>>8))
#define LSB(x)			(((x)&0xff))

