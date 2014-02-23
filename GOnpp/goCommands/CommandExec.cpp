//Copyright (C)2014 tike <timflex@gmx.de>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <algorithm>
#include "CommandExec.h"

#ifdef min
#undef min
#endif

CommandExec::CommandExec(tstring& cmd, tstring& dir)
{
	this->cmd = tstring(cmd);
	this->dir = tstring(dir);

	memset(&this->sa, 0, sizeof(SECURITY_ATTRIBUTES));
	this->sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
	this->sa.bInheritHandle = TRUE; 
	this->sa.lpSecurityDescriptor = NULL;

	memset(&pi, 0, sizeof(LPPROCESS_INFORMATION));

	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;

	this->exitStatus = 0;

	this->stdOut = tstring();
	this->stdOutRd = NULL;
	this->stdOutWr = NULL;
	this->StdOutRead = FALSE;
	this->StdOutClosed = FALSE;

	this->stdErr = tstring();
	this->stdErrRd = NULL;
	this->stdErrWr = NULL;
	this->StdErrRead = FALSE;
	this->StdErrClosed = FALSE;
}


CommandExec::~CommandExec(void)
{
	//free(this->cmd);
	//free(this->dir);

	//if(this->stdOut != NULL) free(this->stdOut);
	if( ! this->StdOutClosed) CloseHandle(this->stdOutRd);

	//if(this->stdErr != NULL) free(this->stdErr);
	if( ! this->StdErrClosed) CloseHandle(this->stdErrRd);
}

BOOL CommandExec::setupPipe(PHANDLE read, PHANDLE write){
	if ( ! CreatePipe(read, write, &this->sa, 0) ){
		return FALSE;
	} 
	if ( ! SetHandleInformation(*read, HANDLE_FLAG_INHERIT, 0) ){
		return FALSE;
	}
	return TRUE;
}

BOOL CommandExec::Start(void)
{
	if( ! this->setupPipe(&this->stdOutRd, &this->stdOutWr)) return FALSE;
	if( ! this->setupPipe(&this->stdErrRd, &this->stdErrWr)) return FALSE;

	this->si.hStdOutput = this->stdOutWr;
	this->si.hStdError = this->stdErrWr;

	BOOL processStarted = ::CreateProcess(
				NULL,		// lpApplicationName
			(TCHAR*)this->cmd.c_str(),	// lpCommandLine
				NULL,		// Process Attributes
				NULL,		// lpThreadAttributes
				TRUE,		// inheritHandles
				0,			// CreationFlags
				NULL,		// lpEnvironment
				this->dir.c_str(),	// lpCurrentDirectory
				&this->si,	// StartupInfo
				&this->pi	// ProcessInfo
		);
	CloseHandle(this->stdOutWr);
	CloseHandle(this->stdErrWr);
	return processStarted;
}

#define BUFSIZE 4096
// reads from the given handle until EOF
// returns a string containing all output
// remember to FREE THE RESULTING STRING when you're done with it!
BOOL CommandExec::readOutput(HANDLE handle, tstring& output){
	DWORD outputLen = BUFSIZE * 10;
	BOOL bSuccess = FALSE;
	DWORD Read = 0;
	DWORD converted = 0;
	CHAR chBuf[BUFSIZE];
	TCHAR TBuf[BUFSIZE+1];
	TBuf[BUFSIZE] = NULL;

	for (;;) {
		bSuccess = ReadFile(handle, chBuf, BUFSIZE, &Read, NULL);
		
		//upon reaching EOF, we will break out of the loop here,
		//during the next iteration (after the last read has been 
		//properly processed...
		if( ! bSuccess || Read == 0 ) break;

		#ifdef UNICODE
			// convert chBuf to TCHAR
			converted = MultiByteToWideChar(CP_UTF8, 0, chBuf, Read, TBuf, BUFSIZE);
			/*if (converted != Read){
				DWORD err = GetLastError();
				switch(err){
					case ERROR_INSUFFICIENT_BUFFER:
						// A supplied buffer size was not large enough, or it was incorrectly set to NULL.
						break;
					case ERROR_INVALID_FLAGS:
						// The values supplied for flags were not valid.
						break;
					case ERROR_INVALID_PARAMETER:
						// Any of the parameter values was invalid.
						break;
					case ERROR_NO_UNICODE_TRANSLATION:
						// Invalid Unicode was found in a string.
						break;
				}
				break;
			}*/
			// zero terminate the string 
			TBuf[converted] = NULL;
		#else
			TBuff = chBuff;
		#endif

		output.append(TBuf); 
	}
	return TRUE;
}

BOOL CommandExec::Wait(void)
{
	if (!GetExitCodeProcess(this->pi.hProcess, &this->exitStatus)) return FALSE;
	while(this->exitStatus == STILL_ACTIVE){
		if (!GetExitCodeProcess(this->pi.hProcess, &this->exitStatus)) return FALSE;
	}
	CloseHandle(this->pi.hProcess);
	CloseHandle(this->pi.hThread);
	return TRUE;
}

DWORD CommandExec::ExitStatus(void)
{
	return this->exitStatus;
}

BOOL CommandExec::ReadOutput(void)
{
	if ( ! this->readOutput(this->stdOutRd, this->stdOut)){
		return FALSE;
	}
	this->StdOutRead = TRUE;
	if (CloseHandle(this->stdOutRd)) this->StdOutClosed = TRUE;

	if ( ! this->readOutput(this->stdErrRd, this->stdErr)){
		return FALSE;
	}
	this->StdErrRead = TRUE;
	if (CloseHandle(this->stdErrRd)) this->StdErrClosed = TRUE;
	
	return TRUE;
}

tstring CommandExec::GetStdOut(void){
	return tstring(this->stdOut);
}

tstring CommandExec::GetStdErr(void){
	return tstring(this->stdErr);
}


tstring CommandExec::GetCombined(void)
{
	tstring out = tstring(this->stdOut);
	out.append(this->stdErr);
	return out;
}

