/*
 (C) 2003-2012  Petr Lastovicka
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License.
*/  
#include "hdr.h"
#pragma hdrstop
#include "hotkeyp.h"

int
 Nvolume=2,
 diskfreePrec=1,      //disk free space precision (digits after decimal point)
 noMsg,
 sentToActiveWnd,
 lockPaste;

UINT cancelAutoPlay;
char volumeStr[256];
bool blockedKeys[256];
char *showTextLast;
PasteTextData pasteTextData;
bool keyReal[256];
static bool shifts[Nshift],shifts0[Nshift2];
const int shiftTab[Nshift]={VK_SHIFT,VK_CONTROL,VK_MENU,VK_LWIN,VK_RWIN};
const int shift2Tab[Nshift2]={VK_RMENU,VK_LSHIFT,VK_RSHIFT,VK_RCONTROL,VK_LMENU,VK_LWIN,VK_RWIN,VK_LCONTROL};

static unsigned seed; //random number seed
static HWND hiddenWin,keyWin;
static HideInfo hiddenApp;
HideInfo trayIconA[30];

struct Tvk {
 char *s;
 int vk;
} vks[]={
{"ESCAPE", VK_ESCAPE},{"ESC", VK_ESCAPE},{"F10", VK_F10},
{"F11", VK_F11},{"F12", VK_F12},{"F1", VK_F1},{"F2", VK_F2},
{"F3", VK_F3},{"F4", VK_F4},{"F5", VK_F5},{"F6", VK_F6},{"F7", VK_F7},
{"F8", VK_F8},{"F9", VK_F9},{"PRINTSCREEN", VK_SNAPSHOT},
{"PRINTSCRN", VK_SNAPSHOT},{"SCROLLLOCK", VK_SCROLL},
{"PAUSE", VK_PAUSE},{"BREAK", VK_CANCEL},{"CLEAR", VK_CLEAR},
{"INSERT", VK_INSERT},{"INS", VK_INSERT},{"DELETE", VK_DELETE},
{"DEL", VK_DELETE},{"HOME", VK_HOME},{"END", VK_END},
{"PAGEUP", VK_PRIOR},{"PAGEDOWN", VK_NEXT},{"LEFT", VK_LEFT},
{"RIGHT", VK_RIGHT},{"UP", VK_UP},{"DOWN", VK_DOWN},{"TAB", VK_TAB},
{"CAPSLOCK", VK_CAPITAL},{"CAPS", VK_CAPITAL},{"BACKSPACE", VK_BACK},
{"BS", VK_BACK},{"ENTER", VK_RETURN},{"SPACE", VK_SPACE},
{"SHIFT", VK_SHIFT},{"CONTROL", VK_CONTROL},{"CTRL", VK_CONTROL},
{"RSHIFT", VK_RSHIFT},{"RCONTROL", VK_RCONTROL},{"RCTRL", VK_RCONTROL},
{"^", VK_CONTROL},{"ALT", VK_MENU},{"RALT", VK_RMENU},{"MENU", VK_MENU},{"RMENU", VK_RMENU},
{"WIN", VK_LWIN},{"LWIN", VK_LWIN},{"RWIN", VK_RWIN},{"APPS", VK_APPS},
{"NUMLOCK", VK_NUMLOCK},{"DIVIDE", VK_DIVIDE},{"MULTIPLY", VK_MULTIPLY},
{"SUBTRACT", VK_SUBTRACT},{"ADD", VK_ADD},{"DECIMAL", VK_DECIMAL},
{"NUM0", VK_NUMPAD0},{"NUM1", VK_NUMPAD1},{"NUM2", VK_NUMPAD2},
{"NUM3", VK_NUMPAD3},{"NUM4", VK_NUMPAD4},{"NUM5", VK_NUMPAD5},
{"NUM6", VK_NUMPAD6},{"NUM7", VK_NUMPAD7},{"NUM8", VK_NUMPAD8},
{"NUM9", VK_NUMPAD9},{"BACK", VK_BROWSER_BACK},
{"FORWARD", VK_BROWSER_FORWARD},{"REFRESH", VK_BROWSER_REFRESH},
{"SEARCH", VK_BROWSER_SEARCH},{"FAVORITES", VK_BROWSER_FAVORITES},
{"BROWSER", VK_BROWSER_HOME},{"MUTE", VK_VOLUME_MUTE},
{"VOLUME_DOWN", VK_VOLUME_DOWN},{"VOLUME_UP", VK_VOLUME_UP},
{"NEXT_TRACK", VK_MEDIA_NEXT_TRACK},{"PREV_TRACK", VK_MEDIA_PREV_TRACK},
{"STOP", VK_MEDIA_STOP},{"PLAY_PAUSE", VK_MEDIA_PLAY_PAUSE},{"MEDIA_SELECT", VK_LAUNCH_MEDIA_SELECT},
{"MAIL", VK_LAUNCH_MAIL},{"POWER", VK_SLEEP},{"LAUNCH_APP1", VK_LAUNCH_APP1},{"LAUNCH_APP2", VK_LAUNCH_APP2},
{"LBUTTON", 100100+M_Left},{"RBUTTON", 100100+M_Right},{"MBUTTON", 100100+M_Middle},
{"XBUTTON1", 100100+M_X1},{"XBUTTON2", 100100+M_X1+1},
{"WHEELUP", 100100+M_WheelUp},{"WHEELDOWN", 100100+M_WheelDown},
{"WHEELRIGHT", 100100+M_WheelRight},{"WHEELLEFT", 100100+M_WheelLeft},
{"WAIT", 100000},{"SHOW", 100001},{"SLEEP", 100002},{"REP", 100003},
{"DOUBLECLICK", 100004},
};

char *controlPanels[]={
"appwiz","desk","inetcpl","main","mmsys",
"powercfg","sysdm","timedate"
};

//---------------------------------------------------------------------------
void shiftsUp();

bool isShiftKey(int c)
{
 for(int i=0; i<Nshift; i++){
   if(c==shiftTab[i]) return true;
 }
 return false;
}

int isExtendedKey(int c)
{
  unsigned char A[]={VK_INSERT,VK_HOME,VK_PRIOR,VK_DELETE,VK_END,VK_NEXT,VK_LEFT,VK_RIGHT,VK_UP,VK_DOWN,VK_NUMLOCK,VK_DIVIDE,VK_LWIN,VK_RWIN,VK_APPS,VK_RCONTROL,VK_RMENU,VK_RSHIFT,0};
  return strchr((char*)A,c)!=0;
}

int vkToScan(int c)
{
  return MapVirtualKey(c,0)|(isExtendedKey(c)<<8);
}

void keyEventDown(int c)
{
  keybd_event((BYTE)c,(BYTE)MapVirtualKey(c,0), isExtendedKey(c) ? KEYEVENTF_EXTENDEDKEY : 0, extraInfoIGNORE);
}

void keyEventUp(int c)
{
  keybd_event((BYTE)c,(BYTE)MapVirtualKey(c,0), isExtendedKey(c) ? KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP : KEYEVENTF_KEYUP, extraInfoIGNORE);
}

void keyDown1(int c)
{
 if(!keyWin || isShiftKey(c)){
   keyEventDown(c);
 }
 if(keyWin){
   PostMessage(keyWin,shifts[shAlt]?WM_SYSKEYDOWN:WM_KEYDOWN,c, 1|(vkToScan(c)<<16));
   Sleep(1);
 }
}

void keyDown(int c)
{
 for(int i=0; i<Nshift; i++){
   if(c==shiftTab[i]) shifts[i]=true;
 }
 keyDown1(c);
}

void keyDownI(int c)
{
  if(GetAsyncKeyState(c)>=0) keyDown(c);
}

void keyUp1(int c)
{
 if(!keyWin || isShiftKey(c)){
   keyEventUp(c);
 }
 if(keyWin){
   PostMessage(keyWin,shifts[shAlt]?WM_SYSKEYUP:WM_KEYUP,c, 0xC0000001|(vkToScan(c)<<16));
   Sleep(1);
 }
}

void keyUp(int c)
{
 keyUp1(c);
 shiftsUp();
}

void keyPress(int c, int updown)
{
  switch(updown){
  default:
    keyDown(c);
    if(!isShiftKey(c)) keyUp(c);
    break;
  case 1:
    keyUp(c);
    break;
  case 2:
    keyDown1(c);
    break;
  }
}

void shiftsUp()
{
 for(int i=0; i<Nshift; i++){
   if(shifts[i]){
     shifts[i]=false;
     keyUp1(shiftTab[i]);
   }
 }
}

void mouse_event1(DWORD f)
{
  mouse_event(f,0,0,0,0);
}

void mouse_event2(DWORD f, int d)
{
  mouse_event(f,0,0,d,0);
}

void mouse_eventDown(int i)
{
 static const DWORD D[]={MOUSEEVENTF_LEFTDOWN,MOUSEEVENTF_RIGHTDOWN,0,0,MOUSEEVENTF_MIDDLEDOWN};

 if(i<sizeA(D)){
   mouse_event1(D[i]);
 }else if(i==M_WheelDown){
   mouse_event2(MOUSEEVENTF_WHEEL,-WHEEL_DELTA);
 }else if(i==M_WheelUp){
   mouse_event2(MOUSEEVENTF_WHEEL,WHEEL_DELTA);
 }else if(i==M_WheelLeft){
   mouse_event2(MOUSEEVENTF_HWHEEL,-WHEEL_DELTA);
 }else if(i==M_WheelRight){
   mouse_event2(MOUSEEVENTF_HWHEEL,WHEEL_DELTA);
 }else if(i<15){
   mouse_event2(MOUSEEVENTF_XDOWN,1<<(i-5));
 }
}

void mouse_eventUp(int i)
{
 static const DWORD U[]={MOUSEEVENTF_LEFTUP,MOUSEEVENTF_RIGHTUP,0,0,MOUSEEVENTF_MIDDLEUP};
 
 if(i<sizeA(U)){
   mouse_event1(U[i]);
 }else if(i<15){
   mouse_event2(MOUSEEVENTF_XUP,1<<(i-5));
 }
}

void doubleClick()
{
  mouse_event1(MOUSEEVENTF_LEFTDOWN);
  mouse_event1(MOUSEEVENTF_LEFTUP);
  Sleep(1);
  mouse_event1(MOUSEEVENTF_LEFTDOWN);
  mouse_event1(MOUSEEVENTF_LEFTUP);
}

void waitWhileKey()
{
 bool pressed;
 int i,k;

 for(k=0; k<100; k++){
   pressed=false;
   for(i=0; i<256; i++){
     if(GetAsyncKeyState(i)<0) pressed=true;
   }
   if(!pressed) break;
   Sleep(50);
 }
}

static int waitCmd,waitCount;
static char *waitParam;

bool waitWhileKey(int cmd, char *param)
{
  if(!hookK){
    waitWhileKey();
  }else{
    for(int i=0; i<256; i++){
      if(keyReal[i]){
        if(waitCount>40){
          keyReal[i]=false;
          break;
        }
        waitCmd=cmd;
        cpStr(waitParam,param);
        SetTimer(hWin,9,60,0);
        waitCount++;
        return false;
      }
    }
  }
  waitCount=0;
  return true;
}

void checkWait()
{
  command(waitCmd,waitParam);
}

