#include "goCommand.h"
#include <shlwapi.h>

goCommand::goCommand(LPCTSTR cmd, LPCTSTR flags, bool wants_pkg)
{
	this->cmd = cmd;
	this->flags = flags;
	this->wantsPkg = wants_pkg;

	this->currentDir = NULL;
	this->currentFile = NULL;

	this->goPath = NULL;
	this->goPkg = NULL;

	this->commandLine = NULL;
	this->stdOut = NULL;
	this->stdErr = NULL;
}


goCommand::~goCommand(void)
{
	if ( currentDir != NULL) free(currentDir);
	if ( currentFile != NULL) free(currentFile);

	if ( goPath != NULL) free(goPath);
	if ( goPkg != NULL) free(goPkg);

	if ( commandLine != NULL) free(commandLine);
	if ( stdOut != NULL) free(stdOut);
	if ( stdErr != NULL) free(stdErr);
}

DWORD goCommand::runCmd(LPCTSTR go_cmd, LPTSTR current_file){
	if ( ! this->initializeFileVals(current_file)) return 100;
	if ( ! this->buildCommandLine(go_cmd)) return 101;
	
	CommandExec *exec = new CommandExec(this->commandLine, this->currentDir);
	if ( ! exec->Start()) return 200;
	if ( ! exec->Wait()) return 201;
	if ( ! exec->ReadOutput()) return 203;

	this->exitStatus = exec->ExitStatus();
	this->stdOut = exec->GetStdOut();
	this->stdErr = exec->GetStdErr();

	delete(exec);
	return this->exitStatus;
}

LPTSTR goCommand::GetCommand(void)
{
	if (this->commandLine == NULL) return NULL;

	LPTSTR cl = (LPTSTR) _tcsdup(this->commandLine);
	return cl;
}

LPTSTR goCommand::GetstdOut(void)
{
	if (this->stdOut == NULL) return NULL;

	LPTSTR cl = (LPTSTR) _tcsdup(this->stdOut);
	return cl;
}

LPTSTR goCommand::GetstdErr(void)
{
	if (this->stdErr == NULL) return NULL;

	LPTSTR cl = (LPTSTR) _tcsdup(this->stdErr);
	return cl;
}



BOOL goCommand::initializeFileVals(LPTSTR current_file)
{
	this->currentFile = (LPTSTR) _tcsdup(current_file);
	if (this->currentFile == NULL) return FALSE;

	this->currentDir = (LPTSTR) _tcsdup(this->currentFile);
	if (this->currentDir == NULL) return FALSE;
	if ( ! PathRemoveFileSpec(this->currentDir)) return FALSE;

	return TRUE;
}

// path operations
BOOL goCommand::initializeGoVals(void){
	LPCTSTR pathSep = TEXT("\\");
	LPTSTR dir = NULL;
	LPTSTR start_address = NULL;
	LPTSTR pkg = NULL;

	dir = (LPTSTR) _tcsdup(this->currentDir);
	if (dir == NULL) return FALSE;
	start_address = dir;

	this->goPath = (LPTSTR) calloc(MAX_PATH, sizeof(TCHAR));
	if (this->goPath == NULL) {
		free(start_address);
		return FALSE;
	}

	this->goPkg = (LPTSTR) calloc(MAX_PATH, sizeof(TCHAR));
	if (this->goPkg == NULL){
		free(start_address);
		return FALSE;
	}
	
	dir = _tcstok_s(dir, pathSep, &pkg);
	_tcsncpy(this->goPath, dir, _tcslen(dir)+1);
	while(dir != NULL){
		if (_tcscmp(dir, TEXT("src")) == 0){
			_tcsncpy(this->goPkg, pkg, _tcslen(pkg)+1);
			free(start_address);
			return TRUE;
		}
		PathAppend(this->goPath, dir);
		dir = _tcstok_s(NULL, pathSep, &pkg);
	}
	free(start_address);
	return FALSE;
}

BOOL goCommand::buildCommandLine(LPCTSTR go_cmd){
	TCHAR pkg[MAX_PATH];
	if (this->wantsPkg){
		if( ! this->initializeGoVals()) return FALSE;
		_tcsncpy(pkg, this->goPkg, MAX_PATH);
	} else {
		_tcsncpy(pkg, this->currentFile, MAX_PATH);
	}
	PathQuoteSpaces(pkg);

	size_t size = _tcslen(go_cmd) + _tcslen(this->cmd) +_tcslen(pkg) + 2;
	LPTSTR args = (LPTSTR) calloc(size + 1, sizeof(TCHAR));
	if (args == NULL) return FALSE;

	_tcsncat(args, go_cmd, __min(_tcslen(go_cmd), size - _tcslen(args)));
	_tcsncat(args, _T(" "), __min(sizeof(args), size - _tcslen(args)));
	_tcsncat(args, cmd, __min( _tcslen(cmd), size -_tcslen(args)));
	_tcsncat(args, _T(" "), __min(sizeof(args), size - _tcslen(args)));
	_tcsncat(args, pkg, __min( _tcslen(pkg), size -_tcslen(args)));

	this->commandLine = args;
	return TRUE;
}


void goCommand::DisplayOutput(NppData nppData)
{
	switch(this->exitStatus){
		case 2:
			::MessageBox(nppData._nppHandle, this->stdErr, this->commandLine, MB_OK);
			break;
		case 1:
		case 0:
			::MessageBox(nppData._nppHandle, this->stdOut, this->commandLine, MB_OK);
			break;
	}
}