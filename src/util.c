#include "globals.h"
#include "../game.h"
#include "util.h"
#include "sound.h"

//Due to rewritten sound engine it now has reentrance problem.
//So we call sound_update on every wait of NMI, but not inside NMI handler
void frame_wait(void)
{
    ppu_wait_nmi();
    sound_update();
}

//Copy data from source to SCREEN_BUFFER (for collision handling) 
// and create vram copy command in queue (for visual represenation)
void buffer_and_vram_cpy(unsigned char x, unsigned char y, const unsigned char* src, unsigned char length){
    memcpy(SCREEN_BUFFER + COORD_TO_NT_OFFSET(x, y), src, length);
    add_vram_cpy_cmd(NAMETABLE_A + COORD_TO_NT_OFFSET(x, y), src, length);
}
void buffer_and_vram_fill(unsigned int offset, unsigned char byte, unsigned char length){
    memfill(SCREEN_BUFFER + offset, byte, length);
    add_vram_fill_cmd((unsigned char *)(NAMETABLE_A + offset), byte, length);
}

//Convert number byte to decimal digits in number_string
void set_number_string(unsigned char number){
    
    
	init_8_char_string(number_string);
    while (number >= 10) {
        number -= 10;
        ++number_string[5];  // Tens digit
    }
    number_string[6] = number;    // Store ones digit (position 6)	
}

void init_8_char_string(unsigned char *str) {
	
    memfill(str, 0, 7);
    str[7] = 0xFF;		
}
// Find index of first non-zero character in number string
// Returns: index to start drawing from
// If all zeros, keeps one or keeps last TWO zeros depending on single_zero parameter
unsigned char skip_leading_zeros(const unsigned char *num_str, unsigned char single_zero) {
    i = 1;
    
    // Find the first non-zero digit or the $FF terminator
    while (num_str[i] == 0) {
        ++i;
    }
    
    if (num_str[i] == 0xFF) {
        /* All zeros: back up to keep at least one zero. */
        --i;
        
        /* Default behavior (single_zero == 0) keeps two zeros. */
        if (single_zero == 0) {
            --i;
        }
    }
    
    /* 
     * For non-zero numbers, 'i' is already pointing exactly at the first 
     * non-zero digit. Returning it as-is prevents drawing an extra leading 
     * zero (e.g., correctly draws "9000" instead of "09000").
     */
    return i;
}
