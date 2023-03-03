#include <3ds.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/types.h>

//---------------------------------------------------------------------------------
void failExit(const char *fmt, ...) {
//---------------------------------------------------------------------------------

	consoleInit(GFX_TOP, NULL);
	va_list ap;

	printf(CONSOLE_RED);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf(CONSOLE_RESET);
	printf("\nPress B to exit\n");

	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_B) exit(0);
	}
}


void u32ToU8(unsigned long int const value, uint8_t * const buffer){
    buffer[0] = (value >> 24) & 0xFF;
    buffer[1] = (value >> 16) & 0xFF;
    buffer[2] = (value >> 8) & 0xFF;
    buffer[3] = value & 0xFF;
}

void s32ToU8(signed long int const value, uint8_t * const buffer){
    u32ToU8((unsigned long int)value, buffer);
}

void floatToU8(const float value, uint8_t * const buffer){
    uint8_t *res = ( uint8_t* )&value;
    buffer[0] = res[0];
    buffer[1] = res[1];
    buffer[2] = res[2];
    buffer[3] = res[3];
}

void u16ToU8(unsigned short const value, uint8_t * const buffer){
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = value & 0xFF;
}
void s16ToU8(signed short int const value, uint8_t * const buffer){
    u16ToU8((unsigned short int)value, buffer);
}

unsigned long int U8ToU32(u8 *buffer){
    return (((uint8_t) buffer[0]) << 24)
          |(((uint8_t) buffer[1]) << 16)
          |(((uint8_t) buffer[2]) << 8)
          |(((uint8_t) buffer[3]));
}
signed long int U8ToS32(u8 *buffer){
    return (signed long int)U8ToU32(buffer);
}

unsigned short int U8ToU16(uint8_t const * const buffer){
    return ((uint8_t) buffer[0] << 8)
          |((uint8_t) buffer[1]);
}
signed short int U8ToS16(uint8_t const * const buffer){
    return (signed short int)U8ToU16(buffer);
}