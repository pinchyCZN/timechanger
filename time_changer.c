#include <winsock2.h>
#include <commctrl.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
void open_console()
{
	char title[MAX_PATH]={0};
	HWND hcon;
	FILE *hf;
	static BYTE consolecreated=FALSE;
	static int hcrt=0;

	if(consolecreated==TRUE)
	{
		GetConsoleTitle(title,sizeof(title));
		if(title[0]!=0){
			hcon=FindWindow(NULL,title);
			ShowWindow(hcon,SW_SHOW);
		}
		hcon=(HWND)GetStdHandle(STD_INPUT_HANDLE);
		FlushConsoleInputBuffer(hcon);
		return;
	}
	AllocConsole();
	hcrt=_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),0x4000);

	fflush(stdin);
	hf=_fdopen(hcrt,"w");
	*stdout=*hf;
	setvbuf(stdout,NULL,_IONBF,0);
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_SHOW);
		SetForegroundWindow(hcon);
	}
	consolecreated=TRUE;
}


int get_month_int(char *str)
{
	char monstr[80];
	strcpy(monstr,str);
	strupr(monstr);
	if(strstr(monstr,"JAN"))
		return 1;
	if(strstr(monstr,"FEB"))
		return 2;
	if(strstr(monstr,"MAR"))
		return 3;
	if(strstr(monstr,"APR"))
		return 4;
	if(strstr(monstr,"MAY"))
		return 5;
	if(strstr(monstr,"JUN"))
		return 6;
	if(strstr(monstr,"JUL"))
		return 7;
	if(strstr(monstr,"AUG"))
		return 8;
	if(strstr(monstr,"SEP"))
		return 9;
	if(strstr(monstr,"OCT"))
		return 10;
	if(strstr(monstr,"NOV"))
		return 11;
	if(strstr(monstr,"DEC"))
		return 12;
	return 0;
}
int get_time_daytime_protocol(char *timeserver,int port,int update)
{
	int i;
	SYSTEMTIME time;
	int hour,min,sec,day,year,month_int=0;
	char dayofweek[20],month[20];
	WSADATA ws;
	int tcp_socket;
	struct sockaddr_in peer;
	char recvbuffer[255];

	if(WSAStartup(0x0101,&ws)!=0)
	{
		printf("unable to WSAStartup()\n");
		return FALSE;
	}

	peer.sin_family = AF_INET;
	peer.sin_port = htons((u_short)port);
	peer.sin_addr.s_addr = inet_addr(timeserver);

	tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	printf("connecting to %s port:%i\n",timeserver,port);
	if(connect(tcp_socket, (struct sockaddr*)&peer, sizeof(peer)) != 0)
	{
		closesocket(tcp_socket);
		printf("unable to connect\n");
		return FALSE;
	}

	memset(recvbuffer,0,sizeof(recvbuffer));
	recv(tcp_socket, recvbuffer, sizeof(recvbuffer),0); //format Fri Feb   6 16:40:28 2009
	closesocket(tcp_socket);

	printf("Response:  %s\n",recvbuffer);
	if(strstr(recvbuffer,"NIST")!=0) //NIST format: JJJJJ YR-MO-DA HH:MM:SS TT L H msADV UTC(NIST) OTM.
	{
		if(6!=sscanf(recvbuffer,"%*s %2d-%2d-%2d %2d:%2d:%2d",&year,&month_int,&day,&hour,&min,&sec))
			printf("sscanf error\n");
		year=year+2000;
		printf("scanned as %4d-%02d-%02d %02d:%02d:%02d\n",year,month_int,day,hour,min,sec);
	}
	else
	{
		if(7!=sscanf(recvbuffer,"%s %s %d %2d:%2d:%2d %d",dayofweek,month,&day,&hour,&min,&sec,&year))
			printf("sscanf error\n");
		printf("scanned as %s %s  %i %02d:%02d:%02d %d\n",dayofweek,month,day,hour,min,sec,year);
	}

	memset(&time,0,sizeof(time));
	GetSystemTime(&time);

	printf("old time %02i:%02i:%02i  %02i/%02i/%04i\n",time.wHour,time.wMinute,time.wSecond,
		time.wMonth,time.wDay,time.wYear);
	time.wHour=hour;
	time.wMinute=min;
	time.wSecond=sec;
	time.wYear=year;
	time.wDay=day;
	if(month_int!=0)
		time.wMonth=month_int;
	else
	{
		i=get_month_int(month);
		if(i==0)
			printf("error determining month string, no change to month made\n");
		else
			time.wMonth=i;
	}
	printf("new time %02i:%02i:%02i  %02i/%02i/%04i\n",time.wHour,time.wMinute,time.wSecond,
		time.wMonth,time.wDay,time.wYear);

	if(update)
	{
		if(SetSystemTime(&time)!=0)
		{
			printf("time updated\n");
			return TRUE;
		}
		else
		{
			printf("time update failed\n");
			return FALSE;
		}
	}
	return TRUE;
}
enum{
	CMD_RESETTIME,
};
void __cdecl thread(void *args[])
{
	HANDLE event=0;
	HWND hwnd=0;
	int *command=0;
	event=args[0];
	command=args[1];
	hwnd=args[3];
	if(event==0 || command==0){
		MessageBox(0,"null pointer passed to thread","error",MB_OK|MB_SYSTEMMODAL);
		return;
	}
	while(TRUE){
		int id;
		id=WaitForSingleObject(event,INFINITE);
		if(id==WAIT_OBJECT_0){
			switch(*command){
			case CMD_RESETTIME:
				get_time_daytime_protocol("70.84.194.243",1313,TRUE);
				if(hwnd)
					PostMessage(hwnd,WM_APP,0,0);
				break;
			}
		}
		ResetEvent(event);
	}
}
int update_time(int h,int m,int s)
{
	SYSTEMTIME time;
	GetSystemTime(&time);
	time.wHour=h;
	time.wMinute=m;
	time.wSecond=s;
	SetSystemTime(&time);
	return 0;
}
int update_controls(HWND hwnd)
{
	char time[80];
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	GetTimeFormat(LOCALE_USER_DEFAULT,0,&systime,"HH':'mm':'ss",time,sizeof(time));
	SetDlgItemText(hwnd,IDC_CURRENT_TIME,time);
	SendDlgItemMessage(hwnd,IDC_HOUR,TBM_SETPOS,TRUE,systime.wHour);
	SendDlgItemMessage(hwnd,IDC_MINUTE,TBM_SETPOS,TRUE,systime.wMinute);
	SendDlgItemMessage(hwnd,IDC_SECOND,TBM_SETPOS,TRUE,systime.wSecond);
	return 0;
}
int get_last_error()
{
	char str[512];
	DWORD dw = GetLastError(); 
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        str,sizeof(str),NULL);
	return printf("msg(0x%08X,%i)=%s\n",dw,dw,str);
}
LRESULT CALLBACK dialogproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
	static int thread_task;
	static HANDLE event;
	static int tracking=FALSE;


	if(FALSE)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NCMOUSEMOVE&&msg!=WM_CTLCOLORSTATIC
		&&msg!=WM_TIMER)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}


	switch(msg){
	case WM_INITDIALOG:
		SendDlgItemMessage(hwnd,IDC_HOUR,TBM_SETRANGE,TRUE,MAKELONG(0,23));
		SendDlgItemMessage(hwnd,IDC_MINUTE,TBM_SETRANGE,TRUE,MAKELONG(0,59));
		SendDlgItemMessage(hwnd,IDC_SECOND,TBM_SETRANGE,TRUE,MAKELONG(0,59));
		{
			static void *list[3];
			event=CreateEvent(NULL,TRUE,FALSE,"thread");
			list[0]=event;
			list[1]=&thread_task;
			//list[3]=hwnd;
			ResetEvent(event);
			//_beginthread(thread,0,list);
		}
		PostMessage(hwnd,WM_APP,0,0);
		SetTimer(hwnd,1337,1000,0);
		break;
	case WM_TIMER:
		if(tracking)
			break;
		update_controls(hwnd);
		break;
	case WM_HSCROLL:
		{
			int update=FALSE;
			switch LOWORD(wparam){
			case SB_THUMBPOSITION:
				update=TRUE;
				break;
			case SB_THUMBTRACK:
				tracking=TRUE;
				update=TRUE;
				break;
			case SB_ENDSCROLL:
				tracking=FALSE;
				update=TRUE;
				break;
			}
			if(update){
				int h,m,s;
				h=SendDlgItemMessage(hwnd,IDC_HOUR,TBM_GETPOS,0,0);
				m=SendDlgItemMessage(hwnd,IDC_MINUTE,TBM_GETPOS,0,0);
				s=SendDlgItemMessage(hwnd,IDC_SECOND,TBM_GETPOS,0,0);
				update_time(h,m,s);
				PostMessage(hwnd,WM_APP,0,0);
				printf("updated time %02i:%02i:%02i\n",h,m,s);
			}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		case IDC_RESETTIME:
			thread_task=CMD_RESETTIME;
			{
				int i=SetEvent(event);
				get_last_error();
				//get_time_daytime_protocol("70.84.194.243",1313,TRUE);

			}
			break;
		case IDC_HOUR:
			{
				int i;
				i=i;
			}
			break;
		case IDC_MINUTE:
			break;
		case IDC_SECOND:
			break;

		}
		break;
	case WM_APP:
		update_controls(hwnd);
		switch(lparam){
		default:
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hwnd,0);
		break;
	}
	return 0;
}
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	open_console();
	DialogBox(hInstance,IDD_DIALOG1,NULL,dialogproc);
	return 0;
}
