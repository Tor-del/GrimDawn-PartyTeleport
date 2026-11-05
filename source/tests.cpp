#include "bridge.cpp"
#include <cassert>
#include <cstdio>
#include "i18n.h"
namespace {
int calls=0;bool partyOk=true,can=true,multi=true,otherRegion=false;pt::Vec destination{123.9f,4.3f,-82.4f};
unsigned mockIds[]={1,2};
unsigned char mockCache[0x280]{};
Vector players{reinterpret_cast<unsigned char*>(mockIds),reinterpret_cast<unsigned char*>(mockIds+2),reinterpret_cast<unsigned char*>(mockIds+2)};
Vector cache{mockCache,mockCache+sizeof(mockCache),mockCache+sizeof(mockCache)};
void* object(void*) {return reinterpret_cast<void*>(1);}
void* all(void*) {return &players;}
void* cached(void*) {return &cache;}
void* localizer() {return reinterpret_cast<void*>(1);}
const wchar_t* localized(void*,const char* tag) {return strcmp(tag,"tagAreaTwo")==0?L"Devil's Crossing":L"Burrwitch Village";}
void setArea(size_t offset,const char* text) {
    auto area=reinterpret_cast<NarrowString*>(mockCache+offset+0x40);*area={};
    area->capacity=15;area->length=strlen(text);strcpy_s(area->small,text);
}
input::MouseEvent rawMouse;
void* mouseRead(void*,void* out,int) {memcpy(out,&rawMouse,sizeof(rawMouse));return out;}
unsigned self(void*) {return 1;}
bool yes(void*) {return true;}
bool no(void*) {return false;}
bool allowed(void*) {return can;}
bool multiplayer(void*) {return multi;}
bool sameParty(void*,unsigned a,unsigned b) {return partyOk && a==1 && b==2;}
WorldVec* location(void*,WorldVec* out,unsigned id) {
    *reinterpret_cast<void**>(out)=reinterpret_cast<void*>(uintptr_t(id==2&&otherRegion?2:1));
    *reinterpret_cast<unsigned*>(out->bytes+8)=id;return out;
}
pt::Vec* position(const WorldVec* in,pt::Vec* out) {*out=*reinterpret_cast<const unsigned*>(in->bytes+8)==1?pt::Vec{1,1,1}:destination;return out;}
void tele(void*,int x,int y,int z,int effect,bool play) {assert(x==123&&y==4&&z==-82&&effect==1&&play);++calls;}
void run() {lastTick=0;tick(reinterpret_cast<void*>(1));}
void request(pt::Shared& s,unsigned target=2) {
    s.requestId=target;s.requestGeneration=generation;s.requestTime=GetTickCount64();s.heartbeat=GetTickCount64();
}
}
int main() {
    pt::Shared s{};s.status=1;shared=&s;void* mockEngine=reinterpret_cast<void*>(1);engine=&mockEngine;
    getPlayer=object;getManager=object;getParty=object;getInfo=object;getRegion=object;
    getPlayers=all;getCache=cached;getId=self;isMultiplayer=multiplayer;isAlive=yes;isTeleporting=no;canTeleport=allowed;
    together=sameParty;getLocation=location;getPosition=position;teleport=tele;
    getLocalizer=localizer;localize=localized;
    *reinterpret_cast<unsigned*>(mockCache+8)=2;
    auto name=reinterpret_cast<GameString*>(mockCache+0xA0);name->capacity=7;name->length=4;wcscpy_s(name->small,L"Ally");
    *reinterpret_cast<unsigned*>(mockCache+0x140+8)=1;
    setArea(0,"tagAreaOne");setArea(0x140,"tagAreaOne");
    run();assert(s.view.count==1&&s.view.rows[0].id==2&&s.view.rows[0].enabled);
    assert(s.view.rows[0].distance>100);
    otherRegion=true;run();assert(s.view.rows[0].distance==-1);otherRegion=false;
    setArea(0,"tagAreaTwo");run();assert(s.view.rows[0].otherArea&&s.view.rows[0].distance==-1);
    assert(!wcscmp(s.view.rows[0].area,L"Devil's Crossing"));
    setArea(0,"tagAreaOne");run();assert(!s.view.rows[0].otherArea&&s.view.rows[0].distance>100);
    auto area=reinterpret_cast<NarrowString*>(mockCache+0x40);area->length=9999;
    char invalidArea[128];assert(!areaTagFor(reinterpret_cast<void*>(1),2,invalidArea)&&!invalidArea[0]);setArea(0,"tagAreaOne");
    // Real mouse-read wrapper: overlay press is neutralized, including wheel;
    // a gesture that began on the overlay stays consumed outside until release.
    s.uiPid=GetCurrentProcessId();s.heartbeat=GetTickCount64();s.clickUntil=s.heartbeat+150;
    input::state=&s;input::originalMouse=mouseRead;
    rawMouse={};rawMouse.bytes[0]=31;rawMouse.bytes[8]=120;rawMouse.bytes[12]=1;rawMouse.bytes[21]=1;
    input::MouseEvent mouse{};input::readMouse(nullptr,&mouse,0);
    assert(!mouse.bytes[12]&&!mouse.bytes[8]&&mouse.bytes[0]==31&&mouse.bytes[21]==1);
    s.uiPid=0;rawMouse.bytes[8]=0;rawMouse.bytes[13]=1;
    input::readMouse(nullptr,&mouse,0);assert(!mouse.bytes[12]&&mouse.bytes[13]);
    rawMouse.bytes[12]=0;input::readMouse(nullptr,&mouse,0);assert(input::consumedButtons==0);
    rawMouse.bytes[12]=1;rawMouse.bytes[8]=120;input::readMouse(nullptr,&mouse,0);assert(mouse.bytes[12]&&mouse.bytes[8]);
    s.uiPid=GetCurrentProcessId();s.heartbeat=GetTickCount64()-3000;s.clickUntil=GetTickCount64()+150;
    input::readMouse(nullptr,&mouse,0);assert(mouse.bytes[12]); // dead UI fails open
    assert(std::abs(pt::distanceBetween({1,1,1},{4,5,13})-13)<0.0001f);
    assert(pt::distanceBetween({NAN,1,1},{1,1,1})==-1);
    assert(!ui::russian && !wcscmp(ui::tr(L"English",L"Русский"),L"English"));
    ui::russian=true;assert(!wcscmp(ui::tr(L"English",L"Русский"),L"Русский"));
    const wchar_t* missing=L"PartyTeleport.dll is missing beside the launcher.";
    assert(wcscmp(ui::error(missing),missing));ui::russian=false;assert(!wcscmp(ui::error(missing),missing));
    request(s);run();assert(calls==1&&s.view.result==1);
    request(s);run();assert(calls==1); // cooldown
    lastTeleport=0;request(s,99);run();assert(calls==1); // non-member
    request(s);s.requestGeneration--;run();assert(calls==1);
    request(s);s.requestTime-=2000;run();assert(calls==1);
    request(s);s.heartbeat-=3000;run();assert(calls==1);
    request(s);can=false;run();assert(calls==1&&!s.view.rows[0].enabled);can=true;
    request(s);partyOk=false;run();assert(calls==1&&s.view.count==0);partyOk=true;run();
    request(s);destination.x=NAN;run();assert(calls==1&&!s.view.rows[0].enabled);destination.x=123.9f;
    request(s);multi=false;run();assert(calls==1&&s.view.count==0);multi=true;run();
    request(s);run();assert(calls==2);
    assert(!pt::valid({INFINITY,0,0})&&!pt::valid({0,0,0})&&!pt::fresh(1,2,100));
    puts("PASS: bridge/teleport guards, distances, localized cross-area labels, malformed area tags, DirectInput mouse filtering/drag/release/wheel, dead-UI fallback and translations.");
}
