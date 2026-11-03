// Exercise the application's real window handlers using hidden test windows.
#define wmain partyTeleportMain
#include "app.cpp"
#undef wmain
#include <cassert>
int inject(int,wchar_t**,DWORD&) {assert(false && "UI tests must not inject");return 1;}
namespace {
void clickSetting(ui::SettingAction action) {
    RECT r=ui::settingRect(action);LPARAM point=MAKELPARAM((r.left+r.right)/2,(r.top+r.bottom)/2);
    SendMessageW(settingsWindow,WM_LBUTTONDOWN,MK_LBUTTON,point);
    SendMessageW(settingsWindow,WM_LBUTTONUP,0,point);
}
}
int main() {
    wchar_t folder[32768];GetCurrentDirectoryW(32768,folder);
    ini=folder;ini+=L"\\ui-test.ini";ui::settingsPath=ini;
    pt::Shared s{};s.status=1;s.view.time=GetTickCount64();shared=&s;
    process=CreateEventW(nullptr,TRUE,FALSE,nullptr); // unsignalled: model a live game
    WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=proc;wc.lpszClassName=L"PartyTeleport.UI.Test";
    assert(RegisterClassW(&wc));
    // Remain hidden; no interaction with the user's game or pointer.
    gameWindow=CreateWindowExW(0,wc.lpszClassName,L"Test",WS_POPUP,0,0,1200,800,nullptr,nullptr,wc.hInstance,nullptr);
    for(int i=0;i<3;++i) {buttons[i]=CreateWindowExW(0,wc.lpszClassName,L"Test",WS_POPUP,0,0,width,height,nullptr,nullptr,wc.hInstance,nullptr);SetWindowLongPtrW(buttons[i],GWLP_USERDATA,i);}
    wc.lpfnWndProc=settingsProc;wc.lpszClassName=L"PartyTeleport.Settings.Test";assert(RegisterClassW(&wc));
    settingsWindow=CreateWindowExW(0,wc.lpszClassName,L"Test",WS_POPUP,0,0,ui::settingsWidth,ui::settingsHeight,nullptr,nullptr,wc.hInstance,nullptr);
    assert(settingsWindow);
    handleHotkey(true,false);assert(!edit);handleHotkey(false,false);
    handleHotkey(true,true);assert(edit);
    handleHotkey(true,true);assert(edit); // held F8 does not repeat
    handleHotkey(false,true);handleHotkey(true,true);assert(!edit);handleHotkey(false,true);
    assert(!IsWindowVisible(settingsWindow));
    clickSetting(ui::SettingAction::Russian);assert(!ui::russian); // inactive controls cannot act
    handleHotkey(true,true);assert(edit);handleHotkey(false,true);
    clickSetting(ui::SettingAction::Russian);assert(ui::russian);
    clickSetting(ui::SettingAction::English);assert(!ui::russian);
    wchar_t language[16];GetPrivateProfileStringW(L"General",L"Language",L"",language,16,ini.c_str());assert(!wcscmp(language,L"en"));
    gap=72;clickSetting(ui::SettingAction::More);assert(gap==74);clickSetting(ui::SettingAction::Less);assert(gap==72);
    gap=250;clickSetting(ui::SettingAction::More);assert(gap==250);gap=32;clickSetting(ui::SettingAction::Less);assert(gap==32);
    gap=72;anchorX=230;anchorY=150;anchorStart={230,150};dragStart={100,100};drag=true;
    moveDrag({140,125});assert(anchorX==270&&anchorY==175);
    view.count=1;view.time=GetTickCount64();view.rows[0].id=2;view.rows[0].enabled=1;
    SendMessageW(buttons[0],WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(10,10));
    SendMessageW(buttons[0],WM_LBUTTONUP,0,MAKELPARAM(10,10));assert(!s.requestId); // setup never teleports
    clickSetting(ui::SettingAction::Done);assert(!edit&&!IsWindowVisible(settingsWindow));
    int oldX=anchorX;moveDrag({200,200});assert(anchorX==oldX);
    handleHotkey(true,true);assert(edit);clickSetting(ui::SettingAction::Close);
    MSG msg{};assert(PeekMessageW(&msg,nullptr,WM_QUIT,WM_QUIT,PM_REMOVE)&&msg.message==WM_QUIT);
    for(auto h:buttons) DestroyWindow(h);DestroyWindow(settingsWindow);DestroyWindow(gameWindow);CloseHandle(process);
    DeleteFileW(ini.c_str());
    puts("PASS: F8 focus/edge toggle, settings-only actions, language persistence, spacing limits, movement, no teleport in setup, Done and Exit.");
}
