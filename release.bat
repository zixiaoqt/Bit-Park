@echo off
::���������������window��parkingserver�������ͷ�������Բ�ͬ���û���ϵͳ����Ҫ��ȷ��������ı���
:: ProjectFolder������Ŀ¼
:: BuildFolder��buildĿ¼��cmake���ɵ�vs���̾��ڴ�Ŀ¼��
:: ReleaseFolder������Ŀ¼�����淢�������Ŀ¼
:: cmake: cmake����·��
:: git: git����·��
:: msbuild: VS��������·��
:: libmysql: libmysql.dll·��
:: vcredist: VC�ɷַ����·��
:: HOME: ����git����ִ�У����õ�HOME��������

set ProjectFolder=e:\work\Parking_Server\
set BuildFolder=build\
set ReleaseFolder=d:\parkingserver-release\parkingserver\

set cmake="D:\Program Files (x86)\CMake\bin\cmake.exe"
set git="D:\Program Files (x86)\Git\bin\git.exe"
set msbuild="C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe"

set vcredist=D:\parkingserver-release\parkingserver\parkingserver-0.7.6\vcredist_vs2013_x86.exe

set HOME=C:\Users\lua\

cd "%ProjectFolder%"

echo git pull
call %git% pull

if not exist %BuildFolder% (
  echo create build folder
  md "%BuildFolder%""
)

cd "%BuildFolder%"

echo cmake ..
call %cmake% ..

echo msbuild ...
call %msbuild% parking-server.sln /property:Configuration=Release


for /f "skip=2 tokens=3" %%i in (%ProjectFolder%%BuildFolder%version.h) do (if not "%%i"=="_CMS_VERSION_" (for /f "delims=.; tokens=1-4" %%a in (%%i) do set "var=%%a.%%b.%%c.%%d"))
echo extract version %var%

set VersionFolder=parkingserver-%var%
if not exist %ReleaseFolder%%VersionFolder% (
   echo create folder %VersionFolder%
   md "%ReleaseFolder%%VersionFolder%"
)

echo copy all files ...
copy "%ProjectFolder%%BuildFolder%Release\parking-server.exe" "%ReleaseFolder%%VersionFolder%"
copy "%ProjectFolder%%BuildFolder%Release\parking-daemon.exe" "%ReleaseFolder%%VersionFolder%"
copy "%ProjectFolder%%BuildFolder%Release\install_service.bat" "%ReleaseFolder%%VersionFolder%"
copy "%ProjectFolder%%BuildFolder%Release\delete_service.bat" "%ReleaseFolder%%VersionFolder%"
copy "%ProjectFolder%install.txt" "%ReleaseFolder%%VersionFolder%"
copy "%ProjectFolder%senselock\lib\elitee.dll" "%ReleaseFolder%%VersionFolder%"
copy "%ProjectFolder%senselock\lib\EleCrypt.dll" "%ReleaseFolder%%VersionFolder%"
copy "%ProjectFolder%mysql++\lib\release\mysqlpp.dll" "%ReleaseFolder%%VersionFolder%"
copy "%libmysql%" "%ReleaseFolder%%VersionFolder%"
copy "%vcredist%" "%ReleaseFolder%%VersionFolder%"

cd "%ProjectFolder%"