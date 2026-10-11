#include "bcm2711gpiodrv.h"
#include <dev/protocols/devm.h>
#include <dev/protocols/gpio.h>
#include <string.h>
#include <util/log.h>
#include <util/msg.h>
#include <zuzu/service.h>
#include <zuzu/zuzu.h>

#define LOG_TAG "bcm2711gpiodrv"
#define GPIOS_COMPATIBLE "brcm,bcm2711-gpio"

#define GPIO_MAX_PIN 57
/* The console UART: its pinmux is not ours to change. */
#define PIN_IS_UART(p) ((p) == 14 || (p) == 15)

static Bcm2711GpioMMIO *spi;

static Err CheckPin(uint32_t pin)
{
    if (pin > GPIO_MAX_PIN)
        return ERR_BADARG;
    if (PIN_IS_UART(pin))
        return ERR_NOPERM;
    return ZUZU_OK;
}

static Err GpioSetMode(uint32_t pin, uint32_t mode)
{
    if (mode > 7u)
        return ERR_BADARG;

    uint32_t reg = pin / 10;
    uint32_t shift = (pin % 10) * 3;

    uint32_t val = spi->gpfsel[reg];
    val &= ~(0x7u << shift);
    val |= (mode << shift);
    spi->gpfsel[reg] = val;
    return ZUZU_OK;
}

static Err GpioSetPull(uint32_t pin, uint32_t pull)
{
    if (pull > GPIO_PULL_DOWN)
        return ERR_BADARG;

    uint32_t reg = pin / 16;
    uint32_t shift = (pin % 16) * 2;

    uint32_t val = spi->pup_pdn_cntrl[reg];
    val &= ~(0x3u << shift);
    val |= (pull << shift);
    spi->pup_pdn_cntrl[reg] = val;
    return ZUZU_OK;
}

static void GpioWrite(uint32_t pin, uint32_t value)
{
    uint32_t reg = pin / 32;
    uint32_t mask = 1u << (pin % 32);

    if (value)
        spi->gpset[reg] = mask;
    else
        spi->gpclr[reg] = mask;
}

static uint32_t GpioRead(uint32_t pin)
{
    return (spi->gplev[pin / 32] >> (pin % 32)) & 1u;
}

static void ReplyGpio(Err status, uint32_t value)
{
    GpioReply rep = {.status = status, .value = value};
    MsgWrite(&rep, sizeof(rep));
    Reply(sizeof(rep), -1);
}

static void HandleRequest(const PortWaitResult *r)
{
    /* This protocol never takes a handle; don't leak one a client sent anyway. */
    if (r->granted >= 0)
        HandleClose(r->granted);

    GpioRequest req;
    if (r->xlen < sizeof(req)) {
        ReplyGpio(ERR_BADARG, 0);
        return;
    }
    memcpy(&req, GetMessageBox(), sizeof(req));

    Err err = CheckPin(req.pin);
    if (err != ZUZU_OK) {
        ReplyGpio(err, 0);
        return;
    }

    switch (req.cmd) {
    case GPIO_CMD_SET_MODE:
        ReplyGpio(GpioSetMode(req.pin, req.arg), 0);
        break;
    case GPIO_CMD_SET_PULL:
        ReplyGpio(GpioSetPull(req.pin, req.arg), 0);
        break;
    case GPIO_CMD_WRITE:
        GpioWrite(req.pin, req.arg);
        ReplyGpio(ZUZU_OK, 0);
        break;
    case GPIO_CMD_READ:
        ReplyGpio(ZUZU_OK, GpioRead(req.pin));
        break;
    default:
        ReplyGpio(ERR_NOSYS, 0);
        break;
    }
}

static Handle WaitForDevsvc(void)
{
    for (;;) {
        Handle h = LookupService("/svc/devsvc");
        if (h >= 0)
            return h;
        Sleep(10);
    }
}

int main(void)
{
    Handle devsvc_port = WaitForDevsvc();

    static const char *const compat[] = {GPIOS_COMPATIBLE};
    Handle mmio_handle = RequestDevice(devsvc_port, compat, 1, NULL);
    if (mmio_handle < 0) {
        LOG_ERROR(LOG_TAG, "GPIO not found");
        return 1;
    }

    void *mmio = MemMap(mmio_handle, 0, PROT_RW);
    if (PtrIsErr(mmio)) {
        LOG_ERROR(LOG_TAG, "could not map GPIO registers");
        return 1;
    }
    spi = (Bcm2711GpioMMIO *)mmio;

    Handle port = CreatePort();
    if (port < 0 || RegisterService("/svc/gpio", port) < 0) {
        LOG_ERROR(LOG_TAG, "could not register /svc/gpio");
        return 1;
    }
    LOG_INFO(LOG_TAG, "serving /svc/gpio");

    for (;;) {
        PortWaitResult r = FormatToPortWait(WaitOn(port, TIMEOUT_INFINITE));
        if (r.status == ZUZU_OK)
            HandleRequest(&r);
    }
}