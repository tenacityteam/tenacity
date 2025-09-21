/**********************************************************************

  Tenacity

  DynamicCompressor.h

  Max Maisel (based on Compressor effect)
  Avery King (split from the original Compressor2.h)

**********************************************************************/

#include "DynamicCompressor.h"

#include "../widgets/IntFormat.h"
#include "../widgets/LinearDBFormat.h"

#include "AColor.h"
#include "EffectInterface.h"
#include "EffectEditor.h"
#include "LoadEffects.h"
#include "ShuttleGui.h"
#include "SliderTextCtrl.h"

#include <memory>
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/valgen.h>

#include <numeric>

namespace{ BuiltinEffectsModule::Registration<EffectDynamicCompressor> reg; }

BEGIN_EVENT_TABLE(EffectDynamicCompressor, wxEvtHandler)
   EVT_CHECKBOX(wxID_ANY, EffectDynamicCompressor::OnUpdateUI)
   EVT_CHOICE(wxID_ANY, EffectDynamicCompressor::OnUpdateUI)
   EVT_SLIDERTEXT(wxID_ANY, EffectDynamicCompressor::OnUpdateUI)
END_EVENT_TABLE()

inline int ScaleToPrecision(double scale)
{
    return ceil(log10(scale));
}

inline bool IsInRange(double val, double min, double max)
{
    return val >= min && val <= max;
}

EffectDynamicCompressor::EffectDynamicCompressor()
    : mIgnoreGuiEvents(false),
    mAlgorithmCtrl(nullptr),
    mPreprocCtrl(nullptr),
    mAttackTimeCtrl(nullptr),
    mLookaheadTimeCtrl(nullptr)
{
}

