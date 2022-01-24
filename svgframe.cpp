///////////////////////////////////////////////////////////////////////////////
// Name:        svgframe.h
// Purpose:     wxFrame for testing SVG rasterization with NanoSVG and Direct2D
// Author:      PB
// Created:     2022-01-20
// Copyright:   (c) 2022 PB
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////


#include <wx/wx.h>
#include <wx/busyinfo.h>
#include <wx/choicdlg.h>
#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/dcbuffer.h>
#include <wx/ffile.h>
#include <wx/filectrl.h>
#include <wx/filename.h>
#include <wx/numdlg.h>
#include <wx/slider.h>
#include <wx/splitter.h>
#include <wx/statline.h>
#include <wx/utils.h>

#include "svgframe.h"
#include "svgbench.h"
#include "bmpbndl_svg_d2d.h"

#ifndef wxHAS_BMPBUNDLE_IMPL_SVG_D2D
    #pragma message("Direct2D support for SVG unavailable")
#endif // #ifndef wxHAS_BMPBUNDLE_IMPL_SVG_D2D

// ============================================================================
// wxBitmapBundlePanel
// ============================================================================

class wxBitmapBundlePanel : public wxScrolledCanvas
{
public:
    wxBitmapBundlePanel(wxWindow* parent, const wxSize& bitmapSize);

    void SetBitmapBundle(const wxBitmapBundle& bundle);
    void SetBitmapSize(const wxSize& size);
private:
    wxBitmapBundle m_bitmapBundle;
    wxSize         m_bitmapSize;

    void OnPaint(wxPaintEvent&);
};

wxBitmapBundlePanel::wxBitmapBundlePanel(wxWindow* parent, const wxSize& bitmapSize)
    : wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE),
      m_bitmapSize(bitmapSize)
{
    wxASSERT(m_bitmapSize.x > 0 && m_bitmapSize.y > 0);

    SetScrollRate(FromDIP(4), FromDIP(4));

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &wxBitmapBundlePanel::OnPaint, this);
}

void wxBitmapBundlePanel::SetBitmapBundle(const wxBitmapBundle& bundle)
{
    m_bitmapBundle = bundle;
    Refresh(); Update();
}

void wxBitmapBundlePanel::SetBitmapSize(const wxSize& size)
{
    wxCHECK_RET(size.x > 0 && size.y > 0, "invalid bitmapSize");

    m_bitmapSize = size;

    if ( m_bitmapSize != GetVirtualSize() )
    {
        InvalidateBestSize();
        SetVirtualSize(m_bitmapSize);
    }

    Refresh(); Update();
}

void wxBitmapBundlePanel::OnPaint(wxPaintEvent&)
{
    const wxSize   clientSize(GetClientSize());

    wxAutoBufferedPaintDC dc(this);
    wxBitmap              bitmap;

    DoPrepareDC(dc);

    dc.SetBackground(*wxWHITE);
    dc.Clear();
    bitmap = m_bitmapBundle.GetBitmap(m_bitmapSize);


    if ( bitmap.IsOk() )
    {
        wxBrush          hatchBrush(*wxBLUE, wxBRUSHSTYLE_CROSSDIAG_HATCH);
        wxDCBrushChanger bc(dc, hatchBrush);
        wxDCPenChanger   pc(dc, wxNullPen);

        dc.DrawRectangle(wxPoint(0, 0), m_bitmapSize);
        dc.DrawBitmap(bitmap, 0, 0, true);
    }
}

// ============================================================================
// wxTestSVGFrame
// ============================================================================

