This project just have Chinese README file, because ckj can't speak English well now :(

But dzj can. (Although he can't speak English well either.)

# Web_Sever

## 介绍 Introduction
这是一个使用纯 C++ 编写的库。

This is a library that is written by C++ only.

您可以使用这个库来编写自己的网站服务器，例如一个私人博客站或者一个网盘的服务器。

You can use this library to write your website server, such as a private blog or a net disk server.

## 使用方法 Usage

首先您需要导入 `Web_Sever` 库：

First you should import this library:

```cpp
#include "Web_Sever.h"
```

接下来，在 `main` 函数内创建一个 `WEBSEVER` 类型的变量：

And then, create a variable which is `WEBSERVER` in `main` function:

```cpp
#include "Web_Sever.h"

int main()
{
	WEBSEVER sever;
	return 0;
}
```

编写一个请求处理函数：

Write a requests handler:

```cpp
#include "Web_Sever.h"

SENDDATA run(WEBDATA dat);

int main()
{
	WEBSEVER sever;
	return 0;
}

SENDDATA run(WEBDATA dat)
{
	SENDDATA teamp;
	FORMDATA tmpdat;
	
	tmpdat.isfile = true;
	tmpdat.fpath = "." + dat.fpath;
	teamp.data = tmpdat;
	
	FILE* tmp = fopen(tmpdat.fpath.c_str(), "r");
	if (tmp != NULL)
	{
		teamp.code = 200;
		fclose(tmp);
	}
	else
	{
		teamp.code = 404;
	}
	
	return teamp;
}
```

**注意，请求处理函数的返回值必须为 `SENDDATA` 且必须有且仅有一个类型为 `WEBDATA` 的参数。**

**Attention, request handler must return `SENDDATA` and must have a parameters which is `WEBDATA`.

最后，初始化 `sever` 并运行！

Last, initialize `server` and run!

```cpp
#include "Web_Sever.h"

SENDDATA run(WEBDATA dat);

int main()
{
	WEBSEVER sever;
	sever.init(5000, run);
	// sever.detach(); // 在另一个线程运行（不阻塞）Run in another thread.
	sever.join(); // 在当前进程运行（阻塞）Run in current thread.
	return 0;
}

SENDDATA run(WEBDATA dat)
{
	SENDDATA teamp;
	FORMDATA tmpdat;
	
	tmpdat.isfile = true;
	tmpdat.fpath = "." + dat.fpath;
	teamp.data = tmpdat;
	
	FILE* tmp = fopen(tmpdat.fpath.c_str(), "r");
	if (tmp != NULL)
	{
		teamp.code = 200;
		fclose(tmp);
	}
	else
	{
		teamp.code = 404;
	}
	
	return teamp;
}
```
