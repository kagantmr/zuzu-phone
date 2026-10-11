/**
 * @file bcm2711gpiodrv.h
 * @brief Header file for the zuzu BCM2711 board GPIO driver
 */

#ifndef BCM2711_GPIODRV_H
#define BCM2711_GPIODRV_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    volatile uint32_t gpfsel[6];        /* 0x00 - 0x14: function select */
    volatile uint32_t reserved1;        /* 0x18 */
    volatile uint32_t gpset[2];         /* 0x1C - 0x20: output set */
    volatile uint32_t reserved2;        /* 0x24 */
    volatile uint32_t gpclr[2];         /* 0x28 - 0x2C: output clear */
    volatile uint32_t reserved3;        /* 0x30 */
    volatile uint32_t gplev[2];         /* 0x34 - 0x38: pin level (read) */
    volatile uint32_t reserved4;        /* 0x3C */
    volatile uint32_t gpeds[2];         /* 0x40 - 0x44: event detect status */
    volatile uint32_t reserved5;        /* 0x48 */
    volatile uint32_t gpren[2];         /* 0x4C - 0x50: rising edge detect enable */
    volatile uint32_t reserved6;        /* 0x54 */
    volatile uint32_t gpfen[2];         /* 0x58 - 0x5C: falling edge detect enable */
    volatile uint32_t reserved7;        /* 0x60 */
    volatile uint32_t gphen[2];         /* 0x64 - 0x68: high detect enable */
    volatile uint32_t reserved8;        /* 0x6C */
    volatile uint32_t gplen[2];         /* 0x70 - 0x74: low detect enable */
    volatile uint32_t reserved9;        /* 0x78 */
    volatile uint32_t gparen[2];        /* 0x7C - 0x80: async rising edge detect enable */
    volatile uint32_t reserved10;       /* 0x84 */
    volatile uint32_t gpafen[2];        /* 0x88 - 0x8C: async falling edge detect enable */
    volatile uint32_t reserved11[21];   /* padding to reach 0xE4 */
    volatile uint32_t pup_pdn_cntrl[4]; /* 0xE4 - 0xF0: pull-up / pull-down cntrl */
} Bcm2711GpioMMIO;

#endif /* BCM2711_GPIODRV_H */