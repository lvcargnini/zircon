// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <ddk/protocol/platform-device.h>
#include <ddktl/device.h>
#include <ddktl/mmio.h>
#include <ddktl/protocol/i2c.h>
#include <ddktl/protocol/usb-dci.h>
#include <fbl/macros.h>
#include <fbl/unique_ptr.h>
#include <lib/zx/handle.h>
#include <lib/zx/interrupt.h>

#include <threads.h>

namespace mt_usb_dci {

class MtUsbDci;
using MtUsbDciType = ddk::Device<MtUsbDci, ddk::Unbindable>;

class MtUsbDci : public MtUsbDciType, public ddk::UsbDciProtocol<MtUsbDci> {
public:
    explicit MtUsbDci(zx_device_t* parent, pdev_protocol_t* pdev, i2c_protocol_t* i2c)
        : MtUsbDciType(parent), pdev_(*pdev), i2c_(i2c) {}

    static zx_status_t Create(zx_device_t* parent);

    // Device protocol implementation.
    void DdkUnbind();
    void DdkRelease();

    // USB DCI protocol implementation.
     void UsbDciRequestQueue(usb_request_t* req);
     zx_status_t UsbDciSetInterface(const usb_dci_interface_t* interface);
     zx_status_t UsbDciConfigEp(const usb_endpoint_descriptor_t* ep_desc, const
                                usb_ss_ep_comp_descriptor_t* ss_comp_desc);
     zx_status_t UsbDciDisableEp(uint8_t ep_address);
     zx_status_t UsbDciEpSetStall(uint8_t ep_address);
     zx_status_t UsbDciEpClearStall(uint8_t ep_address);
     zx_status_t UsbDciGetBti(zx_handle_t* out_bti);
     size_t UsbDciGetRequestSize();

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(MtUsbDci);

    zx_status_t Init();
    int IrqThread();

    pdev_protocol_t pdev_;
    ddk::I2cProtocolProxy i2c_;
    usb_dci_interface_t dci_intf_ = {};
    zx::handle bti_;

    mmio_buffer_t usb_mmio_;
    mmio_buffer_t phy_mmio_;

    zx::interrupt irq_;
    thrd_t irq_thread_;
};

} // namespace mt_usb_dci

__BEGIN_CDECLS
zx_status_t mt_usb_dci_bind(void* ctx, zx_device_t* parent);
__END_CDECLS
