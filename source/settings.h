#pragma once
#include "i18n.h"
#include <cstdio>
namespace ui {
constexpr int settingsWidth=314,settingsHeight=224;
enum class SettingAction {None,Move,English,Russian,Less,More,Done,Close};
inline RECT settingRect(SettingAction action) {
    switch(action) {
    case SettingAction::Move:return {0,0,settingsWidth,58};
    case SettingAction::English:return {12,66,151,98};
    case SettingAction::Russian:return {163,66,302,98};
    case SettingAction::Less:return {12,112,50,144};
    case SettingAction::More:return {264,112,302,144};
    case SettingAction::Done:return {12,176,151,210};
    case SettingAction::Close:return {163,176,302,210};
    default:return {};
    }
}
inline SettingAction settingAt(POINT point) {
    for(auto action:{SettingAction::Move,SettingAction::English,SettingAction::Russian,
        SettingAction::Less,SettingAction::More,SettingAction::Done,SettingAction::Close}) {
        RECT r=settingRect(action);if(PtInRect(&r,point)) return action;
    }
    return SettingAction::None;
}
inline void paintSettings(HDC dc,int gap,SettingAction hover=SettingAction::None) {
    RECT all{0,0,settingsWidth,settingsHeight};auto bg=CreateSolidBrush(RGB(27,25,20));FillRect(dc,&all,bg);DeleteObject(bg);
    auto bronze=CreateSolidBrush(RGB(145,112,60));FrameRect(dc,&all,bronze);DeleteObject(bronze);
    SetBkMode(dc,TRANSPARENT);
    auto font=CreateFontW(-14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Verdana");
    auto oldFont=SelectObject(dc,font);SetTextColor(dc,RGB(232,214,175));
    RECT titleRect{14,7,302,31};DrawTextW(dc,tr(L"Party Teleport · Settings",L"Party Teleport · Настройка"),-1,&titleRect,DT_SINGLELINE|DT_VCENTER);
    SetTextColor(dc,RGB(165,155,131));RECT hint{14,31,302,54};
    DrawTextW(dc,tr(L"Drag this header to move the buttons",L"Тяните заголовок для сдвига кнопок"),-1,&hint,DT_SINGLELINE|DT_VCENTER);
    for(auto action:{SettingAction::English,SettingAction::Russian,SettingAction::Less,
        SettingAction::More,SettingAction::Done,SettingAction::Close}) {
        RECT r=settingRect(action);
        bool selected=(action==SettingAction::English&&!russian)||(action==SettingAction::Russian&&russian);
        auto fill=CreateSolidBrush(selected?RGB(69,58,36):hover==action?RGB(53,46,33):RGB(36,33,26));
        FillRect(dc,&r,fill);DeleteObject(fill);
        auto frame=CreateSolidBrush(selected?RGB(194,155,88):RGB(107,84,48));FrameRect(dc,&r,frame);DeleteObject(frame);
        const wchar_t* text=L"";
        switch(action) {
        case SettingAction::English:text=L"English";break;
        case SettingAction::Russian:text=L"Русский";break;
        case SettingAction::Less:text=L"−";break;
        case SettingAction::More:text=L"+";break;
        case SettingAction::Done:text=tr(L"Done",L"Готово");break;
        case SettingAction::Close:text=tr(L"Exit app",L"Выход");break;
        default:break;
        }
        SetTextColor(dc,action==SettingAction::Close?RGB(218,154,130):RGB(232,214,175));
        DrawTextW(dc,text,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_CENTER);
    }
    wchar_t spacing[64];swprintf_s(spacing,tr(L"Row spacing: %d px",L"Шаг строк: %d px"),gap);
    RECT label{55,112,259,144};SetTextColor(dc,RGB(202,188,157));DrawTextW(dc,spacing,-1,&label,DT_SINGLELINE|DT_VCENTER|DT_CENTER);
    SelectObject(dc,oldFont);DeleteObject(font);
}
}
