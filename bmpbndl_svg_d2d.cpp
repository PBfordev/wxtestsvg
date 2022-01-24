/////////////////////////////////////////////////////////////////////////////
// Name:        bmpbndl_svg_d2d.h
// Purpose:     wxBitmapBundleImpl using Direct2D to rasterize SVG
// Author:      PB
// Created:     2022-01-18
// Copyright:   (c) 2022 PB
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


#include "bmpbndl_svg_d2d.h"

#ifdef wxHAS_BMPBUNDLE_IMPL_SVG_D2D

#include "wx/ffile.h"
#include "wx/platinfo.h"
#include "wx/msw/private/graphicsd2d.h"
#include "wx/rawbmp.h"

#include <combaseapi.h>

// Creates wxBitmapBundle using wxBitmapBundleImplSVGD2D
wxBitmapBundle CreateFromImplSVGD2D(const wxString& fileName, const wxSize& size)
{
    wxFFile file(fileName, "rb");

    if ( file.IsOpened() )
    {
        const wxFileOffset lenAsOfs = file.Length();
        if ( lenAsOfs != wxInvalidOffset )
        {
            const size_t len = static_cast<size_t>(lenAsOfs);

            wxCharBuffer buf(len);
            char* const ptr = buf.data();
            if ( file.Read(ptr, len) == len )
                return wxBitmapBundle::FromImpl(new wxBitmapBundleImplSVGD2D(ptr, size));
        }
    }

    return wxBitmapBundle();
}

// --- 8< ----------------------------------



// ============================================================================
// wxBitmapBundleImplSVGD2D implementation
// ============================================================================

// This is a maximum size wxBitmapBundleImplSVGD2D can rasterize to.
// The larger the size, the larger memory needed and the memory is
// allocated (ms_bitmap and ms_context)  upon the first use of
// wxBitmapBundleImplSVGD2D and freed only when wxWidgets shuts down.
// The size should be large enough for wxBitmapBundle purpose.
const wxSize wxBitmapBundleImplSVGD2D::ms_maxBitmapSize(512, 512);

bool wxBitmapBundleImplSVGD2D::ms_initialized = false;
wxCOMPtr<IWICImagingFactory> wxBitmapBundleImplSVGD2D::ms_WICIFactory;
wxCOMPtr<ID2D1Factory> wxBitmapBundleImplSVGD2D::ms_D1Factory;
wxCOMPtr<IWICBitmap> wxBitmapBundleImplSVGD2D::ms_bitmap;
wxCOMPtr<ID2D1DeviceContext5> wxBitmapBundleImplSVGD2D::ms_context;

wxBitmapBundleImplSVGD2D::wxBitmapBundleImplSVGD2D(const char* data, const wxSize& sizeDef)
    : wxBitmapBundleImplSVG(sizeDef)
{
    wxCHECK_RET(data, "null data");
    wxCHECK_RET(IsAvailable(), "wxBitmapBundleImplSVGD2D rasterization unavailable");

    wxCOMPtr<IStream> SVGStream;

    if ( CreateIStreamFromPtr(data, SVGStream) )
        CreateSVGDocument(SVGStream);
}

bool wxBitmapBundleImplSVGD2D::IsOk() const
{
    return m_SVGDocument.get() != NULL;
}

bool wxBitmapBundleImplSVGD2D::CreateSVGDocument(const wxCOMPtr<IStream>& SVGStream)
{
    const D2D1_SIZE_F viewportSize = D2D1::SizeF(32, 32); // viewportSize is ignored when creating SVGDocument

    HRESULT hr;

    hr = ms_context->CreateSvgDocument(SVGStream, viewportSize, &m_SVGDocument);
    if ( FAILED (hr) )
    {
        wxLogApiError("ID2D1DeviceContext5::CreateSvgDocument", hr);
        return false;
    }

    // @TODO: Deal with units?

    wxCOMPtr<ID2D1SvgElement> root;
    D2D1_SVG_LENGTH           width, height;

    m_SVGDocument->GetRoot(&root);

    width.value = height.value = 0;
    if ( root->IsAttributeSpecified(L"width") )
        root->GetAttributeValue(L"width", &width);
    if ( root->IsAttributeSpecified(L"height") )
        root->GetAttributeValue(L"height", &height);

    // If the SVG is missing width/height attributes,
    // try to get the viewbox
    if ( (width.value == 0. || height.value == 0.)
         && root->IsAttributeSpecified(L"viewBox") )
    {
        D2D1_SVG_VIEWBOX viewBox;

        hr = root->GetAttributeValue(L"viewBox", D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX, &viewBox, sizeof(viewBox));
        if ( SUCCEEDED(hr) )
        {
            width.value = viewBox.width;
            height.value = viewBox.height;

            wxLogDebug("SVG doesn't have width/height specified, using viewBox instead");
            m_SVGDocument->SetViewportSize(D2D1::SizeF(viewBox.width, viewBox.height));
        }
    }

    if ( width.value == 0. || height.value == 0. )
    {
        wxLogDebug("Couldn't obtain SVG dimensions");
        m_SVGDocument.reset();
        return false;
    }

    m_SVGDocumentDimensions = wxSize(width.value, height.value);
    return true;
}

