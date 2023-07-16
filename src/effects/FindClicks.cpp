// SPDX-License-Identifier: GPL-2.0-or-later
/**********************************************************************

  Audacity: A Digital Audio Editor

  FindClicks.cpp

  Steve Lhomme / Craig DeForest

*******************************************************************//**

\class EffectFindClick
\brief Locates clicks and inserts labels when found

  ClickRemoval effect turned into an analyzer

*//*******************************************************************/

class LabelTrack;

#include "LoadEffects.h"
#include "Effect.h"

#include "../shuttle/Shuttle.h"
#include "../shuttle/ShuttleGui.h"
#include "../widgets/valnum.h"

#include <wx/valgen.h>
#include <wx/textctrl.h>

#include "../LabelTrack.h"
#include "../WaveTrack.h"

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name       Type     Key               Def      Min      Max      Scale
Param( Threshold, int,     wxT("Threshold"),  200,     0,       900,     1  );
Param( Width,     int,     wxT("Width"),      20,      0,       40,      1  );

class EffectFindClick final : public Effect
{
public:
    static const ComponentInterfaceSymbol Symbol;

    EffectFindClick();
    virtual ~EffectFindClick();

    // ComponentInterface implementation

    ComponentInterfaceSymbol GetSymbol() override;
    TranslatableString GetDescription() override;
    ManualPageID ManualPage() override;

    // EffectDefinitionInterface implementation

    EffectType GetType() override;

    // EffectClientInterface implementation

    bool DefineParams( ShuttleParams & S ) override;
    bool GetAutomationParameters(CommandParameters & parms) override;
    bool SetAutomationParameters(CommandParameters & parms) override;

    // Effect implementation
    void PopulateOrExchange(ShuttleGui &);
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    bool Process() override;

private:

    void OnWidthText(wxCommandEvent & evt);
    void OnThreshText(wxCommandEvent & evt);
    void OnWidthSlider(wxCommandEvent & evt);
    void OnThreshSlider(wxCommandEvent & evt);

private:
    int mThreshold = DEF_Threshold;
    int mWidth     = DEF_Width;

    wxSlider *mWidthS = nullptr;
    wxSlider *mThreshS = nullptr;
    wxTextCtrl *mWidthT = nullptr;
    wxTextCtrl *mThreshT = nullptr;

    bool ProcessOne(LabelTrack *, int count, const WaveTrack *,
                    sampleCount start, sampleCount len);
    void MarkClicks(size_t len, Floats &buffer, LabelTrack *, const WaveTrack *, sampleCount start, wxString & label) const;

    DECLARE_EVENT_TABLE()
};

enum
{
   ID_Thresh = 10000,
   ID_Width
};


// Implementation
const ComponentInterfaceSymbol EffectFindClick::Symbol
{ XO("Find Clicks") };

namespace{ BuiltinEffectsModule::Registration< EffectFindClick > reg; }

BEGIN_EVENT_TABLE(EffectFindClick, wxEvtHandler)
    EVT_SLIDER(ID_Thresh, EffectFindClick::OnThreshSlider)
    EVT_SLIDER(ID_Width, EffectFindClick::OnWidthSlider)
    EVT_TEXT(ID_Thresh, EffectFindClick::OnThreshText)
    EVT_TEXT(ID_Width, EffectFindClick::OnWidthText)
END_EVENT_TABLE()

EffectFindClick::EffectFindClick()
{
}

EffectFindClick::~EffectFindClick()
{
}


// ComponentInterface implementation

ComponentInterfaceSymbol EffectFindClick::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectFindClick::GetDescription()
{
   return XO("Creates labels where clicks are detected");
}

ManualPageID EffectFindClick::ManualPage()
{
   return L"Find_Clicks";
}

// EffectDefinitionInterface implementation

EffectType EffectFindClick::GetType()
{
   return EffectTypeAnalyze;
}

// EffectClientInterface implementation

bool EffectFindClick::DefineParams( ShuttleParams & S )
{
   S.SHUTTLE_PARAM( mThreshold, Threshold );
   S.SHUTTLE_PARAM( mWidth, Width );
   return true;
}

bool EffectFindClick::GetAutomationParameters(CommandParameters & parms)
{
   parms.Write(KEY_Threshold, mThreshold);
   parms.Write(KEY_Width, mWidth);

   return true;
}

bool EffectFindClick::SetAutomationParameters(CommandParameters & parms)
{
   ReadAndVerifyInt(Threshold);
   ReadAndVerifyInt(Width);

   mThreshold = Threshold;
   mWidth = Width;

   return true;
}

