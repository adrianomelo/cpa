/***************************************************************
 * Name:      XCPAApp.h
 * Purpose:   Defines Application Class
 * Author:    gustavo ()
 * Created:   2009-02-02
 * Copyright: gustavo ()
 * License:
 **************************************************************/

#ifndef XCPAAPP_H
#define XCPAAPP_H

#include <wx/app.h>



class XCPAApp : public wxApp
{
    public:
        virtual bool OnInit();
        virtual int OnExit();


};

DECLARE_APP(XCPAApp)

#endif // XCPAAPP_H