// Obtains the bitmap drawn on ms_bitmap at (0, 0, size.x, size.y)
bool wxBitmapBundleImplSVGD2D::GetSVGBitmapFromSharedBitmap(const wxSize& size, wxBitmap& bmp)
{
    const WICRect lockRect = { 0, 0, size.x, size.y };

    HRESULT                  hr;
    wxCOMPtr<IWICBitmapLock> lock;

    hr = ms_bitmap->Lock(&lockRect, WICBitmapLockRead, &lock);
    if ( FAILED(hr) )
    {
        wxLogApiError("IWICBitmap::Lock", hr);
        return false;
    }

    wxBitmap bmpOut = wxBitmap(size, 32);

    if ( !bmpOut.IsOk() )
    {
        wxLogDebug("Failed to created wxBitmap bmpOut");
        return false;
    }

    UINT  stride     = 0;
    UINT  bufferSize = 0;
    BYTE* buffer     = NULL;

    hr = lock->GetStride(&stride);
    if ( FAILED(hr) )
    {
        wxLogApiError("IWICBitmapLock::GetStride", hr);
        return false;
    }

    if ( stride != static_cast<UINT>(ms_maxBitmapSize.x) * 4 )
    {
        wxLogDebug("Unexpected bitmap stride");
        return false;
    }

    hr = lock->GetDataPointer(&bufferSize, &buffer);
    if ( FAILED(hr) )
    {
        wxLogApiError("IWICBitmapLock::GetDataPointer", hr);
        return false;
    }

    const unsigned char* src = &buffer[0];
    // if size.x < ms_maxBitmapSize.x, we need to skip the rest of the ms_bitmap row
    const size_t         rowBytesToSkip = (ms_maxBitmapSize.x - size.x) * 4;

    wxAlphaPixelData           bmpdata(bmpOut);
    wxAlphaPixelData::Iterator dst(bmpdata);

    for ( int y = 0; y < size.y; ++y )
    {
        dst.MoveTo(bmpdata, 0, y);

        // @TODO: Make sure the alpha are OK

        for ( int x = 0; x < size.x; ++x )
        {
            const unsigned char a = src[3];

            dst.Red()   = src[0] * a / 255;
            dst.Green() = src[1] * a / 255;
            dst.Blue()  = src[2] * a / 255;
            dst.Alpha() = a;

            ++dst;
            src += 4;
        }
        src += rowBytesToSkip;
    }

    bmp = bmpOut;
    return true;
}

wxBitmap wxBitmapBundleImplSVGD2D::DoRasterize(const wxSize& size)
{
    if ( !IsOk() )
    {
        wxLogDebug("invalid m_SVGDocument");
        return wxBitmap();
    }

    if ( !CanRasterizeAtSize(size) )
    {
        wxLogDebug("invalid rasterization size %dx%d", size.x, size.y);
        return wxBitmap();
    }

    const float         scaleValue = wxMin((float)size.x / m_SVGDocumentDimensions.x, (float)size.y / m_SVGDocumentDimensions.y);
    const D2D1_POINT_2F scaleCenter = D2D1::Point2F(size.x / 2.0f, size.y / 2.0f);
    const D2D1_SIZE_F   transl = D2D1::SizeF((size.x - m_SVGDocumentDimensions.x) / 2.0f, (size.y - m_SVGDocumentDimensions.y) / 2.0f);

    D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Identity();
    wxBitmap          bmp;

    // center
    transform = transform * D2D1::Matrix3x2F::Translation(transl);
    // scale
    transform = transform * D2D1::Matrix3x2F::Scale(scaleValue, scaleValue, scaleCenter);

    ms_context->BeginDraw();
    ms_context->Clear();
    ms_context->SetTransform(transform);
    ms_context->DrawSvgDocument(m_SVGDocument);
    ms_context->EndDraw();

    if ( GetSVGBitmapFromSharedBitmap(size, bmp) )
        return bmp;

    return wxBitmap();
}


