/*
  Copyright (c) 2012 Arduino.  All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * @brief USB HID port (HID USB).
 */

#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_

#include <boards.h>
#include "Stream.h"

#define USB_MAX_PRODUCT_LENGTH 32
#define USB_MAX_MANUFACTURER_LENGTH 32
#define USB_MAX_SERIAL_NUMBER_LENGTH  20


// You could use this for a serial number, but you'll be revealing the device ID to the host,
// and hence burning it for cryptographic purposes.
const char* getDeviceIDString();

#define USB_COMPOSITE_MAX_PARTS 6
#define USB_COMPOSITE_MAX_PLUGINS 6

class USBCompositeDevice;

class USBPlugin {
private:
    USBCompositeDevice* device;
public:
    virtual bool init() = 0;
    virtual bool stop() = 0;
    virtual bool registerParts() = 0;
    USBPlugin(USBCompositeDevice* _device) { device = _device; }
};

#define DEFAULT_SERIAL_STRING "00000000000000000001"

class USBCompositeDevice {
private:
	bool enabled = false;
    bool haveSerialNumber = false;
    uint8_t iManufacturer[USB_DESCRIPTOR_STRING_LEN(USB_MAX_MANUFACTURER_LENGTH)];
    uint8_t iProduct[USB_DESCRIPTOR_STRING_LEN(USB_MAX_PRODUCT_LENGTH)];
    uint8_t iSerialNumber[USB_DESCRIPTOR_STRING_LEN(USB_MAX_SERIAL_NUMBER_LENGTH)]; 
    uint16 vendorId;
    uint16 productId;
    USBCompositePart* parts[USB_COMPOSITE_MAX_PARTS];
    USBPlugin* plugins[USB_COMPOSITE_MAX_PLUGINS];
    uint32 numParts;
public:
	USBDevice(void) {
        vendorId = 0;
        productId = 0;
        setManufacturerString(NULL);
        setProductString(NULL);
        setSerialString(DEFAULT_SERIAL_STRING);
        iManufacturer[0] = 0;
        iProduct[0] = 0;
        iSerialNumber[0] = 0;
    }
    void setVendorId(uint16 vendor=0);
    void setProductId(uint16 product=0);
    void setManufacturerString(const char* manufacturer=NULL);
    void setProductString(const char* product=NULL);
    void setSerialString(const char* serialNumber=DEFAULT_SERIAL_STRING);
    bool begin(void);
    void end(void);
    void clear();
    bool add(USBPlugin* plugin);
    bool add(USBCompositeDevice* part);
};


class HIDReporter {
    private:
        uint8_t* buffer;
        unsigned bufferSize;
        uint8_t reportID;
        
    public:
        void sendReport(); 
        
    public:
        // if you use this init function, the buffer starts with a reportID, even if the reportID is zero,
        // and bufferSize includes the reportID; if reportID is zero, sendReport() will skip the initial
        // reportID byte
        HIDReporter(uint8_t* _buffer, unsigned _size, uint8_t _reportID);
        // if you use this init function, the buffer has no reportID function in it
        HIDReporter(uint8_t* _buffer, unsigned _size);
        uint16_t getFeature(uint8_t* out=NULL, uint8_t poll=1);
        uint16_t getOutput(uint8_t* out=NULL, uint8_t poll=1);
        uint16_t getData(uint8_t type, uint8_t* out, uint8_t poll=1); // type = HID_REPORT_TYPE_FEATURE or HID_REPORT_TYPE_OUTPUT
        void setFeature(uint8_t* feature);
};

//================================================================================
//================================================================================
//	Mouse

#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
#define MOUSE_ALL (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)

class HIDMouse : public HIDReporter {
protected:
    uint8_t _buttons;
	void buttons(uint8_t b);
    uint8_t reportBuffer[5];
public:
	HIDMouse(uint8_t reportID=HID_MOUSE_REPORT_ID) : HIDReporter(reportBuffer, sizeof(reportBuffer), reportID), _buttons(0) {}
	void begin(void);
	void end(void);
	void click(uint8_t b = MOUSE_LEFT);
	void move(signed char x, signed char y, signed char wheel = 0);
	void press(uint8_t b = MOUSE_LEFT);		// press LEFT by default
	void release(uint8_t b = MOUSE_LEFT);	// release LEFT by default
	bool isPressed(uint8_t b = MOUSE_ALL);	// check all buttons by default
};

