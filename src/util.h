#ifndef UTIL_H
#define UTIL_H

void buffer_and_vram_fill(unsigned int offset, unsigned char byte, unsigned char length);
void buffer_and_vram_cpy(unsigned char x, unsigned char y, const unsigned char* src, unsigned char length);
void set_number_string(unsigned char number);
void init_8_char_string(unsigned char *str);
unsigned char skip_leading_zeros(const unsigned char *num_str, unsigned char single_zero);

#endif