void forceNoKey()
{
  for(int i=0; i<256; i++){
    if((GetAsyncKeyState(i)<0 || i==VK_CONTROL || i==VK_MENU)){
      keyEventUp(i);
    }
  }
}

void forceNoShift()
{
 int i;

 if(isWin9X){
   for(i=0; i<Nshift; i++){
     shifts0[i]= GetAsyncKeyState(shiftTab[i])<0;
     keyEventUp(shiftTab[i]);
   }
 }else{
   for(i=0; i<Nshift2; i++){
     shifts0[i]= GetAsyncKeyState(shift2Tab[i])<0;
   }
   for(i=0; i<Nshift2; i++){
     if(shifts0[i]){
       //NOTE: if right Alt is released in PuTTY, then CTRL+key shortcuts in PuTTY terminal stop working
       keyEventUp(shift2Tab[i]);
     }else{
       //keyReal is unreliable if UAC is enabled in Windows Vista and later
       keyReal[shift2Tab[i]] = false;
     }
   }
 }
}

void restoreShift()
{
 int i;

 if(isWin9X){
   for(i=0; i<Nshift; i++){
     if(shifts0[i]){
       if(i!=shLWin && i!=shRWin) keyEventDown(shiftTab[i]);
     }
   }
 }else{
   if(hookK){
     for(i=0; i<Nshift2; i++){
       shifts0[i]=keyReal[shift2Tab[i]];
     }
   }
   else if(shifts0[shRAlt]) shifts0[shLCtrl]=false;
 
   for(i=0; i<Nshift2; i++){
     if(shifts0[i]) keyEventDown(shift2Tab[i]);
   }
   if((GetAsyncKeyState(VK_LWIN)<0 || GetAsyncKeyState(VK_RWIN)<0 || GetAsyncKeyState(VK_LMENU)<0) && !shifts0[shLCtrl]){
     //prevent the Start menu or a normal menu to show after Win+key or Alt+key
     keyEventDown(VK_CONTROL);
     keyEventUp(VK_CONTROL);
   }

   if(hookK){
     //user could release shift just after reading keyReal value, but before calling keyEventDown
     for(i=0; i<Nshift2; i++){
       if(shifts0[i] && !keyReal[shift2Tab[i]]){
         keyEventUp(shift2Tab[i]);
       }
     }
   }
 }
}

int hex(char c)
{
  if(c>='0' && c<='9') return c-'0';
  if(c>='A' && c<='F') return c-'A'+10;
  if(c>='a' && c<='f') return c-'A'+10;
  return -1;
}

int parseKey(char *&s, int &vk, int &updown)
{
 int i;
 char z;

 updown=0;
 if(*s=='\\' && *(s+1)!='\\'){
   s++;
   if(*s=='x' || *s=='X'){
     if((i=hex(s[1]))>=0 && (vk=hex(s[2]))>=0){
       vk+=i<<4;
       s+=3;
l1:
       if(*s=='u' && s[1]=='p'){
         s+=2;
         updown=1;
       }
       else if(*s=='d' && s[1]=='o' && s[2]=='w' && s[3]=='n'){
         s+=4;
         updown=2;
       }
       if(*s==' ' || *s=='.') s++;
       return 1;
     }
   }
   for(Tvk *v=vks; ; v++){
     if(v==endA(vks)){
       z=*s;
       if(z>='A' && z<='Z' || z>='0' && z<='9'){
         vk=z;
         s++;
         goto l1;
       }
       shiftsUp();
       char *k=s;
       while(*s && *s!=' ' && *s!='.' && *s!='\\') s++;
       z=*s;
       *s=0;
       msglng(744,"Unknown key name: %s",k);
       *s=z;
       return 0;
     }                   
     if(!_strnicmp(s,v->s,strlen(v->s))){
       s+=strlen(v->s);
       vk=v->vk;
       goto l1;
     }
   }
 }else{
   if(*s=='\\') s++;
   SHORT c= VkKeyScanEx(*s,GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(),0)));
   if(c==-1){
     return 3;
   }else{
     vk=c;
     s++;
     return 2;
   }
 }
}

void parseMacro1(char *s, int cmd)
{
 int vk,d,updown,code;
 char *s1;
 bool rep=false;
 
 forceNoShift();
 while(*s){
   code=parseKey(s,vk,updown);
   if(!updown){
     if(cmd==904) updown=2; //down
     if(cmd==906) updown=1; //up
   }
   switch(code){
   case 0: //error
     return;
   case 1: //special key or command
     if(vk<100000){
       keyPress(vk,updown);
     }else if(vk>=100100){
       if(updown!=1) mouse_eventDown(vk%100);
       if(updown!=2) mouse_eventUp(vk%100);
     }else{
       switch(vk){
       case 100000:
         waitWhileKey();
         break;
       case 100001:
         SetForegroundWindow(keyWin); 
         Sleep(10); 
         break;
       case 100002:
         s1=s;
         d=strtol(s,&s,10);
         if(*s==' ' || *s=='.') s++;
         if(d==0){
           Sleep(s1==s ? 1000 : 10);
         }else{
           amax(d,100);
           Sleep(d*100);
         }
         break;
       case 100003:
         rep=true;
         break;
       case 100004:
         doubleClick();
         break;
       }
     }
   break;
   case 2: //character
     if(updown!=1){
       if(vk&0x100) keyDown(VK_SHIFT);
       if(vk&0x200) keyDown(VK_CONTROL);
       if(vk&0x400) keyDown(VK_MENU);
     }
     keyPress(LOBYTE(vk),updown);
   break;
   default: //localized character
     if(cmd==906) break;
     char buf[8];
     CharToOemBuff(s,buf,1);
     s++;
     _itoa(*(unsigned char*)buf,buf,10);
     keyDown(VK_MENU);
     for(char *t=buf; *t; t++){
       vk= VK_NUMPAD0 + *t - '0';
       keyDown(vk);
       keyUp1(vk);
     }
     keyUp(VK_MENU);
   break;
   }
 }
 shiftsUp();
 if(rep || hookK && !isWin9X) restoreShift();
}

void parseMacro(char *s)
{
  keyWin=0;
  parseMacro1(s,0);
}

//---------------------------------------------------------------------------
HWND getActiveWindow()
{
 HWND w=GetForegroundWindow();
 if(!w) msglng(746,"There is no foreground window");
 return w;
}

struct TfindWinInfo
{
  char *s;
  HWND found;
  int eval;
  DWORD pid;
};

BOOL CALLBACK enumWinStr(HWND hWnd,LPARAM param)
{
 int e;
 DWORD pid;
 TfindWinInfo *data= (TfindWinInfo*) param;
 char buf[512];

 if(!data->pid || !GetWindowThreadProcessId(hWnd,&pid) || data->pid==pid){
   e=0;
   //compare window title
   GetWindowText(hWnd,buf,sizeof(buf));
   if(strstr(buf,data->s)){
     e=20; //substring
     if(!strcmp(buf,data->s)) e+=10; //exact match
   }
   //compare window class
   GetClassName(hWnd,buf,sizeof(buf));
   if(!strcmp(buf,data->s)) e+=10;
   //better score for visible window
   if(e && IsWindowVisible(hWnd)){
     e+=100;
     LONG exstyle=GetWindowLong(hWnd,GWL_EXSTYLE);
     if(!(exstyle & WS_EX_TOOLWINDOW)) e+=50;
   }
   if(e>data->eval){
     data->eval=e;
     data->found=hWnd;
   }
 }
 return TRUE;
}

HWND getWindow2(char *s, DWORD pid)
{
 if(!s || !*s) return getActiveWindow();
 TfindWinInfo i;
 i.found=0;
 i.eval=0;
 i.s=s;
 i.pid=pid;
 EnumWindows((WNDENUMPROC)enumWinStr, (LPARAM)&i);
 HWND w=i.found;
 if(!w) w=findWindow(s,0);
 if(!w && !noMsg){
   //msglng(745,"Window \"%s\" not found", s);
   if(trayicon && iconDelay) SetTimer(hWin,6,iconDelay/4,0);
 }
 return w;
}

HWND getWindow(char *s)
{
  return getWindow2(s,0);
}

bool checkProcess(DWORD pid, char *exe)
{
 static char lastExe[15];
 static HANDLE lastProcess;
 static DWORD lastPid;
 PROCESSENTRY32 pe;
 char const *result;

 if(pid==lastPid){
   result=lastExe;
 }else{
   result="";
   pe.dwSize = sizeof(PROCESSENTRY32);
   HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
   if(h!=(HANDLE)-1){
     Process32First(h,&pe);
     do{
       if(pe.th32ProcessID==pid){
         CloseHandle(lastProcess);
         lastProcess=OpenProcess(PROCESS_QUERY_INFORMATION,0,lastPid=pid);
         strncpy(lastExe,result=cutPath(pe.szExeFile),sizeA(lastExe));
         break;
       }
     }while(Process32Next(h,&pe));
     CloseHandle(h);
   }
 }
 return !_strnicmp(result,cutPath(exe),15);
}


struct TwndProp {
  HWND w;
  DWORD pid;
  char title[512];
  char wndClass[512];
  char *s;
};

bool checkWindowOr(TwndProp *info);


bool checkWindowUnary(TwndProp *info)
{
  bool b=true;
  char *s,c;

  if(*info->s == '('){
    info->s++;
    b=checkWindowOr(info);
    if(*info->s == ')') info->s++;
  }
  else if(*info->s == '!'){
    info->s++;
    b= !checkWindowUnary(info);
  }
  else{
    s=info->s;
    if(*s=='\''){
      s= ++info->s;
      while(*s && *s!='\'') s++;
    }else{
      int par=0;
      while(*s && *s!='&' && *s!='|'){
        if(*s=='(') par++;
        if(*s==')') if(!par--) break;
        s++;
      }
    }
    c=*s;
    *s=0;
    //compare window class
    if(strcmp(info->wndClass,info->s))
      //compare window title
      if(!strstr(info->title,info->s))
        //compare exe file name
        if(!info->pid || !checkProcess(info->pid,info->s))
          b=false;
    *s=c;
    if(c=='\'') s++;
    info->s=s;
  }
  return b;
}

bool checkWindowAnd(TwndProp *info)
{
  bool b= checkWindowUnary(info);
  while(*info->s == '&'){
    info->s++;
    b&= checkWindowUnary(info);
  }
  return b;
}

bool checkWindowOr(TwndProp *info)
{
  bool b= checkWindowAnd(info);
  while(*info->s == '|'){
    info->s++;
    b|= checkWindowAnd(info);
  }
  return b;
}

HWND checkWindow(char *s)
{
  TwndProp info;
  info.w = GetForegroundWindow();
  if(s && *s){
    //get window properties
    GetClassName(info.w, info.wndClass, sizeof(info.wndClass));
    GetWindowText(info.w, info.title, sizeof(info.title));
    info.pid=0;
    GetWindowThreadProcessId(info.w, &info.pid);
    //parse expression
    info.s=s;
    bool b =checkWindowOr(&info);
    ///if(*info.s) msg("Parse error");
    if(!b) return 0;
  }
  return info.w;
}

