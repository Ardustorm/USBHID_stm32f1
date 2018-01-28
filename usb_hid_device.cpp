/* Copyright (c) 2011, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#include "USBHID.h"

#include <string.h>
#include <stdint.h>
#include <libmaple/nvic.h>
#include "usb_hid.h"
#include "usb_serial.h"
#include "usb_generic.h"
#include <libmaple/usb.h>
#include <string.h>
#include <libmaple/iwdg.h>

#include <wirish.h>

/*
 * USB HID interface
 */

#if defined(SERIAL_USB)
static void rxHook(unsigned, void*);
static void ifaceSetupHook(unsigned, void*);
#endif

#define USB_TIMEOUT 50

USBHIDDevice::USBHIDDevice(void) {
}

static void generateUSBDescriptor(uint8* out, int maxLength, const char* in) {
    int length = strlen(in);
    if (length > maxLength)
        length = maxLength;
    int outLength = USB_DESCRIPTOR_STRING_LEN(length);
    
    out[0] = outLength;
    out[1] = USB_DESCRIPTOR_TYPE_STRING;
    
    for (int i=0; i<length; i++) {
        out[2+2*i] = in[i];
        out[3+2*i] = 0;
    }    
}

static char* putSerialNumber(char* out, int nibbles, uint32 id) {
    for (int i=0; i<nibbles; i++, id >>= 4) {
        uint8 nibble = id & 0xF;
        if (nibble <= 9)
            *out++ = nibble + '0';
        else
            *out++ = nibble - 10 + 'a';
    }
    return out;
}

const char* getDeviceIDString() {
    static char string[80/4+1];
    char* p = string;
    
    uint32 id = (uint32) *(uint16*) (0x1FFFF7E8+0x02);
    p = putSerialNumber(p, 4, id);
    
    id = *(uint32*) (0x1FFFF7E8+0x04);
    p = putSerialNumber(p, 8, id);
    
    id = *(uint32*) (0x1FFFF7E8+0x08);
    p = putSerialNumber(p, 8, id);
    
    *p = 0;
    
    return string;
}

void USBHIDDevice::begin(const uint8_t* report_descriptor, uint16_t report_descriptor_length, uint16_t idVendor, uint16_t idProduct,
        const char* manufacturer, const char* product, const char* serialNumber) {
            
    uint8_t* manufacturerDescriptor;
    uint8_t* productDescriptor;
    uint8_t* serialNumberDescriptor;
    
	if(!enabled) {        
        if (manufacturer != NULL) {
            generateUSBDescriptor(iManufacturer, USB_HID_MAX_MANUFACTURER_LENGTH, manufacturer);
            manufacturerDescriptor = iManufacturer;
        }
        else {
            manufacturerDescriptor = NULL;
        }

        if (product != NULL) {
            generateUSBDescriptor(iProduct, USB_HID_MAX_PRODUCT_LENGTH, product);
            productDescriptor = iProduct;
        }
        else {
            productDescriptor = NULL;
        }
        
        if (serialNumber != NULL) {
            generateUSBDescriptor(iSerialNumber, USB_HID_MAX_SERIAL_NUMBER_LENGTH, serialNumber);
            serialNumberDescriptor = iSerialNumber;
        }
        else {
            serialNumberDescriptor = NULL;
        }

        usb_generic_set_info(idVendor, idProduct, manufacturerDescriptor, productDescriptor, 
            serialNumberDescriptor);
        usb_hid_set_report_descriptor(report_descriptor, report_descriptor_length);
        if (serialSupport) {
            numParts = 2;
            parts[0] = &usbHIDPart;
            parts[1] = &usbSerialPart;            
        }
        else {
            numParts = 1;
            parts[0] = &usbHIDPart;
        }
        usb_generic_set_parts(parts, numParts);
        usb_generic_enable();        
            
#if defined(SERIAL_USB)
        if (serialSupport) {
            composite_cdcacm_set_hooks(USBHID_CDCACM_HOOK_RX, rxHook);
            composite_cdcacm_set_hooks(USBHID_CDCACM_HOOK_IFACE_SETUP, ifaceSetupHook);
        }
#endif
            
		enabled = true;
	}
}

void USBHIDDevice::setSerial(uint8 _serialSupport) {
    serialSupport = _serialSupport;
}

void USBHIDDevice::begin(const HIDReportDescriptor* report, uint16_t idVendor, uint16_t idProduct,
        const char* manufacturer, const char* product, const char* serialNumber) {
    begin(report->descriptor, report->length, idVendor, idProduct, manufacturer, product, serialNumber);
}

void USBHIDDevice::setBuffers(uint8_t type, volatile HIDBuffer_t* fb, int count) {
    usb_hid_set_buffers(type, fb, count);
}

bool USBHIDDevice::addBuffer(uint8_t type, volatile HIDBuffer_t* buffer) {
    return 0 != usb_hid_add_buffer(type, buffer);
}

void USBHIDDevice::end(void){
	if(enabled){
        usb_generic_disable();
		enabled = false;
	}
}

void HIDReporter::sendReport() {
//    while (usb_is_transmitting() != 0) {
//    }

    unsigned toSend = bufferSize;
    uint8* b = buffer;
    
    while (toSend) {
        unsigned delta = usb_hid_tx(b, toSend);
        toSend -= delta;
        b += delta;
    }
    
//    while (usb_is_transmitting() != 0) {
//    }

    /* flush out to avoid having the pc wait for more data */
    usb_hid_tx(NULL, 0);
}
        
