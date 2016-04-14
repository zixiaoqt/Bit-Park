# How to compile
```
git clone git@gitlab.com:higoge/Parking_Server.git
git submodule init
git submodule update
mkdir build
cd build && cmake ..
make
```

需要准备工具：
1. 编译器（windows下安装vs）
2. 安装git
3. 编译安装boost
4. 安装cmake
5. 下载mysql++,编译：
    - linux：按照其说明，使用make编译
    - windows：不需要编译按照mysql++库，但需要下安装MySQL Connector C win32版（不要x64版)，放置的路径要不与cmake中指定的MYSQL_PREFIX路径一致，要不修改cmake中的MYSQL_PREFIX路径，使其找到connector库
6. 为了不使用jsoncpp库，而是使用其原文件，在jsoncpp路径下执行：
```
python amalgamate.py
```

注意:
msvc12下使用boost1.56的coroutine库时可能会遇到重定义的错误，可以通过以下方法解决:
1. 进入到boost的根目录
2. git apply patch/boost-1_56_0-coroutine_inline_msvc12.patch (文件位于工程目录patch下，不需要编译boost)
3. 重新编译工程

补充：
1. 请删除Cygwin自带的cmake，下载Windows的cmake，加入环境变量。
2. 如果不在环境变量中加入BOOST_ROOT，可在项目的cmake执行时动态加入。
```
cmake .. -DBOOST_ROOT=E:/02.CODE/BOOST_1_56_0
```
**注意**：不要使用cygwin的`/cygdriver/e/`，而要使用系统绝对路径，否则找不到Boost库。
3. Windows下`git config --global autocrlf true`，否则打补丁、编译源码都会出问题。
4. vs2013编译是需要将项目属性->链接器->高级->映像具有安全异常处理程序->否。


加密狗说明：
1. 使用eledevm工具，根据设备pin，修改厂商描述为BITCOM，服务程序中会根据这个信息打开加密狗
2. 为了安全，使用eledevm工具修改设备pin码
3. 编写模块程序
	a) 从http://www.keil.com上下载Keil C51,并安装
	b) 使用elesimuc工具选择Keil的安装路径
	c) 使用eleprjw工具创建Keil工程，并用IDE打开工程，使用方式与VS类似
	d) 修改代码，编译，生成exf文件
	e) 使用eledevm工具，在“下载模块文件”标签中向其传输程序，并修改模块名称为dongle
4. 为了安全，服务程序与加密狗中的程序通信进行了加密，具体看代码
5. 以上用到的工具都在tools下，详细信息可以看文档，文档介绍的很详细。

#windows服务安装
1. 将install下的install_service.bat和delete_service.bat拷到应用程序生成目录中。
2. 执行install_service.bat会安装服务并运行。
3. 执行delete_service.bat会停止服务并删除。
