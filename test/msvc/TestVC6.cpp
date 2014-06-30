// TestVC6.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../test-c.h"
#include "../test-cpp.h"

static void OpenFile(CString strFile);


int main(int argc, char* argv[])
{
	do_test_c(argc, argv);
	do_test_cpp(argc, argv);

	OpenFile("TestCpp-Results.xml");
	OpenFile("TestC-Results.xml");

	return 0;
}

static void OpenFile(CString strFile)
{
	SHELLEXECUTEINFO ShExecInfo = {0}; 
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO); 
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS; 
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL; 
	ShExecInfo.lpFile = strFile; 
	ShExecInfo.lpParameters = ""; 
	ShExecInfo.lpDirectory = NULL; 
	ShExecInfo.nShow = SW_MAXIMIZE; 
	ShExecInfo.hInstApp = NULL; 
	
	ShellExecuteEx(&ShExecInfo); 
}