wxTestSVGFrame::wxTestSVGFrame()
    : wxFrame(nullptr, wxID_ANY, "wxTestSVG")
{
    SetIcon(wxICON(wxICON_AAA)); // from wx.rc

    wxSplitterWindow* splitterMain = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition,
                                                          wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    wxPanel*          controlPanel = new wxPanel(splitterMain); // for controls
    wxPanel*          bitmapPanel  = new wxPanel(splitterMain); // for wxBitmapBundlePanels

    wxBoxSizer*       controlPanelSizer = new wxBoxSizer(wxVERTICAL);

    controlPanelSizer->Add(new wxStaticText(controlPanel, wxID_ANY, "&Bitmap Size"),
                           wxSizerFlags().CenterHorizontal().Border(wxALL & ~wxBOTTOM));

    m_bitmapSizeSlider = new wxSlider(controlPanel, wxID_ANY, m_bitmapSize.x, 16, 512,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxSL_HORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );
    m_bitmapSizeSlider->SetTickFreq(16);
    m_bitmapSizeSlider->Bind(wxEVT_SLIDER, &wxTestSVGFrame::OnBitmapSizeChanged, this);
    controlPanelSizer->Add(m_bitmapSizeSlider, wxSizerFlags().Expand().Border(wxALL & ~wxTOP));

    controlPanelSizer->Add(new wxStaticLine(controlPanel), wxSizerFlags().Expand().Border());

    wxButton* benchmarkFolderBtn = new wxButton(controlPanel, wxID_ANY, "Benchmark &Curent Folder...");
    benchmarkFolderBtn->Bind(wxEVT_BUTTON, &wxTestSVGFrame::OnBenchmarkFolder, this);
    controlPanelSizer->Add(benchmarkFolderBtn, wxSizerFlags().Expand().Border());

    wxButton* changeFolderBtn = new wxButton(controlPanel, wxID_ANY, "Change &Folder...");
    changeFolderBtn->Bind(wxEVT_BUTTON, &wxTestSVGFrame::OnChangeFolder, this);
    controlPanelSizer->Add(changeFolderBtn, wxSizerFlags().Expand().Border());

    m_fileCtrl = new wxFileCtrl(controlPanel, wxID_ANY, wxGetCwd(), ".", "SVG files (*.svg)|*.svg",
                                wxFC_DEFAULT_STYLE | wxFC_NOSHOWHIDDEN);
    m_fileCtrl->Bind(wxEVT_FILECTRL_FILEACTIVATED, &wxTestSVGFrame::OnFileActivated, this);
    m_fileCtrl->Bind(wxEVT_FILECTRL_SELECTIONCHANGED, &wxTestSVGFrame::OnFileSelected, this);
    controlPanelSizer->Add(m_fileCtrl, wxSizerFlags(1).Expand().Border());

    controlPanel->SetSizerAndFit(controlPanelSizer);

    m_panelNano = new wxBitmapBundlePanel(bitmapPanel, m_bitmapSize);
#ifdef wxHAS_BMPBUNDLE_IMPL_SVG_D2D
    if ( wxBitmapBundleImplSVGD2D::IsAvailable() )
        m_panelD2D = new wxBitmapBundlePanel(bitmapPanel, m_bitmapSize);
#endif // #ifdef wxHAS_BMPBUNDLE_IMPL_SVG_D2D
    wxFlexGridSizer* bitmapPanelSizer = new wxFlexGridSizer(m_panelD2D ? 2 : 1);

    bitmapPanelSizer->Add(new wxStaticText(bitmapPanel, wxID_ANY, "NanoSVG"),
                           wxSizerFlags().CenterHorizontal().Border(wxALL));

    if ( m_panelD2D )
    {
        bitmapPanelSizer->Add(new wxStaticText(bitmapPanel, wxID_ANY, "Direct2D"),
                               wxSizerFlags().CenterHorizontal().Border(wxALL));
    }

    bitmapPanelSizer->Add(m_panelNano, wxSizerFlags(1).Expand().Border());

    if ( m_panelD2D )
        bitmapPanelSizer->Add(m_panelD2D, wxSizerFlags(1).Expand().Border());

    bitmapPanelSizer->AddGrowableCol(0, 1);
    if ( bitmapPanelSizer->GetCols() > 1 )
        bitmapPanelSizer->AddGrowableCol(1, 1);
    bitmapPanelSizer->AddGrowableRow(1, 1);

    bitmapPanel->SetSizerAndFit(bitmapPanelSizer);

    SetMinClientSize(FromDIP(wxSize(800, 600)));
    splitterMain->SetMinimumPaneSize(FromDIP(100));
    splitterMain->SetSashGravity(0.3);
    splitterMain->SplitVertically(controlPanel, bitmapPanel, FromDIP(256));

    if ( !m_panelD2D )
        CallAfter([] { wxLogWarning("SVG rasterization with Direct2D unavailable."); } );
}

