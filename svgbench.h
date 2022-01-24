///////////////////////////////////////////////////////////////////////////////
// Name:        svgbench.h
// Purpose:     Benchmark SVG rasterization with NanoSVG and Direct2D
// Author:      PB
// Created:     2022-01-20
// Copyright:   (c) 2022 PB
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef TEST_SVG_BENCH_H_DEFINED
#define TEST_SVG_BENCH_H_DEFINED

#include <vector>

#include <wx/wx.h>

// ============================================================================
// wxTestSVGRasterizationBenchmark
// ============================================================================

class wxTestSVGRasterizationBenchmark
{
public:
    wxTestSVGRasterizationBenchmark();

    void Setup(const wxString& dirName, const wxArrayString& fileNames,
               const std::vector<wxSize>& sizes);

    bool Run(bool hasD2DSVG, size_t runCount, wxString& report, wxString& detailedReport);

private:
    // times in ms for one file and one bitmap size
    typedef std::vector<long>               VectorLong;
    typedef std::vector<VectorLong>         MatrixLong2;
    typedef std::vector<MatrixLong2>        MatrixLong3;

    struct Stats
    {
        long min{0};
        long max{0};
        long mdn{0};
        long avg{0};
    };
    typedef std::vector<Stats>       VectorStats;
    typedef std::vector<VectorStats> MatrixStats;

    typedef wxBitmapBundle (*CreateBitmapBundleFn)(const wxString&);

    wxString            m_dirName;
    wxArrayString       m_fileNames;
    std::vector<wxSize> m_sizes;

    // benchmarks a single file for all bitmap sizes
    bool BenchmarkFile(CreateBitmapBundleFn createBundleFn,
                       const wxString& fileName,
                       size_t runCount, MatrixLong2& times);

    void CreateReport(const MatrixStats& statsNano, const MatrixStats* statsD2D,
                      size_t runCount, wxString& reportText);

    void CreateDetailedReport(const MatrixLong3& timesNano, const MatrixStats& statsNano,
                              const MatrixLong3* timesD2D, const MatrixStats* statsD2D,
                              bool asHTML, wxString& reportText);

    static Stats CalcStatsForVectorLong(const VectorLong& data);
};


// ============================================================================
// wxTestSVGBenchmarkReportFrame
// ============================================================================

class wxTestSVGBenchmarkReportFrame: public wxFrame
{
public:
    wxTestSVGBenchmarkReportFrame(wxWindow* parent, const wxString& dirName,
                                  const wxString& report, const wxString& detailedReport);
private:
    enum 
    {
        ID_SAVE_DETAILED = wxID_HIGHEST + 1
    };

    wxString m_dirName;
    wxString m_defaultName;
    wxString m_report, m_detailedReport;
    
    void OnSaveReport(wxCommandEvent&);
    void OnSaveDetailedReport(wxCommandEvent&);

    static bool WriteHTMLReport(const wxString& fileName, const wxString& reportText);
};

#endif // #ifndef TEST_SVG_BENCH_H_DEFINED