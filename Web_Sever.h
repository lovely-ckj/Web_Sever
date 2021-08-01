#ifndef Web_Sever_h
#define Web_Sever_h

#include <Windows.h>
#include <string>
#include <functional>
#include <map>
#include <thread>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

/******************************* 声明 *********************************/

#define METHOD_GET 1
#define METHOD_POST 2

#define BUF_SIZE 16384

#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

struct FORMDATA // 表单单元结构体（也可以用来储存响应给浏览器的内容）
{
	bool isfile;       // 是否一个文件（内容是否在本地内存中）
	std::string dat;   // 内容（在 RAM 中，isfile 为 false 时才有意义）
	std::string fpath; // 存内容的文件路径（在本地内存中，isfile 为 true 时才有意义）
};

struct SENDDATA // 响应结构体
{
	int code;      // 状态码（例：200 OK、404 NOT FOUND）
	FORMDATA data; // 响应给浏览器的内容
};

class WEBDATA  // 存放浏览器请求的类
{
public:
	int method;            // 请求类型（为 METHOD_GET 或 METHOD_POST，分别代表 Get 请求和 Post 请求）
	std::string retype;    // 请求文件的 HTTP 编码
	std::string fpath;     // 请求文件的路径
	std::map<std::string, std::string> cookie;  // cookie 信息表
	std::map<std::string, FORMDATA> form;       // 表单信息表

public:
	bool init(SOCKET soc); // 接收请求并初始化
};

class WEBSEVER  // 网站服务器类
{
private:
	SOCKET soc;                              // 服务器套接字
	std::function<SENDDATA(WEBDATA)> runfun; // 存放由用户定义的请求处理函数

public:
	bool init(int port, std::function<SENDDATA(WEBDATA)> run); // 初始化服务器
	// port 为端口号，run 必须是一个仅有一个 WEBDATA 类型的参数且返回 SENDDATA 类型的函数
	bool join();   // 阻塞运行服务器
	bool detach(); // 不阻塞运行服务器（多线程运行）
	~WEBSEVER();   // 析构函数
};

void webslove(SOCKET client, std::function<SENDDATA(WEBDATA)> runfun); // 请求处理函数

short int hexChar2dec(char c);                 // url 解码辅助函数
std::string urldecode(std::string URL);        // url 解码
char dec2hexChar(short int n);                 // url 编码辅助函数
std::string urlcode(std::string URL);          // url 编码

// 各种编码互转
std::wstring Utf82Unicode(const std::string utf8string);  // utf-8 转 unicode
std::string WideByte2Acsi(std::wstring wstrcode);         // unicode 转 ascii
std::string UTF_82ASCII(std::string strUtf8Code);         // utf-8 转 ascii
std::wstring Acsi2WideByte(std::string strascii);         // ascii 转 unicode
std::string Unicode2Utf8(const std::wstring widestring);  // unicode 转 utf-8
std::string ASCII2UTF_8(std::string strAsciiCode);        // ascii 转 utf-8

long long getfilelen(std::string fpath);  // 获取文件长度



/******************************* 定义 *********************************/


bool WEBSEVER::init(int port, std::function<SENDDATA(WEBDATA)> run)
{
	runfun = run;
	WSADATA wsadata;
	SOCKADDR_IN sevaddr;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		return false;
	}
	soc = socket(PF_INET, SOCK_STREAM, 0);
	if (soc == INVALID_SOCKET)
	{
		return false;
	}
	memset(&sevaddr, 0, sizeof(sevaddr));
	sevaddr.sin_family = AF_INET;
	sevaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sevaddr.sin_port = htons(port);
	if (bind(soc, (SOCKADDR*)&sevaddr, sizeof(sevaddr)) == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}

