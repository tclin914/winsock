#include <windows.h>
#include <list>
using namespace std;

#include "resource.h"

#define SERVER_PORT 7799

#define BUFSIZE 4096

#define WM_SOCKET_NOTIFY (WM_USER + 1)

#define WM_SOCKET_CLIENT (WM_USER + 2)
#define FD_Connecting 0
#define FD_Reading	  1
#define FD_Writing    2

#define HEAD "<html>\r\n<head>\r\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />\r\n<title>Network Programming Homework 3</title>\r\n</head>\r\n"
#define BODY "<body bgcolor=#336699>\r\n"
#define FONT "<font face=\"Courier New\" size=2 color=#FFFF99>\r\n"
#define TABLE1 "<table width=\"800\" border=\"1\">\r\n<tr>\r\n"
#define TABLE_ELEMENT "<td>%s</td>\r\n"
#define TABLE2 "</tr>\r\n<tr>\r\n<td valign=\"top\" id=\"m0\"></td><td valign=\"top\" id=\"m1\"></td><td valign=\"top\" id=\"m2\"></td>\r\n<td valign=\"top\" id=\"m3\"></td><td valign=\"top\" id=\"m4\"></td>\r\n</tr>\r\n</table>\r\n"
#define SCRIPT_MESSAGE "<script>document.all[\'%s\'].innerHTML += \"%s<br>\";</script>\r\n"
#define SCRIPT_COMMAND "<script>document.all[\'%s\'].innerHTML += \"%s <b>%s</b><br>\";</script>\r\n"
#define TAIL "</font>\r\n</body>\r\n</html>\r\n"

BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
int EditPrintf(HWND, TCHAR *, ...);
void parseString(char* string, int* nbHost);
int sendMsg(const SOCKET sock, char* string);
int readline(FILE* fp, char *ptr, int maxlen);
//=================================================================
//	Global Variables
//=================================================================
list<SOCKET> Socks;

char* hosts[5];
char* ports[5];
char* files[5];
FILE* filefps[5];
int status = FD_Connecting;

const char* mlist[5] = {
	"m0",
	"m1",
	"m2",
	"m3",
	"m4"
};

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

	char recv_buf[BUFSIZE];
	char msg_buf[BUFSIZE];
	char command_buf[BUFSIZE];
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

	switch (Message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwnd, IDC_RESULT);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_LISTEN:

			WSAStartup(MAKEWORD(2, 0), &wsaData);

			//create master socket
			msock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			if (msock == INVALID_SOCKET) {
				EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\r\n"));
				WSACleanup();
				return TRUE;
			}

			err = WSAAsyncSelect(msock, hwnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE);

			if (err == SOCKET_ERROR) {
				EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
				closesocket(msock);
				WSACleanup();
				return TRUE;
			}

			//fill the address info about server
			sa.sin_family = AF_INET;
			sa.sin_port = htons(SERVER_PORT);
			sa.sin_addr.s_addr = INADDR_ANY;

			//bind socket
			err = bind(msock, (LPSOCKADDR)&sa, sizeof(struct sockaddr));

			if (err == SOCKET_ERROR) {
				EditPrintf(hwndEdit, TEXT("=== Error: binding error ===\r\n"));
				WSACleanup();
				return FALSE;
			}

			err = listen(msock, 2);

			if (err == SOCKET_ERROR) {
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
		switch (WSAGETSELECTEVENT(lParam))
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

				send(ssock, HEAD, strlen(HEAD), 0);
				send(ssock, BODY, strlen(BODY), 0);
				send(ssock, FONT, strlen(FONT), 0);
				send(ssock, TABLE1, strlen(TABLE1), 0);
				for (int i = 0; i < nbHost; i++)
				{
					sprintf(msg_buf, TABLE_ELEMENT, hosts[i]);
					send(ssock, msg_buf, strlen(msg_buf), 0);
				}
				send(ssock, TABLE2, strlen(TABLE2), 0);
				send(ssock, TAIL, strlen(TAIL), 0);

				//open batch file
				for (int i = 0; i < nbHost; i++)
				{
					filefps[i] = fopen(files[i], "r");
					EditPrintf(hwndEdit, TEXT("=== open batch file ===\r\n"));
				}

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
				status = FD_Reading;
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
		//deal with connecting ras/rwg event
	case WM_SOCKET_CLIENT:
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_READ:
			EditPrintf(hwndEdit, TEXT("=== CLIENT FD_READ %d ===\r\n"), status);
			if (status == FD_Reading)
			{
				n = recv(csock, recv_buf, BUFSIZE - 1, 0);
				recv_buf[n - 1] = '\0';
				if (n > 0)
				{
					EditPrintf(hwndEdit, TEXT("=== receive from server: %s n: %d ===\r\n"), recv_buf, n);
					if (sendMsg(ssock, recv_buf))
					{
						len = readline(filefps[0], command_buf, sizeof(command_buf));
						command_buf[len - 1] = 13;
						command_buf[len] = 10;
						command_buf[len + 1] = '\0';
						n = send(csock, command_buf, len + 1, 0);
						if (n > 0)
						{
							command_buf[len - 1] = '\0';
							sprintf(msg_buf, SCRIPT_COMMAND, mlist[0], "%", command_buf);
							EditPrintf(hwndEdit, TEXT("=== command: %s ===\r\n"), msg_buf);
							send(ssock, msg_buf, strlen(msg_buf), 0);
						}
						//status = FD_Writing;
					}
				}

			}
			break;
		case FD_WRITE:
			EditPrintf(hwndEdit, TEXT("=== CLIENT FD_WRITE ===\r\n"));
			if (status == FD_Writing)
			{

				if (n > 0)
				{
					status = FD_Reading;
				}
			}
			break;
		case FD_CLOSE:
			EditPrintf(hwndEdit, TEXT("=== CLIENT FD_CLOSE ===\r\n"));
			closesocket(csock);
			break;
		}
	default:
		return FALSE;


	};

	return TRUE;
}

