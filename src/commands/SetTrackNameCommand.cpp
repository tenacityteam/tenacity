/**********************************************************************

  Tenacity

  SetTrackNameCommand.cpp

  Avery King extracted from CommonTrackControls.cpp

**********************************************************************/

#include "SetTrackNameCommand.h"

#include "ShuttleGui.h"

const ComponentInterfaceSymbol SetTrackNameCommand::Symbol
{ XO("Set Track Name") };

void SetTrackNameCommand::PopulateOrExchange(ShuttleGui & S)
{
    S.AddSpace(0, 5);

    S.StartMultiColumn(2, wxALIGN_CENTER);
    {
        S.TieTextBox(XXO("Name:"),mName,60);
    }
    S.EndMultiColumn();
}
