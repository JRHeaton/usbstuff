//
//  JR360Controller.h
//  usbstuff
//
//  Created by John Heaton on 2/9/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#ifndef JR360Controller_H
#define JR360Controller_H

#include <iostream>
#include <stdint.h>
#include <map>
#include <IOKit/usb/IOUSBLib.h>

typedef struct JR360AsyncReadCtx JR360AsyncReadCtx;

typedef enum {
    kJR360XboxButtonStateOff = 0,
    kJR360XboxButtonStateBlinkAllThenOff = 1,
    kJR360XboxButtonStateBlink1ThenOn = 2,
    kJR360XboxButtonStateBlink2ThenOn = 3,
    kJR360XboxButtonStateBlink3ThenOn = 4,
    kJR360XboxButtonStateBlink4ThenOn = 5,
    kJR360XboxButtonState1On = 6,
    kJR360XboxButtonState2On = 7,
    kJR360XboxButtonState3On = 8,
    kJR360XboxButtonState4On = 9,
} JR360XboxButtonState;

typedef enum {
    kJR360ButtonXbox,
    kJR360ButtonA,
    kJR360ButtonB,
    kJR360ButtonX,
    kJR360ButtonY,
    kJR360ButtonBack,
    kJR360ButtonStart,
    kJR360ButtonDPadLeft,
    kJR360ButtonDPadRight,
    kJR360ButtonDPadUp,
    kJR360ButtonDPadDown,
    kJR360ButtonBumperLeft,
    kJR360ButtonBumperRight,
    kJR360ButtonStickLeft,
    kJR360ButtonStickRight
} JR360Button;

class JR360Controller;
typedef void (^JR360ButtonToggleHandler)(JR360Controller *cont, JR360Button button, bool depressed);
typedef void (^JR360TriggerHandler)(JR360Controller *cont, bool left, uint8_t value);

class JR360Controller {
public:
    JR360Controller(uint16_t idVendor, uint16_t idProduct);
    
    bool valid();
    
    bool open(); // opens usb device, configures, opens interfaces
    bool reset();
    void close();
    
    bool vibrate(uint8_t left, uint8_t right); // specify 0 for both to turn off
    bool setXboxButtonState(JR360XboxButtonState state);
    bool setPlayer(int player=0, bool blink=true); // 1-4
    
    bool buttonIsDepressed(JR360Button button) { return !buttonToggleStates.count(button) ? 0 : buttonToggleStates[button]; }
    uint8_t triggerState(bool left) { return !triggerStates.count(left) ? 0 : triggerStates[left]; }
    
    // IOKit interfaces
    IOUSBDeviceInterface650 **deviceInterface;
    IOUSBInterfaceInterface650 **interfaces[4];
    
    uint8_t buttonInputPipe;
    uint8_t outputPipe;
    io_service_t deviceService;
    
    std::map<JR360Button, JR360ButtonToggleHandler> toggleHandlers;
    std::map<bool, JR360TriggerHandler> triggerHandlers;
    
    struct {
        unsigned funcs:1;
        unsigned input:1;
        unsigned info:1;
    } debug;
    
protected:
    mach_port_t async_port[4];
    static void inputCallback(JR360AsyncReadCtx *ctx, IOReturn result, uint16_t len);
    std::map<JR360Button, bool> buttonToggleStates;
    std::map<bool, uint8_t> triggerStates;
};

#endif /* JR360Controller_H */
