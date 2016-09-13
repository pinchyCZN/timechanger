#include <winsock2.h>
#include <commctrl.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
void hide_console()
{
	char title[MAX_PATH]={0};
	HANDLE hcon;

	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_HIDE);
		SetForegroundWindow(hcon);
	}
}
int move_console(int x,int y)
{
	char title[MAX_PATH]={0};
	HWND hcon;
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		SetWindowPos(hcon,0,x,y,0,0,SWP_NOZORDER|SWP_NOSIZE);
	}
	return 0;
}
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

HANDLE thread_event;
void __cdecl thread(void *args[])
{
	HWND hwnd=0;
	int *command=0;
	command=args[0];
	hwnd=args[1];
	thread_event=CreateEvent(NULL,FALSE,FALSE,"thread_event");

	if(thread_event==0 || command==0){
		MessageBox(0,"null parameters for thread","error",MB_OK|MB_SYSTEMMODAL);
		return;
	}
	while(TRUE){
		int id;
		id=WaitForSingleObject(thread_event,INFINITE);
		if(id==WAIT_OBJECT_0){
			switch(*command){
			case CMD_RESETTIME:
				get_time_daytime_protocol("70.84.194.243",1313,TRUE);
				if(hwnd)
					PostMessage(hwnd,WM_APP,0,0);
				Sleep(1000);
				break;
			}
		}
	}
}
int update_time(int h,int m,int s)
{
	SYSTEMTIME time;
	GetLocalTime(&time);
	time.wHour=h;
	time.wMinute=m;
	time.wSecond=s;
	SetLocalTime(&time);
	return 0;
}
int update_date(int y,int m,int d)
{
	SYSTEMTIME time;
	GetLocalTime(&time);
	time.wYear=y;
	time.wMonth=m;
	time.wDay=d;
	SetLocalTime(&time);
	return 0;
}
int update_controls(HWND hwnd)
{
	char time[20];
	char date[20];
	char str[40];
	SYSTEMTIME systime;
	GetLocalTime(&systime);
	GetTimeFormat(LOCALE_USER_DEFAULT,0,&systime,"HH':'mm':'ss",time,sizeof(time));
	GetDateFormat(LOCALE_USER_DEFAULT,0,&systime,"M'/'d'/'yyyy",date,sizeof(date));
	_snprintf(str,sizeof(str),"%s  -  %s",time,date);
	SetDlgItemText(hwnd,IDC_LOCALTIME,str);
	if(IsDlgButtonChecked(hwnd,IDC_SETDATE)){
		SendDlgItemMessage(hwnd,IDC_HOUR,TBM_SETPOS,TRUE,systime.wYear-1900);
		SendDlgItemMessage(hwnd,IDC_MINUTE,TBM_SETPOS,TRUE,systime.wMonth);
		SendDlgItemMessage(hwnd,IDC_SECOND,TBM_SETPOS,TRUE,systime.wDay);
	}
	else{
		SendDlgItemMessage(hwnd,IDC_HOUR,TBM_SETPOS,TRUE,systime.wHour);
		SendDlgItemMessage(hwnd,IDC_MINUTE,TBM_SETPOS,TRUE,systime.wMinute);
		SendDlgItemMessage(hwnd,IDC_SECOND,TBM_SETPOS,TRUE,systime.wSecond);
	}
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
int set_range(HWND hwnd,int set_date)
{
	if(set_date){
		SendDlgItemMessage(hwnd,IDC_HOUR,TBM_SETRANGE,TRUE,MAKELONG(0,150));
		SendDlgItemMessage(hwnd,IDC_MINUTE,TBM_SETRANGE,TRUE,MAKELONG(0,12));
		SendDlgItemMessage(hwnd,IDC_SECOND,TBM_SETRANGE,TRUE,MAKELONG(0,31));
	}
	else{
		SendDlgItemMessage(hwnd,IDC_HOUR,TBM_SETRANGE,TRUE,MAKELONG(0,23));
		SendDlgItemMessage(hwnd,IDC_MINUTE,TBM_SETRANGE,TRUE,MAKELONG(0,59));
		SendDlgItemMessage(hwnd,IDC_SECOND,TBM_SETRANGE,TRUE,MAKELONG(0,59));
	}
}
LRESULT CALLBACK dialogproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
	static int thread_task;
	static int freeze_timer=FALSE;
	static int tracking=FALSE;
	static SYSTEMTIME current_time;

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
		set_range(hwnd,FALSE);
		{
			static void *list[2];
			list[0]=&thread_task;
			list[1]=hwnd;
			_beginthread(thread,0,list);
		}
		PostMessage(hwnd,WM_APP,0,0);
		SetTimer(hwnd,1337,1000,0);
#ifdef _DEBUG
		{
			RECT rect;
			GetWindowRect(hwnd,&rect);
			open_console();
			move_console(rect.right,rect.top);
		}
#endif;
		break;
	case WM_TIMER:
		if(tracking)
			break;
		if(freeze_timer){
			update_time(current_time.wHour,current_time.wMinute,current_time.wSecond);
		}
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
				if(IsDlgButtonChecked(hwnd,IDC_SETDATE)){
					int y,m,d;
					y=SendDlgItemMessage(hwnd,IDC_HOUR,TBM_GETPOS,0,0);
					m=SendDlgItemMessage(hwnd,IDC_MINUTE,TBM_GETPOS,0,0);
					d=SendDlgItemMessage(hwnd,IDC_SECOND,TBM_GETPOS,0,0);
					y+=1900;
					update_date(y,m,d);
					current_time.wYear=y;
					current_time.wMonth=m;
					current_time.wDay=d;
					PostMessage(hwnd,WM_APP,0,0);
					printf("updated date %04i:%02i:%02i\n",y,m,d);
				}
				else{
					int h,m,s;
					h=SendDlgItemMessage(hwnd,IDC_HOUR,TBM_GETPOS,0,0);
					m=SendDlgItemMessage(hwnd,IDC_MINUTE,TBM_GETPOS,0,0);
					s=SendDlgItemMessage(hwnd,IDC_SECOND,TBM_GETPOS,0,0);
					update_time(h,m,s);
					current_time.wHour=h;
					current_time.wMinute=m;
					current_time.wSecond=s;
					PostMessage(hwnd,WM_APP,0,0);
					printf("updated time %02i:%02i:%02i\n",h,m,s);
				}
			}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		case IDC_SETDATE:
			{
				int i;
				static struct CONTROL_TEXT{
					int control;char *text_time;char *text_date;
				}control_text[]={
					{IDC_TXTHOUR_YEAR,"Hour","Year"},
					{IDC_TXTMIN_MONTH,"Minute","Month"},
					{IDC_TXTSECOND_DAY,"Second","Day"}};
				
				for(i=0;i<sizeof(control_text)/sizeof(struct CONTROL_TEXT);i++){
					char *s=control_text[i].text_time;
					if(IsDlgButtonChecked(hwnd,IDC_SETDATE))
						s=control_text[i].text_date;
					SetDlgItemText(hwnd,control_text[i].control,s);
				}
				set_range(hwnd,IsDlgButtonChecked(hwnd,IDC_SETDATE));
				update_controls(hwnd);
			}
			break;
		case IDC_RESETTIME:
			thread_task=CMD_RESETTIME;
			if(thread_event)
				SetEvent(thread_event);
			break;
		case IDC_HOUR:
			break;
		case IDC_MINUTE:
			break;
		case IDC_SECOND:
			break;
		case IDC_TOP:
			SetWindowPos( hwnd, IsDlgButtonChecked( hwnd, LOWORD(wparam) )? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
			break;
		case IDC_FREEZE:
			if(IsDlgButtonChecked(hwnd,LOWORD(wparam))){
				freeze_timer=TRUE;
				GetLocalTime(&current_time);
			}
			else
				freeze_timer=FALSE;
			break;
		case IDC_DEBUG:
			if(IsDlgButtonChecked(hwnd,LOWORD(wparam))){
				RECT rect;
				GetWindowRect(hwnd,&rect);
				open_console();
				move_console(rect.right,rect.top);
			}
			else
				hide_console();
			break;
		}
		break;
	case WM_APP:
		update_controls(hwnd);
		{
			SYSTEMTIME time;
			GetLocalTime(&time);
			current_time=time;
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
    INITCOMMONCONTROLSEX ctrls;
	ctrls.dwSize=sizeof(ctrls);
    ctrls.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx(&ctrls);

	DialogBox(hInstance,IDD_DIALOG1,NULL,dialogproc);
	return 0;
}
