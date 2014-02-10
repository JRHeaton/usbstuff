//
//  JR360Controller.cpp
//  usbstuff
//
//  Created by John Heaton on 2/9/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#include "JR360Controller.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>

#define _ERR(v, exp) if((exp) != (v))
#define iff if(this->debug.funcs)
#define ifi if(ctx->controller->debug.input)
#define dbg_func(fmt, args...) iff printf("line %u:%s: " fmt, __LINE__, __PRETTY_FUNCTION__, args)
#define dbg_input(fmt, args...) ifi printf("input" fmt, args)
#define dbg_info(fmt, args...) if(this->debug.info) printf("line %u:%s: " fmt, __LINE__, __PRETTY_FUNCTION__, args)

struct JR360AsyncReadCtx {
    JR360Controller *controller;
    uint8_t interface, pipe, *buf;
    uint16_t size;
};

struct JR360Msg {
    uint16_t header;
} __attribute__((packed));

typedef unsigned flag;
struct JR360StateMsg {
    uint16_t header;
    flag dpad_up:1;
    flag dpad_down:1;
    flag dpad_left:1;
    flag dpad_right:1;
    
    flag start:1;
    flag back:1;
    flag left_click:1;
    flag right_click:1;
    // -----------------
    
    flag left_bumper:1;
    flag right_bumper:1;
    flag xbox_button:1;
    flag unk0:1;
    
    flag a_button:1;
    flag b_button:1;
    flag x_button:1;
    flag y_button:1;
    // -----------------
    
    flag left_trigger:8;
    flag right_trigger:8;
    
    flag left_stick_x:8;
    flag left_stick_unk0:8;
    flag left_stick_y:8;
    flag left_stick_unk1:8;
    
    flag right_stick_x:8;
    flag right_stick_unk0:8;
    flag right_stick_y:8;
    flag right_stick_unk1:8;
} __attribute__((packed));

struct JR360XboxBtnMsg {
    uint16_t header;
    flag code:8;
} __attribute__((packed));

#define JR_XBOXBTN_HEADER 0x0301
#define JR_STATE_HEADER 0x1400

void JR360Controller::inputCallback(JR360AsyncReadCtx *ctx, IOReturn result, uint16_t len) {
    dbg_input("[INTERFACE: %d; PIPE: %d; LENGTH: %d]: ", ctx->interface, ctx->pipe, len);
    
    struct JR360Msg *st = (struct JR360Msg *)ctx->buf;
    if(st->header == JR_STATE_HEADER) {
        struct JR360StateMsg *s = (struct JR360StateMsg *)st;
        dbg_input("----------------------------------------------------------------------\n"
                  "state: dpad_up=%u, dpad_down=%u, dpad_left=%u, dpad_right=%u, "
               "start=%u, select=%u, lclick=%u, rclick=%u\nlbumper=%u, rbumper=%u, "
               "xbox=%u, a_btn=%u, b_btn=%u, x_btn=%u, y_btn=%u, left_stick=(%d,%d), right_stick=(%d,%d)\n",
               s->dpad_up, s->dpad_down, s->dpad_left, s->dpad_right,
               s->start, s->back, s->left_click, s->right_click,
               s->left_bumper, s->right_bumper, s->xbox_button, s->a_button,
               s->b_button, s->x_button, s->y_button, s->left_stick_x, s->left_stick_y,
               s->right_stick_x, s->right_stick_y
               );
        
        
#define H(btn, member) if(ctx->controller->toggleHandlers.count(btn)) { \
    bool exists = ctx->controller->buttonToggleStates.count(btn); \
    bool changed = (!exists && s->member != 0) || ctx->controller->buttonToggleStates[btn] != s->member; \
    ctx->controller->buttonToggleStates[btn] = s->member; \
    if(changed) \
        ctx->controller->toggleHandlers[btn](ctx->controller, btn, s->member); \
}
        H(kJR360ButtonA, a_button);
        H(kJR360ButtonB, b_button);
        H(kJR360ButtonX, x_button);
        H(kJR360ButtonY, y_button);
        H(kJR360ButtonDPadDown, dpad_down);
        H(kJR360ButtonDPadLeft, dpad_left);
        H(kJR360ButtonDPadRight, dpad_right);
        H(kJR360ButtonDPadUp, dpad_up);
        H(kJR360ButtonStart, start);
        H(kJR360ButtonBack, back);
        H(kJR360ButtonStickLeft, left_click);
        H(kJR360ButtonStickRight, right_click);
        H(kJR360ButtonBumperLeft, left_bumper);
        H(kJR360ButtonBumperRight, right_bumper);
        H(kJR360ButtonXbox, xbox_button);
    
#undef H
#define H(btn, member) if(ctx->controller->triggerHandlers.count(btn)) { \
    bool changed = !ctx->controller->triggerStates.count(btn) || ctx->controller->triggerStates[btn] != s->member; \
    ctx->controller->triggerStates[btn] = s->member; \
    if(changed) \
        ctx->controller->triggerHandlers[btn](ctx->controller, btn, s->member); \
}
        
        H(true, left_bumper);
        H(false, right_bumper);
    }
    
    else ifi {
        for(int i=0;i<len;++i) {
            printf("%02x ", ctx->buf[i]);
        }
        puts("");
    }
    
    IOUSBInterfaceInterface650 **intf = ctx->controller->interfaces[ctx->interface];
    (*intf)->ReadPipeAsync(intf, ctx->pipe, ctx->buf, ctx->size, (IOAsyncCallback1)&JR360Controller::inputCallback, ctx);
}