// static
void wxBitmapBundleImplSVGD2D::Initialize()
{
    if ( IsInitialized() )
        return;

    ms_initialized = true;

    const wxPlatformInfo& platformInfo = wxPlatformInfo::Get();

    // we need at least Windows 10 Fall Creators Update
    if ( !platformInfo.CheckOSVersion(10, 0, 16299) )
        return;

    HRESULT hr;

    hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, wxIID_PPV_ARGS(IWICImagingFactory, &ms_WICIFactory));
    if ( FAILED(hr) )
    {
        wxLogApiError("CoCreateInstance::CLSID_WICImagingFactory", hr);
        return;
    }

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &ms_D1Factory);
    if ( FAILED(hr) )
    {
        wxLogApiError("D2D1CreateFactory", hr);
        return;
    }

    UINT    width, height;

    width  = static_cast<UINT>(ms_maxBitmapSize.x);
    height = static_cast<UINT>(ms_maxBitmapSize.y);

    hr = ms_WICIFactory->CreateBitmap(width, height, GUID_WICPixelFormat32bppPRGBA, WICBitmapCacheOnDemand, &ms_bitmap);
    if ( FAILED(hr) )
    {
        wxLogApiError("IWICImagingFactory::CreateBitmap", hr);
        return;
    }

    D2D1_RENDER_TARGET_PROPERTIES props;
    wxCOMPtr<ID2D1RenderTarget>   target;

    props.type                  = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    props.pixelFormat.format    = DXGI_FORMAT_UNKNOWN;
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_UNKNOWN;
    props.dpiX = props.dpiY     = 96;
    props.usage                 = D2D1_RENDER_TARGET_USAGE_NONE;
    props.minLevel              = D2D1_FEATURE_LEVEL_DEFAULT;

    hr = ms_D1Factory->CreateWicBitmapRenderTarget(ms_bitmap, props, &target);
    if ( FAILED(hr) )
    {
        wxLogApiError("ID2D1Factory::CreateWicBitmapRenderTarget", hr);
        ms_bitmap.reset();
        return;
    }

    hr = target->QueryInterface(wxIID_PPV_ARGS(ID2D1DeviceContext5, &ms_context));
    if ( FAILED(hr) )
    {
        wxLogApiError("ID2D1RenderTarget::QueryInterface(ID2D1DeviceContext5)", hr);
        target.reset();
        ms_bitmap.reset();
        return;
    }
}

// static
void wxBitmapBundleImplSVGD2D::Uninitialize()
{
    ms_context.reset();
    ms_bitmap.reset();
    ms_D1Factory.reset();
    ms_WICIFactory.reset();
    ms_initialized = false;
}

// static
bool wxBitmapBundleImplSVGD2D::IsInitialized()
{
    return ms_initialized;
}

// static
bool wxBitmapBundleImplSVGD2D::IsAvailable()
{
    if ( !IsInitialized() )
        Initialize();

    return ms_context.get() != NULL;
}

// static
bool wxBitmapBundleImplSVGD2D::wxBitmapBundleImplSVGD2D::CanRasterizeAtSize(const wxSize& size)
{
    const int width  = size.x;
    const int height = size.y;

    return width > 0 && height > 0
           && width <= ms_maxBitmapSize.x
           && height <= ms_maxBitmapSize.y;
}

// static
 bool wxBitmapBundleImplSVGD2D::CreateIStreamFromPtr(const char* data, wxCOMPtr<IStream>& SVGStream)
 {
     const size_t dataLen = strlen(data);

    // It could be better to use SHCreateMemStream, but more complicated
    // to implement: https://docs.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-shcreatememstream#remarks

    HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, dataLen);

    if ( !hMem )
    {
        wxLogDebug("Failed to allocate memory for SVG stream");
        return false;
    }

    LPVOID buff = ::GlobalLock(hMem);

    if ( !buff )
    {
        wxLogDebug("Failed to lock memory for SVG stream");
        ::GlobalFree(hMem);
        return false;
    }

    memcpy(buff, data, dataLen);
    ::GlobalUnlock(hMem);

    const HRESULT hr = ::CreateStreamOnHGlobal(hMem, TRUE, &SVGStream);

    if ( FAILED(hr) )
    {
        ::GlobalFree(hMem);
        wxLogApiError("::CreateStreamOnHGlobal", hr);
        return false;
    }

    ULARGE_INTEGER largeSize;

    largeSize.QuadPart = dataLen;
    SVGStream->SetSize(largeSize);
    return true;
 }

class wxBitmapBundleImplSVGD2DModule : public wxModule
{
public:
    wxBitmapBundleImplSVGD2DModule()
    {}

    virtual bool OnInit() wxOVERRIDE
    {
        // we will initialize static members of wxBitmapBundleImplSVGD2D
        // only on demand, in wxBitmapBundleImplSVGD2D::Initialize()
        return true;
    }

    virtual void OnExit() wxOVERRIDE
    {
        wxBitmapBundleImplSVGD2D::Uninitialize();
    }

private:
    wxDECLARE_DYNAMIC_CLASS(wxBitmapBundleImplSVGD2DModule);
};

wxIMPLEMENT_DYNAMIC_CLASS(wxBitmapBundleImplSVGD2DModule, wxModule);

#endif // #ifdef wxHAS_BMPBUNDLE_IMPL_SVG_D2D