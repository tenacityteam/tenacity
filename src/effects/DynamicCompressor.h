/**********************************************************************

  Tenacity

  DynamicCompressor.h

  Max Maisel (based on Compressor effect)
  Avery King (split from the original Compressor2.h)

**********************************************************************/

#pragma once

#include "DynamicCompressorBase.h"
#include "StatefulEffectUIServices.h"

#include <wx/choice.h>
#include <wx/windowptr.h>

#include "widgets/Plot.h"
#include "SliderTextCtrl.h"
#include "wx/event.h"

class EffectDynamicCompressor final
: public DynamicCompressorBase,
  public StatefulEffectUIServices
{
    public:
        EffectDynamicCompressor();
        ~EffectDynamicCompressor() override = default;

        std::unique_ptr<EffectEditor> PopulateOrExchange(
            ShuttleGui& S, EffectInstance&,
            EffectSettingsAccess&, const EffectOutputs*
        ) override;

        bool TransferDataToWindow(const EffectSettings&) override;
        bool TransferDataFromWindow(EffectSettings&) override;
        bool DoTransferDataFromWindow();

    private:
        wxWeakRef<wxWindow> mUIParent;

        Plot* mGainPlot;
        Plot* mResponsePlot;
        wxChoice* mAlgorithmCtrl;
        wxChoice* mPreprocCtrl;
        SliderTextCtrl* mAttackTimeCtrl;
        SliderTextCtrl* mLookaheadTimeCtrl;
        bool mIgnoreGuiEvents;

        void OnUpdateUI(wxCommandEvent & evt);
        void UpdateUI();
        void UpdateCompressorPlot();
        void UpdateResponsePlot();
        void UpdateRealtimeParams();

        DECLARE_EVENT_TABLE()
};
