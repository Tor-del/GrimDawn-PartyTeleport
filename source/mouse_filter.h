#pragma once
#include "protocol.h"
#include <cstring>
namespace input {
// Exact-hash DirectInput.dll ABI: floats X/Y, int wheel, eight button bytes,
// four modifier bytes and four trailing bytes. Preserve position/modifiers.
struct MouseEvent {unsigned char bytes[28];};
inline void filter(MouseEvent& event,bool overOverlay,unsigned& consumed) {
    unsigned pressed=0;
    for(unsigned i=0;i<8;++i) if(event.bytes[12+i]) pressed|=1u<<i;
    consumed&=pressed;
    if(overOverlay) consumed|=pressed;
    for(unsigned i=0;i<8;++i) if(overOverlay||(consumed&(1u<<i))) event.bytes[12+i]=0;
    if(overOverlay) memset(event.bytes+8,0,4);
}
inline bool overOverlay(pt::Shared* shared) {
    if(!shared||!pt::enter(shared)) return false;
    DWORD pid=shared->uiPid;ULONGLONG windows[4],heartbeat=shared->heartbeat,lease=shared->clickUntil;
    memcpy(windows,shared->windows,sizeof(windows));pt::leave(shared);
    auto now=GetTickCount64();
    if(!pid||!pt::fresh(now,heartbeat,2000)) return false;
    // Short lease covers a buffered click whose release hides a settings window.
    if(lease>now&&lease-now<=200) return true;
    POINT point;if(!GetCursorPos(&point)) return false;
    HWND at=WindowFromPoint(point);if(!at||!IsWindowVisible(at)) return false;
    DWORD owner=0;GetWindowThreadProcessId(at,&owner);if(owner!=pid) return false;
    for(auto handle:windows) if(handle==reinterpret_cast<ULONGLONG>(at)) return true;
    return false;
}
using MouseRead=void*(*)(void*,void*,int);
inline MouseRead originalMouse=nullptr;
inline pt::Shared* state=nullptr;
inline thread_local unsigned consumedButtons=0;
inline void* readMouse(void* device,void* output,int index) {
    void* result=originalMouse(device,output,index);
    filter(*static_cast<MouseEvent*>(output),overOverlay(state),consumedButtons);
    return result;
}
}
