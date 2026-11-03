#include "protocol.h"
#include "../vendor/minhook/include/MinHook.h"
#include <cstring>
namespace {
pt::Shared* shared;
using Object = void*(*)(void*);
using Bool = bool(*)(void*);
using Id = unsigned(*)(void*);
using Update = void(*)(void*,int);
struct Vector { unsigned char* begin; unsigned char* end; unsigned char* capacity; };
struct WorldVec { alignas(8) unsigned char bytes[24]; };
struct GameString { union { const wchar_t* data; wchar_t small[8]; }; size_t length,capacity; };
static_assert(sizeof(GameString)==32);
using Location = WorldVec*(*)(void*,WorldVec*,unsigned);
using Position = pt::Vec*(*)(const WorldVec*,pt::Vec*);
using Together = bool(*)(void*,unsigned,unsigned);
using Teleport = void(*)(void*,int,int,int,int,bool);
Object getPlayer,getManager,getParty,getPlayers,getCache,getInfo,getRegion;
Bool isMultiplayer,isAlive,isTeleporting,canTeleport;
Id getId;
Location getLocation;
Position getPosition;
Together together;
Teleport teleport;
Update original;
void** engine;
unsigned generation=1, lastIds[3]{}, lastSelfId=0;
void* lastPlayer;
ULONGLONG lastTick,lastTeleport;
LONG result;
bool vectorCount(const Vector* v,size_t stride,size_t maximum,size_t& n) {
    auto b=reinterpret_cast<uintptr_t>(v->begin), e=reinterpret_cast<uintptr_t>(v->end);
    if (e<b || (e-b)%stride || (e-b)/stride>maximum) return false;
    n=(e-b)/stride; return !n || b;
}
bool nameFor(void* manager,unsigned id,wchar_t* out) {
    // SHA256-gated PlayerNetBasicInfo: size 0x140, entity id +8, UTF16 string +0xA0.
    auto cache=static_cast<Vector*>(getCache(manager)); size_t count;
    if (!cache || !vectorCount(cache,0x140,64,count)) return false;
    for(size_t i=0;i<count;++i) {
        auto row=cache->begin+i*0x140;
        if (*reinterpret_cast<unsigned*>(row+8)!=id) continue;
        auto s=reinterpret_cast<const GameString*>(row+0xA0);
        if (!s->length || s->length>128 || s->capacity<s->length) return false;
        auto source=s->capacity>=8?s->data:s->small;
        if(!source) return false;
        size_t n=s->length<63?s->length:63;
        memcpy(out,source,n*sizeof(wchar_t)); out[n]=0; return true;
    }
    return false;
}
bool positionFor(void* manager,unsigned id,pt::Vec& pos) {
    WorldVec location{}; getLocation(manager,&location,id);
    // Unknown/unloaded region: do not attempt conversion of a null region.
    if(!*reinterpret_cast<void**>(&location)) return false;
    getPosition(&location,&pos); return pt::valid(pos);
}
float distanceFor(void* manager,unsigned selfId,unsigned otherId) {
    WorldVec a{},b{};
    getLocation(manager,&a,selfId);getLocation(manager,&b,otherId);
    auto regionA=*reinterpret_cast<void**>(&a),regionB=*reinterpret_cast<void**>(&b);
    if(!regionA || regionA!=regionB) return -1;
    pt::Vec from{},to{};getPosition(&a,&from);getPosition(&b,&to);
    return pt::distanceBetween(from,to);
}
void tick(void* game) {
    if(shared->status!=1) return;
    ULONGLONG now=GetTickCount64(); if(now-lastTick<100) return; lastTick=now;
    __try {
        pt::Snapshot view{}; view.time=now;
        void* player=getPlayer(game);
        void* info=engine && *engine?getInfo(*engine):nullptr;
        bool active=player && info && isMultiplayer(info) && getRegion(player);
        void* manager=active?getManager(game):nullptr;
        void* party=active?getParty(game):nullptr;
        unsigned selfId=active?getId(game):0;
        bool allowed=active && isAlive(player) && !isTeleporting(player) && canTeleport(game) && now-lastTeleport>=3000;
        unsigned ids[3]{};
        if(manager && party) {
            auto players=static_cast<Vector*>(getPlayers(manager)); size_t count;
            if(players && vectorCount(players,sizeof(unsigned),64,count)) {
                for(size_t i=0;i<count && view.count<3;++i) {
                    unsigned id=reinterpret_cast<unsigned*>(players->begin)[i];
                    if(!id || id==selfId || !together(party,selfId,id)) continue;
                    auto& row=view.rows[view.count];
                    if(!nameFor(manager,id,row.name)) continue;
                    row.id=id; ids[view.count]=id; pt::Vec pos{};
                    row.enabled=allowed && positionFor(manager,id,pos);
                    row.distance=distanceFor(manager,selfId,id);
                    ++view.count;
                }
            }
        }
        if(player!=lastPlayer || selfId!=lastSelfId || memcmp(ids,lastIds,sizeof(ids))) {
            ++generation; lastPlayer=player; lastSelfId=selfId; memcpy(lastIds,ids,sizeof(ids));
        }
        view.generation=generation;
        unsigned target=0,requestedGeneration=0; ULONGLONG requestedAt=0,heartbeat=0;
        if(pt::enter(shared)) {
            target=shared->requestId; requestedGeneration=shared->requestGeneration;
            requestedAt=shared->requestTime; heartbeat=shared->heartbeat; shared->requestId=0;
            pt::leave(shared);
        }
        if(target) {
            result=2;
            if(requestedGeneration==generation && pt::fresh(now,requestedAt,1000) && pt::fresh(now,heartbeat,2000)) {
                for(LONG i=0;i<view.count;++i) if(view.rows[i].id==target && view.rows[i].enabled) {
                    pt::Vec pos{};
                    if(positionFor(manager,target,pos) && together(party,selfId,target) && canTeleport(game)) {
                        // Native teleport activity handles loading and network dispatch.
                        teleport(game,static_cast<int>(pos.x),static_cast<int>(pos.y),static_cast<int>(pos.z),1,true);
                        lastTeleport=now; result=1;
                        for(LONG j=0;j<view.count;++j) view.rows[j].enabled=0;
                    }
                }
            }
        }
        view.result=result;
        if(pt::enter(shared)) { shared->view=view; pt::leave(shared); }
    } __except(EXCEPTION_EXECUTE_HANDLER) { InterlockedExchange(&shared->status,4); }
}
void update(void* game,int elapsed) { original(game,elapsed); tick(game); }
template<class T> bool bind(HMODULE module,const char* symbol,T& out) {
    out=reinterpret_cast<T>(GetProcAddress(module,symbol)); return out!=nullptr;
}
DWORD WINAPI init(void*) {
    HANDLE map=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(pt::Shared),pt::mappingName(GetCurrentProcessId()).c_str());
    shared=map?static_cast<pt::Shared*>(MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(pt::Shared))):nullptr;
    if(!shared) return 1;
    *shared={}; shared->signature=pt::magic;
    auto g=GetModuleHandleW(L"Game.dll"), e=GetModuleHandleW(L"Engine.dll");
    wchar_t path[32768]; if(!g||!e) { shared->status=2; return 2; }
    GetModuleFileNameW(g,path,32768); std::wstring folder=path; folder.resize(folder.find_last_of(L"\\/"));
    if(!supported(folder)) { shared->status=2; return 2; }
    bool ok=true;