int EditPrintf(HWND hwndEdit, TCHAR * szFormat, ...)
{
	TCHAR   szBuffer[1024];
	va_list pArgList;

	va_start(pArgList, szFormat);
	wvsprintf(szBuffer, szFormat, pArgList);
	va_end(pArgList);

	SendMessage(hwndEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
	SendMessage(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)szBuffer);
	SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
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

int sendMsg(const SOCKET sock, char* string)
{
	char msg_buf[BUFSIZE];
	memset(msg_buf, 9, BUFSIZE);
	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);
	int c = 0;
	while (*string != '\0')
	{
		if (*string == '%' && *(string + 1) == ' ')
		{
			return 1;
		}

		if (*string == '\n')
		{
			buf[c] = '\0';
			sprintf(msg_buf, SCRIPT_MESSAGE, mlist[0], buf);
			send(sock, msg_buf, strlen(msg_buf), 0);
			memset(msg_buf, 0, BUFSIZE);
			memset(buf, 0, BUFSIZE);
			c = 0;
			++string;
			continue;
		}
		else if (*string == '"') {
			buf[c] = '&';
			buf[++c] = 'q';
			buf[++c] = 'u';
			buf[++c] = 'o';
			buf[++c] = 't';
			buf[++c] = ';';
		}
		else if (*string == '<') {
			buf[c] = '&';
			buf[++c] = 'l';
			buf[++c] = 't';
			buf[++c] = ';';
		}
		else if (*string == '>') {
			buf[c] = '&';
			buf[++c] = 'g';
			buf[++c] = 't';
			buf[++c] = ';';
		}
		else if (*string == '\t')
		{
			buf[c] = '&';
			buf[++c] = '#';
			buf[++c] = '9';
		}
		else
		{
			buf[c] = *string;
		}
		++c;
		++string;
	}
	if (strlen(buf) > 0)
	{
		sprintf(msg_buf, SCRIPT_MESSAGE, mlist[0], buf);
		send(sock, msg_buf, strlen(msg_buf), 0);
	}
	return 0;
}

int readline(FILE* fp, char *ptr, int maxlen)
{
	int n, rc;
	char c;
	*ptr = 0;
	for (n = 1; n < maxlen; n++)
	{
		rc = fread(&c, sizeof(char), 1, fp);
		if (rc == 1)
		{
			*ptr++ = c;
			if (c == '\n')  break;
		}
		else if (rc == 0)
		{
			if (n == 1)     return 0;
			else         break;
		}
		else return (-1);
	}
	return n;
}