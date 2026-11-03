#include "protocol.h"
#include <cstdio>
#include <algorithm>
#include "button.h"
int inject(int,wchar_t**,DWORD&);
namespace {
pt::Shared* shared;
pt::Snapshot view{};
HWND buttons[3]{},gameWindow;
DWORD gamePid;
HANDLE process;
std::wstring ini;
int anchorX=230,anchorY=150,gap=72,width=250,height=36;
bool edit=false,hidden=false,preview=false,drag=false;
bool hovered[3]{};
POINT dragStart,anchorStart;
ULONGLONG pending;
unsigned clickId[3]{},clickGeneration[3]{};
void save() {
    WritePrivateProfileStringW(L"Buttons",L"X",std::to_wstring(anchorX).c_str(),ini.c_str());
    WritePrivateProfileStringW(L"Buttons",L"Y",std::to_wstring(anchorY).c_str(),ini.c_str());
    WritePrivateProfileStringW(L"Buttons",L"Gap",std::to_wstring(gap).c_str(),ini.c_str());
}
BOOL CALLBACK findWindow(HWND hwnd,LPARAM) {
    DWORD pid=0; GetWindowThreadProcessId(hwnd,&pid);
    if(pid==gamePid && IsWindowVisible(hwnd) && !GetWindow(hwnd,GW_OWNER)) {gameWindow=hwnd;return FALSE;}
    return TRUE;
}
bool foreground() {
    HWND fg=GetForegroundWindow(); if(fg==gameWindow) return true;
    for(auto h:buttons) if(fg==h) return true;
    return false;
}
bool snapshot(pt::Snapshot& value) {
    if(!shared||!pt::enter(shared)) return false;
    value=shared->view; shared->heartbeat=GetTickCount64(); pt::leave(shared); return true;
}
void layout() {
    if(!preview && WaitForSingleObject(process,0)==WAIT_OBJECT_0) {PostQuitMessage(0);return;}
    if(!preview && !IsWindow(gameWindow)) {gameWindow=nullptr;EnumWindows(findWindow,0);}
    pt::Snapshot next{}; if(snapshot(next)) view=next;
    auto now=GetTickCount64();
    bool visible=preview || (gameWindow && foreground() && !IsIconic(gameWindow) && shared && shared->status==1 && pt::fresh(now,view.time,1000));
    POINT origin{}; RECT area{};
    if(gameWindow) {ClientToScreen(gameWindow,&origin);GetClientRect(gameWindow,&area);}
    int x=anchorX,y=anchorY;
    if(gameWindow) {x=std::clamp(x,0,std::max(0,int(area.right)-width));y=std::clamp(y,0,std::max(0,int(area.bottom)-height-2*gap));}
    for(int i=0;i<3;++i) {
        bool show=visible && !hidden && (edit || i<view.count);
        SetWindowPos(buttons[i],HWND_TOPMOST,origin.x+x,origin.y+y+i*gap,width,height,SWP_NOACTIVATE|(show?SWP_SHOWWINDOW:SWP_HIDEWINDOW));
        InvalidateRect(buttons[i],nullptr,FALSE);
    }
}
LRESULT CALLBACK proc(HWND h,UINT msg,WPARAM w,LPARAM l) {
    int i=static_cast<int>(GetWindowLongPtrW(h,GWLP_USERDATA));
    switch(msg) {
    case WM_MOUSEACTIVATE:return MA_NOACTIVATE;
    case WM_ERASEBKGND:return 1;
    case WM_LBUTTONDOWN:
        if(edit) {drag=true;GetCursorPos(&dragStart);anchorStart={anchorX,anchorY};SetCapture(h);}
        else if(i<view.count && view.rows[i].enabled && pt::fresh(GetTickCount64(),view.time,1000)) {
            clickId[i]=view.rows[i].id;clickGeneration[i]=view.generation;SetCapture(h);
        }
        return 0;
    case WM_MOUSEMOVE:
        if(!hovered[i]) {hovered[i]=true;TRACKMOUSEEVENT tracking{sizeof(TRACKMOUSEEVENT),TME_LEAVE,h,0};TrackMouseEvent(&tracking);InvalidateRect(h,nullptr,FALSE);}
        if(edit && drag) {POINT p;GetCursorPos(&p);anchorX=anchorStart.x+p.x-dragStart.x;anchorY=anchorStart.y+p.y-dragStart.y;layout();}
        return 0;
    case WM_MOUSELEAVE:hovered[i]=false;InvalidateRect(h,nullptr,FALSE);return 0;
    case WM_LBUTTONUP: {
        unsigned id=clickId[i],generation=clickGeneration[i];clickId[i]=0;
        ReleaseCapture();
        if(edit) {drag=false;save();return 0;}
        POINT p{static_cast<short>(LOWORD(l)),static_cast<short>(HIWORD(l))};RECT r;GetClientRect(h,&r);
        auto now=GetTickCount64();
        if(!preview && PtInRect(&r,p) && id && generation==view.generation && i<view.count && id==view.rows[i].id && view.rows[i].enabled &&
           foreground() && pt::fresh(now,view.time,1000) && now-pending>1000 && shared && shared->status==1 && pt::enter(shared)) {
            shared->requestId=id;shared->requestGeneration=generation;shared->requestTime=now;shared->heartbeat=now;
            pt::leave(shared);pending=now;
        }
        return 0;
    }
    case WM_CAPTURECHANGED:drag=false;clickId[i]=0;return 0;
    case WM_MOUSEWHEEL:
        if(edit) {gap=std::clamp(gap+(static_cast<short>(HIWORD(w))>0?2:-2),32,250);save();layout();}return 0;
    case WM_RBUTTONUP:
        if(edit) {edit=false;save();layout();}return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;auto dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);
        bool enabled=!edit && i<view.count && view.rows[i].enabled && GetTickCount64()-pending>1000;
        pt::Row row{};if(i<view.count) row=view.rows[i];
        ui::paintButton(dc,r,row,edit,enabled,hovered[i]);
        EndPaint(h,&ps);return 0;
    }
    case WM_DESTROY:return 0;
    }
    return DefWindowProcW(h,msg,w,l);
}
}
int wmain(int argc,wchar_t** argv) {
    SetProcessDPIAware();
    if(!ui::initialize(argc,argv)) {MessageBoxW(nullptr,L"Could not open the per-user settings folder.",ui::title,MB_ICONERROR);return 4;}
    for(int i=1;i+1<argc;++i) if(!_wcsicmp(argv[i],L"--render-preview")) return ui::renderPreview(argv[i+1])?0:5;
    for(int i=1;i<argc;++i) if(!_wcsicmp(argv[i],L"--preview")) preview=true;
    if(!preview) {
        int code=inject(argc,argv,gamePid);if(code || (argc>1&&!_wcsicmp(argv[1],L"--check"))) return code;
        process=OpenProcess(SYNCHRONIZE,FALSE,gamePid);if(!process) return 2;
        HANDLE map=nullptr;
        for(int n=0;n<100&&!map;++n) {map=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,pt::mappingName(gamePid).c_str());if(!map) Sleep(100);}
        shared=map?static_cast<pt::Shared*>(MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(pt::Shared))):nullptr;
        for(int n=0;n<100 && shared && shared->status==0;++n) Sleep(100);
        if(!shared||shared->signature!=pt::magic||shared->status!=1) {MessageBoxW(nullptr,ui::tr(L"The mod did not attach. Check the game version and restart Grim Dawn before retrying.",L"Мод не подключился. Проверьте версию игры и перезапустите Grim Dawn перед повторной попыткой."),ui::title,MB_ICONERROR);return 3;}
        if(argc>1&&!_wcsicmp(argv[1],L"--attach-check")) return 0;
        EnumWindows(findWindow,0);
    } else {
        view.count=3;view.generation=1;
        const wchar_t* names[]={L"Aetherwalker",L"Riftkeeper",L"Nightblade"};
        for(int i=0;i<3;++i) {view.rows[i].id=i+1;view.rows[i].enabled=i!=2;view.rows[i].distance=i==2?-1:float(134-i*116);wcscpy_s(view.rows[i].name,names[i]);}
    }
    HANDLE mutex=CreateMutexW(nullptr,TRUE,(L"Local\\PartyTeleport.UI."+std::to_wstring(gamePid)).c_str());
    if(!mutex||GetLastError()==ERROR_ALREADY_EXISTS) return 0;
    ini=ui::settingsPath;
    anchorX=std::clamp(int(GetPrivateProfileIntW(L"Buttons",L"X",230,ini.c_str())),0,16000);
    anchorY=std::clamp(int(GetPrivateProfileIntW(L"Buttons",L"Y",150,ini.c_str())),0,16000);
    gap=std::clamp(int(GetPrivateProfileIntW(L"Buttons",L"Gap",72,ini.c_str())),32,250);
    WNDCLASSW wc{};wc.lpfnWndProc=proc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"PartyTeleport.Button";wc.hCursor=LoadCursorW(nullptr,IDC_HAND);RegisterClassW(&wc);
    for(int i=0;i<3;++i) {
        buttons[i]=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,wc.lpszClassName,ui::title,WS_POPUP,0,0,width,height,nullptr,nullptr,wc.hInstance,nullptr);
        SetWindowLongPtrW(buttons[i],GWLP_USERDATA,i);
    }
    SetTimer(nullptr,1,100,nullptr);
    bool was9=false,was10=false,was11=false,was12=false;
    MSG msg;
    while(GetMessageW(&msg,nullptr,0,0)>0) {
        if(msg.message==WM_TIMER) {
            bool focused=preview||foreground();
            bool k9=(GetAsyncKeyState(VK_F9)&0x8000)!=0,k10=(GetAsyncKeyState(VK_F10)&0x8000)!=0,k11=(GetAsyncKeyState(VK_F11)&0x8000)!=0,k12=(GetAsyncKeyState(VK_F12)&0x8000)!=0;
            if(focused&&k9&&!was9) {edit=!edit;save();}
            if(focused&&k10&&!was10) break;
            if(focused&&k11&&!was11) hidden=!hidden;
            if(focused&&k12&&!was12) ui::toggleLanguage();
            was9=k9;was10=k10;was11=k11;was12=k12;layout();
        }
        TranslateMessage(&msg);DispatchMessageW(&msg);
    }
    for(auto h:buttons) DestroyWindow(h);
    return 0;
}
