#ifndef SPI_PROTOCOL_H
#define SPI_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <util/msg.h>
#include <zuzu/err.h>
#include <zuzu/zuzu.h>


#define SPI_SERVICE_NAME "/svc/spi"

#define SPI_CMD_XFER_INLINE                                                                        \
    1 /* req {SpiRequest} + len TX bytes. reply {SpiReply} + len RX bytes (none with NO_RX). */
#define SPI_CMD_ATTACH_BUF                                                                         \
    2 /* grant: a memory object (PERM_MAP|PERM_TXFR). req {SpiRequest}; reply {SpiReply}, len =   \
         size of the buffer in bytes. Replaces this client's earlier buffer. */
#define SPI_CMD_XFER                                                                               \
    3 /* req {SpiRequest}: TX bytes taken from the attached buffer at `offset`; RX bytes written   \
         back there unless NO_RX. reply {SpiReply}. */
#define SPI_CMD_INFO 4 /* req {SpiRequest} (only cmd is read). reply {SpiInfoReply}. */

/* ---- transfer configuration -------------------------------------------------- */

#define SPI_MODE_0 0u /* CPOL 0, CPHA 0 */
#define SPI_MODE_1 1u /* CPOL 0, CPHA 1 */
#define SPI_MODE_2 2u /* CPOL 1, CPHA 0 */
#define SPI_MODE_3 3u /* CPOL 1, CPHA 1 */

#define SPI_MAX_CS 2u /* CE0, CE1 */

/* `cfg` packs the chip select and the mode: bits [1:0] = chip select, bits [3:2] = mode. */
#define SPI_CFG(cs, mode) ((((uint32_t)(mode) & 0x3u) << 2) | ((uint32_t)(cs) & 0x3u))
#define SPI_CFG_CS(cfg) ((uint32_t)(cfg) & 0x3u)
#define SPI_CFG_MODE(cfg) (((uint32_t)(cfg) >> 2) & 0x3u)

#define SPI_FLAG_NO_RX (1u << 0) /* write-only: the received bytes are discarded */

typedef struct {
    uint32_t cmd;
    uint32_t cfg;    /* SPI_CFG(cs, mode) */
    uint32_t hz;     /* SCLK in Hz; the driver rounds down to what the divider allows */
    uint32_t len;    /* bytes to transfer */
    uint32_t offset; /* XFER only: byte offset into the attached buffer */
    uint32_t flags;  /* SPI_FLAG_* */
} SpiRequest;

typedef struct {
    Err status;
    uint32_t len; /* bytes transferred (ATTACH_BUF: size of the buffer) */
} SpiReply;

typedef struct {
    Err status;
    uint32_t core_hz;      /* clock the divider works from */
    uint32_t max_hz;       /* fastest SCLK the driver will program */
    uint32_t fifo_depth;   /* bytes */
    uint32_t max_inline;   /* largest len XFER_INLINE accepts */
} SpiInfoReply;

/* The request header is the same size for every command, so the TX bytes of an inline
 * request always start right after it. */
#define SPI_MAX_INLINE (MSG_BUF_SIZE - (uint32_t)sizeof(SpiRequest))

/* ---- client helpers ---------------------------------------------------------- */

/* Sends `req` (plus `extra` bytes already placed after it in MessageBuf()), and
 * returns the reply header. On success the reply payload is still in MessageBuf(). */
static inline Err SpiRoundTrip(Handle spi_port, const SpiRequest *req, uint32_t extra,
                               Handle grant, SpiReply *rep, uint32_t *reply_len)
{
    SvcResult r = Call(spi_port, (uint32_t)sizeof(*req) + extra, grant);
    if (r.r0 != ZUZU_OK)
        return (Err)r.r0;
    if ((uint32_t)r.r1 < sizeof(*rep))
        return ERR_MALFORMED;
    memcpy(rep, GetMessageBox(), sizeof(*rep));
    if (reply_len)
        *reply_len = (uint32_t)r.r1;
    return rep->status;
}

/*
 * Short full-duplex transfer. `tx` may be NULL (zero bytes are sent) and `rx` may be NULL
 * (the received bytes are discarded; the driver is told not to send them back).
 */
static inline Err SpiXferInline(Handle spi_port, uint32_t cs, uint32_t mode, uint32_t hz,
                                const void *tx, void *rx, uint32_t len)
{
    if (len == 0 || len > SPI_MAX_INLINE)
        return ERR_BADARG;

    SpiRequest req = {
        .cmd = SPI_CMD_XFER_INLINE,
        .cfg = SPI_CFG(cs, mode),
        .hz = hz,
        .len = len,
        .flags = rx ? 0u : SPI_FLAG_NO_RX,
    };
    uint8_t *box = GetMessageBox();
    memcpy(box, &req, sizeof(req));
    if (tx)
        memcpy(box + sizeof(req), tx, len);
    else
        memset(box + sizeof(req), 0, len);

    SpiReply rep;
    uint32_t reply_len = 0;
    Err rc = SpiRoundTrip(spi_port, &req, len, -1, &rep, &reply_len);
    if (rc != ZUZU_OK)
        return rc;
    if (rx) {
        if (reply_len < sizeof(rep) + len)
            return ERR_MALFORMED;
        memcpy(rx, (const uint8_t *)GetMessageBox() + sizeof(rep), len);
    }
    return ZUZU_OK;
}

/*
 * Gives the driver a shared buffer for XFER. `mem` is a memory object the caller has
 * mapped and keeps; it is duplicated for the grant. On success *size is the buffer
 * length the driver accepted.
 */
static inline Err SpiAttachBuf(Handle spi_port, Handle mem, uint32_t *size)
{
    SvcResult dup = HandleDuplicate(mem, PERM_MAP | PERM_TXFR, MARKER_NONE);
    if (dup.r0 != ZUZU_OK)
        return (Err)dup.r0;

    SpiRequest req = {.cmd = SPI_CMD_ATTACH_BUF};
    MsgWrite(&req, sizeof(req));
    SpiReply rep;
    Err rc = SpiRoundTrip(spi_port, &req, 0, (Handle)dup.r1, &rep, NULL);
    HandleClose((Handle)dup.r1);
    if (rc == ZUZU_OK && size)
        *size = rep.len;
    return rc;
}

/* Bulk transfer against the buffer given to SpiAttachBuf, starting at `offset`. */
static inline Err SpiXfer(Handle spi_port, uint32_t cs, uint32_t mode, uint32_t hz,
                          uint32_t offset, uint32_t len, uint32_t flags)
{
    SpiRequest req = {
        .cmd = SPI_CMD_XFER,
        .cfg = SPI_CFG(cs, mode),
        .hz = hz,
        .len = len,
        .offset = offset,
        .flags = flags,
    };
    MsgWrite(&req, sizeof(req));
    SpiReply rep;
    return SpiRoundTrip(spi_port, &req, 0, -1, &rep, NULL);
}

static inline Err SpiInfo(Handle spi_port, SpiInfoReply *out)
{
    SpiRequest req = {.cmd = SPI_CMD_INFO};
    MsgWrite(&req, sizeof(req));
    SvcResult r = Call(spi_port, (uint32_t)sizeof(req), -1);
    if (r.r0 != ZUZU_OK)
        return (Err)r.r0;
    if ((uint32_t)r.r1 < sizeof(*out))
        return ERR_MALFORMED;
    memcpy(out, GetMessageBox(), sizeof(*out));
    return out->status;
}

#ifdef __cplusplus
}
#endif

#endif /* SPI_PROTOCOL_H */