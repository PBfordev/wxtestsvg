About
---------
wxTestSVG compares appearance and performance of NanoSVG and Direct2D
SVG renderers, when used as a rasterization tool for `wxBitmapBundle`.

![wxTestSVG Screenshot](wxtestsvg-screenshot.png?raw=true)


Surprisingly, NanoSVG is much faster than my implementation using
Direct2D, at least with simple SVGs rasterized at lower resolutions.
The Direct2D implementaton becomes faster only when rasterizing at 
higher resolutions (256+ pixels) or some more complex SVGs.
However, the reason for the Direct2D implementation being slow may be my
incorrect implementation of the Direct2D rasterizer, using `ID2D1DeviceContext5`
to render the SVG onto an `IWICBitmap`.

Neither renderer is even close enough to support anything near to
full SVG feature set, for example, they both lack any support for
the text element.


Build Requirements
---------
wxWidgets including NanoSVG, i.e., v3.1.6 and newer.
As any current mingw distribution lacks up-to-date Direct2D headers,
Direct2D rasterizer is available only with MSVC (2017+).


Runtime Requirements
---------
So far the less-incomplete support for SVG rendering with Direct2D is
available since Windows 10 Fall Creators Update (v1709) and this is 
the minumum supported Windows version.

Licence
---------
[wxWidgets licence](https://github.com/wxWidgets/wxWidgets/blob/master/docs/licence.txt)