void ignoreHotkey(HotKey *hk)
{
 int i,j,id,c,vk;
 int b;

 id= int(hk-hotKeyA);
 registerHK(id,true);
 vk=hk->vkey;
 if(isWin9X && vk==vkMouse) hk->ignore=true;
 keyWin=0;
 if(hk->modifiers & MOD_SHIFT) keyDownI(VK_SHIFT);
 if(hk->modifiers & MOD_CONTROL) keyDownI(VK_CONTROL);
 if(hk->modifiers & MOD_ALT) keyDownI(VK_MENU);
 if(hk->modifiers & MOD_WIN) keyDownI(VK_LWIN);
 if(vk==vkMouse){
   c=(int)hk->scanCode;
   buttonToArray(c);
   for(j=24; j>=0; j-=4){
     i=(c>>j)&15;
     if(i>=15) continue;
     if(i==M_WheelUp || i==M_WheelDown || i==M_WheelRight || i==M_WheelLeft){
       mouse_eventDown(i);
     }else{
       b=1<<i;
       if(ignoreButtons & b){
         ignoreButtons&=~b;
         mouse_eventDown(i);
       }
     }
   }
   shiftsUp();
 }else if(vk!=vkLirc && vk!=vkJoy){
   keyDown(vk);
   if(!hookK || !keyReal[vk]) keyUp(vk);
 }
 registerHK(id,false);
}

//---------------------------------------------------------------------------
//26: //keys to another window
//74: //keys to active window
//21: //command to another window
//73: //command to active window
//94: //macro to active window
//30: //test if the window is active 
bool cmdToWindow(int cmd, char *param)
{
 char *u1,*u2,*u3,*s2,*s3,c2,c3,*s;
 HWND w;
 bool result=false;

 c2=c3=0;
 u2=u3=s3=0;
 //1.parameter
 for(s=param; *s!=' '; s++){
   if(!*s) return false;
 }
 u1=s;
 do{ s++; }while(*s==' ');
 //2.parameter
 s2=s;
 if(*s=='\"'){
   s2=(++s);
   while(*s && *s!='\"') s++;
 }else{
   while(*s && *s!=' ') s++;
 }
 if(*s){
   u2=s;
   do{ s++; }while(*s==' ');
   if(*s){
     //3.parameter
     s3=s;
     if(*s=='\"'){
       s3=(++s);
       while(*s && *s!='\"') s++;
       u3=s;
     }
   }
 }
 //put terminating zero characters
 *u1=0;
 if(u2){ c2=*u2; *u2=0; }
 if(u3){ c3=*u3; *u3=0; }
 //find window
 HWND(*f)(char*) = (cmd<30 ? getWindow:checkWindow);
 if(s3 && c2==' ' && (!u3 || *u3!='\"')){
   *u2=' ';
   noMsg++;
   w=f(s2);
   noMsg--;
   if(w){
     s3=0;
   }else{
     *u2=0;
     w=f(s2);
   }
 }else{
   w=f(s2);
 }
 //send keys or command
 if(w){
   result=true;
   if(cmd!=30){
     DWORD pid;
     if(s3 && GetWindowThreadProcessId(w,&pid)){
       w=getWindow2(s3,pid);
     }
     if(w){
       if(cmd&1){
         PostMessage(w,WM_COMMAND,strtol(param,&s,10),0);
       }else{
         keyWin= (cmd>90) ? 0 : w;
         parseMacro1(param,cmd);
       }
     }
     if(cmd>30) sentToActiveWnd=2;
   }
 }else if(cmd>30){
   if(!sentToActiveWnd) sentToActiveWnd=1;
 }
 //restore parameter
 *u1=' ';
 if(u2) *u2=c2;
 if(u3) *u3=c3;
 return result;
}
//---------------------------------------------------------------------------
int strToIntStr(char *param, char **param2)
{
  int result = strtol(param,param2,10);
  if(**param2==':'){
    *param2=param;
    return 0;
  }
  while(*param2 && **param2==' ') (*param2)++;
  return result;
}

DWORD iconProc(char *param, int hide)
{
  char *name;
  int id= strToIntStr(param,&name);
  for(int cnt=0; cnt<10; cnt++){
    Sleep(500*cnt);
    HWND w=getWindow(name);
    if(w && hideIcon(w,id,hide)) break; //success
  }
  delete[] param;
  return 0;
}

DWORD WINAPI hideIconProc(LPVOID param)
{
  return iconProc((char*)param,1);
}

DWORD WINAPI restoreIconProc(LPVOID param)
{
  return iconProc((char*)param,0);
}
//---------------------------------------------------------------------------

BOOL CALLBACK hideProc(HWND w, LPARAM param)
{
  HideInfo *info = (HideInfo*)param;
  DWORD pid;
  char buf[16];

  if(IsWindowVisible(w) && 
    GetWindowThreadProcessId(w,&pid) && pid==info->pid){
    if(!checkProcess(pid,"explorer.exe") || 
      GetClassName(w,buf,sizeA(buf)) && !strcmp(buf,"ExploreWClass")){
      HICON icon = (HICON) GetClassLongPtr(w,GCLP_HICONSM);
      if(icon && (!info->icon || w==info->activeWnd)){
        info->icon= icon;
      }
      ShowWindow(w,SW_HIDE);
      WndItem *i = new WndItem();
      i->w= w;
      i->nxt= info->list;
      info->list= i;
    }
  }
  return TRUE;
}

void hideApp(HWND w, HideInfo *info)
{
  if(w && GetWindowThreadProcessId(w,&info->pid)){
    info->icon=0;
    //remember foreground window
    info->activeWnd=w;
    DWORD pid;
    if(GetWindowThreadProcessId(info->activeWnd,&pid) && pid!=info->pid){
      info->activeWnd=0;
    }
    //hide all windows which belong to the same process as window w
    EnumWindows(hideProc,(LPARAM)info);
  }
}

void getExeIcon(HideInfo *info)
{
  //get icon from the exe file
  MODULEENTRY32 me;
  me.dwSize = sizeof(MODULEENTRY32);
  HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,info->pid);
  if(h!=(HANDLE)-1){
    Module32First(h,&me);
    ExtractIconEx(me.szExePath,0,0,&info->icon,1);
    CloseHandle(h);
  }
  if(!info->icon) info->icon = (HICON)GetClassLongPtr(hWin,GCLP_HICONSM);
}

void unhideApp(HideInfo *info)
{
  if(!info->pid) return;
  for(WndItem *i= info->list;  i; )
  {
    ShowWindow(i->w,SW_SHOW);
    WndItem *i0 = i;
    i=i->nxt;
    delete i0;
  }
  info->list=0;
  info->pid=0;
  SetForegroundWindow(info->activeWnd);
}

void unhideAll()
{
  for(int i=0; i<sizeA(trayIconA); i++){
    deleteTrayIcon(i+100);
    unhideApp(&trayIconA[i]);
  }
  unhideApp(&hiddenApp);
  if(hiddenWin) ShowWindow(hiddenWin,SW_SHOW);
}
void Tpopup::show(bool toggle)
{
 createPopups();
 if(!delay || !hWnd) return;
 int ind= static_cast<int>(this-popup);
 if(!IsWindowVisible(hWnd)){
   aminmax(x,1,10000);
   aminmax(y,1,10000);
   aminmax(width,50,2000);
   SetWindowPos(hWnd,HWND_TOPMOST,
     int(GetSystemMetrics(SM_CXSCREEN)*x/10000)-width/2,
     int(GetSystemMetrics(SM_CYSCREEN)*y/10000)-height/2,
     width,height,SWP_NOACTIVATE);
   ShowWindow(hWnd,SW_SHOWNOACTIVATE);
   SetTimer(hWin,60+ind, refresh, 0);
 }else if(toggle){
   //hide
   PostMessage(hWin,WM_TIMER,50+ind,0);
   return;
 }
 if(cmdLineCmd>=0){
   UpdateWindow(hWnd);
   Sleep(delay);
   ShowWindow(hWnd,SW_HIDE);
 }else{
   SetTimer(hWin,50+ind, delay, 0);
 }
}

void volumeCmd(char *which, int value, int action)
{
 volume(which,value,action);
 //show window
 popupVolume.height= volumeH();
 popupVolume.show(false);
}

//------------------------------------------------------------------

static int cdDoorResult;
static HANDLE hDevice;
static BYTE inqData[36];

struct SCSI_PASS_THROUGH_DIRECT {
  USHORT Length;
  UCHAR ScsiStatus;
  UCHAR PathId;
  UCHAR TargetId;
  UCHAR Lun;
  UCHAR CdbLength;
  UCHAR SenseInfoLength;
  UCHAR DataIn;
  ULONG DataTransferLength;
  ULONG TimeOutValue;
  PVOID DataBuffer;
  ULONG SenseInfoOffset;
  UCHAR Cdb[16];
};

struct SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
  SCSI_PASS_THROUGH_DIRECT spt;
  UCHAR ucSenseBuf[32];
};