typedef struct {
    uint8_t reportID;
    uint8_t buttons;
    int16_t x;
    int16_t y;
    uint8_t wheel;
} __packed AbsMouseReport_t;

class HIDAbsMouse : public HIDReporter {
protected:
	void buttons(uint8_t b);
    AbsMouseReport_t report;
public:
	HIDAbsMouse(uint8_t reportID=HID_MOUSE_REPORT_ID) : HIDReporter((uint8_t*)&report, sizeof(report), reportID) {
        report.buttons = 0;
        report.x = 0;
        report.y = 0;
        report.wheel = 0;
    }
	void begin(void);
	void end(void);
	void click(uint8_t b = MOUSE_LEFT);
	void move(int16_t x, int16_t y, int8_t wheel = 0);
	void press(uint8_t b = MOUSE_LEFT);		// press LEFT by default
	void release(uint8_t b = MOUSE_LEFT);	// release LEFT by default
	bool isPressed(uint8_t b = MOUSE_ALL);	// check all buttons by default
};

typedef struct {
    uint8_t reportID;
    uint16_t button;
} __packed ConsumerReport_t;

class HIDConsumer : public HIDReporter {
protected:
    ConsumerReport_t report;
public:
    enum { 
           BRIGHTNESS_UP = 0x6F, 
           BRIGHTNESS_DOWN = 0x70, 
           VOLUME_UP = 0xE9, 
           VOLUME_DOWN = 0xEA,
           MUTE = 0xE2, 
           PLAY_OR_PAUSE = 0xCD
           // see pages 75ff of http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
           };
	HIDConsumer(uint8_t reportID=HID_CONSUMER_REPORT_ID) : HIDReporter((uint8_t*)&report, sizeof(report), reportID) {
        report.button = 0;
    }
	void begin(void);
	void end(void);
    void press(uint16_t button);
    void release();
};

//================================================================================
//================================================================================
//	Keyboard

#define SHIFT 0x80
const uint8_t _asciimap[128] =
{
	0x00,             // NUL
	0x00,             // SOH
	0x00,             // STX
	0x00,             // ETX
	0x00,             // EOT
	0x00,             // ENQ
	0x00,             // ACK
	0x00,             // BEL
	0x2a,             // BS	Backspace
	0x2b,             // TAB	Tab
	0x28,             // LF	Enter
	0x00,             // VT
	0x00,             // FF
	0x00,             // CR
	0x00,             // SO
	0x00,             // SI
	0x00,             // DEL
	0x00,             // DC1
	0x00,             // DC2
	0x00,             // DC3
	0x00,             // DC4
	0x00,             // NAK
	0x00,             // SYN
	0x00,             // ETB
	0x00,             // CAN
	0x00,             // EM
	0x00,             // SUB
	0x00,             // ESC
	0x00,             // FS
	0x00,             // GS
	0x00,             // RS
	0x00,             // US

	0x2c,		   //  ' '
	0x1e|SHIFT,	   // !
	0x34|SHIFT,	   // "
	0x20|SHIFT,    // #
	0x21|SHIFT,    // $
	0x22|SHIFT,    // %
	0x24|SHIFT,    // &
	0x34,          // '
	0x26|SHIFT,    // (
	0x27|SHIFT,    // )
	0x25|SHIFT,    // *
	0x2e|SHIFT,    // +
	0x36,          // ,
	0x2d,          // -
	0x37,          // .
	0x38,          // /
	0x27,          // 0
	0x1e,          // 1
	0x1f,          // 2
	0x20,          // 3
	0x21,          // 4
	0x22,          // 5
	0x23,          // 6
	0x24,          // 7
	0x25,          // 8
	0x26,          // 9
	0x33|SHIFT,      // :
	0x33,          // ;
	0x36|SHIFT,      // <
	0x2e,          // =
	0x37|SHIFT,      // >
	0x38|SHIFT,      // ?
	0x1f|SHIFT,      // @
	0x04|SHIFT,      // A
	0x05|SHIFT,      // B
	0x06|SHIFT,      // C
	0x07|SHIFT,      // D
	0x08|SHIFT,      // E
	0x09|SHIFT,      // F
	0x0a|SHIFT,      // G
	0x0b|SHIFT,      // H
	0x0c|SHIFT,      // I
	0x0d|SHIFT,      // J
	0x0e|SHIFT,      // K
	0x0f|SHIFT,      // L
	0x10|SHIFT,      // M
	0x11|SHIFT,      // N
	0x12|SHIFT,      // O
	0x13|SHIFT,      // P
	0x14|SHIFT,      // Q
	0x15|SHIFT,      // R
	0x16|SHIFT,      // S
	0x17|SHIFT,      // T
	0x18|SHIFT,      // U
	0x19|SHIFT,      // V
	0x1a|SHIFT,      // W
	0x1b|SHIFT,      // X
	0x1c|SHIFT,      // Y
	0x1d|SHIFT,      // Z
	0x2f,          // [
	0x31,          // bslash
	0x30,          // ]
	0x23|SHIFT,    // ^
	0x2d|SHIFT,    // _
	0x35,          // `
	0x04,          // a
	0x05,          // b
	0x06,          // c
	0x07,          // d
	0x08,          // e
	0x09,          // f
	0x0a,          // g
	0x0b,          // h
	0x0c,          // i
	0x0d,          // j
	0x0e,          // k
	0x0f,          // l
	0x10,          // m
	0x11,          // n
	0x12,          // o
	0x13,          // p
	0x14,          // q
	0x15,          // r
	0x16,          // s
	0x17,          // t
	0x18,          // u
	0x19,          // v
	0x1a,          // w
	0x1b,          // x
	0x1c,          // y
	0x1d,          // z
	0x2f|SHIFT,    //
	0x31|SHIFT,    // |
	0x30|SHIFT,    // }
	0x35|SHIFT,    // ~
	0				// DEL
};

