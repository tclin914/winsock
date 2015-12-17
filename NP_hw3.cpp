#include <windows.h>
#include <list>
using namespace std;

#include "resource.h"

#define SERVER_PORT 7799

#define BUFSIZE 4096

#define WM_SOCKET_NOTIFY (WM_USER + 1)

BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
int EditPrintf (HWND, TCHAR *, ...);
//=================================================================
//	Global Variables
//=================================================================
list<SOCKET> Socks;

struct {
	char* ext;
	char* filetype;
} extensions[] = {
	{ "gif", "image/gif" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "png", "image/png" },
	{ "zip", "image/zip" },
	{ "gz", "image/gz" },
	{ "tar", "image/tar" },
	{ "htm", "text/html" },
	{ "html", "text/html" },
	{ "exe", "text/plain" },
	{ "cgi", "text/html" },
	{ 0, 0 }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	
	return DialogBox(hInstance, MAKEINTRESOURCE(ID_MAIN), NULL, MainDlgProc);
}

BOOL CALLBACK MainDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	WSADATA wsaData;

	static HWND hwndEdit;
	static SOCKET msock, ssock;
	static struct sockaddr_in sa;

	int err;

	char recv_buf[4096];
	char msg_buf[4096];
	FILE* fp;
	int n;

	char* filename;
	char* get;

	switch(Message) 
	{
		case WM_INITDIALOG:
			hwndEdit = GetDlgItem(hwnd, IDC_RESULT);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_LISTEN:

					WSAStartup(MAKEWORD(2, 0), &wsaData);

					//create master socket
					msock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

					if( msock == INVALID_SOCKET ) {
						EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\r\n"));
						WSACleanup();
						return TRUE;
					}

					err = WSAAsyncSelect(msock, hwnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE);

					if ( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(msock);
						WSACleanup();
						return TRUE;
					}

					//fill the address info about server
					sa.sin_family		= AF_INET;
					sa.sin_port			= htons(SERVER_PORT);
					sa.sin_addr.s_addr	= INADDR_ANY;

					//bind socket
					err = bind(msock, (LPSOCKADDR)&sa, sizeof(struct sockaddr));

					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: binding error ===\r\n"));
						WSACleanup();
						return FALSE;
					}

					err = listen(msock, 2);
		
					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: listen error ===\r\n"));
						WSACleanup();
						return FALSE;
					}
					else {
						EditPrintf(hwndEdit, TEXT("=== Server START ===\r\n"));
					}

					break;
				case ID_EXIT:
					EndDialog(hwnd, 0);
					break;
			};
			break;

		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;

		case WM_SOCKET_NOTIFY:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_ACCEPT:
					ssock = accept(msock, NULL, NULL);
					Socks.push_back(ssock);
					EditPrintf(hwndEdit, TEXT("=== Accept one new client(%d), List size:%d ===\r\n"), ssock, Socks.size());
					break;
				case FD_READ:
				//Write your code for read event here.
					//read request from browser
					n = recv(ssock, recv_buf, BUFSIZE-1, 0);
					if (n <= 0)
					{
						EditPrintf(hwndEdit, TEXT("=== Error: recv from browser error ===\r\n"));
						WSACleanup();
						return FALSE;
					}
					recv_buf[n] = '\0';
					for (size_t i = 0; i < n; i++)
					{
						if (recv_buf[i] == '\r' || recv_buf[i] == '\n')
						{
							recv_buf[i] = '\0';
						}
					}

					//check whether the request is get
					if (strncmp(recv_buf, "GET ", 4) != 0 && strncmp(recv_buf, "get ", 4) != 0)
					{
						EditPrintf(hwndEdit, TEXT("=== Error: only accept get error ===\r\n"));
						WSACleanup();
						return FALSE;
					}

					for (size_t i = 4; i < n; i++)
					{
						if (recv_buf[i] == ' ')
						{
							recv_buf[i] = '\0';
							break;
						}
					}

					filename = strtok(&recv_buf[5], "?");
					get = strtok(NULL, "?");

					//send(ssock, recv_buf, n, 0);
					break;
				case FD_WRITE:
				//Write your code for write event here
					fp = fopen("server_file/form_get.html", "r");
					while ((n = fread(msg_buf, sizeof(char), BUFSIZE, fp)) > 0) {
						send(ssock, msg_buf, n, 0);
					}
					
					/*send(ssock, TEST, strlen(TEST), 0);
					send(ssock, HELLO, strlen(HELLO), 0);*/
					break;
				case FD_CLOSE:
					break;
			};
			break;
		
		default:
			return FALSE;


	};

	return TRUE;
}

int EditPrintf (HWND hwndEdit, TCHAR * szFormat, ...)
{
     TCHAR   szBuffer [1024] ;
     va_list pArgList ;

     va_start (pArgList, szFormat) ;
     wvsprintf (szBuffer, szFormat, pArgList) ;
     va_end (pArgList) ;

     SendMessage (hwndEdit, EM_SETSEL, (WPARAM) -1, (LPARAM) -1) ;
     SendMessage (hwndEdit, EM_REPLACESEL, FALSE, (LPARAM) szBuffer) ;
     SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0) ;
	 return SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0); 
}