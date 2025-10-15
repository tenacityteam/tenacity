/**********************************************************************

  Tenacity

  SetTrackNameCommand.h

  Avery King extracted from CommonTrackControls.cpp

**********************************************************************/

/// An example of using an AudacityCommand simply to create a dialog.
/// We can add additional functions later, if we want to make it
/// available to scripting.
/// However there is no reason to, as SetTrackStatus is already provided.
#include "AudacityCommand.h"

class SetTrackNameCommand : public AudacityCommand
{
public:
    static const ComponentInterfaceSymbol Symbol;

    // ComponentInterface overrides
    ComponentInterfaceSymbol GetSymbol() const override
    { return Symbol; }
    //TranslatableString GetDescription() override {return XO("Sets the track name.");};
    //bool VisitSettings( SettingsVisitor & S ) override;
    void PopulateOrExchange(ShuttleGui & S) override;
    //bool Apply(const CommandContext & context) override;

    // Provide an override, if we want the help button.
    // ManualPageID ManualPage() override {return {};}
    public:
        wxString mName;
};