#define KEY_LEFT_CTRL		0x80
#define KEY_LEFT_SHIFT		0x81
#define KEY_LEFT_ALT		0x82
#define KEY_LEFT_GUI		0x83
#define KEY_RIGHT_CTRL		0x84
#define KEY_RIGHT_SHIFT		0x85
#define KEY_RIGHT_ALT		0x86
#define KEY_RIGHT_GUI		0x87

#define KEY_UP_ARROW		0xDA
#define KEY_DOWN_ARROW		0xD9
#define KEY_LEFT_ARROW		0xD8
#define KEY_RIGHT_ARROW		0xD7
#define KEY_BACKSPACE		0xB2
#define KEY_TAB				0xB3
#define KEY_RETURN			0xB0
#define KEY_ESC				0xB1
#define KEY_INSERT			0xD1
#define KEY_DELETE			0xD4
#define KEY_PAGE_UP			0xD3
#define KEY_PAGE_DOWN		0xD6
#define KEY_HOME			0xD2
#define KEY_END				0xD5
#define KEY_CAPS_LOCK		0xC1
#define KEY_F1				0xC2
#define KEY_F2				0xC3
#define KEY_F3				0xC4
#define KEY_F4				0xC5
#define KEY_F5				0xC6
#define KEY_F6				0xC7
#define KEY_F7				0xC8
#define KEY_F8				0xC9
#define KEY_F9				0xCA
#define KEY_F10				0xCB
#define KEY_F11				0xCC
#define KEY_F12				0xCD

typedef struct{
    uint8_t reportID;
	uint8_t modifiers;
	uint8_t reserved;
	uint8_t keys[6];
} __packed KeyReport_t;

class HIDKeyboard : public Print, public HIDReporter {
public:
	KeyReport_t keyReport;
    
protected:    
    uint8_t leds[HID_BUFFER_ALLOCATE_SIZE(1,1)];
    HIDBuffer_t ledData;
    uint8_t reportID;

public:
	HIDKeyboard(uint8_t _reportID=HID_KEYBOARD_REPORT_ID) : 
        HIDReporter((uint8*)&keyReport, sizeof(KeyReport_t), _reportID),
        ledData(leds, HID_BUFFER_SIZE(1,_reportID), _reportID, HID_BUFFER_MODE_NO_WAIT),
        reportID(_reportID)
        {}
	void begin(void);
	void end(void);
    inline uint8 getLEDs(void) {
        return leds[reportID != 0 ? 1 : 0];
    }
	virtual size_t write(uint8_t k);
	virtual size_t press(uint8_t k);
	virtual size_t release(uint8_t k);
	virtual void releaseAll(void);
};


//================================================================================
//================================================================================
//	Joystick

// only works for little-endian machines, but makes the code so much more
// readable
typedef struct {
    uint8_t reportID;
    uint32_t buttons;
    unsigned hat:4;
    unsigned x:10;
    unsigned y:10;
    unsigned rx:10;
    unsigned ry:10;
    unsigned sliderLeft:10;
    unsigned sliderRight:10;
} __packed JoystickReport_t;

