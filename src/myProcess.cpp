#include <wx/process.h>
#include <wx/msgdlg.h>
#include "myProcess.h"

//myProcess::myProcess(wxEvtHandler *parent) : wxProcess(parent)
myProcess::myProcess() : wxProcess()
{
    Terminou = false;
}

myProcess::~myProcess()
{
	//(*Destroy(DlgNovaUnidade)
	//*)
}

void myProcess::OnTerminate(int pid, int status)
{
   //wxMessageBox(_T("Terminou"), "ola");
   Terminou = true;
}