#ifndef CTL_CODE
#define CTL_CODE(DevType, Function, Method, Access) \
	(((DevType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif
#define IOCTL_SCSI_PASS_THROUGH_DIRECT CTL_CODE(4, 0x405, 0, 3)
#define SCSI_IOCTL_DATA_IN 1
#define DTC_WORM 4
#define DTC_CDROM 5


int getCDinfo(char letter)
{
  ULONG returned;
  SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER swb;
  char buf[12];

  sprintf(buf, "\\\\.\\%c:", letter);
  hDevice = CreateFileA(buf, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if(hDevice == INVALID_HANDLE_VALUE){
    hDevice = CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
      FILE_ATTRIBUTE_NORMAL, NULL);
  }
  if(hDevice != INVALID_HANDLE_VALUE){
    memset(&swb, 0, sizeof(swb));
    swb.spt.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    swb.spt.CdbLength = 6;
    swb.spt.SenseInfoLength = 24;
    swb.spt.DataIn = SCSI_IOCTL_DATA_IN;
    swb.spt.DataTransferLength = sizeof(inqData);
    swb.spt.TimeOutValue = 5;
    swb.spt.DataBuffer = &inqData;
    swb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
    swb.spt.Cdb[0] = 0x12;
    swb.spt.Cdb[4] = 0x24;
    if(DeviceIoControl(hDevice, IOCTL_SCSI_PASS_THROUGH_DIRECT,
        &swb, sizeof(swb), &swb,sizeof(swb), &returned, NULL)){
      return 1;
    }
  }
  return 0;
}

//action: 0=info,2=eject,3=close,4=speed
void cdCmd(char letter, int action, int param=0)
{
  ULONG	returned;
  bool bBeenHereBefore=false;
  SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER swb;

  if(getCDinfo(letter)){
    int btDeviceType= inqData[0] & 15;
    if(btDeviceType==DTC_CDROM || btDeviceType==DTC_WORM){
start:
      memset(&swb, 0, sizeof(swb));
      swb.spt.CdbLength = 6;
      if(action==2 || action==3){
        swb.spt.Cdb[0]=0x1B;
        swb.spt.Cdb[4]=(BYTE)action;
      }
      if(action==4){
        swb.spt.Cdb[0] = 0xBB;				
        //swb.spt.Cdb[1] = (BYTE)(LunID<<5);	
        param = unsigned(176.4 * param + 1);
        swb.spt.Cdb[2] = HIBYTE(param);	
        swb.spt.Cdb[3] = LOBYTE(param);	
        swb.spt.Cdb[4] = swb.spt.Cdb[5] = 0xFF;
        swb.spt.CdbLength = 12;
      }
      swb.spt.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
      swb.spt.DataIn = SCSI_IOCTL_DATA_IN;
      swb.spt.TimeOutValue = 15;
      swb.spt.SenseInfoLength = 14;
      swb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
  
      if(DeviceIoControl(hDevice, IOCTL_SCSI_PASS_THROUGH_DIRECT,
          &swb, sizeof(swb), &swb, sizeof(swb), &returned, NULL)){
        int s= swb.ucSenseBuf[2] & 0xf;
        if(s==0) cdDoorResult=1;
        if(s==2 && swb.ucSenseBuf[12]==0x3A){
          cdDoorResult= swb.ucSenseBuf[13];
        }
      }else{
        DWORD dwErrCode = GetLastError();
        if(!bBeenHereBefore &&
          (dwErrCode == ERROR_MEDIA_CHANGED || dwErrCode == ERROR_INVALID_HANDLE)){
          CloseHandle(hDevice);
          getCDinfo(letter);
          bBeenHereBefore=true;
          goto start;
        }
      }
    }
  }
  CloseHandle(hDevice);
}

DWORD WINAPI cdProc(LPVOID param)
{
 MCI_OPEN_PARMS m;
 MCI_PLAY_PARMS pp;
 MCI_STATUS_PARMS sp;
 MCI_GENERIC_PARMS gp;
 
 int op=(((int)param)>>8)&255;
 int speed=((int)param)>>16;
 char d[3];
 d[0]=(char)(int)param;
 d[1]=':';
 d[2]=0;

 EnterCriticalSection(&cdCritSect);
 m.dwCallback=0;
 m.lpstrDeviceType="CDAudio";
 m.lpstrElementName=d;
 m.lpstrAlias=0;
 if(mciSendCommand(0,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_ELEMENT,(WPARAM)&m)){
   msglng(748,"Drive is already used by another process");
 }else{
   if(op==62){ //eject/close
     cdDoorResult=0;
     cdCmd(d[0],0);
     if(cdDoorResult==1){
       cdCmd(d[0],2);
       op=-1;
     }else if(cdDoorResult==2){
       cdCmd(d[0],3);
       op=-1;
     }else{ //Windows 95/98/ME
       DWORD t=GetTickCount(); 
       mciSendCommand(m.wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN,0);
       if(GetTickCount()-t < 200) op=1;
     }          
   }
   switch(op){
    case 0: //eject
     mciSendCommand(m.wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN,0);
    break;
    case 1: //close
     mciSendCommand(m.wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED,0);
    break;
    case 39: //play
     mciSendCommand(m.wDeviceID, MCI_PLAY, 0,0);
    break;
    case 41: //stop
     mciSendCommand(m.wDeviceID, MCI_STOP, 0, (WPARAM)&gp);
    break;
    case 40: //next
    case 42: //previous
     sp.dwItem=MCI_STATUS_CURRENT_TRACK;
     if(!mciSendCommand(m.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (WPARAM)&sp)){
       sp.dwTrack=(DWORD)sp.dwReturn+41-op;
       sp.dwItem=MCI_STATUS_POSITION;
       if(!mciSendCommand(m.wDeviceID, MCI_STATUS, MCI_TRACK|MCI_STATUS_ITEM, (WPARAM)&sp)){
         pp.dwFrom=(DWORD)sp.dwReturn;
         mciSendCommand(m.wDeviceID, MCI_PLAY, MCI_FROM, (WPARAM)&pp);
       }
     }
    break;
    case 100: //speed
     cdCmd(d[0],4,speed);
     break;
   }
   mciSendCommand(m.wDeviceID,MCI_CLOSE,MCI_OPEN_TYPE|MCI_OPEN_ELEMENT,(WPARAM)&m);
 }
 LeaveCriticalSection(&cdCritSect);
 return 0;
}

void createThread(LPTHREAD_START_ROUTINE proc, LPVOID param)
{
  DWORD id;
  CloseHandle(CreateThread(0,0,proc,param,0,&id));
}

//CD command, drive is e.g. "D:"
void audioCD(char *drive, int op, int speed=0)
{
 char d[3];

 d[1]=':';
 d[2]=0;
 if(!drive || !*drive){
   for(d[0]='A'; ; d[0]++){
     if(GetDriveType(d)==DRIVE_CDROM) break;
     if(d[0]=='Z') return;
   }
 }else{
   d[0]=*drive;
 }
 if(GetDriveType(d)!=DRIVE_CDROM){
   msglng(747,"%s is not a CD drive",d);
   return;
 }
 LPVOID param = (LPVOID)((op<<8)|d[0]|(speed<<16));
 if(cmdLineCmd>=0){
   //do not create a thread if running from the command line,
   // because it would be immediately terminated at the end of WinMain
   cdProc(param);
 }else{
   createThread(cdProc,param);
 }
}

DWORD strToDword(char *&s)
{
  return strtoul(s,&s,10);
}

LONG changeDisplay(char *device, DEVMODE &dm, DWORD flag)
{
  if(device){
    TChangeDisplaySettingsEx p= (TChangeDisplaySettingsEx) GetProcAddress(GetModuleHandle("user32.dll"),"ChangeDisplaySettingsExA");
    return p(device,&dm,0,flag,0);
  }else{
    return ChangeDisplaySettings(&dm,flag);
  }
}

//change monitor resolution, color bit depth, frequency, display number
void resolution(char *param)
{
 DWORD w,h,c,f,d,w2,h2,c2,f2;
 char *s;
 DEVMODE dm;
 char device[22];
 strcpy(device,"\\\\.\\DISPLAY0");

 s=param;
 w=strToDword(s);
 h=strToDword(s);
 c=strToDword(s);
 f=strToDword(s);
 d=strToDword(s);
 if(d>0) _itoa(d,device+11,10);

 dm.dmSize = sizeof(dm);
 if(!EnumDisplaySettings((d>0) ? device:0, ENUM_CURRENT_SETTINGS,&dm)){
   msg("EnumDisplaySettings failed for display %d", d); // (error code %d)", d, GetLastError());
   return;
 }
 if(*param){
   while(*s==' ') s++;
   if(*s==','){
     s++;
     w2=strToDword(s);
     h2=strToDword(s);
     c2=strToDword(s);
     f2=strToDword(s);
     if((!w || dm.dmPelsWidth==w) && 
       (w2!=w || (!c || dm.dmBitsPerPel==c) && 
       (!f || dm.dmDisplayFrequency==f))){
       w=w2; h=h2; c=c2; f=f2;
     }
   }
   if(w) dm.dmPelsWidth=w;
   if(h) dm.dmPelsHeight=h;
   if(c) dm.dmBitsPerPel=c;
   if(f) dm.dmDisplayFrequency=f;
 }else{
   if(dm.dmPelsWidth==800){
     dm.dmPelsWidth=1024;
     dm.dmPelsHeight=768;
   }else{
     dm.dmPelsWidth=800;
     dm.dmPelsHeight=600;
   }
 }

 char *dev = (d>0) ? device : 0;
 dm.dmFields=DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL|DM_DISPLAYFREQUENCY;
 LONG err = changeDisplay(dev,dm,CDS_UPDATEREGISTRY);
 if(err==DISP_CHANGE_NOTUPDATED) err = changeDisplay(dev,dm,0);
 switch(err){
   case DISP_CHANGE_SUCCESSFUL:
     break;
   case DISP_CHANGE_BADMODE:
     msg("The graphics mode is not supported");
     break;
   case DISP_CHANGE_RESTART:
     msg("The computer must be restarted in order for the graphics mode to work");
     break;
   default:
     msg("ChangeDisplaySettings failed for display %d (error code %d)", d, err);
     break;
 }
}

struct TsearchInfo
{
  char *result;
  int recurse,action,pass;
  bool done;
  unsigned maxWallpaper;
  char const *last;
  char prev[MAX_PATH];
  char lastPath[MAX_PATH];
};

void getFullPathName(char *fn,char *path)
{
  char *dummy;
  GetFullPathName(fn,256,path,&dummy);
}

void search(char *dir, TsearchInfo *info)
{
 HANDLE h;
 WIN32_FIND_DATA fd;
 char *s,oldDir[MAX_PATH],ext[4];

 GetCurrentDirectory(sizeof(oldDir), oldDir);
 SetCurrentDirectory(dir);

 h = FindFirstFile("*.*",&fd);
 if(h!=INVALID_HANDLE_VALUE){
  do{
   if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
     if(fd.cFileName[0]!='.' && info->recurse){
       search(fd.cFileName,info);
     }
   }else{
     s=strchr(fd.cFileName,0);
     while(*--s!='\\'){
       if(*s=='.'){
         ext[0]=(char)toupper(s[1]);
         ext[1]=(char)toupper(s[2]);
         ext[2]=(char)toupper(s[3]);
         if(ext[0]=='B' && ext[1]=='M' && ext[2]=='P' ||
            ext[0]=='J' && ext[1]=='P' && (ext[2]=='G' || ext[2]=='E') ||
            ext[0]=='G' && ext[1]=='I' && ext[2]=='F'){
           switch(info->action){
           default:
             seed=seed*367413989+174680251;
             if(seed>=info->maxWallpaper){
               info->maxWallpaper=seed;
               getFullPathName(fd.cFileName,info->result);
             }
             break;
           case 1:
             getFullPathName(fd.cFileName,info->result);
             if(info->pass){ 
               info->done=true; 
             }else if(!_stricmp(info->last,fd.cFileName) && 
               !_stricmp(info->lastPath,info->result)){
               info->pass++;
             }
             break;
           case 2:
             getFullPathName(fd.cFileName,info->result);
             if(!_stricmp(info->last,fd.cFileName) && 
               !_stricmp(info->lastPath,info->result)){
               if(info->prev[0]){
                 strcpy(info->result,info->prev);
                 info->done=true; 
               }
             }else strcpy(info->prev,info->result);
             break;
           }
         }
         break;
       }
     }
   }
  }while(!info->done && FindNextFile(h,&fd));
  FindClose(h);
 }
 SetCurrentDirectory(oldDir);
}

void searchC(char *dir, TsearchInfo *info)
{
  info->done=false;
  info->pass=0;
  do{
    search(dir,info);
    info->pass++;
  }while(info->action && !info->done && info->pass<5);
}

//change wallpaper
void changeWallpaper(char *dir, int action)
{
 HKEY key;
 DWORD d=0,d2;
 int foundInd=0, b;
 char result0[MAX_PATH],*u,*s,*wallpaperStr=0,*wallpaperStr2;
 TsearchInfo info;
 
 u=0;
 if(dir[0]=='\"'){
   u=strchr(dir,0)-1;
   if(*u=='\"'){
     dir++;
     *u=0;
   }else{
     u=0;
   }
 }
 info.lastPath[0]=0;
 if(RegOpenKeyEx(HKEY_CURRENT_USER,subkey,0,KEY_QUERY_VALUE,&key)==ERROR_SUCCESS){
   if(RegQueryValueEx(key,"wallpaper",0,0,0,&d)==ERROR_SUCCESS){
     wallpaperStr= new char[d+4];
     *(DWORD*)(wallpaperStr+d)=0;
     if(RegQueryValueEx(key,"wallpaper",0,0,(BYTE*)wallpaperStr, &d)==ERROR_SUCCESS){
       for(s=wallpaperStr; (DWORD)(s-wallpaperStr)<d; s=strchr(s,0)+1){
         b=strcmp(s,dir);
         s=strchr(s,0)+1;
         d2=static_cast<int>(s-wallpaperStr);
         if(d<=d2) d=d2+1;
         if(!b){
           lstrcpyn(info.lastPath,s,sizeof(info.lastPath));
           foundInd=int(s-wallpaperStr);
           break;
         }
       }
     }
   }
   RegCloseKey(key);
 }
 seed=(unsigned)GetTickCount();
 info.action=action;
 info.maxWallpaper=0;
 info.result=result0;
 result0[0]=0;
 info.prev[0]=0;
 info.last=cutPath(info.lastPath);
 if(*dir){
   DWORD a = GetFileAttributes(dir);
   if(a==0xFFFFFFFF){
     msglng(749,"Cannot find folder or file %s",dir);
   }else if(!(a&FILE_ATTRIBUTE_DIRECTORY)){
     info.result=dir;
   }else{
     info.recurse=1;
     searchC(dir,&info);
   }
 }else{
   char buf[256];
   if(GetWindowsDirectory(buf,sizeof(buf))){
     info.recurse=0;
     searchC(buf,&info);
   }
 }
 if(info.result[0]){
   if(RegCreateKey(HKEY_CURRENT_USER, subkey, &key)==ERROR_SUCCESS){
     if(!foundInd){
       d2=(int)strlen(dir)+2;
       wallpaperStr2=wallpaperStr;
       wallpaperStr=new char[d+d2];
       memcpy(wallpaperStr,wallpaperStr2,d);
       delete[] wallpaperStr2;
       strcpy(wallpaperStr+d,dir);
       d+=d2;
       wallpaperStr[foundInd=d-1]=0;
     }
     d2=(int)strlen(info.result)-(int)strlen(wallpaperStr+foundInd);
     d+=d2;
     wallpaperStr2=wallpaperStr;
     wallpaperStr=new char[d];
     memcpy(wallpaperStr,wallpaperStr2,foundInd);
     strcpy(wallpaperStr+foundInd,info.result);
     memcpy(strchr(wallpaperStr+foundInd,0),strchr(wallpaperStr2+foundInd,0),d-foundInd-(int)strlen(info.result));
     delete[] wallpaperStr2;
     RegSetValueEx(key,"wallpaper",0,REG_BINARY,(BYTE*)wallpaperStr,d);
     RegCloseKey(key);
   }
   if(!SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, info.result, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE)){
#ifndef NOWALLPAPER
     CoInitialize(0);
     HRESULT hr;
     IActiveDesktop *pActiveDesktop;
     hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
       IID_IActiveDesktop, (void**)&pActiveDesktop);
     if(hr==S_OK){
       WCHAR resultW[MAX_PATH];
       MultiByteToWideChar(CP_ACP,0,info.result,-1,resultW,sizeA(resultW));
       pActiveDesktop->SetWallpaper(resultW,0);
       pActiveDesktop->ApplyChanges(AD_APPLY_ALL);
       pActiveDesktop->Release();
     }
     CoUninitialize();
     DWORD_PTR d;
     SendMessageTimeout(HWND_BROADCAST,WM_SETTINGCHANGE,SPI_SETDESKWALLPAPER,0,SMTO_ABORTIFHUNG,100,&d);
#endif
   }
   writeini();
 }
 if(u) *u='\"';
 delete[] wallpaperStr;
}

