// ss.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <vfw.h>
#include<io.h>
#pragma comment(lib,"vfw32.lib")

HWND ghWndCap ; //捕获窗的句柄 
CAPDRIVERCAPS gCapDriverCaps ; //视频驱动器的能力 
CAPSTATUS gCapStatus ; //捕获窗的状态 
char szCaptureFile[] = "MYCAP.AVI";
char gachBuffer[20];
bool create=false;
int i=0;
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK StatusCallbackProc(HWND hWnd,int nID,LPSTR lpStatusText)
{
if(!ghWndCap)return FALSE;//获得捕获窗的状态
capGetStatus(ghWndCap,&gCapStatus,sizeof(CAPSTATUS));//更新捕获窗的大小
SetWindowPos(ghWndCap,NULL,0,0,gCapStatus.uiImageWidth,gCapStatus.uiImageHeight,SWP_NOZORDER|SWP_NOMOVE);
if(nID==0){//清除旧的状态信息
SetWindowText(ghWndCap,(LPSTR)"hello");
return (LRESULT)TRUE;
}//显示状态ID和状态文本
wsprintf(gachBuffer,"Status# %d: %s",nID,lpStatusText);
SetWindowText(ghWndCap,(LPSTR)gachBuffer);
return (LRESULT)TRUE;
}
LRESULT CALLBACK ErrorCallbackProc(HWND hWnd,int nErrID,LPSTR lpErrorText)
{

if(!ghWndCap)return FALSE;
    if(nErrID==0)return TRUE;//清除旧的错误
wsprintf(gachBuffer,"Error# %d",nErrID);//显示错误标识和文本
MessageBox(hWnd, lpErrorText, gachBuffer,MB_OK | MB_ICONEXCLAMATION); 
return (LRESULT) TRUE;
} 

LRESULT CALLBACK FrameCallbackProc(HWND hWnd,LPVIDEOHDR lpVHdr)
{
/*
FILE *fp;
fp=fopen("caram.dat","w");
if(!ghWndCap)return FALSE;//假设fp为一打开的.dat文件指针
fwrite(lpVHdr->lpData,1,lpVHdr->dwBufferLength,fp);*/


return (LRESULT)TRUE;
} 

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,PSTR szCmdLine,int iCmdShow)
{
static TCHAR szAppName[]=TEXT("HelloWin");
HWND hwnd;
MSG msg;
WNDCLASS wndclass;
wndclass.style=CS_HREDRAW|CS_VREDRAW;
wndclass.lpfnWndProc=WndProc;
wndclass.cbClsExtra=0;
wndclass.cbWndExtra=0;
wndclass.hInstance=hInstance;
wndclass.hIcon=LoadIcon(NULL,IDI_APPLICATION);
wndclass.hCursor=LoadCursor(NULL,IDC_ARROW);
wndclass.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
wndclass.lpszMenuName=NULL;
wndclass.lpszClassName=szAppName;
if(!RegisterClass(&wndclass))
{
MessageBox(NULL,TEXT("This program requires WindowsNT!"),szAppName,MB_ICONERROR);
return 0;
}
hwnd=CreateWindow(szAppName,TEXT("The Hello Program"),WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,hInstance,NULL);
ShowWindow(hwnd,iCmdShow);
UpdateWindow(hwnd);
while(GetMessage(&msg,NULL,0,0))
{
TranslateMessage(&msg);
DispatchMessage(&msg);
}
return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
HDC hdc;
PAINTSTRUCT ps;
RECT rect;
switch(message)
{
case WM_CREATE:
	{
if(create==false)
		{
	create=true;
ghWndCap=capCreateCaptureWindow((LPSTR)"Capture  Window",WS_CHILD|WS_VISIBLE,0,0,300,240,(HWND)hwnd,(int)0); 
capSetCallbackOnError(ghWndCap,(FARPROC)ErrorCallbackProc); 
capSetCallbackOnStatus(ghWndCap,(FARPROC)StatusCallbackProc);
capSetCallbackOnFrame(ghWndCap,(FARPROC)FrameCallbackProc);
capDriverConnect(ghWndCap,0); // 将捕获窗同驱动器连接  
//获得驱动器的能力,相关的信息放在结构变量gCapDriverCaps中
capDriverGetCaps(ghWndCap,&gCapDriverCaps,sizeof(CAPDRIVERCAPS));
capPreviewRate(ghWndCap, 8); // 设置Preview模式的显示速率 
//capPreview(ghWndCap, TRUE); //启动Preview模式
if(gCapDriverCaps.fHasOverlay) //检查驱动器是否有叠加能力 
capOverlay(ghWndCap,TRUE); //启动Overlay模式
if(gCapDriverCaps.fHasDlgVideoSource)capDlgVideoSource(ghWndCap); //Video source 对话框 
if(gCapDriverCaps.fHasDlgVideoFormat)capDlgVideoFormat(ghWndCap); // Video format 对话框 
if(gCapDriverCaps.fHasDlgVideoDisplay)capDlgVideoDisplay(ghWndCap); // Video display 对话框
/*
if(_access(szCaptureFile,0)!=-1){
MessageBox(hwnd, "exit", gachBuffer,MB_OK | MB_ICONEXCLAMATION); 
DeleteFile(szCaptureFile);
}*/
//capFileSetCaptureFile( ghWndCap, szCaptureFile); //指定捕获文件名 
//capFileAlloc(ghWndCap, (1024L * 1024L * 5)); //为捕获文件分配存储空间 
//capCaptureSequence(ghWndCap); //开始捕获视频序列
//capGrabFrame(ghWndCap); //捕获单帧图像 
	}
	}

return 0;
case WM_PAINT:
hdc=BeginPaint(hwnd,&ps);
GetClientRect(hwnd,&rect);
DrawText(hdc,TEXT("Hello,Windows98!"),-1,&rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
EndPaint(hwnd,&ps);
return 0;
case WM_DESTROY:
{
capSetCallbackOnStatus(ghWndCap,NULL);
capSetCallbackOnError(ghWndCap,NULL); 
capSetCallbackOnFrame(ghWndCap,NULL);
capCaptureAbort(ghWndCap);//停止捕获 
capDriverDisconnect(ghWndCap); //将捕获窗同驱动器断开 
PostQuitMessage(0);
}
return 0;
}
return DefWindowProc(hwnd,message,wParam,lParam);
} 

