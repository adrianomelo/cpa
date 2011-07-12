#include "wx/process.h"
// This is the handler for process termination events
class myProcess : public wxProcess
{
public:
    //myProcess(wxEvtHandler *parent);
    myProcess();
    virtual ~myProcess();
    // instead of overriding this virtual function we might as well process the
    // event from it in the frame class - this might be more convenient in some
    // cases
    virtual void OnTerminate(int pid, int status);
    bool Terminou;
protected:
};
