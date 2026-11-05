#pragma once
#include "i18n.h"
#include "protocol.h"
#include "settings.h"
#include <algorithm>
#include <cstdio>
namespace ui {
inline void paintButton(HDC dc,RECT r,const pt::Row& row,bool edit,bool enabled,bool hover) {
    int h=r.bottom-r.top,w=r.right-r.left;
    for(int y=0;y<h;++y) {
        int shade=(h-y)*15/std::max(1,h);
        COLORREF color=edit?RGB(62+shade,48+shade,29+shade):RGB(23+shade+(hover?7:0),22+shade+(hover?6:0),19+shade);
        auto brush=CreateSolidBrush(color);RECT line{r.left,r.top+y,r.right,r.top+y+1};FillRect(dc,&line,brush);DeleteObject(brush);
    }
    auto bronze=CreateSolidBrush(hover?RGB(193,153,87):RGB(124,95,52));FrameRect(dc,&r,bronze);DeleteObject(bronze);
    RECT inner=r;InflateRect(&inner,-2,-2);auto dark=CreateSolidBrush(RGB(11,12,10));FrameRect(dc,&inner,dark);DeleteObject(dark);
    // Original GDI artwork: a small green rift in a bronze frame.
    auto oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));
    auto pen=CreatePen(PS_SOLID,2,enabled?RGB(112,191,138):RGB(98,98,83));auto oldPen=SelectObject(dc,pen);
    int cy=r.top+h/2;Ellipse(dc,r.left+9,cy-9,r.left+21,cy+9);
    MoveToEx(dc,r.left+11,cy+4,nullptr);LineTo(dc,r.left+20,cy-5);
    SelectObject(dc,oldPen);DeleteObject(pen);SelectObject(dc,oldBrush);
    SetBkMode(dc,TRANSPARENT);
    auto font=CreateFontW(-15,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Verdana");
    auto oldFont=SelectObject(dc,font);
    SetTextColor(dc,enabled?RGB(232,214,175):RGB(145,137,119));
    RECT nameRect=r;nameRect.left+=32;nameRect.right-=10;
    std::wstring text=edit?tr(L"Drag · wheel: spacing",L"Тянуть · колесо: шаг"):row.name;
    if(!edit) {
        nameRect.bottom=r.top+27;
        wchar_t distance[40];std::wstring detail;
        if(!row.otherArea&&row.distance>=0&&std::isfinite(row.distance)) {
            swprintf_s(distance,L"~%.0f %s",row.distance,tr(L"m",L"м"));detail=distance;
        } else detail=row.area[0]?row.area:tr(L"Location unavailable",L"Локация неизвестна");
        RECT d=r;d.left+=32;d.top+=27;d.right-=10;d.bottom-=3;
        auto small=CreateFontW(-13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Verdana");
        SelectObject(dc,small);DrawTextW(dc,detail.c_str(),-1,&d,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        SelectObject(dc,font);DeleteObject(small);
    }
    DrawTextW(dc,text.c_str(),-1,&nameRect,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
    SelectObject(dc,oldFont);DeleteObject(font);
    (void)w;
}
inline bool renderPreview(const wchar_t* file) {
    constexpr int w=760,h=380;
    auto dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    void* pixels=nullptr;auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    if(!bitmap) {DeleteDC(dc);return false;}
    auto old=SelectObject(dc,bitmap);RECT all{0,0,w,h};auto bg=CreateSolidBrush(RGB(15,16,14));FillRect(dc,&all,bg);DeleteObject(bg);
    auto heading=CreateFontW(-21,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Georgia");
    auto oldFont=SelectObject(dc,heading);SetTextColor(dc,RGB(218,189,134));SetBkMode(dc,TRANSPARENT);
    RECT titleRect{24,16,736,52};DrawTextW(dc,title,-1,&titleRect,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    SelectObject(dc,oldFont);DeleteObject(heading);
    pt::Row row{};wcscpy_s(row.name,L"Aetherwalker");row.distance=134;
    RECT a{24,72,344,124};paintButton(dc,a,row,false,true,false);
    row.distance=18;wcscpy_s(row.name,L"Riftkeeper");RECT b{24,144,344,196};paintButton(dc,b,row,false,true,true);
    row.distance=-1;row.otherArea=1;wcscpy_s(row.name,L"Nightblade");wcscpy_s(row.area,tr(L"Devil's Crossing",L"Перекрёсток Дьявола"));
    RECT c{24,216,344,268};paintButton(dc,c,row,false,false,false);
    int state=SaveDC(dc);SetViewportOrgEx(dc,398,68,nullptr);paintSettings(dc,72);RestoreDC(dc,state);
    SetTextColor(dc,RGB(155,150,134));RECT note{24,326,736,354};DrawTextW(dc,tr(L"UI preview · Right-click the overlay for settings",L"Пример интерфейса · Правый клик по оверлею — настройка"),-1,&note,DT_SINGLELINE|DT_VCENTER);
    GdiFlush();
    BITMAPFILEHEADER header{};header.bfType=0x4D42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+w*h*4;
    FILE* output=nullptr;_wfopen_s(&output,file,L"wb");bool ok=false;
    if(output) {
        ok=fwrite(&header,sizeof(header),1,output)==1 && fwrite(&info.bmiHeader,sizeof(BITMAPINFOHEADER),1,output)==1 && fwrite(pixels,w*h*4,1,output)==1;
        fclose(output);
    }
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return ok;
}
}
