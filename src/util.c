#include "../lib/neslib.h"
#include "globals.h"
#include "util.h"

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
// If all zeros, keeps last TWO zeros
unsigned char skip_leading_zeros(const unsigned char *num_str) {
    i = 0;    
    // Find first non-zero
    while (num_str[i] != 0xFF && num_str[i] == 0) {
        ++i;
    }    
    // If all zeros, back up two to show "00"
    if (num_str[i] == 0xFF && i > 1) {
        i -= 2;
    }    
    return i;
}
