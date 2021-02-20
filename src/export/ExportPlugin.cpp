/**********************************************************************

  Tenacity

  ExportPlugin.cpp

  Avery King split from Export.cpp
  Originally written by Dominic Mazzoni

**********************************************************************/

#include "ExportPlugin.h"
#include "Export.h"

// Tenacity libraries
#include <lib-files/wxFileNameWrapper.h>
#include <lib-track/Track.h>

#include "../WaveTrack.h"


ExportPlugin::ExportPlugin()
{
}

ExportPlugin::~ExportPlugin()
{
}

bool ExportPlugin::CheckFileName(wxFileName&, int)
{
    return true;
}

/** \brief Add a NEW entry to the list of formats this plug-in can export
 *
 * To configure the format use SetFormat, SetCanMetaData etc with the index of
 * the format.
 * @return The number of formats currently set up. This is one more than the
 * index of the newly added format.
 */
int ExportPlugin::AddFormat()
{
    FormatInfo nf;
    mFormatInfos.push_back(nf);
    return mFormatInfos.size();
}

int ExportPlugin::GetFormatCount()
{
    return mFormatInfos.size();
}

/**
 * @param index The plugin to set the format for (range 0 to one less than the
 * count of formats)
 */
void ExportPlugin::SetFormat(const wxString & format, int index)
{
    mFormatInfos[index].mFormat = format;
}

void ExportPlugin::SetDescription(const TranslatableString & description, int index)
{
    mFormatInfos[index].mDescription = description;
}

void ExportPlugin::AddExtension(const FileExtension &extension, int index)
{
    mFormatInfos[index].mExtensions.push_back(extension);
}

void ExportPlugin::SetExtensions(FileExtensions extensions, int index)
{
    mFormatInfos[index].mExtensions = std::move(extensions);
}

void ExportPlugin::SetMask(FileNames::FileTypes mask, int index)
{
    mFormatInfos[index].mMask = std::move( mask );
}

void ExportPlugin::SetMaxChannels(unsigned maxchannels, unsigned index)
{
    mFormatInfos[index].mMaxChannels = maxchannels;
}

void ExportPlugin::SetCanMetaData(bool canmetadata, int index)
{
    mFormatInfos[index].mCanMetaData = canmetadata;
}

wxString ExportPlugin::GetFormat(int index)
{
    return mFormatInfos[index].mFormat;
}

TranslatableString ExportPlugin::GetDescription(int index)
{
    return mFormatInfos[index].mDescription;
}

FileExtension ExportPlugin::GetExtension(int index)
{
    return mFormatInfos[index].mExtensions[0];
}

FileExtensions ExportPlugin::GetExtensions(int index)
{
    return mFormatInfos[index].mExtensions;
}

FileNames::FileTypes ExportPlugin::GetMask(int index)
{
    if (!mFormatInfos[index].mMask.empty())
    {
        return mFormatInfos[index].mMask;
    }

    return { { GetDescription(index), GetExtensions(index) } };
}

unsigned ExportPlugin::GetMaxChannels(int index)
{
    return mFormatInfos[index].mMaxChannels;
}

bool ExportPlugin::GetCanMetaData(int index)
{
    return mFormatInfos[index].mCanMetaData;
}

bool ExportPlugin::IsExtension(const FileExtension & ext, int index)
{
    bool isext = false;
    for (int i = index; i < GetFormatCount(); i = GetFormatCount())
    {
        const auto &defext = GetExtension(i);
        const auto &defexts = GetExtensions(i);
        int indofext = defexts.Index(ext, false);
        if (defext.empty() || (indofext != wxNOT_FOUND))
        {
            isext = true;
        }
    }

    return isext;
}

bool ExportPlugin::DisplayOptions(wxWindow * WXUNUSED(parent), int WXUNUSED(format))
{
    return false;
}

void ExportPlugin::OptionsCreate(ShuttleGui &S, int WXUNUSED(format))
{
    S.StartHorizontalLay(wxCENTER);
    {
        S.StartHorizontalLay(wxCENTER, 0);
        {
            S.Prop(1).AddTitle(XO("No format specific options"));
        }
        S.EndHorizontalLay();
    }
    S.EndHorizontalLay();
}

/// Creates a mixer by computing the time warp factor
std::unique_ptr<Mixer> ExportPlugin::CreateMixer(const TrackList &tracks,
            bool selectionOnly,
            double startTime, double stopTime,
            unsigned numOutChannels, size_t outBufferSize, bool outInterleaved,
            double outRate, sampleFormat outFormat,
            MixerSpec *mixerSpec)
{
    WaveTrackConstArray inputTracks;

    bool anySolo = !(( tracks.Any<const WaveTrack>() + &WaveTrack::GetSolo ).empty());

    auto range = tracks.Any< const WaveTrack >()
        + (selectionOnly ? &Track::IsSelected : &Track::Any )
        - ( anySolo ? &WaveTrack::GetNotSolo : &WaveTrack::GetMute);
    for (auto pTrack: range)
    {
        inputTracks.push_back(
            pTrack->SharedPointer< const WaveTrack >()
        );
    }

    // MB: the stop time should not be warped, this was a bug.
    return std::make_unique<Mixer>(
        inputTracks,
        // Throw, to stop exporting, if read fails:
        true,
        Mixer::WarpOptions{tracks},
        startTime, stopTime,
        numOutChannels, outBufferSize, outInterleaved,
        outRate, outFormat,
        true, mixerSpec
    );
}

void ExportPlugin::InitProgress(std::unique_ptr<ProgressDialog> &pDialog,
   const TranslatableString &title, const TranslatableString &message)
{
    if (!pDialog)
    {
        pDialog = std::make_unique<ProgressDialog>( title, message );
    } else
    {
        pDialog->SetTitle( title );
        pDialog->SetMessage( message );
        pDialog->Reinit();
    }
}

void ExportPlugin::InitProgress(std::unique_ptr<ProgressDialog> &pDialog,
   const wxFileNameWrapper &title, const TranslatableString &message)
{
    return InitProgress(
        pDialog, Verbatim( title.GetName() ), message
    );
}