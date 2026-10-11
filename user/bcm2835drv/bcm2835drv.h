#ifndef BCM2835_DRV_H
#define BCM2835_DRV_H

#include <stddef.h>
#include <stdint.h>

#define SPIDRV_COMPATIBLE "brcm,bcm2835-spi"

#define SPI_CORE_CLOCK_HZ 500000000u

typedef struct {
    volatile uint32_t cs;   /* 0x00: control and status */
    volatile uint32_t fifo; /* 0x04: TX (write) / RX (read) FIFO */
    volatile uint32_t clk;  /* 0x08: clock divider */
    volatile uint32_t dlen; /* 0x0C: data length (DMA mode only) */
    volatile uint32_t ltoh; /* 0x10: LoSSI output hold delay */
    volatile uint32_t dc;   /* 0x14: DMA DREQ controls */
} Bcm2835MMIO;

_Static_assert(sizeof(Bcm2835MMIO) == 0x18, "SPI0 register block must be 0x18 bytes");
_Static_assert(offsetof(Bcm2835MMIO, clk) == 0x08, "CLK must sit at 0x08");

/* ---- CS register ------------------------------------------------------------- */
#define SPI_CS_CS_MASK 0x3u /* [1:0] chip select: 0 = CE0, 1 = CE1 */
#define SPI_CS_CPHA (1u << 2)
#define SPI_CS_CPOL (1u << 3)
#define SPI_CS_CLEAR_TX (1u << 4)
#define SPI_CS_CLEAR_RX (1u << 5)
#define SPI_CS_CSPOL (1u << 6) /* chip select active high when set */
#define SPI_CS_TA (1u << 7)    /* transfer active */
#define SPI_CS_DMAEN (1u << 8)
#define SPI_CS_INTD (1u << 9)  /* interrupt on done */
#define SPI_CS_INTR (1u << 10) /* interrupt when RX needs reading */
#define SPI_CS_ADCS (1u << 11) /* DMA only: auto deassert chip select */
#define SPI_CS_REN (1u << 12)  /* read enable (bidirectional mode) */
#define SPI_CS_LEN (1u << 13)  /* LoSSI enable */
#define SPI_CS_DONE (1u << 16) /* transfer done (read-only) */
#define SPI_CS_RXD (1u << 17)  /* RX FIFO has data */
#define SPI_CS_TXD (1u << 18)  /* TX FIFO can accept data */
#define SPI_CS_RXR (1u << 19)  /* RX FIFO needs reading (>= 3/4 full) */
#define SPI_CS_RXF (1u << 20)  /* RX FIFO full */
#define SPI_CS_CSPOL0 (1u << 21)
#define SPI_CS_CSPOL1 (1u << 22)
#define SPI_CS_CSPOL2 (1u << 23)
#define SPI_CS_DMA_LEN (1u << 24)
#define SPI_CS_LEN_LONG (1u << 25)

/* SPI mode 0..3 maps to {CPOL, CPHA} = {0,0} {0,1} {1,0} {1,1}. */
#define SPI_CS_MODE(m) ((((uint32_t)(m) & 0x2u) ? SPI_CS_CPOL : 0u) | \
                        (((uint32_t)(m) & 0x1u) ? SPI_CS_CPHA : 0u))

/* ---- CLK register ------------------------------------------------------------ */
#define SPI_CLK_CDIV_MASK 0xFFFFu /* 0 means 65536. Use even values; the datasheet says
                                   * power of two. */

/* ---- DC register ------------------------------------------------------------- */
#define SPI_DC_TDREQ_SHIFT 0
#define SPI_DC_TPANIC_SHIFT 8
#define SPI_DC_RDREQ_SHIFT 16
#define SPI_DC_RPANIC_SHIFT 24

/* FIFO depth in bytes (16 words of 32 bits, used one byte at a time here). */
#define SPI_FIFO_DEPTH 64u

#endif /* BCM2835_DRV_H */