void webslove(SOCKET client, std::function<SENDDATA(WEBDATA)> runfun)
{
	WEBDATA webdata;
	if (!webdata.init(client))
	{
		closesocket(client);
		return;
	}
	SENDDATA re = runfun(webdata);
	if (re.code == 200)
	{
		if (re.data.isfile)
		{
			long long length = getfilelen(re.data.fpath);
			std::stringstream head;
			head << "HTTP/1.1 200 OK\r\n";
			head << "Server: CKJ's Server\r\n";
			head << "Date: Mon, 12 Jul 2021 11:40:53 GMT\r\n";
			head << "Content-Type: " << webdata.retype << "\r\n";
			head << "Content-Length: " << length << "\r\n";
			head << "Connection: close\r\n\r\n";
			send(client, head.str().c_str(), head.str().size(), 0);
			long long hasread = 0;
			std::ifstream in(re.data.fpath, std::ios::binary);
			while (hasread < length)
			{
				long long nowlen = min(BUF_SIZE, length - hasread);
				char* buf = new char[nowlen];
				in.read(buf, nowlen);
				long long nowread = send(client, buf, nowlen, 0);
				if (nowread <= 0)
				{
					in.close();
					closesocket(client);
					return;
				}
				hasread += nowread;
				delete[] buf;
			}
			in.close();
		}
		else
		{
			std::stringstream head;
			head << "HTTP/1.1 200 OK\r\n";
			head << "Server: CKJ's Server\r\n";
			head << "Date: Mon, 12 Jul 2021 11:40:53 GMT\r\n";
			head << "Content-Type: " << webdata.retype << "\r\n";
			head << "Content-Length: " << re.data.dat.size() << "\r\n";
			head << "Connection: close\r\n\r\n";
			send(client, head.str().c_str(), head.str().size(), 0);
			send(client, re.data.dat.c_str(), re.data.dat.size(), 0);
		}
	}
	else
	{
		std::stringstream head;
		head << "HTTP/1.1 " << re.code << "\r\n";
		head << "Server: CKJ's Server\r\n";
		head << "Date: Mon, 12 Jul 2021 11:40:53 GMT\r\n\r\n";
		send(client, head.str().c_str(), head.str().size(), 0);
	}
	closesocket(client);
}

bool WEBSEVER::join()
{
	if (listen(soc, 15) == SOCKET_ERROR)
	{
		return false;
	}
	while (1)
	{
		SOCKADDR_IN ciladdr;
		int tmp = sizeof(ciladdr);
		SOCKET client = accept(soc, (SOCKADDR*)&ciladdr, &tmp);
		if (client == INVALID_SOCKET)
		{
			continue;
		}
		webslove(client, runfun);
		Sleep(1);
	}
	return true;
}

bool WEBSEVER::detach()
{
	if (listen(soc, 15) == SOCKET_ERROR)
	{
		return false;
	}
	std::thread t([](SOCKET sevsoc, WEBSEVER item)->void {
		while (1)
		{
			SOCKADDR_IN ciladdr;
			int tmp;
			SOCKET client = accept(sevsoc, (SOCKADDR*)&ciladdr, &tmp);
			std::thread t(webslove, client, item.runfun);
			t.detach();
			Sleep(1);
		}
		}, soc, *this);
	t.detach();
}

WEBSEVER::~WEBSEVER()
{
	closesocket(soc);
	WSACleanup();
}

