// ROBOT DOLLY workspace v0.2 - standalone Win32 preview (not connected to SL).
// Right-click any panel to split or close it; drag borders to resize.
#define UNICODE
#define _UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include <memory>
#include <vector>
#include "../workspace_model.h"
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")

namespace {
using robotdolly::AreaId;
using robotdolly::LayoutNode;
using robotdolly::SplitAxis;
constexpr int Gap=8,Header=32,ComboBase=2000;
const wchar_t* Names[]={L"3D World",L"Chat",L"Inventory",L"Landmarks"};
const char* Types[]={"sl.world","sl.chat","sl.inventory","sl.landmarks"};
robotdolly::EditorRegistry registry=robotdolly::makeDefaultRegistry();
std::unique_ptr<robotdolly::Workspace> space;
struct Pane {AreaId id; RECT box; HWND selector;};
struct Divider {AreaId left,right; RECT hit,bounds; SplitAxis axis;};
std::vector<Pane> panes;
std::vector<Divider> dividers;
HWND mainWindow=nullptr;
AreaId menuArea=0;
Divider dragDivider={};
bool dragging=false;
bool cornerDragging=false;
AreaId cornerArea=0;
POINT cornerStart={};
POINT cornerCurrent={};
int nextCombo=0;

AreaId firstLeaf(const LayoutNode* n) {
    while(n->is_split)n=n->first.get();
    return n->area.id;
}
AreaId lastLeaf(const LayoutNode* n) {
    while(n->is_split)n=n->second.get();
    return n->area.id;
}
const robotdolly::Area* findArea(const LayoutNode* n,AreaId id) {
    if(!n->is_split)return n->area.id==id?&n->area:nullptr;
    if(auto* found=findArea(n->first.get(),id))return found;
    return findArea(n->second.get(),id);
}
void fill(HDC dc,RECT r,COLORREF color) {
    HBRUSH brush=CreateSolidBrush(color);
    FillRect(dc,&r,brush);DeleteObject(brush);
}
void text(HDC dc,int x,int y,const wchar_t* str,COLORREF color) {
    SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);
    TextOutW(dc,x,y,str,lstrlenW(str));
}
void makeLayout(const LayoutNode* node,RECT bounds) {
    if(!node->is_split) {
        Pane* pane=nullptr;
        for(auto& item:panes)if(item.id==node->area.id){pane=&item;break;}
        if(!pane){
            HWND control=CreateWindowExW(0,L"COMBOBOX",L"",
                WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
                0,0,100,200,mainWindow,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(ComboBase+nextCombo++)),
                GetModuleHandleW(nullptr),nullptr);
            for(auto* name:Names)SendMessageW(control,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));
            panes.push_back({node->area.id,bounds,control});
            pane=&panes.back();
        }
        pane->box=bounds;
        for(int i=0;i<4;++i)if(node->area.editor_type==Types[i])
            SendMessageW(pane->selector,CB_SETCURSEL,i,0);
        MoveWindow(pane->selector,bounds.left+6,bounds.top+5,
            std::max(40,static_cast<int>(bounds.right-bounds.left)-12),200,TRUE);
        return;
    }
    RECT a=bounds,b=bounds,bar=bounds;
    if(node->axis==SplitAxis::Horizontal) {
        int width=static_cast<int>(bounds.right-bounds.left)-Gap;
        int part=std::clamp(static_cast<int>(width*node->fraction),0,std::max(0,width));
        a.right=bounds.left+part;
        b.left=a.right+Gap;bar.left=a.right;bar.right=b.left;
    }else{
        int height=static_cast<int>(bounds.bottom-bounds.top)-Gap;
        int part=std::clamp(static_cast<int>(height*node->fraction),0,std::max(0,height));
        a.bottom=bounds.top+part;
        b.top=a.bottom+Gap;bar.top=a.bottom;bar.bottom=b.top;
    }
    dividers.push_back({lastLeaf(node->first.get()),firstLeaf(node->second.get()),bar,bounds,node->axis});
    makeLayout(node->first.get(),a);
    makeLayout(node->second.get(),b);
}
void relayout(){
    if(!mainWindow||!space)return;
    RECT r;GetClientRect(mainWindow,&r);
    r.left+=8;r.top+=8;r.right-=8;r.bottom-=8;
    dividers.clear();
    makeLayout(&space->root(),r);
    for(auto it=panes.begin();it!=panes.end();){
        if(!findArea(&space->root(),it->id)){
            DestroyWindow(it->selector);it=panes.erase(it);
        }else ++it;
    }
    InvalidateRect(mainWindow,nullptr,TRUE);
}
void drawPane(HDC dc,const Pane& pane,const robotdolly::Area& area){
    RECT r=pane.box;if(r.right<=r.left||r.bottom<=r.top)return;
    fill(dc,r,RGB(43,46,52));
    RECT header=r;header.bottom=std::min(r.bottom,r.top+Header);
    fill(dc,header,RGB(66,70,78));
    // Blender-inspired corner grip: drag diagonally to create a new area.
    RECT grip={r.right-17,r.bottom-17,r.right-3,r.bottom-3};
    if(grip.right>grip.left&&grip.bottom>grip.top){
        HPEN pen=CreatePen(PS_SOLID,1,RGB(165,174,186));
        HGDIOBJ prior=SelectObject(dc,pen);
        for(int i=0;i<3;++i){
            MoveToEx(dc,grip.right-3-i*4,grip.bottom-2,nullptr);
            LineTo(dc,grip.right-2,grip.bottom-3-i*4);
        }
        SelectObject(dc,prior);DeleteObject(pen);
    }
    int x=r.left+12,y=r.top+48;
    if(area.editor_type=="sl.world"){
        text(dc,x,y,L"SECOND LIFE WORLD",RGB(228,231,235));
        text(dc,x,y+27,L"Live rendering will come later.",RGB(172,180,190));
    }else if(area.editor_type=="sl.chat"){
        text(dc,x,y,L"NEARBY / IM / GROUP CHAT",RGB(228,231,235));
        text(dc,x,y+27,L"Conversation placeholder",RGB(172,180,190));
    }else if(area.editor_type=="sl.inventory"){
        text(dc,x,y,L"> My Inventory",RGB(228,231,235));
        text(dc,x,y+27,L"  > Objects",RGB(172,180,190));
        text(dc,x,y+54,L"  > Textures",RGB(172,180,190));
    }else{
        text(dc,x,y,L"LOCATION SHORTCUTS",RGB(228,231,235));
        text(dc,x,y+27,L"Home | Workshop | Favourites",RGB(172,180,190));
    }
}
LRESULT CALLBACK windowProc(HWND hwnd,UINT msg,WPARAM w,LPARAM l){
    switch(msg){
    case WM_CREATE:{
        mainWindow=hwnd;
        space=std::make_unique<robotdolly::Workspace>(registry);
        auto world=space->root().area.id;
        auto chat=space->split(world,SplitAxis::Horizontal,"sl.chat");
        space->setEditor(chat,"sl.chat","nearby");
        space->split(chat,SplitAxis::Vertical,"sl.inventory");
        relayout();return 0;
    }
    case WM_SIZE:relayout();return 0;
    case WM_COMMAND:{
        if(HIWORD(w)==CBN_SELCHANGE){
            for(const auto& p:panes)if(p.selector==reinterpret_cast<HWND>(l)){
                int i=static_cast<int>(SendMessageW(p.selector,CB_GETCURSEL,0,0));
                if(i>=0&&i<4)space->setEditor(p.id,Types[i]);
                InvalidateRect(hwnd,nullptr,FALSE);return 0;
            }
        }
        switch(LOWORD(w)){
        case 4101:space->split(menuArea,SplitAxis::Horizontal,"sl.chat");relayout();return 0;
        case 4102:space->split(menuArea,SplitAxis::Vertical,"sl.chat");relayout();return 0;
        case 4103:space->closeArea(menuArea);relayout();return 0;
        }
        return 0;
    }
    case WM_CONTEXTMENU:{
        POINT at={GET_X_LPARAM(l),GET_Y_LPARAM(l)};
        if(at.x==-1&&at.y==-1){GetCursorPos(&at);}
        POINT local=at;ScreenToClient(hwnd,&local);
        for(const auto& p:panes)if(PtInRect(&p.box,local)){
            menuArea=p.id;
            HMENU menu=CreatePopupMenu();
            AppendMenuW(menu,MF_STRING,4101,L"Split Left / Right");
            AppendMenuW(menu,MF_STRING,4102,L"Split Top / Bottom");
            AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
            AppendMenuW(menu,MF_STRING,4103,L"Join / Close This Area");
            TrackPopupMenu(menu,TPM_LEFTALIGN|TPM_RIGHTBUTTON,at.x,at.y,0,hwnd,nullptr);
            DestroyMenu(menu);
            return 0;
        }
        return 0;
    }
    case WM_SETCURSOR:{
        if(LOWORD(l)==HTCLIENT){
            POINT p;GetCursorPos(&p);ScreenToClient(hwnd,&p);
            for(const auto& pane:panes){
                RECT corner={pane.box.right-19,pane.box.bottom-19,pane.box.right,pane.box.bottom};
                if(PtInRect(&corner,p)){
                    SetCursor(LoadCursorW(nullptr,IDC_SIZENWSE));return TRUE;
                }
            }
            for(const auto& d:dividers)if(PtInRect(&d.hit,p)){
                SetCursor(LoadCursorW(nullptr,d.axis==SplitAxis::Horizontal?IDC_SIZEWE:IDC_SIZENS));
                return TRUE;
            }
        }
        break;
    }
    case WM_LBUTTONDOWN:{
        POINT p={GET_X_LPARAM(l),GET_Y_LPARAM(l)};
        for(const auto& pane:panes){
            RECT corner={pane.box.right-19,pane.box.bottom-19,pane.box.right,pane.box.bottom};
            if(PtInRect(&corner,p)){
                cornerDragging=true;cornerArea=pane.id;cornerStart=p;
                cornerCurrent=p;SetCapture(hwnd);return 0;
            }
        }
        for(const auto& d:dividers)if(PtInRect(&d.hit,p)){
            dragDivider=d;dragging=true;SetCapture(hwnd);return 0;
        }
        break;
    }
    case WM_MOUSEMOVE:{
        if(cornerDragging){
            cornerCurrent={GET_X_LPARAM(l),GET_Y_LPARAM(l)};
            return 0;
        }
        if(dragging){
            int origin=dragDivider.axis==SplitAxis::Horizontal?dragDivider.bounds.left:dragDivider.bounds.top;
            int extent=dragDivider.axis==SplitAxis::Horizontal?
                static_cast<int>(dragDivider.bounds.right-dragDivider.bounds.left):
                static_cast<int>(dragDivider.bounds.bottom-dragDivider.bounds.top);
            int mouse=dragDivider.axis==SplitAxis::Horizontal?GET_X_LPARAM(l):GET_Y_LPARAM(l);
            if(extent>Gap)space->resizeBetween(dragDivider.left,dragDivider.right,
                std::clamp(static_cast<float>(mouse-origin)/static_cast<float>(extent-Gap),0.1f,0.9f));
            relayout();return 0;
        }
        break;
    }
    case WM_LBUTTONUP:
        if(cornerDragging){
            POINT finish={GET_X_LPARAM(l),GET_Y_LPARAM(l)};
            const int dx=finish.x-cornerStart.x;
            const int dy=finish.y-cornerStart.y;
            cornerDragging=false;
            ReleaseCapture();
            if(std::max(abs(dx),abs(dy))>=35){
                // A primarily horizontal gesture makes side-by-side areas.
                const auto axis=abs(dx)>=abs(dy)?SplitAxis::Horizontal:SplitAxis::Vertical;
                space->split(cornerArea,axis,"sl.chat");
                relayout();
            }
            return 0;
        }
        if(dragging){dragging=false;ReleaseCapture();return 0;}break;
    case WM_CAPTURECHANGED:dragging=false;cornerDragging=false;return 0;
    case WM_PAINT:{
        PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);
        RECT r;GetClientRect(hwnd,&r);fill(dc,r,RGB(26,28,32));
        for(const auto& p:panes)if(auto* area=findArea(&space->root(),p.id))drawPane(dc,p,*area);
        for(const auto& d:dividers)fill(dc,d.hit,RGB(12,13,16));
        EndPaint(hwnd,&ps);return 0;
    }
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(hwnd,msg,w,l);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show){
    WNDCLASSW wc={};wc.lpfnWndProc=windowProc;wc.hInstance=instance;
    wc.lpszClassName=L"RobotDollyWorkspaceV02";
    wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if(!RegisterClassW(&wc))return 1;
    HWND hwnd=CreateWindowExW(0,wc.lpszClassName,L"ROBOT DOLLY - Split Workspace v0.2",
        WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1150,740,
        nullptr,nullptr,instance,nullptr);
    if(!hwnd)return 2;
    ShowWindow(hwnd,show);
    MSG m;while(GetMessageW(&m,nullptr,0,0)>0){
        TranslateMessage(&m);DispatchMessageW(&m);
    }
    return static_cast<int>(m.wParam);
}
