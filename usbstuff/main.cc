#include "JR360Controller.h"
#include "PushClient.h"
#include <vector>

int BROWSE_COLOR =84;
int SELECT_COLOR =23;
int DELETE_HL_COLOR =41;

int main() {
    UInt16 v = 0x0e6f;
    UInt16 pid = 0x0213;
    __block JR360Controller c(v, pid);
    __block PushClient p;
    
    __block int x = 0, y = 0;
    p.clearAll();
    p.gridPadOn(x, y, BROWSE_COLOR);
    p.buttonOn("1/4");
    p.buttonOn("1/4t");
    p.buttonOn("1/8").buttonOn("1/8t").buttonOn("1/16");
    p.buttonHandler = ^(std::string name,
                        UInt8 CID,
                        bool on) {
        if(on) {
            if(name == "1/4")
                c.setPlayer(1);
            else if (name == "1/4t")
                c.setPlayer(2);
            else if (name == "1/8")
                c.setPlayer(3);
            else if (name == "1/8t")
                c.setPlayer(4);
            else if (name == "1/16")
                c.setXboxButtonState(kJR360XboxButtonStateOff);
        } else
            p.buttonOn(CID);
    };

    struct coord {
        int x; int y;
    };
    __block std::vector<struct coord> coords;
    
    c.toggleHandlers[kJR360ButtonBumperLeft] = ^(JR360Controller *cont, JR360Button button, bool depressed) {
        if(depressed) {
            BROWSE_COLOR = arc4random() % 127;
            SELECT_COLOR = arc4random() % 127;
            DELETE_HL_COLOR = arc4random() % 127;
            for(auto it=coords.begin();it!=coords.end();it++) {
                p.gridPadOn((*it).x, (*it).y, SELECT_COLOR);
            }
            
            p.gridPadOn(x, y, BROWSE_COLOR);
        }
    };
    c.toggleHandlers[kJR360ButtonDPadLeft] =
    c.toggleHandlers[kJR360ButtonDPadRight] =
    c.toggleHandlers[kJR360ButtonDPadUp] =
    c.toggleHandlers[kJR360ButtonDPadDown] =
    ^(JR360Controller *cont, JR360Button button, bool depressed) {
        if(!depressed)
            return;
        
        int found=0;
        for(int i=0;i<coords.size();++i) {
            struct coord cc = coords[i];
            
            if(cc.x == x && cc.y == y) {
                found = 1;
                break;
            }
        }
        if(!found)
            p.gridPadOff(x, y);
        else
            p.gridPadOn(x, y, SELECT_COLOR);
        
        switch (button) {
            case kJR360ButtonDPadLeft: x--; if(x < 0) x = 0; break;
            case kJR360ButtonDPadRight: x++; if(x > 7) x = 7; break;
            case kJR360ButtonDPadUp: y--; if(y < 0) y = 0; break;
            case kJR360ButtonDPadDown: y++; if(y > 7) y = 7; break;
            default: break;
        }
        
        int ff=0;
        for(auto xx = coords.begin(); xx != coords.end(); ++xx) {
            struct coord _c = *xx;
            
            if(_c.x == x && _c.y == y) {
                ff = 1;
                break;
            }
        }
        p.gridPadOn(x, y, ff ? SELECT_COLOR-1 : BROWSE_COLOR);
    };
    c.toggleHandlers[kJR360ButtonXbox] = ^(JR360Controller *cont, JR360Button button, bool depressed) {
        if(depressed) {
            coords.clear();
            p.clearGrid();
            p.gridPadOn(x, y, BROWSE_COLOR);
        }
    };
    c.toggleHandlers[kJR360ButtonA] = ^(JR360Controller *cont, JR360Button button, bool depressed) {
        if(depressed) {
            int ff=0;
            for(auto xx = coords.begin(); xx != coords.end(); ++xx) {
                struct coord _c = *xx;
                
                if(_c.x == x && _c.y == y) {
                    ff = 1;
                    break;
                }
            }
            if(!ff)
                coords.push_back((struct coord){ x, y });
        }
        p.gridPadOn(x, y, depressed ? SELECT_COLOR : BROWSE_COLOR);

    };
    c.toggleHandlers[kJR360ButtonBack] = ^(JR360Controller *cont, JR360Button button, bool depressed) {
        if(depressed) {
            coords.clear();
            for(int i=0;i<8;++i) {
                for(int x=0;x<8;++x) {
                    coords.push_back((struct coord){ x, i });
                }
            }
            
            p.allGridPads(SELECT_COLOR);
            p.gridPadOn(x, y, SELECT_COLOR-1);
        }
    };
    c.toggleHandlers[kJR360ButtonX] = ^(JR360Controller *cont, JR360Button button, bool depressed) {
        if(depressed) {
            for(auto xx = coords.begin(); xx != coords.end(); ++xx) {
                struct coord _c = *xx;
                
                if(_c.x == x && _c.y == y) {
                    coords.erase(xx, xx+1);
                    break;
                }
            }
        }
        
        p.gridPadOn(x, y, depressed ? DELETE_HL_COLOR : BROWSE_COLOR);
    };
    p.controlChangeHandler = ^(UInt8 cid, UInt8 val) {
        if(cid == 75 || cid == 76) {
            static UInt8 vibl, vibr = 0;
            static int lpl, lpr;
            
#define DIFF 5
            uint8_t &vibration = cid == 75 ? vibl : vibr;
            
            if(val > 100)
                vibration = (vibration-DIFF) < 0 ? 0 : vibration-DIFF;
            else
                vibration = (vibration+DIFF) > 0xff ? 0xff : vibration+DIFF;
            
#define _PER(vib) ((vib * 100) / 255)
            int _lpl = _PER(vibl);
            int _lpr = _PER(vibr);
            char buf[128];
            sprintf(buf, "                                  L:%d%% R:%d%%", _lpl, _lpr);
            p.writeLCD("How To Play:                      Vibration:");
            if(_lpl != lpl || _lpr != lpr) {
                p.writeLCD(buf, 1);
                lpl = _lpl;
                lpr = _lpr;
            }
            
            c.vibrate(vibl, vibr);
        }
    };
    
    c.open();
    c.setPlayer(1);
    
    p.writeLCD("How To Play:                      Vibration:       Functions:");
    p.writeLCD("                                  L:0% R:0%", 1);
    p.writeLCD("Move around & tap                                  Clr  Fll  Clr Del", 2);
    p.writeLCD("A to lock in                                       XBOX back LB  X", 3);
    
    CFRunLoopRun();
}