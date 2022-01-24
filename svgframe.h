///////////////////////////////////////////////////////////////////////////////
// Name:        svgframe.h
// Purpose:     wxFrame for testing SVG rasterization with NanoSVG and Direct2D
// Author:      PB
// Created:     2022-01-20
// Copyright:   (c) 2022 PB
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef TEST_SVG_FRAME_H_DEFINED
#define TEST_SVG_FRAME_H_DEFINED

#include <wx/wx.h>

class wxFileCtrl;
class wxFileCtrlEvent;

class wxBitmapBundlePanel;

class wxTestSVGFrame : public wxFrame
{
public:
    wxTestSVGFrame();
private:
    wxSize               m_bitmapSize{128, 128};

    wxSlider*            m_bitmapSizeSlider{nullptr};
    wxFileCtrl*          m_fileCtrl{nullptr};
    wxBitmapBundlePanel* m_panelNano{nullptr};
    wxBitmapBundlePanel* m_panelD2D{nullptr};

    void OnBenchmarkFolder(wxCommandEvent&);
    void OnChangeFolder(wxCommandEvent&);
    void OnFileSelected(wxFileCtrlEvent& event);
    void OnFileActivated(wxFileCtrlEvent& event);
    void OnBitmapSizeChanged(wxCommandEvent& event);
};

#endif // #ifndef TEST_SVG_FRAME_H_DEFINED