///////////////////////////////////////////////////////////////////////////////
// Name:        svgapp.cpp
// Purpose:     Application which just shows wxTestSVGFrame
// Author:      PB
// Created:     2022-01-20
// Copyright:   (c) 2022 PB
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>

#include "svgframe.h"

//TODO: compare D2D1_RENDER_TARGET_TYPE_SOFTWARE and default

class wxTestSVGApp : public wxApp
{
    bool OnInit() override
    {
        SetVendorName("PB");
        SetAppName("wxTestSVG");

        (new wxTestSVGFrame)->Show();
        return true;
    }
}; wxIMPLEMENT_APP(wxTestSVGApp);
