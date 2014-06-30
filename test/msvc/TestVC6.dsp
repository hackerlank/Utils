# Microsoft Developer Studio Project File - Name="TestVC6" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=TestVC6 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TestVC6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TestVC6.mak" CFG="TestVC6 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TestVC6 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "TestVC6 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TestVC6 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 iconv.lib CUnitLib.lib /nologo /subsystem:console /machine:I386 /libpath:"../lib" /libpath:"../../lib/iconv" /libpath:"../../lib/CUnit"

!ELSEIF  "$(CFG)" == "TestVC6 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 iconvd.lib CUnitLibD.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"../../lib/iconv" /libpath:"../../lib/CUnit"

!ENDIF 

# Begin Target

# Name "TestVC6 - Win32 Release"
# Name "TestVC6 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TestVC6.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Tests"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\charset.c
# End Source File
# Begin Source File

SOURCE=..\charset.h
# End Source File
# Begin Source File

SOURCE=..\cppstring.cpp
# End Source File
# Begin Source File

SOURCE=..\cppstring.h
# End Source File
# Begin Source File

SOURCE=..\datatype.c
# End Source File
# Begin Source File

SOURCE=..\datatype.h
# End Source File
# Begin Source File

SOURCE=..\filesystem.c
# End Source File
# Begin Source File

SOURCE=..\filesystem.h
# End Source File
# Begin Source File

SOURCE=..\math.c
# End Source File
# Begin Source File

SOURCE=..\math.h
# End Source File
# Begin Source File

SOURCE=..\others.c
# End Source File
# Begin Source File

SOURCE=..\others.h
# End Source File
# Begin Source File

SOURCE=..\string.c
# End Source File
# Begin Source File

SOURCE=..\string.h
# End Source File
# Begin Source File

SOURCE="..\test-c.c"

!IF  "$(CFG)" == "TestVC6 - Win32 Release"

# ADD CPP /MD
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "TestVC6 - Win32 Debug"

# ADD CPP /MDd
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\test-c.h"
# End Source File
# Begin Source File

SOURCE="..\test-cpp.cpp"
# End Source File
# Begin Source File

SOURCE="..\test-cpp.h"
# End Source File
# End Group
# Begin Group "Utils"

# PROP Default_Filter ""
# Begin Group "lib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\lib\ConvertUTF.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\ConvertUTF.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\string16.cpp
# End Source File
# Begin Source File

SOURCE=..\..\lib\string16.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\ccutil.cpp
# End Source File
# Begin Source File

SOURCE=..\..\ccutil.h
# End Source File
# Begin Source File

SOURCE=..\..\config.h
# End Source File
# Begin Source File

SOURCE=..\..\cutil.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\cutil.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