JR360Controller::JR360Controller(uint16_t idVendor, uint16_t idProduct) {
    // Let's find all devices, limiting to the given properties matching
    CFMutableDictionaryRef matching = IOServiceMatching(kIOUSBDeviceClassName);
    CFNumberRef cfVendor, cfProduct;
    cfVendor = CFNumberCreate(NULL, kCFNumberSInt16Type, &idVendor);
    cfProduct = CFNumberCreate(NULL, kCFNumberSInt16Type, &idProduct);
    CFDictionarySetValue(matching, CFSTR(kUSBVendorID), cfVendor);
    CFDictionarySetValue(matching, CFSTR(kUSBProductID), cfProduct);
    deviceService = IOServiceGetMatchingService(kIOMasterPortDefault, matching);
    CFRelease(cfVendor);
    CFRelease(cfProduct);
}

bool JR360Controller::valid() {
    return !!deviceService;
}


// note: all the assumptions about indices for configurations,
// interfaces, etc are derived from my own RE/testing on only
// the controllers I OWN(2). I assume they'll work for yours
bool JR360Controller::open() {
    if(deviceService) {
        // service is valid, there is a device.
        // let's open it up and iterate through the interfaces
        
        // create plugin interface (user client to kernel)
        IOCFPlugInInterface **plugin;
        SInt32 score;
        _ERR(0, IOCreatePlugInInterfaceForService(deviceService, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugin, &score)) {
            printf("error getting plugin interface\n");
            return 0;
        }
    
        IOUSBDeviceInterface650 **dev;
        _ERR(0, (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID650), (LPVOID *)&dev)) {
            printf("error getting dev interface\n");
            return 0;
        }
        
        // close the plugin interface
        (*plugin)->Release(plugin);
        
        // open the device for rw
        (*dev)->USBDeviceOpen(dev);
        (*dev)->ResetDevice(dev);
        // set the first (only) configuration
        (*dev)->SetConfiguration(dev, 1);
        
        io_iterator_t it;
        IOUSBFindInterfaceRequest req;
        memset(&req, (uint8_t)kIOUSBFindInterfaceDontCare, sizeof(req));
        (*dev)->CreateInterfaceIterator(dev, &req, &it);
        
        int int_ind = 0;
        io_service_t intfService;
        while((intfService = IOIteratorNext(it)) && int_ind < 4/*extra security in case*/) {
            IOCreatePlugInInterfaceForService(intfService, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plugin, &score);
            
            // get the interface's... um... interface
            IOUSBInterfaceInterface650 **intf;
            (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID650), (LPVOID *)&intf);
            (*plugin)->Release(plugin);
            
            (*intf)->USBInterfaceOpen(intf);
            (*intf)->CreateInterfaceAsyncPort(intf, &async_port[int_ind]);
            
            CFRunLoopSourceRef src;
            (*intf)->CreateInterfaceAsyncEventSource(intf, &src);
            CFRunLoopAddSource(CFRunLoopGetCurrent(), src, kCFRunLoopCommonModes);
            
            // iterate through endpoints
            uint8_t numEndpoints;
            (*intf)->GetNumEndpoints(intf, &numEndpoints);
            
            for(int i=0;i<numEndpoints;++i) {
                // get info about this endpoint's pipe
                uint8_t dir, num, tran, interval;
                uint16_t mpktsize;
                (*intf)->GetPipeProperties(intf, i, &dir, &num, &tran, &mpktsize, &interval);
                
                dbg_info("dir: 0x%x, maxPacket: %u\n", dir, mpktsize);
                
                // if input
                if(dir & kUSBIn) {
                    dbg_func("found pipe: int %d, pipe %d input (%d)\n", int_ind, i, dir);
                    uint8_t *buf = (uint8_t *)malloc(mpktsize);
                    
                    JR360AsyncReadCtx *ctx = (JR360AsyncReadCtx *)calloc(1, sizeof(JR360AsyncReadCtx));
                    ctx->controller = this;
                    ctx->interface = int_ind;
                    ctx->pipe = i;
                    ctx->buf = buf;
                    ctx->size = mpktsize;
                    
                    IOReturn ret;
                    (*intf)->ReadPipeAsync(intf, i, buf, mpktsize, (IOAsyncCallback1)&JR360Controller::inputCallback, ctx);
                    
                    dbg_func("ReadPipeAsync: 0x%x\n", ret);
                }
            }
            
            // store it and move on
            interfaces[int_ind] = intf;
            int_ind++;
        }
        
        deviceInterface = dev;
        outputPipe = 2;
        buttonInputPipe = 1;
    }
    
    return 1;
}

bool JR360Controller::reset() {
    return !(*deviceInterface)->ResetDevice(deviceInterface);
}

void JR360Controller::close() {
    for(int i=0;i<4;++i) {
        (*interfaces[i])->USBInterfaceClose(*interfaces[i]);
    }
    (*deviceInterface)->USBDeviceClose(deviceInterface);
}

bool JR360Controller::vibrate(uint8_t left, uint8_t right) {
    uint8_t buf[8] = { 00, 8, 00, left, right, 00, 00, 00 };
    
    return !(*interfaces[0])->WritePipe(interfaces[0], outputPipe, buf, 8);
}

bool JR360Controller::setXboxButtonState(JR360XboxButtonState state) {
    struct JR360XboxBtnMsg msg;
    msg.header = JR_XBOXBTN_HEADER;
    msg.code = state;
    
    return !(*interfaces[0])->WritePipe(interfaces[0], outputPipe, &msg, sizeof(msg));
}

bool JR360Controller::setPlayer(int player, bool blink) {
    player = player > 4 ? 4 : player;
    player = player < 1 ? 1 : player;
    
    int base = !blink ? kJR360XboxButtonState1On : kJR360XboxButtonStateBlink1ThenOn;
    base += (player -1);
    
    return setXboxButtonState((JR360XboxButtonState)base);
}