/*
    Header include the implementation of the Custom 8-bit Sharp LR35902(https://en.wikipedia.org/wiki/Game_Boy) that's
    based on a modified Z80 and 8080 chip Possibly a SM83 core. 
    The processor of the GB runs at a clock speed of around 4.19Mhz
*/
#pragma once
#include<stddef.h>
#include<stdint.h>
#include"memory.h"

#define REGISTER_UNION(UPPER, LOWER)    \
union{                                  \
struct{ uint8_t UPPER; uint8_t LOWER; }; uint16_t UPPER ##LOWER;  };

typedef struct{
    union{
        struct{
            uint8_t a;
            struct{
                uint8_t z:1, n:1, h:1, c:1;
            } f;
        };
        uint16_t af;
    };
    REGISTER_UNION(b, c);
    REGISTER_UNION(d, e);
    REGISTER_UNION(h, l);
    uint16_t sp;
    uint16_t pc;
    Memory memory;
    size_t t_cycles;
} CPU;

CPU* createCPU(void);

void updateCPU(CPU*);