// Effect implementation

//// UI implementation

void EffectFindClick::PopulateOrExchange(ShuttleGui & S)
{
   S.AddSpace(0, 5);
   S.SetBorder(10);

   S.StartMultiColumn(3, wxEXPAND);
   S.SetStretchyCol(2);
   {
      // Threshold
      mThreshT = S.Id(ID_Thresh)
         .Validator<IntegerValidator<int>>(
            &mThreshold, NumValidatorStyle::DEFAULT,
            MIN_Threshold, MAX_Threshold
         )
         .AddTextBox(XXO("&Threshold (lower is more sensitive):"),
                     wxT(""),
                     10);

      mThreshS = S.Id(ID_Thresh)
         .Name(XO("Threshold"))
         .Style(wxSL_HORIZONTAL)
         .Validator<wxGenericValidator>(&mThreshold)
         .MinSize( { 150, -1 } )
         .AddSlider( {}, mThreshold, MAX_Threshold, MIN_Threshold);

      // Click width
      mWidthT = S.Id(ID_Width)
         .Validator<IntegerValidator<int>>(
            &mWidth, NumValidatorStyle::DEFAULT, MIN_Width, MAX_Width)
         .AddTextBox(XXO("Max &Spike Width (higher is more sensitive):"),
                     wxT(""),
                     10);

      mWidthS = S.Id(ID_Width)
         .Name(XO("Max Spike Width"))
         .Style(wxSL_HORIZONTAL)
         .Validator<wxGenericValidator>(&mWidth)
         .MinSize( { 150, -1 } )
         .AddSlider( {}, mWidth, MAX_Width, MIN_Width);
   }
   S.EndMultiColumn();

   return;
}

bool EffectFindClick::TransferDataToWindow()
{
    return mUIParent->TransferDataToWindow();
}

bool EffectFindClick::TransferDataFromWindow()
{
    if (!mUIParent->Validate())
    {
        return false;
    }

    return mUIParent->TransferDataFromWindow();
}

void EffectFindClick::OnWidthText(wxCommandEvent & WXUNUSED(evt))
{
    mWidthT->GetValidator()->TransferFromWindow();
    mWidthS->GetValidator()->TransferToWindow();
}

void EffectFindClick::OnThreshText(wxCommandEvent & WXUNUSED(evt))
{
    mThreshT->GetValidator()->TransferFromWindow();
    mThreshS->GetValidator()->TransferToWindow();
}

void EffectFindClick::OnWidthSlider(wxCommandEvent & WXUNUSED(evt))
{
    mWidthS->GetValidator()->TransferFromWindow();
    mWidthT->GetValidator()->TransferToWindow();
}

void EffectFindClick::OnThreshSlider(wxCommandEvent & WXUNUSED(evt))
{
    mThreshS->GetValidator()->TransferFromWindow();
    mThreshT->GetValidator()->TransferToWindow();
}

//// Processing implementation

bool EffectFindClick::Process()
{
    std::shared_ptr<AddedAnalysisTrack> addedTrack;
    std::optional<ModifiedAnalysisTrack> modifiedTrack;
    const wxString name{ _("Clicks") };

    auto clt = *inputTracks()->Any< const LabelTrack >().find_if(
        [&]( const Track *track ){ return track->GetName() == name; } );

    LabelTrack *lt{};
    if (!clt)
    {
        addedTrack = AddAnalysisTrack(name);
        lt = addedTrack->get();
    }
    else
    {
        modifiedTrack.emplace(ModifyAnalysisTrack(clt, name));
        lt = modifiedTrack->get();
    }

    int trackCount = 0;

    for (auto t : inputTracks()->Selected< const WaveTrack >())
    {
        double trackStart = t->GetStartTime();
        double trackEnd = t->GetEndTime();
        double t0 = mT0 < trackStart ? trackStart : mT0;
        double t1 = mT1 > trackEnd ? trackEnd : mT1;

        if (t1 > t0)
        {
            auto start = t->TimeToLongSamples(t0);
            auto end = t->TimeToLongSamples(t1);
            auto len = end - start;

            if (!ProcessOne(lt, trackCount, t, start, len))
                return false;
        }

        trackCount++;
    }

    // No cancellation, so commit the addition of the track.
    if (addedTrack)
        addedTrack->Commit();
    if (modifiedTrack)
        modifiedTrack->Commit();

    return true;
}