static_assert(sizeof(JoystickReport_t)==13, "Wrong endianness/packing!");

class HIDJoystick : public HIDReporter {
protected:
	JoystickReport_t joyReport; 
    bool manualReport = false;
	void safeSendReport(void);
public:
	inline void send(void) {
        sendReport();
    }
    void setManualReportMode(bool manualReport); // in manual report mode, reports only sent when send() is called
    bool getManualReportMode();
	void begin(void);
	void end(void);
	void button(uint8_t button, bool val);
	void X(uint16_t val);
	void Y(uint16_t val);
	void position(uint16_t x, uint16_t y);
	void Xrotate(uint16_t val);
	void Yrotate(uint16_t val);
	void sliderLeft(uint16_t val);
	void sliderRight(uint16_t val);
	void slider(uint16_t val);
	void hat(int16_t dir);
	HIDJoystick(uint8_t reportID=HID_JOYSTICK_REPORT_ID) : HIDReporter((uint8_t*)&joyReport, sizeof(joyReport), reportID) {
        joyReport.buttons = 0;
        joyReport.hat = 15;
        joyReport.x = 512;
        joyReport.y = 512;
        joyReport.rx = 512;
        joyReport.ry = 512;
        joyReport.sliderLeft = 0;
        joyReport.sliderRight = 0;
    }
};

extern USBHIDDevice USBHID;

template<unsigned txSize,unsigned rxSize>class HIDRaw : public HIDReporter {
private:
    uint8_t txBuffer[txSize];
    uint8_t rxBuffer[HID_BUFFER_ALLOCATE_SIZE(rxSize,0)];
    HIDBuffer_t buf;
public:
	HIDRaw() : HIDReporter(txBuffer, sizeof(txBuffer)) {}
	void begin(void) {
        buf.buffer = rxBuffer;
        buf.bufferSize = HID_BUFFER_SIZE(rxSize,0);
        buf.reportID = 0;
        USBHID.addOutputBuffer(&buf);
    }
	void end(void);
	void send(const uint8_t* data, unsigned n=sizeof(txBuffer)) {
        memset(txBuffer, 0, sizeof(txBuffer));
        memcpy(txBuffer, data, n>sizeof(txBuffer)?sizeof(txBuffer):n);
        sendReport();
    }
};

class USBCompositeSerial : public Stream {
public:
    USBCompositeSerial (void);

    void begin(void);

	// Roger Clark. Added dummy function so that existing Arduino sketches which specify baud rate will compile.
	void begin(unsigned long);
	void begin(unsigned long, uint8_t);
    void end(void);

	operator bool() { return true; } // Roger Clark. This is needed because in cardinfo.ino it does if (!Serial) . It seems to be a work around for the Leonardo that we needed to implement just to be compliant with the API

    virtual int available(void);// Changed to virtual

    uint32 read(uint8 * buf, uint32 len);
   // uint8  read(void);

	// Roger Clark. added functions to support Arduino 1.0 API
    virtual int peek(void);
    virtual int read(void);
    int availableForWrite(void);
    virtual void flush(void);	
	
    size_t write(uint8);
    size_t write(const char *str);
    size_t write(const uint8*, uint32);

    uint8 getRTS();
    uint8 getDTR();
    uint8 isConnected();
    uint8 pending();
};

extern HIDMouse Mouse;
extern HIDKeyboard Keyboard;
extern HIDJoystick Joystick;
extern USBCompositeSerial CompositeSerial;
extern HIDKeyboard BootKeyboard;

extern const HIDReportDescriptor* hidReportMouse;
extern const HIDReportDescriptor* hidReportKeyboard;
extern const HIDReportDescriptor* hidReportJoystick;
extern const HIDReportDescriptor* hidReportKeyboardMouse;
extern const HIDReportDescriptor* hidReportKeyboardJoystick;
extern const HIDReportDescriptor* hidReportKeyboardMouseJoystick;
extern const HIDReportDescriptor* hidReportBootKeyboard;

#define HID_MOUSE                   hidReportMouse
#define HID_KEYBOARD                hidReportKeyboard
#define HID_JOYSTICK                hidReportJoystick
#define HID_KEYBOARD_MOUSE          hidReportKeyboardMouse
#define HID_KEYBOARD_JOYSTICK       hidReportKeyboardJoystick
#define HID_KEYBOARD_MOUSE_JOYSTICK hidReportKeyboardMouseJoystick
#define HID_BOOT_KEYBOARD           hidReportBootKeyboard

#endif
        