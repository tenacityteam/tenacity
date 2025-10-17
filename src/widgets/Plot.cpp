/**********************************************************************

   Tenacity

   Plot.cpp

   Max Maisel

*******************************************************************//**

\class Plot
\brief A customizable generic plot widget.

*//*******************************************************************/


#include "Plot.h"
#include "LinearDBFormat.h"
#include "LinearUpdater.h"
#include "Ruler.h"
#include "AColor.h"
#include "ThemeLegacy.h"
#include "AllThemeResources.h"

#include <wx/brush.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>

Plot::Plot(wxWindow *parent, wxWindowID winid,
    float x_min, float x_max, float y_min, float y_max,
    const TranslatableString& xlabel, const TranslatableString& ylabel,
    const RulerUpdater& xupdater, const RulerFormat& xformat,
    const RulerUpdater& yupdater, const RulerFormat& yformat, int count,
    const wxPoint& pos, const wxSize& size, long style)
    :
    wxPanelWrapper(parent, winid, pos, size, style),
    m_xmin(x_min), m_xmax(x_max), m_ymin(y_min), m_ymax(y_max),
    m_plots(count)
{
    Bind(wxEVT_PAINT, &Plot::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent) { // Resize handler
        Refresh(false);
    });

    m_xruler = std::make_unique<Ruler>(xupdater, xformat);
    m_xruler->SetOrientation(wxHORIZONTAL);
    m_xruler->SetUnits(xlabel);
    m_xruler->SetFlip(true);

    m_yruler = std::make_unique<Ruler>(yupdater, yformat);
    m_yruler->SetOrientation(wxVERTICAL);
    m_yruler->SetUnits(ylabel);
}

void Plot::OnPaint(wxPaintEvent & evt)
{
    wxPaintDC dc(this);

    int width, height;
    GetSize(&width, &height);

    #if defined(__WXMSW__)
    dc.Clear();
    #endif

    // Ruler
    int w = 0;
    int h = 0;

    m_xruler->SetBounds(0, 0, width, height);
    m_xruler->SetRange(m_xmin, m_xmax);
    m_xruler->GetMaxSize(NULL, &h);

    m_yruler->SetBounds(0, 0, width, height);
    m_yruler->SetRange(m_ymax, m_ymin);
    m_yruler->GetMaxSize(&w, NULL);

    m_xruler->SetBounds(w, height - h, width, height);
    m_yruler->SetBounds(0, 0, w, height - h);

    m_xruler->SetTickColour( theTheme.Colour( clrGraphLabels ));
    m_yruler->SetTickColour( theTheme.Colour( clrGraphLabels ));

    wxRect border;
    border.x = w;
    border.y = 0;
    border.width = width - w;
    border.height = height - h + 1;

    dc.SetBrush(*wxWHITE_BRUSH);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(border);

    m_xruler->DrawGrid(dc, border.height, true, true, border.x, border.y);
    m_yruler->DrawGrid(dc, border.width, true, true, border.x, border.y);

    for(const auto& plot : m_plots)
    {
        wxASSERT(plot.xdata.size() == plot.ydata.size());
        if(plot.xdata.size() == 0)
            continue;
        dc.SetPen(*plot.pen);

        size_t xsize = plot.xdata.size();
        for(size_t i = 1; i < xsize; ++i)
        {
            AColor::Line(dc,
                XToScreen(plot.xdata[i-1], border),
                YToScreen(plot.ydata[i-1], border),
                XToScreen(plot.xdata[i], border),
                YToScreen(plot.ydata[i], border));
        }
    }

    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(*wxBLACK_PEN);
    dc.DrawRectangle(border);
    m_xruler->Draw(dc);
    m_yruler->Draw(dc);
}

void Plot::OnSize(wxSizeEvent & evt)
{
    Refresh(false);
}

int Plot::XToScreen(float x, wxRect& rect)
{
    return rect.x + lrint((x-m_xmin)*rect.width/(m_xmax-m_xmin));
}

int Plot::YToScreen(float y, wxRect& rect)
{
    return rect.y + rect.height - lrint((y-m_ymin)*rect.height/(m_ymax-m_ymin));
}
