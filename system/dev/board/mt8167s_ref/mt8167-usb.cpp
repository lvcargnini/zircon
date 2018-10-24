// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/debug.h>
#include <ddk/device.h>
#include <ddk/io-buffer.h>
#include <ddk/platform-defs.h>
#include <hw/reg.h>
#include <lib/zx/handle.h>

#include <soc/mt8167/mt8167-hw.h>

#include "mt8167.h"

namespace board_mt8167 {

static const pbus_mmio_t dci_mmios[] = {
    {
        .base = MT8167_USB0_BASE,
        .length = MT8167_USB0_LENGTH,
    },
    {
        .base = MT8167_USBPHY_BASE,
        .length = MT8167_USBPHY_LENGTH,
    },
};

static const pbus_irq_t dci_irqs[] = {
    {
        .irq = MT8167_IRQ_USB_MCU,
        .mode = ZX_INTERRUPT_MODE_EDGE_HIGH,
    },
};

static const pbus_i2c_channel_t dci_i2cs[] = {
    {
        .bus_id = 2,
        .address = 0x60,
    },
};

static const pbus_bti_t dci_btis[] = {
    {
        .iommu_index = 0,
        .bti_id = BTI_USB_DCI,
    },
};

static pbus_dev_t dci_dev = [](){
    pbus_dev_t dev;
    dev.name = "mt-usb-dci";
    dev.vid = PDEV_VID_MEDIATEK;
    dev.did = PDEV_DID_MEDIATEK_USB_DCI;
    dev.mmio_list = dci_mmios;
    dev.mmio_count = countof(dci_mmios);
    dev.irq_list = dci_irqs;
    dev.irq_count = countof(dci_irqs);
    dev.i2c_channel_list = dci_i2cs;
    dev.i2c_channel_count = countof(dci_i2cs);
    dev.bti_list = dci_btis;
    dev.bti_count = countof(dci_btis);
    return dev;
}();

zx_status_t Mt8167::UsbInit() {
    auto status = pbus_.DeviceAdd(&dci_dev);
    if (status != ZX_OK) {
        zxlogf(ERROR, "%s: DeviceAdd failed %d\n", __func__, status);
        return status;
    }

    return ZX_OK;
}

} // namespace board_mt8167