HIDReporter::HIDReporter(uint8_t* _buffer, unsigned _size, uint8_t _reportID) {
    if (_reportID == 0) {
        buffer = _buffer+1;
        bufferSize = _size-1;
    }
    else {
        buffer = _buffer;
        bufferSize = _size;
    }
    memset(buffer, 0, bufferSize);
    reportID = _reportID;
    if (_size > 0 && reportID != 0)
        buffer[0] = _reportID;
}

HIDReporter::HIDReporter(uint8_t* _buffer, unsigned _size) {
    buffer = _buffer;
    bufferSize = _size;
    memset(buffer, 0, _size);
    reportID = 0;
}

void HIDReporter::setFeature(uint8_t* in) {
    return usb_hid_set_feature(reportID, in);
}

uint16_t HIDReporter::getData(uint8_t type, uint8_t* out, uint8_t poll) {
    return usb_hid_get_data(type, reportID, out, poll);
}

uint16_t HIDReporter::getFeature(uint8_t* out, uint8_t poll) {
    return usb_hid_get_data(HID_REPORT_TYPE_FEATURE, reportID, out, poll);
}

uint16_t HIDReporter::getOutput(uint8_t* out, uint8_t poll) {
    return usb_hid_get_data(HID_REPORT_TYPE_OUTPUT, reportID, out, poll);
}

#if defined(SERIAL_USB)

enum reset_state_t {
    DTR_UNSET,
    DTR_HIGH,
    DTR_NEGEDGE,
    DTR_LOW
};

static reset_state_t reset_state = DTR_UNSET;

static void ifaceSetupHook(unsigned hook, void *requestvp) {
    (void)hook;
    uint8 request = *(uint8*)requestvp;

        // Ignore requests we're not interested in.
    if (request != USBHID_CDCACM_SET_CONTROL_LINE_STATE) {
        return;
    }

    // We need to see a negative edge on DTR before we start looking
    // for the in-band magic reset byte sequence.
    uint8 dtr = composite_cdcacm_get_dtr();
    switch (reset_state) {
    case DTR_UNSET:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    case DTR_HIGH:
        reset_state = dtr ? DTR_HIGH : DTR_NEGEDGE;
        break;
    case DTR_NEGEDGE:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    case DTR_LOW:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    }

	if ((composite_cdcacm_get_baud() == 1200) && (reset_state == DTR_NEGEDGE)) {
		iwdg_init(IWDG_PRE_4, 10);
		while (1);
	}
}

#define RESET_DELAY 100000
static void wait_reset(void) {
  delay_us(RESET_DELAY);
  nvic_sys_reset();
}

#define STACK_TOP 0x20000800
#define EXC_RETURN 0xFFFFFFF9
#define DEFAULT_CPSR 0x61000000
static void rxHook(unsigned hook, void *ignored) {
    (void)hook;
    (void)ignored;
    /* FIXME this is mad buggy; we need a new reset sequence. E.g. NAK
     * after each RX means you can't reset if any bytes are waiting. */
    if (reset_state == DTR_NEGEDGE) {
        reset_state = DTR_LOW;

        if (composite_cdcacm_data_available() >= 4) {
            // The magic reset sequence is "1EAF".
            static const uint8 magic[4] = {'1', 'E', 'A', 'F'};	

            uint8 chkBuf[4];

            // Peek at the waiting bytes, looking for reset sequence,
            // bailing on mismatch.
            composite_cdcacm_peek_ex(chkBuf, composite_cdcacm_data_available() - 4, 4);
            for (unsigned i = 0; i < sizeof(magic); i++) {
                if (chkBuf[i] != magic[i]) {
                    return;
                }
            }

            // Got the magic sequence -> reset, presumably into the bootloader.
            // Return address is wait_reset, but we must set the thumb bit.
            uintptr_t target = (uintptr_t)wait_reset | 0x1;
            asm volatile("mov r0, %[stack_top]      \n\t" // Reset stack
                         "mov sp, r0                \n\t"
                         "mov r0, #1                \n\t"
                         "mov r1, %[target_addr]    \n\t"
                         "mov r2, %[cpsr]           \n\t"
                         "push {r2}                 \n\t" // Fake xPSR
                         "push {r1}                 \n\t" // PC target addr
                         "push {r0}                 \n\t" // Fake LR
                         "push {r0}                 \n\t" // Fake R12
                         "push {r0}                 \n\t" // Fake R3
                         "push {r0}                 \n\t" // Fake R2
                         "push {r0}                 \n\t" // Fake R1
                         "push {r0}                 \n\t" // Fake R0
                         "mov lr, %[exc_return]     \n\t"
                         "bx lr"
                         :
                         : [stack_top] "r" (STACK_TOP),
                           [target_addr] "r" (target),
                           [exc_return] "r" (EXC_RETURN),
                           [cpsr] "r" (DEFAULT_CPSR)
                         : "r0", "r1", "r2");

            /* Can't happen. */
            ASSERT_FAULT(0);
        }
    }
}
#endif

USBHIDDevice USBHID;
