/////////////////////////////////////////////////////////////////////////////
// Name:        bmpbndl_svg_d2d.h
// Purpose:     wxBitmapBundleImpl using Direct2D to rasterize SVG
// Author:      PB
// Created:     2022-01-18
// Copyright:   (c) 2022 PB
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef wxBitmapBundleImplSVGD2D_PRIVATE_H
#define wxBitmapBundleImplSVGD2D_PRIVATE_H

#include "wx/wx.h"

#if defined(__WXMSW__) && wxUSE_GRAPHICS_DIRECT2D
    #ifdef __has_include
        #if __has_include(<d2d1_3.h>)
            #include <d2d1_3.h>

            #ifdef _D2D1_SVG_
                #define wxHAS_BMPBUNDLE_IMPL_SVG_D2D
            #endif // #ifdef _D2D1_SVG_

        #endif // #if __has_include("d2d1_3.h")
    #endif // #ifdef __has_include
#endif // #if defined(__WXMSW__) && wxUSE_GRAPHICS_DIRECT2D

#ifdef wxHAS_BMPBUNDLE_IMPL_SVG_D2D

#include "wx/bmpbndl.h"
#include "wx/msw/private/comptr.h"

// Creates wxBitmapBundle using wxBitmapBundleImplSVGD2D
wxBitmapBundle CreateFromImplSVGD2D(const wxString& fileName, const wxSize& size);

// --- 8< ----------------------------------



// ============================================================================
// wxBitmapBundleImplSVG
// ============================================================================

class wxBitmapBundleImplSVG : public wxBitmapBundleImpl
{
public:
    wxBitmapBundleImplSVG(const wxSize& sizeDef)
        : m_sizeDef(sizeDef)
    {
    }

    virtual wxSize GetDefaultSize() const wxOVERRIDE
    {
        return m_sizeDef;
    };

    virtual wxSize GetPreferredSizeAtScale(double scale) const wxOVERRIDE
    {
        return m_sizeDef*scale;
    }

    virtual wxBitmap GetBitmap(const wxSize& size) wxOVERRIDE
    {
        if ( !m_cachedBitmap.IsOk() || m_cachedBitmap.GetSize() != size )
        {
            m_cachedBitmap = DoRasterize(size);
        }

        return m_cachedBitmap;
    }

protected:
    virtual wxBitmap DoRasterize(const wxSize& size) = 0;

    const wxSize m_sizeDef;

    // Cache the last used bitmap (may be invalid if not used yet).
    //
    // Note that we cache only the last bitmap and not all the bitmaps ever
    // requested from GetBitmap() for the different sizes because there would
    // be no way to clear such cache and its growth could be unbounded,
    // resulting in too many bitmap objects being used in an application using
    // SVG for all of its icons.
    wxBitmap m_cachedBitmap;

    wxDECLARE_NO_COPY_CLASS(wxBitmapBundleImplSVG);
};


// ============================================================================
// wxBitmapBundleImplSVGD2D declaration
// ============================================================================

/*
    wxBitmapBundleImpl using SVG support in Direct2D to
    rasterize an SVG to wxBitmap at given size.

    Run-time limitations: requires Windows 10 Fall Creators Update
    and newer, for limitations of the Direct2D SVG renderer see
    https://docs.microsoft.com/en-us/windows/win32/direct2d/svg-support
    For example, text elements are not supported at all.

    wxBitmapBundleImplSVGD2D implementation limitations: maximum size of
    bitmap for rasterization cannot be larger then wxBitmapBundleImplSVGD2D::ms_maxBitmapSize.
    The SVG must have its width and height specified, if it does not, its viewBox is used
    for the dimensions. If it does noth have width/height nor viewBox, the SVG cannot be rendered.
 */

class wxBitmapBundleImplSVGD2D : public wxBitmapBundleImplSVG
{
public:
    // data must be 0 terminated, wxBitmapBundleImplSVGD2D doesn't
    // take its ownership and it can be deleted after the ctor
    // was called.
    wxBitmapBundleImplSVGD2D(const char* data, const wxSize& sizeDef);

    bool IsOk() const;

    // only available on Windows 10 Fall Creators Update and newer
    static bool IsAvailable();

private:
    // maximum dimension of the bitmap an SVG can be rasterized to
    static const wxSize ms_maxBitmapSize;

    // whether there was an attempt to initialize, regardless of its success
    static bool ms_initialized;

    static wxCOMPtr<IWICImagingFactory>  ms_WICIFactory;
    static wxCOMPtr<ID2D1Factory>        ms_D1Factory;
    // large bitmap shared by all instances of wxBitmapBundleImplSVGD2D
    // on which the SVGs are rendered onto
    static wxCOMPtr<IWICBitmap>          ms_bitmap;
    static wxCOMPtr<ID2D1DeviceContext5> ms_context;

    static void Initialize();
    static void Uninitialize();
    static bool IsInitialized();

    // can rasterize bitmaps only up to ms_maxBitmapSize
    static bool CanRasterizeAtSize(const wxSize& size);

    static bool CreateIStreamFromPtr(const char* data, wxCOMPtr<IStream>& SVGStream);

    wxCOMPtr<ID2D1SvgDocument> m_SVGDocument;
    wxSize                     m_SVGDocumentDimensions;

    virtual wxBitmap DoRasterize(const wxSize& size) wxOVERRIDE;

    bool CreateSVGDocument(const wxCOMPtr<IStream>& SVGStream);

    bool GetSVGBitmapFromSharedBitmap(const wxSize& size, wxBitmap& bmp);

    friend class wxBitmapBundleImplSVGD2DModule;

    wxDECLARE_NO_COPY_CLASS(wxBitmapBundleImplSVGD2D);
};

#endif // #ifdef wxHAS_BMPBUNDLE_IMPL_SVG_D2D

#endif // #ifndef wxBitmapBundleImplSVGD2D_PRIVATE_H