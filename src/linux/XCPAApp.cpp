/***************************************************************
 * Name:      XCPAApp.cpp
 * Purpose:   Code for Application Class
 * Author:    gustavo ()
 * Created:   2009-02-02
 * Copyright: gustavo ()
 * License:
 **************************************************************/

#include "XCPAApp.h"

//(*AppHeaders
#include "XCPAMainDlg.h"
#include <wx/image.h>
//*)

IMPLEMENT_APP(XCPAApp);

bool XCPAApp::OnInit()
{
    //(*AppInitialize
    bool wxsOK = true;
    wxInitAllImageHandlers();
    if ( wxsOK )
    {
    	XCPAMainDlg Dlg(0);
    	SetTopWindow(&Dlg);
    	Dlg.ShowModal();
    	wxsOK = false;
    }
    //*)
    return wxsOK;

}

int XCPAApp::OnExit()
{
    return 0;
}
