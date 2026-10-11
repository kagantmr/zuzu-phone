#ifndef GPIO_PROTOCOL_H
#define GPIO_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <util/msg.h>
#include <zuzu/err.h>
#include <zuzu/zuzu.h>

/*
 * GPIO protocol, served at "/svc/gpio". Every request is one GpioRequest and is
 * answered with exactly one GpioReply, synchronously. Pin numbers are the SoC's
 * (BCM numbering); which pin does what is the board's business, not this protocol's.
 */

#define GPIO_CMD_SET_MODE 1 /* req {cmd, pin, mode}: GPIO_MODE_*. */
#define GPIO_CMD_SET_PULL 2 /* req {cmd, pin, pull}: GPIO_PULL_*. */
#define GPIO_CMD_WRITE 3    /* req {cmd, pin, value}: 0 or 1. */
#define GPIO_CMD_READ 4     /* req {cmd, pin}; reply.value is the pin level (0 or 1). */

/* Pin function. Values are the hardware FSEL encodings. To drive a pin without a glitch,
 * WRITE the level you want first, then switch it to GPIO_MODE_OUTPUT. */
#define GPIO_MODE_INPUT 0u
#define GPIO_MODE_OUTPUT 1u
#define GPIO_MODE_ALT0 4u
#define GPIO_MODE_ALT1 5u
#define GPIO_MODE_ALT2 6u
#define GPIO_MODE_ALT3 7u
#define GPIO_MODE_ALT4 3u
#define GPIO_MODE_ALT5 2u

#define GPIO_PULL_NONE 0u
#define GPIO_PULL_UP 1u
#define GPIO_PULL_DOWN 2u

typedef struct {
    uint32_t cmd;
    uint32_t pin;
    uint32_t arg;
} GpioRequest;

typedef struct {
    Err status;
    uint32_t value;
} GpioReply;

/* One round trip. On success *value (if non-NULL) receives the reply's value. */
static inline Err GpioCall(Handle gpio_port, uint32_t cmd, uint32_t pin, uint32_t arg,
                           uint32_t *value)
{
    GpioRequest req = {.cmd = cmd, .pin = pin, .arg = arg};
    MsgWrite(&req, sizeof(req));
    SvcResult r = Call(gpio_port, sizeof(req), -1);
    if (r.r0 != ZUZU_OK)
        return (Err)r.r0;
    GpioReply rep;
    if ((uint32_t)r.r1 < sizeof(rep))
        return ERR_MALFORMED;
    memcpy(&rep, GetMessageBox(), sizeof(rep));
    if (rep.status == ZUZU_OK && value)
        *value = rep.value;
    return rep.status;
}

#ifdef __cplusplus
}
#endif

#endif /* GPIO_PROTOCOL_H */