bool WEBDATA::init(SOCKET soc)
{
	char* buf = new char[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);
	int length = recv(soc, buf, BUF_SIZE, 0);
	if (length <= 0)
	{
		delete[] buf;
		return false;
	}
	std::string tmpbuf = buf;
	long long splitpos = tmpbuf.find("\r\n\r\n");
	if (splitpos == tmpbuf.npos)
	{
		delete[] buf;
		return false;
	}
	std::string head = tmpbuf.substr(0, splitpos);
	std::string formdat = tmpbuf.substr(splitpos + strlen("\r\n\r\n"));
	long long http11pos = head.find("HTTP/1.1");
	long long acceptpos = head.find("Accept: ") + strlen("Accept: ");
	if (http11pos >= head.npos || acceptpos >= head.npos)
	{
		delete[] buf;
		return false;
	}
	if (head.find("GET ") == 0)
	{
		fpath = head.substr(strlen("GET "), http11pos - 1 - strlen("GET "));
		method = METHOD_GET;
	}
	else
	{
		fpath = head.substr(strlen("POST "), http11pos - 1 - strlen("POST "));
		method = METHOD_POST;
	}
	if (method == METHOD_GET)
	{
		long long formpos = fpath.find("?");
		if (formpos != fpath.npos)
		{
			std::string formstr = fpath.substr(formpos + 1) + '&';
			fpath = fpath.substr(0, formpos);
			long long nowpos = 0, nxtpos = 0;
			while ((nxtpos = formstr.find("&", nowpos)) != formstr.npos)
			{
				std::string nowform = formstr.substr(nowpos, nxtpos - nowpos);
				long long nowcut = nowform.find("=");
				if (nowcut == nowform.npos)
				{
					nowpos = nxtpos + 1;
					continue;
				}
				std::string nowname = nowform.substr(0, nowcut);
				nowname = UTF_82ASCII(urldecode(nowname));
				FORMDATA tmp;
				tmp.isfile = false;
				tmp.dat = nowform.substr(nowcut + 1);
				tmp.dat = UTF_82ASCII(urldecode(tmp.dat));
				form[nowname] = tmp;
				nowpos = nxtpos + 1;
			}
		}
	}
	else
	{
		long long lengthpos = head.find("Content-Length: ") + strlen("Content-Length: ");
		if (lengthpos != head.npos)
		{
			long long truelen = 0;
			sscanf(head.substr(lengthpos, head.find("\r\n", lengthpos) - lengthpos).c_str(), "%lld", &truelen);
			long long contypepos = head.find("Content-Type: ") + strlen("Content-Type: ");
			if (contypepos >= head.npos)
			{
				delete[] buf;
				return false;
			}
			long long recvedlen = formdat.length();
			while (recvedlen < truelen)
			{
				memset(buf, 0, BUF_SIZE);
				length = recv(soc, buf, BUF_SIZE, 0);
				if (length <= 0)
				{
					delete[] buf;
					return false;
				}
				formdat += buf;
				recvedlen += length;
			}
			std::string typestr = head.substr(contypepos, head.find("\r\n", contypepos) - contypepos);
			if (typestr == "application/x-www-form-urlencoded")
			{
				formdat += '&';
				long long nowpos = 0, nxtpos = 0;
				while ((nxtpos = formdat.find("&", nowpos)) != formdat.npos)
				{
					std::string nowform = formdat.substr(nowpos, nxtpos - nowpos);
					long long nowcut = nowform.find("=");
					if (nowcut == nowform.npos)
					{
						nowpos = nxtpos + 1;
						continue;
					}
					std::string nowname = nowform.substr(0, nowcut);
					nowname = UTF_82ASCII(urldecode(nowname));
					FORMDATA tmp;
					tmp.isfile = false;
					tmp.dat = nowform.substr(nowcut + 1);
					tmp.dat = urldecode(tmp.dat);
					tmp.dat = UTF_82ASCII(tmp.dat);
					form[nowname] = tmp;
					nowpos = nxtpos + 1;
				}
			}
		}
	}
	fpath = UTF_82ASCII(urldecode(fpath));
	printf("%s\n", fpath.c_str());
	retype = head.substr(acceptpos, head.find(",", acceptpos) - acceptpos);
	if (head.find("Cookie: ") != head.npos)
	{
		int pos = head.find("Cookie: ") + strlen("Cookie: ");
		std::string cookiestr = head.substr(pos, head.find("\r", pos) - pos);
		int nowpos = 0;
		while (cookiestr.find("; ", nowpos) != cookiestr.npos)
		{
			std::string nowcookie = cookiestr.substr(nowpos, cookiestr.find("; ", nowpos) - nowpos);
			nowpos = cookiestr.find("; ", nowpos) + strlen("; ");
			int tokpos = nowcookie.find("=");
			cookie[nowcookie.substr(0, tokpos)] = nowcookie.substr(tokpos + 1);
		}
	}
	delete[] buf;
	return true;
}

short int hexChar2dec(char c)
{
	if ('0' <= c && c <= '9')
	{
		return short(c - '0');
	}
	else if ('a' <= c && c <= 'f')
	{
		return (short(c - 'a') + 10);
	}
	else if ('A' <= c && c <= 'F')
	{
		return (short(c - 'A') + 10);
	}
	else
	{
		return -1;
	}
}