void deleteTemp(char *dir, FILETIME *time)
{
  HANDLE h;
  bool b;
  WIN32_FIND_DATA fd;
  char oldDir[MAX_PATH];
  
  GetCurrentDirectory(sizeof(oldDir), oldDir);
  SetCurrentDirectory(dir);
  h = FindFirstFile("*",&fd);
  if(h!=INVALID_HANDLE_VALUE){
    do{
      b= *(LONGLONG*)&fd.ftLastWriteTime < *(LONGLONG*)time && 
        *(LONGLONG*)&fd.ftCreationTime < *(LONGLONG*)time;
      if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
        if(fd.cFileName[0]!='.'){
          deleteTemp(fd.cFileName,time);
          if(b) RemoveDirectory(fd.cFileName);
        }
      }else{
        if(b) DeleteFile(fd.cFileName);
      }
    }while(FindNextFile(h,&fd));
    FindClose(h);
  }
  SetCurrentDirectory(oldDir);
}

void privilege(char *name)
{
 HANDLE token;
 TOKEN_PRIVILEGES tkp;

 if(OpenProcessToken(GetCurrentProcess(),
    TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &token))
  if(LookupPrivilegeValue(0,name,&tkp.Privileges[0].Luid)){
    tkp.PrivilegeCount=1;
    tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(token, FALSE, &tkp, 0,0,0);
  }
}

//set privileges to shutdown or restart computer
void privilege()
{
 saveAtExit();
 privilege(SE_SHUTDOWN_NAME);
}


BOOL CALLBACK closeProc(HWND w, LPARAM explorerPid)
{
  DWORD pid;
  DWORD_PTR r;

  if(IsWindowVisible(w) && GetWindowThreadProcessId(w,&pid) && 
    pid!=(DWORD)explorerPid && pid!=GetCurrentProcessId()){
    SendMessageTimeout(w,WM_SYSCOMMAND,SC_CLOSE,0,
      SMTO_ABORTIFHUNG|SMTO_BLOCK, 10000, &r);
  }
  return TRUE;
}

static bool isCloseAll;

void closeAll(int param)
{
  if(param==2) isCloseAll=true;
}

void closeAll2()
{
  if(isCloseAll) EnumWindows(closeProc,findProcess("explorer.exe"));
}

void saveDesktopIcons(int action)
{
  HWND w = FindWindowEx(FindWindowEx(FindWindow("Progman","Program Manager"),0,"SHELLDLL_DefView",0),0,WC_LISTVIEW,0/*"FolderView"*/);
  if(w){
    DWORD tid = GetWindowThreadProcessId(w,0);
    if(!klib) klib=LoadLibrary("hook.dll");
    HOOKPROC hproc = (HOOKPROC)GetProcAddress(klib,"_CallWndProcD@12");
    if(hproc){
      HHOOK hook= SetWindowsHookEx(WH_CALLWNDPROC,hproc,(HINSTANCE)klib,tid);
      if(hook){
        SendMessage(w,WM_USER+47348+action,0,0);
        UnhookWindowsHookEx(hook);
        if(action==0 && cmdFromKeyPress && !noMsg)
        {
          msg1(MB_OK|MB_ICONINFORMATION,lng(766,"Desktop icon positions are saved"));
        }
      }
    }
  }
}



void resetDiskfree()
{
 for(int i=0; i<26; i++){
   disks[i].text[0]='\0';
 }
}

void addDrive(char *root)
{
 DWORDLONG free,total;
 DiskInfo *d;
 TdiskFreeFunc p;

 p= (TdiskFreeFunc) GetProcAddress(GetModuleHandle("kernel32.dll"),"GetDiskFreeSpaceExA");
 if(p){
   if(!p(root,(ULARGE_INTEGER*)&free,(ULARGE_INTEGER*)&total,0)) return;
 }else{
   DWORD sectPerClust,bytesPerSect,freeClust,totalClust;
   if(!GetDiskFreeSpace(root,&sectPerClust,&bytesPerSect,&freeClust,&totalClust)) return;
   free= freeClust*(DWORDLONG)sectPerClust*bytesPerSect;
   total= totalClust*(DWORDLONG)sectPerClust*bytesPerSect;
 }
 d= &disks[root[0]-'A'];
 if(d->free!=free || !d->text[0]){
   d->free=free;
   d->bar=(int)(free*1000/total);
   if(free>2000000000){
     sprintf(d->text,"%.*f GB", diskfreePrec,(double)(LONGLONG)free/1073741824);
   }else if(free>2000000){
     sprintf(d->text,"%.*f MB", diskfreePrec,(double)(LONGLONG)free/1048576);
   }else{
     sprintf(d->text,"%d kB", (int)(free/1024));
   }
   RECT rc;
   rc.left=0;
   rc.right= popupDiskFree.width;
   rc.top= popupDiskFree.height;
   rc.bottom= rc.top + fontH;
   InvalidateRect(popupDiskFree.hWnd,&rc,FALSE);
 }
}

void diskFree()
{
 UINT t;
 char c,root[4];

 root[1]=':';
 root[2]='\\';
 root[3]=0;
 popupDiskFree.height=0;
 for(c='A'; c<='Z'; c++){
   root[0]=c;
   t=GetDriveType(root);
   if(t==DRIVE_FIXED || t==DRIVE_REMOTE){
     popupDiskFree.height+= diskSepH+1;
     addDrive(root);
     popupDiskFree.height+= fontH+diskSepH;
   }else{
     disks[c-'A'].text[0]=0;
   }
 }
 popupDiskFree.height++;
}

void screenshot(HWND wnd)
{
 BITMAPFILEHEADER hdr;
 HBITMAP hBmp;
 HDC winDC,dcb;
 BYTE *bits;
 FILE *f;
 RECT rc;
 int w,h,palSize;
 HGDIOBJ oldB;
 BITMAP b;
 BITMAPINFOHEADER &bmi = *(BITMAPINFOHEADER*)_alloca(sizeof(BITMAPINFOHEADER)+1024);

 //create bitmap
 winDC= GetWindowDC(wnd);
 dcb= CreateCompatibleDC(winDC);
 GetWindowRect(wnd,&rc);
 w= rc.right-rc.left;
 h= rc.bottom-rc.top;
 hBmp= CreateCompatibleBitmap(winDC,w,h);
 oldB= SelectObject(dcb,hBmp);
 //snapshot
 BitBlt(dcb,0,0,w,h,winDC,0,0,SRCCOPY);
 GetObject(hBmp,sizeof(BITMAP),&b);
 SelectObject(dcb,oldB);
 //initialize bitmap structure
 bmi.biSize = sizeof(BITMAPINFOHEADER);
 bmi.biWidth = b.bmWidth;
 bmi.biHeight = b.bmHeight;
 bmi.biPlanes = b.bmPlanes;
 bmi.biBitCount = b.bmBitsPixel;
 bmi.biCompression = BI_RGB;
 bmi.biXPelsPerMeter=bmi.biYPelsPerMeter=bmi.biClrImportant=0;
 
 //get pixels
 bits = new BYTE[h*(w*4+4)];
 if(!GetDIBits(dcb,hBmp,0,h,bits,(BITMAPINFO*)&bmi,DIB_RGB_COLORS)){
   msg("GetDIBits failed");
 }else{
   //save file
   if(save1(snapOfn,OFN_PATHMUSTEXIST)){
     f=fopen(snapOfn.lpstrFile,"wb");
     if(!f){
       msglng(733,"Cannot create file %s",snapOfn.lpstrFile);
     }else{
       hdr.bfType = 0x4d42;  //'BM'
       bmi.biClrUsed = (b.bmBitsPixel<16) ? 1<<b.bmBitsPixel : 0;
       palSize = bmi.biClrUsed * sizeof(RGBQUAD);
       hdr.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+palSize+bmi.biSizeImage;
       hdr.bfReserved1 = hdr.bfReserved2 = 0;
       hdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palSize;
       fwrite(&hdr, sizeof(BITMAPFILEHEADER), 1,f);
       fwrite(&bmi, sizeof(BITMAPINFOHEADER)+palSize, 1,f);
       fwrite(bits, bmi.biSizeImage, 1,f);
       fclose(f);
     }
   }
 }
 delete[] bits;
 ReleaseDC(wnd,winDC);
 DeleteObject(hBmp);
 DeleteDC(dcb);
}

