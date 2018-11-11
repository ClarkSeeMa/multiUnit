// ss.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <vfw.h>
#include<io.h>
#pragma comment(lib,"vfw32.lib")

HWND ghWndCap ; //���񴰵ľ�� 
CAPDRIVERCAPS gCapDriverCaps ; //��Ƶ������������ 
CAPSTATUS gCapStatus ; //���񴰵�״̬ 
char szCaptureFile[] = "MYCAP.AVI";
char gachBuffer[20];
bool create=false;
int i=0;
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK StatusCallbackProc(HWND hWnd,int nID,LPSTR lpStatusText)
{
if(!ghWndCap)return FALSE;//��ò��񴰵�״̬
capGetStatus(ghWndCap,&gCapStatus,sizeof(CAPSTATUS));//���²��񴰵Ĵ�С
SetWindowPos(ghWndCap,NULL,0,0,gCapStatus.uiImageWidth,gCapStatus.uiImageHeight,SWP_NOZORDER|SWP_NOMOVE);
if(nID==0){//����ɵ�״̬��Ϣ
SetWindowText(ghWndCap,(LPSTR)"hello");
return (LRESULT)TRUE;
}//��ʾ״̬ID��״̬�ı�
wsprintf(gachBuffer,"Status# %d: %s",nID,lpStatusText);
SetWindowText(ghWndCap,(LPSTR)gachBuffer);
return (LRESULT)TRUE;
}
LRESULT CALLBACK ErrorCallbackProc(HWND hWnd,int nErrID,LPSTR lpErrorText)
{

if(!ghWndCap)return FALSE;
    if(nErrID==0)return TRUE;//����ɵĴ���
wsprintf(gachBuffer,"Error# %d",nErrID);//��ʾ�����ʶ���ı�
MessageBox(hWnd, lpErrorText, gachBuffer,MB_OK | MB_ICONEXCLAMATION); 
return (LRESULT) TRUE;
} 

LRESULT CALLBACK FrameCallbackProc(HWND hWnd,LPVIDEOHDR lpVHdr)
{
/*
FILE *fp;
fp=fopen("caram.dat","w");
if(!ghWndCap)return FALSE;//����fpΪһ�򿪵�.dat�ļ�ָ��
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
capDriverConnect(ghWndCap,0); // ������ͬ����������  
//���������������,��ص���Ϣ���ڽṹ����gCapDriverCaps��
capDriverGetCaps(ghWndCap,&gCapDriverCaps,sizeof(CAPDRIVERCAPS));
capPreviewRate(ghWndCap, 8); // ����Previewģʽ����ʾ���� 
//capPreview(ghWndCap, TRUE); //����Previewģʽ
if(gCapDriverCaps.fHasOverlay) //����������Ƿ��е������� 
capOverlay(ghWndCap,TRUE); //����Overlayģʽ
if(gCapDriverCaps.fHasDlgVideoSource)capDlgVideoSource(ghWndCap); //Video source �Ի��� 
if(gCapDriverCaps.fHasDlgVideoFormat)capDlgVideoFormat(ghWndCap); // Video format �Ի��� 
if(gCapDriverCaps.fHasDlgVideoDisplay)capDlgVideoDisplay(ghWndCap); // Video display �Ի���
/*
if(_access(szCaptureFile,0)!=-1){
MessageBox(hwnd, "exit", gachBuffer,MB_OK | MB_ICONEXCLAMATION); 
DeleteFile(szCaptureFile);
}*/
//capFileSetCaptureFile( ghWndCap, szCaptureFile); //ָ�������ļ��� 
//capFileAlloc(ghWndCap, (1024L * 1024L * 5)); //Ϊ�����ļ�����洢�ռ� 
//capCaptureSequence(ghWndCap); //��ʼ������Ƶ����
//capGrabFrame(ghWndCap); //����֡ͼ�� 
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
capCaptureAbort(ghWndCap);//ֹͣ���� 
capDriverDisconnect(ghWndCap); //������ͬ�������Ͽ� 
PostQuitMessage(0);
}
return 0;
}
return DefWindowProc(hwnd,message,wParam,lParam);
} 