bool EffectFindClick::ProcessOne(LabelTrack *lt, int count, const WaveTrack *track,
                                 sampleCount start, sampleCount len)
{
    wxString label = count % 2 ? wxString(_("Click Right")) : wxString(_("Click Left"));
    constexpr size_t windowSize = 8192;
    if (len <= windowSize / 2)
    {
        Effect::MessageBox(
            XO("Selection must be larger than %d samples.")
            .Format(windowSize / 2),
            wxOK | wxICON_ERROR );
        return false;
    }

    auto idealBlockLen = track->GetMaxBlockSize() * 4;
    if (idealBlockLen % windowSize != 0)
        idealBlockLen += (windowSize - (idealBlockLen % windowSize));

    bool bResult = true;
    decltype(len) s = 0;
    Floats buffer{ idealBlockLen };
    Floats datawindow{ windowSize };
    while ((len - s) > windowSize / 2)
    {
        auto block = limitSampleBufferSize( idealBlockLen, len - s );

        track->GetFloats(buffer.get(), start + s, block);

        for (decltype(block) i = 0; i + windowSize / 2 < block; i += windowSize / 2)
        {
            auto wcopy = std::min( windowSize, block - i );

            for(decltype(wcopy) j = 0; j < wcopy; j++)
                datawindow[j] = buffer[i+j];
            for(auto j = wcopy; j < windowSize; j++)
                datawindow[j] = 0;

            MarkClicks(windowSize, datawindow, lt, track,
                       start + s + i, label);

            for(decltype(wcopy) j = 0; j < wcopy; j++)
                buffer[i+j] = datawindow[j];
        }

        s += block;

        if (TrackProgress(count, s.as_double() / len.as_double()))
        {
            bResult = false;
            break;
        }
    }

    return bResult;
}


void EffectFindClick::MarkClicks(size_t windowSize, Floats & buffer, LabelTrack *lt, const WaveTrack *wt, sampleCount start, wxString & label) const
{
    size_t i;
    size_t j;
    size_t clickStart = SIZE_MAX;

    float msw;
    size_t ww;
    size_t s2 = windowSize / 4;
    Floats ms_seq{ windowSize };
    Floats b2{ windowSize };

    for( i=0; i<windowSize; i++)
        b2[i] = buffer[i]*buffer[i];

    /* Shortcut for rms - multiple passes through b2, accumulating
    * as we go.
    */
    for(i=0;i<windowSize;i++)
        ms_seq[i]=b2[i];

    /* All powers of two up to windowSize / 2 */
    for(i=1; i <windowSize / 2; i *= 2)
    {
        for(j=0;j<windowSize-i; j++)
            ms_seq[j] += ms_seq[j+i];
    }

    /* Normalize the first half */
    for( i=0; i<windowSize / 2; i++ )
        ms_seq[i] /= windowSize / 2;

    /* ww runs from about 4 to mWidth.  wrc is the reciprocal;
    * chosen so that integer roundoff doesn't clobber us.
    */
    for(auto wrc = (size_t)mWidth/4; wrc != 0; wrc /= 2)
    {
        ww = (size_t)mWidth/wrc;

        // scan half the window between 1/4 and 3/4 of the window
        for( i=0; i<windowSize / 2; i++ )
        {
            msw = 0; // mean energy of the width at window/4 after i
            for( j=0; j<ww; j++)
                msw += b2[i+s2 + j];
            msw /= ww;

            if(msw >= mThreshold * ms_seq[i]/10)
            {
                // energy at window/4 after i is higher than threshold at i
                if( clickStart == SIZE_MAX )
                    clickStart = i;
            }
            else if(clickStart != SIZE_MAX)
            {
                // energy at window/4 after i is back to normal at i
                if((i-clickStart) <= ww*2) // within the window
                {
                    double startTime = wt->LongSamplesToTime(start + s2 + clickStart);
                    double endTime   = wt->LongSamplesToTime(start + s2 + i         + ww);
                    lt->AddLabel(SelectedRegion(startTime, endTime), label);

                    float lv = buffer[s2+clickStart];
                    float rv = buffer[s2+i + ww];
                    for(j=clickStart+s2; j<i+s2+ww; j++)
                    {
                        // replace by ramp from lv to rv
                        buffer[j]= (rv*(j-(clickStart+s2)) + lv*(i+ww+s2-j))/(float)(i+ww-clickStart);
                        b2[j] = buffer[j]*buffer[j];
                    }
                }
                clickStart = SIZE_MAX;
            }
        }
    }
}