void moveCmd(char *param,int actionx,int actiony)
{
 RECT rcd,rcw;
 int x,y;
 HWND w;

 w=getWindow(param);
 SystemParametersInfo(SPI_GETWORKAREA,0,&rcd,0);
 GetWindowRect(w,&rcw);
 switch(actionx){
  default: //don't change
   x=rcw.left;
  break;
  case 1: //left
   x=rcd.left;
  break;
  case 2: //right
   x=rcd.right-rcw.right+rcw.left;
  break;
  case 3: //center
   x=(rcd.right + rcd.left - rcw.right + rcw.left)>>1;
  break;
 }
 switch(actiony){
  default: //don't change
   y=rcw.top;
  break;
  case 1: //up
   y=rcd.top;
  break;
  case 2: //down
   y=rcd.bottom-rcw.bottom+rcw.top;
  break;
  case 3: //center
   y=(rcd.bottom + rcd.top - rcw.bottom + rcw.top)>>1;
  break;
 }
 SetWindowPos(w,0, x,y, 0,0, SWP_NOSIZE|SWP_NOZORDER|SWP_ASYNCWINDOWPOS);
}

void priority(int p)
{
 DWORD pid;
 HANDLE h;
 HWND w;

 w=getWindow(0);
 if(w){
   BOOL ok=FALSE;
   if(GetWindowThreadProcessId(w,&pid)){
     if((h=OpenProcess(PROCESS_SET_INFORMATION,0,pid))!=0){
       aminmax(p,0,Npriority-1);
       ok=SetPriorityClass(h,priorCnst[p]);
       CloseHandle(h);
     }            
   }
   if(!ok) msglng(750,"Can't change priority");
 }
}

void setOpacity(HWND w,int o)
{
 if(w && o){
   LONG s=GetWindowLong(w,GWL_EXSTYLE);
   if(((s&0x80000)==0) != (o==255)){
     SetWindowLong(w,GWL_EXSTYLE, s^0x80000);
   }
   TsetOpacityFunc p= (TsetOpacityFunc)GetProcAddress(GetModuleHandle("user32.dll"),"SetLayeredWindowAttributes");
   if(p) p(w,0,(BYTE)min(o,255),2);
 }           
}

static HWND noMinimWnd;

BOOL CALLBACK enumMinimize(HWND hWnd,LPARAM)
{
  LONG s=GetWindowLong(hWnd,GWL_STYLE);
  if((s&WS_MINIMIZEBOX) && IsWindowVisible(hWnd) && 
    !GetWindow(hWnd,GW_OWNER) && hWnd!=noMinimWnd){
    RECT rc;
    GetWindowRect(hWnd,&rc);
    if(IsRectEmpty(&rc)){ 
      //Borland Delphi application
      PostMessage(hWnd,WM_SYSCOMMAND,SC_MINIMIZE,0);
    }else{
      ShowWindow(hWnd, SW_MINIMIZE);
    }
  }
  return TRUE;
}

void PasteTextData::save()
{
  HGLOBAL hmem;
  void *ptr;
  UINT f;
  ClipboardData *cd,*cdn,**pnxt;

  if(GetClipboardOwner()==hWin) return;
  if(OpenClipboard(0)){
    for(cd=prev; cd; cd=cdn){
      cdn=cd->nxt;
      operator delete(cd->data);
      delete cd;
    }
    pnxt= &prev;
    f=0;
    while(( f=EnumClipboardFormats(f) )!=0){
      if((hmem= GetClipboardData(f))!=0){
        if((ptr=GlobalLock(hmem))!=0){
          cd= new ClipboardData;
          cd->format=f;
          cd->size= GlobalSize(hmem);
          cd->data= operator new(cd->size);
          memcpy(cd->data,ptr,cd->size);
          GlobalUnlock(hmem);
          *pnxt= cd;
          pnxt= &cd->nxt;
        }
      }
    }
    *pnxt=0;
    CloseClipboard();
  }
}

void PasteTextData::restore()
{
  HGLOBAL hmem;
  void *ptr;
  int i;
  ClipboardData *cd;
  
  if(prev){
    for(i=0; i<20; i++){
      if(OpenClipboard(hWin)){
        if(EmptyClipboard()){
          for(cd=prev; cd; cd=cd->nxt){
            if((hmem=GlobalAlloc(GMEM_DDESHARE,cd->size))!=0){
              if((ptr=GlobalLock(hmem))!=0){
                memcpy(ptr,cd->data,cd->size);
                GlobalUnlock(hmem);
                SetClipboardData(cd->format,hmem);
              }else{
                GlobalFree(hmem);
              }
            }
          }
        }
        CloseClipboard();
        break;
      }
      Sleep(100);
    }
  }
}

bool pasteFromClipboard(char *s, DWORD &len)
{
  HGLOBAL hmem;
  char *ptr;
  bool result=false;
  
  if(OpenClipboard(0)){
    if((hmem= GetClipboardData(isWin9X ? CF_TEXT : CF_UNICODETEXT))!=0){
      if((ptr=(char*)GlobalLock(hmem))!=0){
        if(isWin9X){
          lstrcpyn(s,ptr,len);
        }else{
          s[len-1]=0;
          WideCharToMultiByte(CP_ACP,0,(WCHAR*)ptr,-1,s,len-1,0,0);
        }
        len= strlen(s);
        result=true;
        GlobalUnlock(hmem);
      }
    }
    CloseClipboard();
  }
  return result;
}

void copyToClipboard1(char *s)
{
  HGLOBAL hmem;
  char *ptr;
  DWORD len=strlen(s)+1;
  
  if((hmem=GlobalAlloc(GMEM_DDESHARE, isWin9X ? len : 2*len))!=0){
    if((ptr=(char*)GlobalLock(hmem))!=0){
      if(isWin9X){
        strcpy(ptr,s);
      }else{
        MultiByteToWideChar(CP_ACP,0,s,-1,(WCHAR*)ptr,len);
      }
      GlobalUnlock(hmem);
      SetClipboardData(isWin9X ? CF_TEXT : CF_UNICODETEXT, hmem);
    }else{
      GlobalFree(hmem);
    }
  }
}

char *formatText(char *param)
{
  int i;
  time_t t;
  DWORD n;
  char *buf,*buf2,*s,*d;
  const int M=1024; //max. length

  setlocale(LC_ALL,"");
  i=static_cast<int>(strlen(param))+2*M;
  buf=new char[i];
  for(d=buf,s=param;  ;s++){
    if(int(d-buf)>=i-M){ *d=0; break; } //buffer overflow
    *d=*s;
    if(*s==0) break;
    if(*s=='%'){
      s++;
      switch(*s){
      default:
        d++;
        *d++=*s;
        break;
      case 0:
        s--;
        break;
      case 'r':
        *d++='\r';
        *d++='\n';
        break;
      case 'u':
        n=M;
        if(GetUserName(d,&n)) d+=n-1;
        break;
      case 'o':
        n=M;
        if(GetComputerName(d,&n)) d+=n;
        break;
      case 'l':
        n=M;
        if(pasteFromClipboard(d,n)) d+=n;
        break;
      }
    }else{
      d++;
    }
  }
  i+=512;
  buf2=new char[i];
  time(&t);
  if(!strftime(buf2,i,buf,localtime(&t))){
    //error
    delete[] buf2;
    return buf;
  }
  delete[] buf;
  return buf2;
}

INT_PTR CALLBACK pasteTextProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM )
{
  int i,w;
  char *s;
  HWND list;

  switch(msg){
  case WM_INITDIALOG:
    SetWindowText(hWnd,lng(1067,"Paste text"));
    //add items to the listbox
    list = GetDlgItem(hWnd,157);
    w=10;
    for(i=0; i<pasteTextData.n; i++){
      s= pasteTextData.L[i];
      aminmax(w,strlen(s),150);
      SendMessage(list, LB_ADDSTRING,0, (LPARAM)s);
    }
    //select the first item
    SendMessage(list, LB_SETCURSEL, 0,0);
    //resize listbox and dialog box
    w *= 9;
    i *= static_cast<int>(SendMessage(list,LB_GETITEMHEIGHT,0,0));
    SetWindowPos(hWnd,HWND_TOPMOST,
      max(0,(GetSystemMetrics(SM_CXSCREEN)-w)>>1),
      (GetSystemMetrics(SM_CYSCREEN)-i)>>1,
      w+2,i+2,0);
    SetWindowPos(list,0,0,0,w,i,SWP_NOZORDER|SWP_NOMOVE);
    SetForegroundWindow(hWnd);
    return TRUE;
    
  case WM_COMMAND:
    switch(LOWORD(wP)){
    case 157:
      if(HIWORD(wP)!=LBN_DBLCLK) break;
    case IDOK:
      EndDialog(hWnd,SendDlgItemMessage(hWnd,157, LB_GETCURSEL,0,0));
      return TRUE;
    case IDCANCEL:
      EndDialog(hWnd,-1);
      return TRUE;
    }
    break;
  }
  return 0;
}

HWND GetFocusGlobal()
{
  GUITHREADINFO gui;
  gui.cbSize = sizeof(GUITHREADINFO);
  TGetGUIThreadInfo p= (TGetGUIThreadInfo) GetProcAddress(GetModuleHandle("user32.dll"),"GetGUIThreadInfo");
  if(p && p(0,&gui)) return gui.hwndFocus;
  return 0;
}

void pasteText(char *param)
{
  int i,n,sel;
  char *s,*e,*t;
  DWORD editSelPos= (DWORD)-1;
  DWORD_PTR p;
  TSendMessageTimeout f=0;

  if(lockPaste) return;
  lockPaste++;
  //split param to items
  n=0;
  for(s=param; ;s=e+2){
    e=strstr(s,"%|");
    while(e){
      for(i=0, t=e; t>=param && *t=='%'; i++, t--) ;
      if(i&1) break;
      e=strstr(e+2,"%|"); 
    }
    if(e) *e=0;
    pasteTextData.L[n++] = formatText(s);
    if(!e) break;
    *e='%';
  }
  //show dialog box for multiple texts
  sel=0;
  if(n>1){
    //remember keyboard focus
    HWND focus= GetFocusGlobal();
    if(focus){
      f = IsWindowUnicode(focus) ? SendMessageTimeoutW : SendMessageTimeoutA;
      if(f(focus,EM_GETLINECOUNT,0,0,SMTO_BLOCK,5000,&p) && p>0){ 
        editSelPos = SendMessage(focus,EM_GETSEL,0,0); //SendMessageTimeout returns edit box length of Unicode edit boxes
      }
    }
    //show dialog box
    pasteTextData.n=n;
    sel= DialogBox(inst,"LIST",0,pasteTextProc);
    if(editSelPos!=-1){
      //restore keyboard focus
      DWORD tid = GetWindowThreadProcessId(focus,0);
      AttachThreadInput(GetCurrentThreadId(),tid,TRUE);
      SetFocus(focus);
      AttachThreadInput(GetCurrentThreadId(),tid,FALSE);
      //restore EditBox selected position
      f(focus,EM_SETSEL,
        (short)LOWORD(editSelPos),(short)HIWORD(editSelPos),SMTO_BLOCK,15000,&p);
    }
  }
  if(sel>=0){
    //get selected item
    delete[] pasteTextData.text;
    pasteTextData.text = pasteTextData.L[sel];
    pasteTextData.L[sel]=0;
    for(i=0; i<n; i++)  delete[] pasteTextData.L[i];
    //remember clipboard content
    pasteTextData.save();
    //paste text
    if(OpenClipboard(hWin)){
      if(EmptyClipboard()){
        pasteTextData.busy=true;
        //WM_RENDERFORMAT will set clipboard data
        SetClipboardData(isWin9X ? CF_TEXT : CF_UNICODETEXT, NULL);
        //Ctrl+V
        lockPaste++;
        SetTimer(hWin,14,10,0);
      }
      CloseClipboard();
    }
  }
  lockPaste--;
}

