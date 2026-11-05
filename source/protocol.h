#pragma once
#include "compatibility.h"
#include <cmath>
namespace pt {
constexpr LONG magic = 0x50543033;
struct Row { unsigned id; wchar_t name[64]; LONG enabled; float distance=-1; LONG otherArea; wchar_t area[96]; };
struct Snapshot { ULONGLONG time; unsigned generation; LONG count; Row rows[3]; LONG result; };
struct Shared {
    LONG signature;
    volatile LONG status, lock;
    Snapshot view;
    unsigned requestId, requestGeneration;
    ULONGLONG requestTime, heartbeat;
    DWORD uiPid;
    ULONGLONG windows[4],clickUntil;
};
inline std::wstring mappingName(DWORD pid) { return L"Local\\GrimDawn.PartyTeleport.v3." + std::to_wstring(pid); }
inline bool enter(Shared* s) { return InterlockedCompareExchange(&s->lock,1,0)==0; }
inline void leave(Shared* s) { InterlockedExchange(&s->lock,0); }
struct Vec { float x,y,z; };
inline bool valid(Vec p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) &&
        std::abs(p.x)<1000000 && std::abs(p.y)<1000000 && std::abs(p.z)<1000000 &&
        (std::abs(p.x)+std::abs(p.y)+std::abs(p.z)>0.01f);
}
inline bool fresh(ULONGLONG now, ULONGLONG then, ULONGLONG limit) { return then && now>=then && now-then<limit; }
inline float distanceBetween(Vec a, Vec b) {
    if(!valid(a) || !valid(b)) return -1;
    return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);
}
}