std::unique_ptr<EffectEditor> EffectDynamicCompressor::PopulateOrExchange(
    ShuttleGui& S, EffectInstance&,
    EffectSettingsAccess&, const EffectOutputs*
)
{
    mUIParent = S.GetParent();

    S.SetBorder(10);

    S.StartHorizontalLay(wxEXPAND, 1);
    {
        PlotData* plot;

        S.StartVerticalLay();
        S.AddVariableText(XO("Envelope dependent gain"), 0,
            wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);

        mGainPlot = new Plot(
            mUIParent, wxID_ANY, -60, 0, -60, 0, XO("dB"), XO("dB"),
            LinearUpdater::Instance(), LinearDBFormat::Instance(), // X ruler
            LinearUpdater::Instance(), LinearDBFormat::Instance()  // Y ruler
        );

        S.AddWindow(mGainPlot);

        mGainPlot->SetMinSize({400, 200});
        plot = mGainPlot->GetPlotData(0);
        plot->pen = std::unique_ptr<wxPen>(
            safenew wxPen(AColor::WideEnvelopePen));
        plot->xdata.resize(61);
        plot->ydata.resize(61);
        std::iota(plot->xdata.begin(), plot->xdata.end(), -60);

        S.EndVerticalLay();
        S.StartVerticalLay();

        S.AddVariableText(XO("Compressor step response"), 0,
            wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);

        mResponsePlot = new Plot(
            mUIParent, wxID_ANY, 0, 5, -0.2, 1.2, XO("s"), XO(""),
            LinearUpdater::Instance(), IntFormat::Instance(),        // X Ruler
            LinearUpdater::Instance(), RealFormat::LinearInstance(), // Y Ruler
            2
        );

        S.AddWindow(mResponsePlot);

        mResponsePlot->SetMinSize({400, 200});
        mResponsePlot->SetName(XO("Compressor step response plot"));
        plot = mResponsePlot->GetPlotData(0);
        plot->pen = std::unique_ptr<wxPen>(
            safenew wxPen(AColor::WideEnvelopePen));
        plot->xdata = {0, RESPONSE_PLOT_STEP_START, RESPONSE_PLOT_STEP_START,
            RESPONSE_PLOT_STEP_STOP, RESPONSE_PLOT_STEP_STOP, 5};
        plot->ydata = {0.1, 0.1, 1, 1, 0.1, 0.1};

        plot = mResponsePlot->GetPlotData(1);
        plot->pen = std::unique_ptr<wxPen>(
            safenew wxPen(AColor::WideEnvelopePen));
        plot->pen->SetColour(wxColor( 230,80,80 )); // Same color as TrackArtist RMS red.
        plot->pen->SetWidth(2);
        plot->xdata.resize(RESPONSE_PLOT_SAMPLES+1);
        plot->ydata.resize(RESPONSE_PLOT_SAMPLES+1);
        for(size_t x = 0; x < plot->xdata.size(); ++x)
            plot->xdata[x] = x * float(RESPONSE_PLOT_TIME) / float(RESPONSE_PLOT_SAMPLES);
        S.EndVerticalLay();
    }
    S.EndHorizontalLay();

    S.SetBorder(5);

    S.StartStatic(XO("Algorithm"));
    {
        wxSize box_size;
        int width;

        S.StartHorizontalLay(wxEXPAND, 1);
        S.StartVerticalLay(1);
        S.StartMultiColumn(2, wxALIGN_LEFT);
        {
            S.SetStretchyCol(1);

            mAlgorithmCtrl = S.Validator<wxGenericValidator>(&mAlgorithm)
                .AddChoice(XO("Envelope Algorithm:"),
                Msgids(kAlgorithmsStrings, nAlgos),
                mAlgorithm);

            box_size = mAlgorithmCtrl->GetMinSize();
            width = mUIParent->GetTextExtent(wxString::Format(
                "%sxxxx",  kAlgorithmsStrings[nAlgos-1].Translation())).GetWidth();
            box_size.SetWidth(width);
            mAlgorithmCtrl->SetMinSize(box_size);
        }
        S.EndMultiColumn();
        S.EndVerticalLay();

        S.AddSpace(15, 0);

        S.StartVerticalLay(1);
        S.StartMultiColumn(2, wxALIGN_LEFT);
        {
            S.SetStretchyCol(1);

            mPreprocCtrl = S.Validator<wxGenericValidator>(&mCompressBy)
                .AddChoice(XO("Compress based on:"),
                Msgids(kCompressByStrings, nBy),
                mCompressBy);
            mPreprocCtrl->SetMinSize(box_size);
        }
        S.EndMultiColumn();
        S.EndVerticalLay();
        S.EndHorizontalLay();

        S.Validator<wxGenericValidator>(&mStereoInd)
            .AddCheckBox(XO("Compress stereo channels independently"),
                StereoInd.def);
    }
    S.EndStatic();

    S.StartStatic(XO("Compressor"));
    {
        int textbox_width = mUIParent->GetTextExtent("10.00001XX").GetWidth();
        SliderTextCtrl* ctrl = nullptr;

        S.StartHorizontalLay(wxEXPAND, true);
        S.StartVerticalLay(1);
        S.StartMultiColumn(3, wxEXPAND);
        {
            wxSizer* sizer = S.GetSizer();
            S.SetStretchyCol(1);

            // Threshold control
            S.AddVariableText(XO("Threshold:"), true,
                wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
            ctrl = S.Name(XO("Threshold"))
                .Style(SliderTextCtrl::HORIZONTAL)
                .AddSliderTextCtrl({}, Threshold.def, Threshold.max,
                Threshold.min, ScaleToPrecision(Threshold.scale), &mThresholdDB);
            ctrl->SetMinTextboxWidth(textbox_width);
            S.AddVariableText(XO("dB"), true,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

            // Ratio control
            S.AddVariableText(XO("Ratio:"), true,
                wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
            ctrl = S.Name(XO("Ratio"))
                .Style(SliderTextCtrl::HORIZONTAL | SliderTextCtrl::LOG)
                .AddSliderTextCtrl({}, Ratio.def, Ratio.max, Ratio.min,
                ScaleToPrecision(Ratio.scale), &mRatio);
            /* i18n-hint: Unless your language has a different convention for ratios,
            * like 8:1, leave as is.*/
            ctrl->SetMinTextboxWidth(textbox_width);
            S.AddVariableText(XO(":1"), true,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

            // Knee width control
            S.AddVariableText(XO("Knee Width:"), true,
                wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
             ctrl = S.Name(XO("Knee Width"))
                .Style(SliderTextCtrl::HORIZONTAL)
                .AddSliderTextCtrl({}, KneeWidth.def, KneeWidth.max,
                KneeWidth.min, ScaleToPrecision(KneeWidth.scale),
                &mKneeWidthDB);
            ctrl->SetMinTextboxWidth(textbox_width);
            S.AddVariableText(XO("dB"), true,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

            // Output gain control
            S.AddVariableText(XO("Output Gain:"), true,
                wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
            ctrl = S.Name(XO("Output Gain"))
                .Style(SliderTextCtrl::HORIZONTAL)
                .AddSliderTextCtrl({}, OutputGain.def, OutputGain.max,
                OutputGain.min, ScaleToPrecision(OutputGain.scale),
                &mOutputGainDB);
            ctrl->SetMinTextboxWidth(textbox_width);
            S.AddVariableText(XO("dB"), true,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
        }
        S.EndMultiColumn();
        S.EndVerticalLay();

        S.AddSpace(15, 0, 0);

        S.StartHorizontalLay(wxEXPAND, true);
        S.StartVerticalLay(1);
        S.StartMultiColumn(3, wxEXPAND);
        {
            S.SetStretchyCol(1);

            // Attack control
            S.AddVariableText(XO("Attack:"), true,
                wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
            mAttackTimeCtrl = S.Name(XO("Attack"))
                .Style(SliderTextCtrl::HORIZONTAL | SliderTextCtrl::LOG)
                .AddSliderTextCtrl({}, AttackTime.def, AttackTime.max,
                AttackTime.min, ScaleToPrecision(AttackTime.scale),
                &mAttackTime, AttackTime.scale / 100, 0.033);
            mAttackTimeCtrl->SetMinTextboxWidth(textbox_width);
            S.AddVariableText(XO("s"), true,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

            // Release control
            S.AddVariableText(XO("Release:"), true,
                wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
            ctrl = S.Name(XO("Release"))
                .Style(SliderTextCtrl::HORIZONTAL | SliderTextCtrl::LOG)
                .AddSliderTextCtrl({}, ReleaseTime.def, ReleaseTime.max,
                ReleaseTime.min, ScaleToPrecision(ReleaseTime.scale),
                &mReleaseTime, ReleaseTime.scale / 100, 0.033);
            ctrl->SetMinTextboxWidth(textbox_width);
            S.AddVariableText(XO("s"), true,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

            // Lookahead control
            S.AddVariableText(XO("Lookahead Time:"), true,
                wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
            mLookaheadTimeCtrl = S.Name(XO("Lookahead Time"))
                .Style(SliderTextCtrl::HORIZONTAL | SliderTextCtrl::LOG)
                .AddSliderTextCtrl({}, LookaheadTime.def, LookaheadTime.max,
                LookaheadTime.min, ScaleToPrecision(LookaheadTime.scale),
                &mLookaheadTime, LookaheadTime.scale / 10);
            mLookaheadTimeCtrl->SetMinTextboxWidth(textbox_width);
            S.AddVariableText(XO("s"), true,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

            // Hold time control
            S.AddVariableText(XO("Hold Time:"), true,
                wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
            ctrl = S.Name(XO("Hold Time"))
                .Style(SliderTextCtrl::HORIZONTAL | SliderTextCtrl::LOG)
                .AddSliderTextCtrl({}, LookbehindTime.def, LookbehindTime.max,
                LookbehindTime.min, ScaleToPrecision(LookbehindTime.scale),
                &mLookbehindTime, LookbehindTime.scale / 10);
            ctrl->SetMinTextboxWidth(textbox_width);
            S.AddVariableText(XO("s"), true,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
        }
        S.EndMultiColumn();
        S.EndVerticalLay();
        S.EndHorizontalLay();
    }
    S.EndStatic();

    return nullptr;
}

bool EffectDynamicCompressor::TransferDataToWindow(const EffectSettings&)
{
    // Transferring data to window causes spurious UpdateUI events
    // which would reset the UI values to the previous value.
    // This guard lets the program ignore them.
    mIgnoreGuiEvents = true;
    if (!mUIParent->TransferDataToWindow())
    {
        mIgnoreGuiEvents = false;
        return false;
    }

    UpdateUI();
    mIgnoreGuiEvents = false;
    return true;
}

bool EffectDynamicCompressor::TransferDataFromWindow(EffectSettings&)
{
    return DoTransferDataFromWindow();
}

bool EffectDynamicCompressor::DoTransferDataFromWindow()
{
    if (!mUIParent->Validate() || !mUIParent->TransferDataFromWindow())
    {
        return false;
    }

    return true;
}

void EffectDynamicCompressor::OnUpdateUI(wxCommandEvent&)
{
    if (!mIgnoreGuiEvents)
    {
        DoTransferDataFromWindow();
    }

    UpdateUI();
}

void EffectDynamicCompressor::UpdateUI()
{
    UpdateCompressorPlot();
    UpdateResponsePlot();
}

void EffectDynamicCompressor::UpdateCompressorPlot()
{
    PlotData* plot;
    plot = mGainPlot->GetPlotData(0);
    wxASSERT(plot->xdata.size() == plot->ydata.size());

    if (!IsInRange(mThresholdDB, Threshold.min, Threshold.max))
    {
        return;
    }

    if (!IsInRange(mRatio, Ratio.min, Ratio.max))
    {
        return;
    }

    if (!IsInRange(mKneeWidthDB, KneeWidth.min, KneeWidth.max))
    {
        return;
    }

    if (!IsInRange(mOutputGainDB, OutputGain.min, OutputGain.max))
    {
        return;
    }

    size_t xsize = plot->xdata.size();
    for(size_t i = 0; i < xsize; ++i)
    {
        plot->ydata[i] = plot->xdata[i] +
            LINEAR_TO_DB(CompressorGain(DB_TO_LINEAR(plot->xdata[i])));
    }

    mGainPlot->SetName(XO("Compressor gain reduction: %.1f dB").
        Format(plot->ydata[xsize-1]));
    mGainPlot->Refresh(false);
}

void EffectDynamicCompressor::UpdateResponsePlot()
{
    PlotData* plot;
    plot = mResponsePlot->GetPlotData(1);
    wxASSERT(plot->xdata.size() == plot->ydata.size());

    if(!IsInRange(mAttackTime, AttackTime.min, AttackTime.max))
        return;
    if(!IsInRange(mReleaseTime, ReleaseTime.min, ReleaseTime.max))
        return;
    if(!IsInRange(mLookaheadTime, LookaheadTime.min, LookaheadTime.max))
        return;
    if(!IsInRange(mLookbehindTime, LookbehindTime.min, LookbehindTime.max))
        return;

    std::unique_ptr<SamplePreprocessor> preproc;
    std::unique_ptr<EnvelopeDetector> envelope;
    float plot_rate = RESPONSE_PLOT_SAMPLES / RESPONSE_PLOT_TIME;

    size_t lookahead_size = CalcLookaheadLength(plot_rate);
    lookahead_size -= (lookahead_size > 0);
    ssize_t block_size = float(TAU_FACTOR) * (mAttackTime + 1.0) * plot_rate;

    preproc = InitPreprocessor(plot_rate, true);
    envelope = InitEnvelope(plot_rate, block_size, true);

    preproc->Reset(0.1);
    envelope->Reset(0.1);

    ssize_t step_start = RESPONSE_PLOT_STEP_START * plot_rate - lookahead_size;
    ssize_t step_stop = RESPONSE_PLOT_STEP_STOP * plot_rate - lookahead_size;

    ssize_t xsize = plot->xdata.size();
    for(ssize_t i = -lookahead_size; i < 2*block_size; ++i)
    {
        if(i < step_start || i > step_stop)
            envelope->ProcessSample(preproc->ProcessSample(0.1));
        else
            envelope->ProcessSample(preproc->ProcessSample(1));
    }

    for(ssize_t i = 0; i < xsize; ++i)
    {
        float x = 1;
        if(i < RESPONSE_PLOT_STEP_START * plot_rate ||
                i > RESPONSE_PLOT_STEP_STOP * plot_rate)
            x = 0.1;

        plot->ydata[i] = x * CompressorGain(
            envelope->ProcessSample(preproc->ProcessSample(0.1)));
    }

    mResponsePlot->Refresh(false);
}