void wndInfo(HWND w)
{
 DWORD pid,d;
 int i;
 HANDLE h;
 HMODULE psapi;
 HMODULE n;
 TmemInfo getMemInfo;
 PROCESS_MEMORY_COUNTERS m;
 PROCESSENTRY32 pe;
 MODULEENTRY32 me;
 char *p,*e,*o,t[128],c[128];

 GetWindowText(w,t,sizeof(t));
 GetClassName(w,c,sizeof(c));
 GetWindowThreadProcessId(w,&pid);
 n=(HMODULE)GetWindowLongPtr(w,GWLP_HINSTANCE);
 e="";
 pe.dwSize = sizeof(PROCESSENTRY32);
 h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
 if(h!=(HANDLE)-1){
   Process32First(h,&pe);
   do{
     if(pe.th32ProcessID==pid){ e=pe.szExeFile; break; }
   }while(Process32Next(h,&pe));
   CloseHandle(h);
 }
 o="";
 me.dwSize = sizeof(MODULEENTRY32);
 h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pid);
 if(h!=(HANDLE)-1){
   Module32First(h,&me);
   do{
     if(me.hModule==n){ o=me.szExePath; break; }
   }while(Module32Next(h,&me));
   CloseHandle(h);
 }
 
 p="";
 m.WorkingSetSize=0;
 if((h=OpenProcess(PROCESS_QUERY_INFORMATION,0,pid))!=0){
   d=GetPriorityClass(h);
   if(!isWin9X){
     psapi=LoadLibrary("psapi.dll");
     getMemInfo= (TmemInfo)GetProcAddress(psapi,"GetProcessMemoryInfo");
     if(getMemInfo){
       getMemInfo(h,&m,sizeof(PROCESS_MEMORY_COUNTERS));
     }
   }
   CloseHandle(h);
   for(i=0; i<sizeA(priorCnst); i++){
     if(priorCnst[i]==d) p=lng(700+i,priorTab[i]);
   }
 }            
 msg1(MB_OK,lng(759,"Title: %s\r\nWindow Class: %s\r\nProcess: %s\r\nModule: %s\r\nPriority: %s\r\nMemory: %d KB"),
   t,c,e,o,p,m.WorkingSetSize/1024);
}

//Windows 7 and later have "show desktop" button (next to the clock) which activates hidden WorkerW window
HWND fixWorkerW(HWND w)
{
 char c[32];
 HWND w2;

 if(GetClassName(w,c,sizeof(c))>0 && !strcmp(c,"WorkerW"))
 {
   w2=FindWindow("Progman","Program Manager");
   if(w2)
   {
     if(GetWindowThreadProcessId(w,0) == GetWindowThreadProcessId(w2,0))
       return w2;
   }
 }
 return w;
}

void click(DWORD down, DWORD up)
{
 forceNoShift();
 mouse_event1(down);
 mouse_event1(up);
 restoreShift();
}