std::string urldecode(std::string URL)
{
	long long pos = 0;
	while ((pos = URL.find("%E2%80%8B", pos)) != URL.npos)
	{
		URL.erase(pos, strlen("%E2%80%8B"));
	}
	std::string result;
	for (int i = 0; i < URL.size(); i++)
	{
		char c = URL[i];
		if (c != '%')
		{
			if (c == '+')
			{
				c = ' ';
			}
			result += c;
		}
		else
		{
			char c1 = URL[++i];
			char c0 = URL[++i];
			int num = 0;
			num += hexChar2dec(c1) * 16 + hexChar2dec(c0);
			result += char(num);
		}
	}
	return result;
}

char dec2hexChar(short int n)
{
	if (0 <= n && n <= 9)
	{
		return char(short('0') + n);
	}
	else if (10 <= n && n <= 15)
	{
		return char(short('A') + n - 10);
	}
	else
	{
		return char(0);
	}
}

std::string urlcode(std::string URL)
{
	std::string result;
	for (unsigned int i = 0; i < URL.size(); i++)
	{
		char c = URL[i];
		if (
			('0' <= c && c <= '9') ||
			('a' <= c && c <= 'z') ||
			('A' <= c && c <= 'Z') ||
			c == '/' || c == '.'
			)
		{
			result += c;
		}
		else
		{
			int j = (short int)c;
			if (j < 0)
			{
				j += 256;
			}
			int i1, i0;
			i1 = j / 16;
			i0 = j - i1 * 16;
			result += '%';
			result += dec2hexChar(i1);
			result += dec2hexChar(i0);
		}
	}
	return result;
}

std::wstring Utf82Unicode(std::string utf8string)
{
	int widesize = ::MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, NULL, 0);
	std::vector<wchar_t> resultstring(widesize);
	int convresult = ::MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, &resultstring[0], widesize);
	return std::wstring(&resultstring[0]);
}

std::string WideByte2Acsi(std::wstring wstr)
{
	LPCWSTR pwszSrc = wstr.c_str();
	int nLen = WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, NULL, 0, NULL, NULL);
	if (nLen == 0)
	{
		return "";
	}
	char* pszDst = new char[nLen];
	if (!pszDst)
	{
		return "";
	}
	WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, pszDst, nLen, NULL, NULL);
	std::string str = pszDst;
	delete[] pszDst;
	pszDst = NULL;
	return str;
}

std::string UTF_82ASCII(std::string strUtf8Code)
{
	std::string strRet("");
	std::wstring wstr = Utf82Unicode(strUtf8Code);
	strRet = WideByte2Acsi(wstr);
	if(strUtf8Code.length() >= 3 && strUtf8Code[0] == -17 && strUtf8Code[1] == -69 && strUtf8Code[2] == -65)
	{
		strRet.erase(0, 1);
	}
	return strRet;
}

std::wstring Acsi2WideByte(std::string str)
{
	LPCSTR pszSrc = str.c_str();
	int nLen = MultiByteToWideChar(CP_ACP, 0, pszSrc, -1, NULL, 0);
	if (nLen == 0)
	{
		return L"";
	}
	wchar_t* pwszDst = new wchar_t[nLen];
	if (!pwszDst)
	{
		return L"";
	}
	MultiByteToWideChar(CP_ACP, 0, pszSrc, -1, pwszDst, nLen);
	std::wstring wstr = pwszDst;
	delete[] pwszDst;
	pwszDst = NULL;
	return wstr;
}

std::string Unicode2Utf8(const std::wstring widestring)
{
	int utf8size = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, NULL, 0, NULL, NULL);
	std::vector<char> resultstring(utf8size);
	int convresult = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, &resultstring[0], utf8size, NULL, NULL);
	return std::string(&resultstring[0]);
}

std::string ASCII2UTF_8(std::string strAsciiCode)
{
	std::string strRet("");
	std::wstring wstr = Acsi2WideByte(strAsciiCode);
	strRet = Unicode2Utf8(wstr);
	return strRet;
}

long long getfilelen(std::string fpath)
{
	std::ifstream in(fpath, std::ios::binary);
	in.seekg(0, in.end);
	long long length = in.tellg();
	in.seekg(0, in.beg);
	in.close();
	return length;
}

#undef BUF_SIZE

#undef min
#undef max

#endif