#define B(M,V,S) ok=bind(M,S,V)&&ok
    B(g,getPlayer,"?GetMainPlayer@GameEngine@GAME@@QEBAPEAVPlayer@2@XZ");
    B(g,getManager,"?GetPlayerManagerClient@GameEngine@GAME@@QEBAPEAVPlayerManagerClient@2@XZ");
    B(g,getParty,"?GetPartyManager@GameEngine@GAME@@QEAAPEAVPartyManager@2@XZ");
    B(g,getId,"?GetPlayerId@GameEngine@GAME@@QEBAIXZ");
    B(g,getPlayers,"?GetAllPlayersInGame@PlayerManagerClient@GAME@@QEBAAEBV?$vector@I@mem@@XZ");
    B(g,getCache,"?GetPlayerInfoCache@PlayerManagerClient@GAME@@QEBAAEBV?$vector@UPlayerNetBasicInfo@GAME@@@mem@@XZ");
    B(g,together,"?AreInPartyTogether@PartyManager@GAME@@QEBA_NII@Z");
    B(g,getLocation,"?GetPlayerLocation@PlayerManagerClient@GAME@@QEBA?AVWorldVec3@2@I@Z");
    B(e,getPosition,"?GetWorldPosition@WorldVec3@GAME@@QEBA?AVVec3@2@XZ");
    B(g,canTeleport,"?MainPlayerCanUsePersonalTeleport@GameEngine@GAME@@QEBA_NXZ");
    B(g,isAlive,"?IsAlive@Player@GAME@@UEBA_NXZ");
    B(g,isTeleporting,"?IsTeleporting@Character@GAME@@QEBA_NXZ");
    B(g,teleport,"?InitiatePlayerTeleport@GameEngine@GAME@@QEAAXHHHW4TeleportEffect@2@_N@Z");
    B(e,engine,"?gEngine@GAME@@3PEAVEngine@1@EA");
    B(e,getInfo,"?GetGameInfo@Engine@GAME@@QEAAPEAVGameInfo@2@XZ");
    B(e,isMultiplayer,"?GetIsMultiPlayer@GameInfo@GAME@@QEBA_NXZ");
    B(e,getRegion,"?GetRegion@Entity@GAME@@QEBAPEAVRegion@2@XZ");
#undef B
    auto target=reinterpret_cast<void*>(GetProcAddress(g,"?Update@GameEngine@GAME@@QEAAXH@Z"));
    if(!ok||!target||MH_Initialize()!=MH_OK||MH_CreateHook(target,update,reinterpret_cast<void**>(&original))!=MH_OK) {shared->status=3;return 3;}
    shared->status=1;
    if(MH_EnableHook(target)!=MH_OK) {shared->status=3;return 3;}
    return 0;
}
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,void*) {
    if(reason==DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(h); HANDLE t=CreateThread(nullptr,0,init,nullptr,0,nullptr); if(t) CloseHandle(t); }
    return TRUE;
}