bool noCmdLine(char *param)
{
  if(cmdLineCmd<0) return false;
  HWND w= FindWindow("PlasHotKey",0);
  if(w){
    COPYDATASTRUCT d;
    d.lpData= param;
    d.cbData= strlen(param)+1;
    d.dwData= cmdLineCmd+13000; 
    SendMessage(w, WM_COPYDATA, (WPARAM)w, (LPARAM) &d);
  }else{
    msg("This command cannot be executed, because HotkeyP.exe is not running");
  }
  return true;
}
//-------------------------------------------------------------------------
void command(int cmd, char *param, HotKey *hk)
{
 HWND w;
 char *param2,*s,*buf;
 int iparam= strToIntStr(param,&param2);
 int vol= iparam ? iparam : 1;
 int i,j;
 HANDLE h;
 HKEY key;
 bool b,*pb;
 DWORD d,pid;
 BOOL ok=FALSE;

 switch(cmd){
  case 0: case 1: case 39: case 40: case 41: case 42: case 62:
   audioCD(param,cmd);
  break;
  case 2: //shut down
   privilege();                        
   closeAll(iparam);
   if(isWin9X || !ExitWindowsEx(EWX_POWEROFF, iparam==1 ? TRUE:FALSE)) //Win2000
     ExitWindowsEx(EWX_SHUTDOWN, iparam==1 ? TRUE:FALSE);   //Win98
  break;
  case 3: //restart
   privilege();
   closeAll(iparam);
   ExitWindowsEx(EWX_REBOOT, iparam==1 ? TRUE:FALSE);
  break;
  case 4: //suspend
   privilege();
   SetSystemPowerState(TRUE, iparam==1 ? TRUE:FALSE);
  break;
  case 5: //log off
   closeAll(iparam);
   ExitWindowsEx(EWX_LOGOFF, iparam==1 ? TRUE:FALSE);
  break;
  case 6: //screen saver
   PostMessage(hWin, WM_SYSCOMMAND, SC_SCREENSAVE, 0);
  break;
  case 7: //maximize
   w=getWindow(param);
   if(w) PostMessage(w,WM_SYSCOMMAND, IsZoomed(w)? SC_RESTORE:SC_MAXIMIZE,0);
  break;
  case 8: //minimize
   w=getWindow(param);
   if(w) PostMessage(w,WM_SYSCOMMAND, IsIconic(w)? SC_RESTORE:SC_MINIMIZE,0);
  break;
  case 9: //close
   PostMessage(fixWorkerW(getWindow(param)), WM_CLOSE, 0,0);
  break;
  case 10: //always on top
   w= getWindow(param);
   SetWindowPos(w,
     GetWindowLong(w,GWL_EXSTYLE)&WS_EX_TOPMOST ?
     HWND_NOTOPMOST:HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_ASYNCWINDOWPOS);
  break;
  case 11: //kill process
   w=getWindow(param);
   if(w){
     if(GetWindowThreadProcessId(w,&pid)){
       if((h=OpenProcess(PROCESS_TERMINATE,0,pid))!=0){
         ok= TerminateProcess(h,(UINT)-1);
         CloseHandle(h);
       }
     }
     if(!ok) msglng(751,"Can't terminate process");
   }
  break;
  case 12: //control panels
  case 50: case 51: case 52: case 53: case 54:
  case 55: case 56: case 57:  
   buf= new char[strlen(param)+64];
   strcpy(buf,"rundll32.exe shell32.dll,Control_RunDLL ");
   if(!*param && cmd!=12){
     param=controlPanels[cmd-50];
   }
   if(*param){
     strcat(buf,param);
     strcat(buf,".cpl");
   }
   WinExec(buf,SW_NORMAL);
   delete[] buf;
  break;
  case 13:
   volumeCmd(param2, vol, 0);
  break;
  case 14:
   volumeCmd(param2, -vol, 0);
  break;
  case 15:
   volumeCmd("Wave", vol, 0);
  break;
  case 16:
   volumeCmd("Wave", -vol, 0);
  break;
  case 17: //mute
   volumeCmd(param2, 1, 1);
  break;
  case 18: //desktop size
   resolution(param);
  break;
  case 19: //monitor power off
   if(!waitWhileKey(cmd,param)) break;
   if(!pcLocked) GetCursorPos(&mousePos);
   w=hWin;
   if(!w) w=FindWindow("Shell_TrayWnd",0);
   DefWindowProc(w,WM_SYSCOMMAND,SC_MONITORPOWER,*param ? iparam:2);
   SetTimer(hWin,2,100,0);
  break;
  case 20: //process priority
   if(!*param) iparam=1;
   for(i=0; i<Npriority; i++){
     if(!_stricmp(priorTab[i],param)) iparam=i;
   }
   priority(iparam);
  break;
  case 22: //empty recycle bin
  {
   HINSTANCE lib = LoadLibrary("shell32.dll");
   if(lib){
     typedef int (__stdcall *TemptyBin)(HWND,LPCTSTR,WORD);
         TemptyBin emptyBin = (TemptyBin) GetProcAddress(lib, "SHEmptyRecycleBinA");
     if(emptyBin) emptyBin(hWin,"", (WORD) iparam);
     FreeLibrary(lib);
   }
  }
  break;
  case 23: //random wallpaper
  case 82: 
   changeWallpaper(param,0);
  break;
  case 24: //disk free space
   aminmax(diskfreePrec,0,6);
   diskFree();
   popupDiskFree.delay= iparam ? iparam:6000;
   popupDiskFree.show();
  break;
  case 25: //hide window
   if(hiddenWin){
     ShowWindow(hiddenWin,SW_SHOW);
     SetForegroundWindow(hiddenWin);
     hiddenWin=0;
   }else{
     w=getWindow(param);
     if(!*param) hiddenWin=w;
     ShowWindow(w, IsWindowVisible(w)? SW_HIDE:SW_SHOW);
   }
  break;
  case 94: //macro to active window
  case 904://down
  case 906://up
  case 26: //keys to another window
  case 74: //keys to active window
  case 21: //command to another window
  case 73: //command to active window
   cmdToWindow(cmd,param);
  break;
  case 27: //macro
   parseMacro(param);
  break;
  case 28: //center window
   moveCmd(param,3,3);
  break;
  case 29:
   moveCmd(param,1,2);
  break;
  case 30:
   moveCmd(param,0,2);
  break;
  case 31:
   moveCmd(param,2,2);
  break;
  case 32:
   moveCmd(param,1,0);
  break;
  case 33:
   moveCmd(param,2,0);
  break;
  case 34:
   moveCmd(param,1,1);
  break;
  case 35:
   moveCmd(param,0,1);
  break;
  case 36:
   moveCmd(param,2,1);
  break;
  case 37: //move mouse cursor
  {
   int x,y;
   if(sscanf(param,"%d %d", &x,&y)!=2){
     mouse_event(MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE,
       32767,32767,0,0);
   }else{
     mouse_event(MOUSEEVENTF_MOVE,x,y,0,0);
   }
  }
  break;
  case 38: //shutdown dialog
   PostMessage(FindWindow("Shell_TrayWnd",0),WM_CLOSE,0,0);
  break;
  case 43:
   click(MOUSEEVENTF_LEFTDOWN,MOUSEEVENTF_LEFTUP);
  break;
  case 44:
   click(MOUSEEVENTF_MIDDLEDOWN,MOUSEEVENTF_MIDDLEUP);
  break;
  case 45:
   click(MOUSEEVENTF_RIGHTDOWN,MOUSEEVENTF_RIGHTUP);
  break;
  case 46: case 47: case 48: case 49: 
   priority(cmd-46);
  break;
  case 59: case 60:
   priority(cmd-55);
  break;
  case 58: //mute wave
   volumeCmd("Wave", 1, 1);
  break;
  case 61: //multi command
  case 70: //commands list
   if(hk){ 
     if(hk->lock) return;
     hk->lock++;
   }
   noMsg++;
   for(s=param; ;s++){
     i=strtol(s,&s,10);
     if(!i) break;
     i--;
     if(i<0 || i>=numKeys){
       msglng(758,"Invalid multi-command number");
       break;
     }
     executeHotKey(i);
     if(cmd==70){
       //move the first number to the end of parameter
       char sep=*s;
       if(!sep || !hk) break;
       strcpy(param,s+1);
       s=strchr(param,0);
       *s=sep;
       _itoa(i+1,s+1,10);
       if(*iniFile) wr(iniFile);
       break;
     }
     if(!*s) break;
   }
   if(hk) hk->lock--;
   noMsg--;
   break;
  case 63: //lock computer
   if(!*param){
     TLockWorkStation p= (TLockWorkStation) GetProcAddress(GetModuleHandle("user32.dll"),"LockWorkStation");
     if(p) p();
   }else if(!pcLocked){
     if(noCmdLine(param)) break;
     GetCursorPos(&mousePos);
     pcLocked=true;
     setHook(); 
     if(!hookK){ pcLocked=false; break; }
     cpStr(pcLockParam,param); 
     lockImg=(HGDIOBJ)LoadImage(0,lockBMP,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
     hWndBeforeLock= GetForegroundWindow();
     hWndLock= CreateWindow("HotkeyLock","",WS_POPUP,0,0,10,10,hWin,0,inst,0);
     pcLockX=GetSystemMetrics(SM_CXSCREEN)>>1;
     pcLockY=GetSystemMetrics(SM_CYSCREEN)>>1;
     double alpha=random(10000)*(3.141592654/5000);
     amax(lockSpeed,100);
     pcLockDx=lockSpeed*cos(alpha)/2;
     pcLockDy=lockSpeed*sin(alpha)/2;
     SetTimer(hWin,8,60,0);
     TregisterServiceProcess p= (TregisterServiceProcess)GetProcAddress(GetModuleHandle("kernel32.dll"),"RegisterServiceProcess");
     if(p) p(0,1);
     if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",0,KEY_QUERY_VALUE|KEY_SET_VALUE,&key)==ERROR_SUCCESS){
       d=4;
       RegQueryValueEx(key,"DisableTaskMgr",0,0,(LPBYTE)&disableTaskMgr,&d);
       d=1;
       RegSetValueEx(key,"DisableTaskMgr",0,REG_DWORD,(LPBYTE)&d,4);
       RegCloseKey(key);
     }
     if(lockMute) volume(0,1,3);
     writeini();
   }
  break;
  case 64: //hibernate
  {
    if(!waitWhileKey(cmd,param)) break;
    isHilited=false;
    modifyTrayIcon();
    HMODULE l= LoadLibrary("powrprof.dll");
    if(l){
      TSetSuspendState p;
      p= (TSetSuspendState) GetProcAddress(l,"SetSuspendState");
      if(p) p(TRUE, iparam==1 ? TRUE:FALSE, FALSE);
      FreeLibrary(l);
    }
  }
  break;
  case 65: //minimize others
    noMinimWnd=GetForegroundWindow();
    for(;;){
      w=GetWindow(noMinimWnd,GW_OWNER);
      if(!w) break;
      noMinimWnd=w;
    }
    EnumWindows(enumMinimize,0);
  break;
  case 66: //information about an active window
    w=getActiveWindow();
    if(w) wndInfo(w);
  break;
  case 67: //paste text, date, time, user name, computer name
    pasteText(param);
  break;
  case 68: //next wallpaper
   changeWallpaper(param,1);
  break;
  case 69: //previous wallpaper
   changeWallpaper(param,2);
  break;
  case 71: //start moving a window
  case 72: //start sizing a window
   w=getWindow(param);
   if(w) PostMessage(w,WM_SYSCOMMAND, cmd==71 ? SC_MOVE:SC_SIZE,0);
  break;
  case 75: //mouse wheel
  case 108://mouse horizontal wheel
   forceNoShift();
   mouse_event2(cmd==75 ? MOUSEEVENTF_WHEEL : MOUSEEVENTF_HWHEEL, iparam ? iparam : -WHEEL_DELTA);
   restoreShift();
  break;
  case 76: //double click
   doubleClick();
  break;
  case 77: //window opacity
   setOpacity(getWindow(param2),iparam);
  break;
  case 78: //volume
   volumeCmd(param2, param!=param2 ? iparam : 50, 2);
  break;
  case 79: //mouse button4
  case 80: //mouse button5
   forceNoShift();
   mouse_event2(MOUSEEVENTF_XDOWN,cmd-78);
   mouse_event2(MOUSEEVENTF_XUP,cmd-78);
   restoreShift();
  break;
  case 81: //show desktop
   parseMacro("\\rep\\wind");
  break;
  case 83: //previous task
  case 84: //next task
   if(!altDown){
     altDown=true;
     keyEventDown(VK_MENU);
   }
   if(cmd==83) keyEventDown(VK_SHIFT);
   keyEventDown(VK_TAB);
   keyEventUp(VK_TAB);
   if(cmd==83) keyEventUp(VK_SHIFT);
   if(!hookM || buttons==-1){
     altDown=false;
     keyEventUp(VK_MENU);
   }
  break;
  case 85: //show text
   if(IsWindowVisible(popupShowText.hWnd) && 
     showTextLast && !strcmp(showTextLast,param)){
     //hide popup window
     PostMessage(hWin,WM_TIMER,50+P_ShowText,0);
   }else{
     cpStr(showTextLast,param);
     if(iparam<100){
       iparam=6000;
     }else{
       param=param2;
     }
     //get text
     delete[] showTextStr;
     showTextStr= formatText(param);
     //calculate window size
     RECT rc;
     rc.left=rc.top=0;
     HDC dc=GetDC(popupShowText.hWnd);
     DrawText(dc,showTextStr,-1,&rc,DT_CALCRECT|DT_NOPREFIX|DT_NOCLIP);
     ReleaseDC(popupShowText.hWnd,dc);
     popupShowText.width=rc.right+showTextBorder*2;
     popupShowText.height=rc.bottom+showTextBorder*2;
     //show popup window
     popupShowText.delay=iparam;
     popupShowText.show();
   }
  break;
  case 86: //disable keys
   b=false;
   for(j=0; j<2; j++){
     for(s=param; *s; ){
       int dummy;
       switch(parseKey(s,i,dummy)){
       case 0: 
         return;
       case 1: case 2: 
         {
           bool &k= blockedKeys[LOBYTE(i)];
           if(!j) b|=k;
           else k=!b;
         }
       break;
       default: 
         s++;
       }
     }
   }
   setHook();
  break;
  case 87: //disable/enable all hotkeys
   pb= &disableAll;
ldisable:
   if(noCmdLine(param)) break;
   unregisterKeys();
   if(param==param2) *pb=!*pb;
   else *pb=(iparam!=0);
   registerKeys();
   setHook();
   modifyTrayIcon();
  break;
  case 88: //disable/enable mouse hotkeys
   pb= &disableMouse;
   goto ldisable;
  case 89: //desktop screenshot
   screenshot(GetDesktopWindow());
  break;
  case 90: //window screenshot
   screenshot(GetForegroundWindow());
  break;
  case 91: //hide systray icon
  case 98:
   param2=0;
   cpStr(param2,param);
   createThread(hideIconProc,param2);
  break;
  case 92: //stop service
  case 93: //start service
  {
    if(isWin9X) break;
    SC_HANDLE schSCManager = OpenSCManager(0,0,SC_MANAGER_CONNECT);  
    if(!schSCManager) msg("Cannot open SC manager");
    else{
      SC_HANDLE schService = OpenService(schSCManager, param, cmd==93 ? SERVICE_START : SERVICE_STOP); 
      if(!schService) msg("Cannot open service %s",param);
      else{
        if(cmd==93){
          if(!StartService(schService,0,0)){
            d=GetLastError();
            if(d!=1056) msg("Cannot start service (error %u)",d);
          }
        }else{
          SERVICE_STATUS s;
          if(!ControlService(schService,SERVICE_CONTROL_STOP,&s)){
            d=GetLastError();
            if(d!=1062) msg("Cannot stop service (error %u)",d);
          }
        }
        CloseServiceHandle(schService); 
      }
      CloseServiceHandle(schSCManager);
    }
  }
  break;
  case 95: //disable/enable joystick hotkeys
   pb= &disableJoystick;
   goto ldisable;
  case 96: //disable/enable remote control hotkeys
   pb= &disableLirc;
   goto ldisable;
  case 97: //disable/enable keyboard hotkeys
   pb= &disableKeys;
   goto ldisable;
  case 99: //restore tray icon
   param2=0;
   cpStr(param2,param);
   createThread(restoreIconProc,param2);
   break;
  case 100: //change CD speed 
   audioCD(param2,cmd,iparam);
   break;
  case 101: //hide application
   if(hiddenApp.list){
     unhideApp(&hiddenApp);
   }else{
     hideApp(getWindow(param), &hiddenApp);
   }
  break;
  case 102: //minimize to system tray
   if(noCmdLine(param)) break;
   w=getWindow(param);
   if(w==hWin && trayicon){
     hideMainWindow();
   }else if(w && IsWindowVisible(w)){
     char buf[64];
     GetWindowText(w,buf,64);
     for(i=0; i<sizeA(trayIconA); i++){
       HideInfo *h = &trayIconA[i];
       if(!h->pid){
         hideApp(w,h);
         getExeIcon(h);
         addTrayIcon(buf, h->icon, 100+i);
         break;
       }
     }
   }
  break;
  case 103: //magnifier
   if(noCmdLine(param)) break;
   if(isZoom){
     zoom.end();
   }else{
     int i= strToIntStr(param2,&param);
     int j= strToIntStr(param,&param2);
     zoom.magnification= iparam ? iparam : 2;
     zoom.width= i ? i+2 : 142;
     zoom.height= j ? j+2 : zoom.width;
     zoom.start();
   }
  break;
  case 104: //clear recent documents
   SHAddToRecentDocs(0,0);
  break;
  case 105: //delete temporary files
   if(GetTempPath(sizeA(exeBuf),exeBuf)){
     if(strlen(exeBuf)>4){
       FILETIME f;
       SYSTEMTIME s;
       GetSystemTime(&s);
       SystemTimeToFileTime(&s,&f);
       *(LONGLONG*)&f -= (iparam ? iparam : 7) *(24*3600*(LONGLONG)10000000);
       deleteTemp(exeBuf,&f);
     }
   }
  break;
  case 106: //save desktop icons
    saveDesktopIcons(0);
  break;
  case 107: //restore desktop icons
    saveDesktopIcons(1);
  break;
  case 109: //remove drive
    if(*param) removeDrive(param[0]);
    else removeUSBdrives();
  break;
 }
}
//---------------------------------------------------------------------------
