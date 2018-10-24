// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mt-usb-dci.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ddk/binding.h>
#include <ddk/debug.h>
#include <ddk/device.h>
#include <ddk/driver.h>
#include <ddk/platform-defs.h>
#include <ddk/protocol/gpio.h>
#include <ddk/protocol/platform-device.h>
#include <ddk/protocol/platform-device-lib.h>
#include <fbl/algorithm.h>
#include <fbl/unique_ptr.h>

namespace mt_usb_dci {

zx_status_t MtUsbDci::Create(zx_device_t* parent) {
    pdev_protocol_t pdev;
    i2c_protocol_t i2c;

    auto status = device_get_protocol(parent, ZX_PROTOCOL_PDEV, &pdev);
    if (status != ZX_OK) {
        return status;
    }
    status = device_get_protocol(parent, ZX_PROTOCOL_I2C, &i2c);
    if (status != ZX_OK) {
        return status;
    }

    fbl::AllocChecker ac;
    auto dci = fbl::make_unique_checked<MtUsbDci>(&ac, parent, &pdev, &i2c);
    if (!ac.check()) {
        return ZX_ERR_NO_MEMORY;
    }

    status = dci->Init();
    if (status != ZX_OK) {
        return status;
    }

    // devmgr is now in charge of the device.
    __UNUSED auto* dummy = dci.release();
    return ZX_OK;
}

zx_status_t MtUsbDci::Init() {
    auto status = pdev_get_bti(&pdev_, 0, bti_.reset_and_get_address());
    if (status != ZX_OK) {
        return status;
    }

    status = pdev_map_mmio_buffer2(&pdev_, 0, ZX_CACHE_POLICY_UNCACHED_DEVICE, &usb_mmio_);
    if (status != ZX_OK) {
        return status;
    }

    status = pdev_map_mmio_buffer2(&pdev_, 1, ZX_CACHE_POLICY_UNCACHED_DEVICE, &phy_mmio_);
    if (status != ZX_OK) {
        return status;
    }

    status = pdev_map_interrupt(&pdev_, 0, irq_.reset_and_get_address());
    if (status != ZX_OK) {
        return status;
    }

    int rc = thrd_create_with_name(&irq_thread_,
                                   [](void* arg) -> int {
                                       return reinterpret_cast<MtUsbDci*>(arg)->IrqThread();
                                   },
                                   reinterpret_cast<void*>(this),
                                   "mt-usb-dci-irq-thread");
    if (rc != thrd_success) {
        return ZX_ERR_INTERNAL;
    }

    status = DdkAdd("mt-usb-dci");
    if (status != ZX_OK) {
        return status;
    }
    return ZX_OK;
}

int MtUsbDci::IrqThread() {
    while (true) {
        auto status = irq_.wait(nullptr);
        if (status == ZX_ERR_CANCELED) {
            return 0;
        } else if (status != ZX_OK) {
            zxlogf(ERROR, "%s: irq_.wait failed: %d\n", __func__, status);
            return -1;
        }
        zxlogf(INFO, "%s: got interrupt!\n", __func__);
    }
}

void MtUsbDci::DdkUnbind() {
    irq_.destroy();
    thrd_join(irq_thread_, nullptr);
}

void MtUsbDci::DdkRelease() {
    mmio_buffer_release(&usb_mmio_);
    mmio_buffer_release(&phy_mmio_);
    delete this;
}

 void MtUsbDci::UsbDciRequestQueue(usb_request_t* req) {
 printf("%s\n", __func__);
 }
 
 zx_status_t MtUsbDci::UsbDciSetInterface(const usb_dci_interface_t* interface) {
    memcpy(&dci_intf_, interface, sizeof(dci_intf_));
    return ZX_OK;
}

 zx_status_t MtUsbDci::UsbDciConfigEp(const usb_endpoint_descriptor_t* ep_desc, const
                            usb_ss_ep_comp_descriptor_t* ss_comp_desc) {
    return ZX_OK;
}

 zx_status_t MtUsbDci::UsbDciDisableEp(uint8_t ep_address) {
    return ZX_OK;
}

 zx_status_t MtUsbDci::UsbDciEpSetStall(uint8_t ep_address) {
    return ZX_OK;
}

 zx_status_t MtUsbDci::UsbDciEpClearStall(uint8_t ep_address) {
    return ZX_OK;
}

 zx_status_t MtUsbDci::UsbDciGetBti(zx_handle_t* out_bti) {
    *out_bti = bti_.get();
    return ZX_OK;
}

size_t MtUsbDci::UsbDciGetRequestSize() {
    return 0;
}

} // namespace mt_usb_dci

zx_status_t mt_usb_dci_bind(void* ctx, zx_device_t* parent) {
    return mt_usb_dci::MtUsbDci::Create(parent);
}
