#include <windows.h>
#include <list>
using namespace std;

#include "resource.h"

#define SERVER_PORT 7799

#define BUFSIZE 4096

#define WM_SOCKET_NOTIFY (WM_USER + 1)

#define WM_SOCKET_CLIENT (WM_USER + 2)

BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
int EditPrintf (HWND, TCHAR *, ...);
void parseString(char* string, int* nbHost);
//=================================================================
//	Global Variables
//=================================================================
list<SOCKET> Socks;

char* hosts[5];
char* ports[5];
char* files[5];
FILE* filefps[5];

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

typedef enum {
	GIF,
	JPG,
	JPEG,
	PNG,
	ZIP,
	GZ,
	TAR,
	HTM,
	HTML,
	EXE,
	CGI,
	NONE
} FileType;

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

	FileType filetype = NONE;
	char* filename = NULL;
	char* get;
	int fnlen;
	int len;
	int nbHost = 0;

	WSADATA wsaCData;
	static SOCKET csock;
	static struct sockaddr_in ca;

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
					EditPrintf(hwndEdit, TEXT("=== FD_READ ===\r\n"));
					//read request from browser
					n = recv(ssock, recv_buf, BUFSIZE - 1, 0);
					if (n <= 0)
					{
						return FALSE;
					}
					recv_buf[n] = '\0';
					for (int i = 0; i < n; i++)
					{
						if (recv_buf[i] == '\r' || recv_buf[i] == '\n')
						{
							recv_buf[i] = '\0';
						}
					}

					//check whether the request is get
					if (strncmp(recv_buf, "GET ", 4) != 0 && strncmp(recv_buf, "get ", 4) != 0)
					{
						return FALSE;
					}

					//remove the last of GET /xxx.html?xxx
					for (int i = 4; i < n; i++)
					{
						if (recv_buf[i] == ' ')
						{
							recv_buf[i] = '\0';
							break;
						}
					}

					//separate filename and get information
					filename = strtok(&recv_buf[5], "?");
					get = strtok(NULL, "?");
					EditPrintf(hwndEdit, TEXT("=== filename:%s ===\r\n"), filename);

					//avoid .. command
					for (size_t i = 1; i < strlen(filename); i++)
					{
						if (filename[i - 1] == '.' && filename[i] == '.')
						{
							return false;
						}
					}

					//check file type
					fnlen = strlen(filename);
					for (int i = 0; extensions[i].ext != 0; i++)
					{
						len = strlen(extensions[i].ext);
						if (strncmp(&filename[fnlen - len], extensions[i].ext, len) == 0)
						{
							filetype = static_cast<FileType>(i);
							break;
						}
					}

					switch (filetype)
					{
						case GIF:
						case JPG:
						case PNG:
						case ZIP:
						case GZ:
						case TAR:
						case HTM:
						case HTML:
							fp = fopen(filename, "r");
							while ((n = fread(msg_buf, sizeof(char), BUFSIZE, fp)) > 0) {
								send(ssock, msg_buf, n, 0);
							}
							closesocket(ssock);
							break;
						case EXE:
							break;
						case CGI:
							EditPrintf(hwndEdit, TEXT("=== get:%s ===\r\n"), get);
							parseString(get, &nbHost);
							EditPrintf(hwndEdit, TEXT("=== nbHost:%d ===\r\n"), nbHost);

							WSAStartup(MAKEWORD(2, 0), &wsaCData);

							//create client socket
							csock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

							if (msock == INVALID_SOCKET) {
								EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\r\n"));
								WSACleanup();
								return TRUE;
							}

							err = WSAAsyncSelect(csock, hwnd, WM_SOCKET_CLIENT, FD_CLOSE | FD_READ | FD_WRITE);

							if (err == SOCKET_ERROR) {
								EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
								closesocket(msock);
								WSACleanup();
								return TRUE;
							}

							//fill the address info about server
							ca.sin_family = AF_INET;
							ca.sin_port = htons(atoi(ports[0]));
							ca.sin_addr.s_addr = inet_addr(hosts[0]);

							connect(csock, (LPSOCKADDR)&ca, sizeof(ca));

							//closesocket(ssock);
							break;
						case NONE:
							EditPrintf(hwndEdit, TEXT("=== FileType NONE ===\r\n"));
							break;
						default:
							break;
					}
					break;
				case FD_WRITE:
				//Write your code for write event here
					EditPrintf(hwndEdit, TEXT("=== FD_WRITE ===\r\n"));
					break;
				case FD_CLOSE:
					break;
			};
			break;
		case WM_SOCKET_CLIENT:
			switch (WSAGETSELECTEVENT(lParam))
			{
				case FD_READ:
					n = recv(csock, recv_buf, BUFSIZE - 1, 0);
					send(ssock, recv_buf, n, 0);
					closesocket(ssock);
					EditPrintf(hwndEdit, TEXT("=== CLIENT FD_READ ===\r\n"));
					break;
				case FD_WRITE:
					EditPrintf(hwndEdit, TEXT("=== CLIENT FD_WRITE ===\r\n"));
					break;
				case FD_CLOSE:
					EditPrintf(hwndEdit, TEXT("=== CLIENT FD_CLOSE ===\r\n"));
					break;
			}
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

void parseString(char* string, int* nbHost)
{
	char* pair = NULL;
	char* key;
	int count = 0;
	pair = strtok(string, "&");
	while (pair != NULL)
	{
		switch (count % 3)
		{
		case 0:
			hosts[count / 3] = (pair + 3);
			if (strlen(pair + 3) > 0) (*nbHost)++;
			break;
		case 1:
			ports[count / 3] = (pair + 3);
			break;
		case 2:
			files[count / 3] = (pair + 3);
			break;
		default:
			break;
		}
		++count;
		pair = strtok(NULL, "&");
	}
}