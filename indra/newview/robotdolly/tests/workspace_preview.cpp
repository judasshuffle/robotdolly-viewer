// ROBOT DOLLY: standalone Win32 workspace preview (not connected to SL).
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include <string>
#include "../workspace_model.h"
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

namespace {
const wchar_t* names[]={L"3D World",L"Chat",L"Inventory",L"Landmarks"};
const char* types[]={"sl.world","sl.chat","sl.inventory","sl.landmarks"};
robotdolly::EditorRegistry registry=robotdolly::makeDefaultRegistry();
robotdolly::Workspace model(registry);
HWND windowHandle=nullptr,selectors[3]={};
int dividerX=680,dividerY=340;
bool movingX=false,movingY=false;
int modes[3]={0,1,2};

RECT area(int n,RECT client) {
    client.left+=8;client.top+=8;client.right-=8;client.bottom-=8;
    if(n==0) client.right=dividerX-4;
    else {client.left=dividerX+4;if(n==1)client.bottom=dividerY-4;else client.top=dividerY+4;}
    return client;
}
void arrange() {
    if(!windowHandle)return;
    RECT r;GetClientRect(windowHandle,&r);
    dividerX=std::clamp(dividerX,140,std::max(141,r.right-140));
    dividerY=std::clamp(dividerY,110,std::max(111,r.bottom-110));
    for(int i=0;i<3;++i) if(selectors[i]){
        RECT a=area(i,r);
        MoveWindow(selectors[i],a.left+7,a.top+4,
            std::max(100,a.right-a.left-14),180,TRUE);
    }
    InvalidateRect(windowHandle,nullptr,TRUE);
}
void drawText(HDC dc,int x,int y,const wchar_t* s,COLORREF color){
    SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);
    TextOutW(dc,x,y,s,lstrlenW(s));
}
void paintArea(HDC dc,RECT a,int mode){
    HBRUSH body=CreateSolidBrush(RGB(44,48,54));
    HBRUSH header=CreateSolidBrush(RGB(68,72,80));
    FillRect(dc,&a,body);
    RECT h=a;h.bottom=std::min(a.bottom,a.top+31);
    FillRect(dc,&h,header);
    DeleteObject(header);DeleteObject(body);
    int x=a.left+13,y=a.top+50;
    if(mode==0){
        drawText(dc,x,y,L"SECOND LIFE WORLD",RGB(226,231,235));
        drawText(dc,x,y+27,L"Live rendering will be connected later.",RGB(166,177,188));
    }else if(mode==1){
        drawText(dc,x,y,L"Nearby chat / IM / Group",RGB(226,231,235));
        drawText(dc,x,y+28,L"Conversation placeholder (not online yet)",RGB(166,177,188));
    }else if(mode==2){
        drawText(dc,x,y,L"> My Inventory",RGB(226,231,235));
        drawText(dc,x,y+28,L"    > Objects",RGB(190,199,211));
        drawText(dc,x,y+56,L"    > Textures",RGB(190,199,211));
        drawText(dc,x,y+84,L"    > Scripts",RGB(190,199,211));
    }else{
        drawText(dc,x,y,L"LOCATION SHORTCUTS",RGB(226,231,235));
        drawText(dc,x,y+28,L"Home   |   Workshop   |   Favourites",RGB(190,199,211));
    }
}
LRESULT CALLBACK procedure(HWND h,UINT message,WPARAM w,LPARAM l){
    switch(message){
    case WM_CREATE:{
        windowHandle=h;
        RECT r;GetClientRect(h,&r);
        dividerX=r.right*2/3;dividerY=r.bottom/2;
        for(int i=0;i<3;++i){
            selectors[i]=CreateWindowExW(0,L"COMBOBOX",L"",
                WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
                0,0,200,180,h,(HMENU)(INT_PTR)(200+i),GetModuleHandleW(nullptr),nullptr);
            for(const auto* name:names)
                SendMessageW(selectors[i],CB_ADDSTRING,0,(LPARAM)name);
            SendMessageW(selectors[i],CB_SETCURSEL,i,0);
        }
        arrange();return 0;
    }
    case WM_SIZE:arrange();return 0;
    case WM_COMMAND:{
        if(HIWORD(w)==CBN_SELCHANGE){
            for(int i=0;i<3;++i)if((HWND)l==selectors[i]){
                int choice=(int)SendMessageW(selectors[i],CB_GETCURSEL,0,0);
                if(choice>=0&&choice<4)modes[i]=choice;
                InvalidateRect(h,nullptr,TRUE);
            }
        }return 0;
    }
    case WM_LBUTTONDOWN:{
        int x=GET_X_LPARAM(l),y=GET_Y_LPARAM(l);
        if(abs(x-dividerX)<8){movingX=true;SetCapture(h);}
        else if(x>dividerX&&abs(y-dividerY)<8){movingY=true;SetCapture(h);}
        return 0;
    }
    case WM_MOUSEMOVE:
        if(movingX){dividerX=GET_X_LPARAM(l);arrange();}
        if(movingY){dividerY=GET_Y_LPARAM(l);arrange();}
        return 0;
    case WM_LBUTTONUP:
        movingX=false;movingY=false;ReleaseCapture();return 0;
    case WM_PAINT:{
        PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);
        RECT r;GetClientRect(h,&r);
        HBRUSH background=CreateSolidBrush(RGB(26,28,33));
        FillRect(dc,&r,background);DeleteObject(background);
        for(int i=0;i<3;++i)paintArea(dc,area(i,r),modes[i]);
        EndPaint(h,&ps);return 0;
    }
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(h,message,w,l);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show){
    WNDCLASSW wc={};wc.lpfnWndProc=procedure;wc.hInstance=instance;
    wc.lpszClassName=L"RobotDollyWorkspace";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if(!RegisterClassW(&wc))return 1;
    HWND h=CreateWindowExW(0,wc.lpszClassName,L"ROBOT DOLLY - Workspace Preview",
        WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1100,720,
        nullptr,nullptr,instance,nullptr);
    if(!h)return 2;
    ShowWindow(h,show);
    MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){
        TranslateMessage(&msg);DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
