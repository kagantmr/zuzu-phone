#include "bcm2835drv.h"
#include "types.h"
#include <dev/protocols/devm.h>
#include <dev/protocols/gpio.h>
#include <util/log.h>
#include <util/msg.h>
#include <zuzu/service.h>
#include <zuzu/zuzu.h>

#define LOG_TAG "bcm2835drv"
#define COMPATIBLE "brcm,bcm2835-spi"

static Bcm2835MMIO *spi;


static Handle WaitForDevsvc(void)
{
    for (;;) {
        Handle h = LookupService("/svc/devsvc");
        if (h >= 0)
            return h;
        Sleep(10);
    }
}

static void InitDevice(void) {
    spi->cs = SPI_CS_CLEAR_RX | SPI_CS_CLEAR_TX;
}

static void HandleRequest(PortWaitResult *r) {

}

int main(void)
{
    Handle devsvc_port = WaitForDevsvc();

    static const char *const compat[] = {COMPATIBLE};
    Handle mmio_handle = RequestDevice(devsvc_port, compat, 1, NULL);
    if (mmio_handle < 0) {
        LOG_ERROR(LOG_TAG, "SPI not found");
        return 1;
    }

    void *mmio = MemMap(mmio_handle, 0, PROT_RW);
    if (PtrIsErr(mmio)) {
        LOG_ERROR(LOG_TAG, "could not map MMIO");
        return 1;
    }
    spi = (Bcm2835MMIO *)mmio;

    InitDevice();

    Handle port = CreatePort();
    if (port < 0 || RegisterService("/svc/spi", port) < 0) {
        LOG_ERROR(LOG_TAG, "could not register /svc/spi");
        return 1;
    }
    
    LOG_INFO(LOG_TAG, "serving /svc/spi");

    for (;;) {
        PortWaitResult r = FormatToPortWait(WaitOn(port, TIMEOUT_INFINITE));
        if (r.status == ZUZU_OK)
            HandleRequest(&r);
    }
}