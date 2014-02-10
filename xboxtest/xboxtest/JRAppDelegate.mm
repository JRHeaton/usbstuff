//
//  JRAppDelegate.m
//  xboxtest
//
//  Created by John Heaton on 2/10/14.
//  Copyright (c) 2014 John Heaton. All rights reserved.
//

#import "JRAppDelegate.h"
#include <iostream>
#include <stdint.h>
#include <map>
#include "IOKit/usb/IOUSBLib.h"

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
    bool setPlayer(int player=0, bool blink=false); // 1-4
    
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

@implementation JRAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    // Override point for customization after application launch.
    self.window.backgroundColor = [UIColor whiteColor];
    [self.window makeKeyAndVisible];
    
    UILabel *label = [UILabel new];
    label.font = [UIFont fontWithName:@"Helvetica" size:28];
    
    UInt16 v = 0x0e6f;
    UInt16 pid = 0x0213;
    JR360Controller c(v, pid);
    label.text = [NSString stringWithFormat:@"open %d", c.open()];
    [label sizeToFit];
    
    [_window addSubview:label];
    label.center = CGPointMake(100, 100);
    
    c.setPlayer(4);
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end
