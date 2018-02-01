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

bool USBHIDDevice::registerParts() {
	return device->add(usbHIDPart);
}

void USBHIDDevice::setReportDescriptor(const uint8_t* report_descriptor, uint16_t report_descriptor_length) {
	usb_hid_set_report_descriptor(report_descriptor, report_descriptor_length);
}

void USBHIDDevice::setReportDescriptor(const HIDReportDescriptor* report) {
    setReportDescriptor(report->descriptor, report->length);
}

void USBHIDDevice::begin(const uint8_t* report_descriptor, uint16_t report_descriptor_length, uint16_t idVendor, uint16_t idProduct,
        const char* manufacturer, const char* product, const char* serialNumber) {
            
	if (enabled)
		return;
	
	setReportDescriptor(report_descriptor, report_descriptor_length);
	
	device->clear();
	device->setVendorId(idVendor);
	device->setProductId(idProduct);
	device->setManufacturerString(manufacturer);
	device->setProductString(product);
	device->setSerialString(serialNumber);
	device->add(usbHIDPart);

	if (serialSupport)
		device->add(CompositeSerial);
            
	device->begin();

	enabled = true;
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
		device->end();
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


USBHIDDevice USBHID;