void wxTestSVGFrame::OnFileSelected(wxFileCtrlEvent& event)
{
    const wxFileName fileName(event.GetDirectory(), event.GetFile());

    m_panelNano->SetBitmapBundle(wxBitmapBundle::FromSVGFile(fileName.GetFullPath(), m_bitmapSize));
#ifdef wxHAS_BMPBUNDLE_IMPL_SVG_D2D
    if ( m_panelD2D )
        m_panelD2D->SetBitmapBundle(CreateFromImplSVGD2D(fileName.GetFullPath(), m_bitmapSize));
#endif // #ifdef wxHAS_BMPBUNDLE_IMPL_SVG_D2D


}

void wxTestSVGFrame::OnFileActivated(wxFileCtrlEvent& event)
{
   const wxFileName fileName(event.GetDirectory(), event.GetFile());

   wxLaunchDefaultApplication(fileName.GetFullPath());
}

void wxTestSVGFrame::OnBenchmarkFolder(wxCommandEvent&)
{
#ifndef NDEBUG
    if ( wxMessageBox("It appears you are running the debug version of the application, "
                      "which is much slower than the release one. Continue anyway?",
                      "Warning", wxYES_NO | wxNO_DEFAULT) != wxYES )
    {
        return;
    }
#endif
    const wxString dirName = m_fileCtrl->GetDirectory();
    wxArrayString  dirFiles;
    wxArrayString  files;
    wxArrayInt     selections;

    {
        wxBusyCursor bc;
        wxDir::GetAllFiles(dirName, &dirFiles, "*.svg", wxDIR_FILES);
    }

    if ( dirFiles.empty() )
    {
        wxLogMessage("No SVG files found in the current folder.");
        return;
    }

    for ( auto& f : dirFiles )
        f = wxFileName(f).GetFullName();

    dirFiles.Sort(wxNaturalStringSortAscending);

    selections.reserve(dirFiles.size());
    for ( size_t i = 0; i < dirFiles.size(); ++i )
        selections.push_back(i);

    if ( wxGetSelectedChoices(selections, wxString::Format("Select Files (%zu files available)", dirFiles.size()),
                              "Benchmark Rasterization", dirFiles, this) == -1
         || selections.empty() )
    {
        return;
    }

    for ( const auto& s : selections )
        files.push_back(dirFiles[s]);

    static const wxSize bitmapSizes[] =
      { {  16,  16},
        {  24,  24},
        {  32,  32},
        {  48,  48},
        {  64,  64},
        {  96,  96},
        { 128, 128},
        { 256, 256},
        { 512, 512} };

    wxArrayString bitmapSizesStrings;

    for ( const auto& bs : bitmapSizes)
        bitmapSizesStrings.push_back(wxString::Format("%d x %d", bs.x, bs.y));

    selections.clear();
    selections.push_back(1); //24
    selections.push_back(3); //48
    selections.push_back(6); //128

    if ( wxGetSelectedChoices(selections, "Select Bitmap Sizes", "Benchmark Rasterization", bitmapSizesStrings, this) == -1
         || selections.empty() )
        return;

    long runCount = wxGetNumberFromUser("Number of runs (between 10 and 100)", "Number", "Benchmark Rasterization", 25, 10, 100);

    if ( runCount == -1 )
        return;

    wxTestSVGRasterizationBenchmark benchmark;

    std::vector<wxSize> sizes;

    for ( const auto& s : selections )
        sizes.push_back(bitmapSizes[s]);

    benchmark.Setup(dirName, files, sizes);

    wxString report, detailedReport;
    bool result = false;

    {
        wxBusyInfo info(wxString::Format("Benchmarking %zu files at %zu sizes, please wait...", 
            files.size(), sizes.size()), this);
        result = benchmark.Run(m_panelD2D != nullptr, runCount, report, detailedReport);
    }

    if ( result )
        new wxTestSVGBenchmarkReportFrame(this, dirName, report, detailedReport);
}

void wxTestSVGFrame::OnChangeFolder(wxCommandEvent&)
{
    const wxString dir = wxDirSelector("Select Folder", m_fileCtrl->GetDirectory(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

    if ( !dir.empty() )
        m_fileCtrl->SetDirectory(dir);
}

void wxTestSVGFrame::OnBitmapSizeChanged(wxCommandEvent& event)
{
    m_bitmapSize = wxSize(event.GetInt(), event.GetInt());

    m_panelNano->SetBitmapSize(m_bitmapSize);
    if ( m_panelD2D )
        m_panelD2D->SetBitmapSize(m_bitmapSize);
}