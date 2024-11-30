The following document describes Tenacity's implementation of theme packages
for version 1.4 above. It does NOT apply to older versions.

If you are looking for the general specification of theme packages, please
look at `Theme Package Specification.md` in the same directory. If you have any
issue with either or both specifications, please do not hesitate to create an
issue over at https://codeberg.org/tenacityteam/tenacity/issues.

# Tenacity Theme Packages
Starting in Tenacity 1.4, there is a completely new theming system, and
existing themes will no longer work in Tenacity. Theme developers will have to
remake all their themes for Tenacity.

## Background

For what it was, Audacity's theming system worked. It allowed customization of
many parts of the UI and provided a way to use a single custom theme. It's
format was quite simple: all theme resources were stored in specially arranged
PNG. We'll call themes in this format the ImageCache format since Tenacity
produces a PNG called ImageCache.png whenever you click the Output Sourcery
button in Tenacity's theme preferences.

As Tenacity development progressed, we wanted to make changes to themes in a
few ways. First off, we wanted to remove all unused theme resources. We find no
reason to keep them since nothing else in Tenacity uses them. Second, we wanted
to allow having multiple custom themes to choose from rather than a single
custom theme.

So why replace such a perfectly good system with something much more
unnecessary? To start, Audacity's theming system wasn't all that perfect. First
off, not all images and colors you see in an ImageCache are actually used. The
only reason they remain is to reduce theme breakage. Thus, we arrive at our
first limitation: **it is quite difficult to remove resources without breaking
existing themes**. Of course, removing resources will still inevitably break
existing themes, but we will discuss more on how this impact can be limited
later.

Another limitation in Audacity's theming system is the fact that a simple PNG
is used for storing theme resources. This works for basic usage, but there are
several limitations for using a PNG. For example, **themes cannot specify other
metadata**, such as its name or a minimum required app version. This is
important for several reasons, most notably relating to allowing multiple
custom theme and to ensuring that a theme works properly for multiple versions.

Finally, yet another limitation, and perhaps the biggest inconvenience towards
maintenance, is when there are multiple variants of a theme. Audacium had
several variants of its default theme, most of which are the same and only vary
in their accent color; consequently, when Audacium and Tenacity merged,
Tenacity inherited those themes. This brings Tenacity to having 17 themes
total, quite a lot of themes! Unfortunately, this puts a burden on maintenance
because if a new theme resource has to be added that is neutral with its accent
color, edits have to be made 10 times to just those themes! Adding the other
themes, this can be quite tedious.

Of course, there are other solutions to remedy some of the problems described
above, such as relying on file names for theme names in having multiple themes.
However, even with some of these solutions implemented, this doesn't change the
fact that the ImageCache format has inherent limitations and is not flexible
enough for Tenacity's use. Therefore, a new theme system was needed.

### Why not keep the old theme system?

The intent behind the new theme system is to introduce breaking changes that
would otherwise require a lot of refactoring of the current theme system. If we
kept the old them system by itself, we would have to figure out a complimentary
format to store data alongside ImageCache formats. This itself could be a ZIP
file containing an ImageCache.png and maybe an info.json file, much like the
new theme system. However, the limitations of the existing ImageCache.png
format would still apply, as the rigidity of the old theme system would still
be present. Remember that the intention of implementing the new theme system is
to allow for better management of theme resources.

On the other hand, keeping the old theme system alongside the new theme system
sounds like a plausible idea. However, Tenacity 1.4 is set to add and remove
some theme resources. While we can simply keep the old theme system resources,
as other features are implemented in the new theme system, the limitations of
the old theme system will become more and more apparent.

## Theme Packages

So what is a theme package? A theme package consists of a set of resources
(images, colors, and potentially others in the future) along with metadata
describing the theme all in a single ZIP file. A theme package is structured in
the following way:

* info.json
* colors.json
* images/
  * playButton.png
  * ...

info.json contains the theme's metadata. colors.json contains all the theme's
colors; in the old ImageCache format, these were all the colors at the bottom.
`images/` contains all the theme's icons and other images Tenacity needs to
use; in the old ImageCache format, these were all the icons and other images at
the top. Notice how images are named after their resource names.

Theme packages are very flexible. If we remove a resource, all a theme needs to
do to keep simple backward compatibility with a previous version of Tenacity is
to keep the theme resource. For example, if we remove playButton.png in
Tenacity 1.8, a theme simply needs to keep the resource to maintain
compatibility with previous versions of Tenacity. If we have multiple variants
of a theme, we can use subthemes to share certain resources across all our
subthemes.

### Subthemes

Subthemes are incomplete themes in a package that rely on a base resource set.
For example, Audacium's light and dark themes are prime examples of subthemes.
Their respective theme packages contain a base resource set that each subtheme
shares.

## Definitions

- **Theme**: A set of assembled resources.
- **Resource**: A component of a theme. This can be a color, image, or
  something else.

- **Theme package**: A special ZIP file containing an `info.json`, a
  `colors.json`, and an `images/` directory containing vairous images. It may
  also contain subthemes (see above).

## `info.json` Metadata Fields

The following subsections document the available metadata fields for a theme
package's `info.json`.

### `name`
**Type**: string
**Required**: yes

The theme name.

### `minAppVersion`
**Type**: string
**Required**: yes

The minimum version the theme package supports.

The version string must follow the following format: `x.y[.z]`. Except in unusual
circumstances, we only make changes to themes across major and minor versions
(you can expect major versions to have major changes). Therefore, it is
recommended to provide only a minor version, i.e., a version string following
the x.y format.

Do not specify any specific git tags, alphas, or betas. Do not also add any
other characters other than numbers and either one or two dots (`.`). Tenacity
will reject those and may render the theme invalid.

**Acceptable** version strings:

* 1.4
* 1.4.0

**Unacceptable** version strings:

* v1.4
* v1.4.0
* 1.4.0-alpha
* 1.4.0-beta3

### `themeType`
**Type**: string
**Required**: no

Indicates the kind of theme whether it's a light theme, dark theme, or neutral
theme (i.e., neither a light nor dark theme). If this value is not provided,
Tenacity assumes a theme is neutral (i.e., it's either a light or dark theme).

### `subthemes`
**Type**: string array
**Required**: only for subthemes

Specifies a list of subthemes that are contained in the package. This is the
only field required in a package's root info.json

Each string must specify a path relative to the archive. The path must start
with a directory at the root of the archive. Paths must use a forward-slash
(`/`) as their separator.

Acceptable path values:

* `light/`
* `light`
* `subthemes/another_dir`

## Resource List

The following sections detail the various theme resources throughout Tenacity.

You may be familiar with some of the names if you've worked on an Audacity
theme before. These resource names were recycled from the old theme system,
although their prefixes have been dropped.

### Colors

* `blank`
* `unselected`
* `selected`
* `sample`
* `selSample`
* `dragSample`
* `muteSample`
* `rms`
* `muteRms`
* `shadow`
* `aboutBackground`
* `trackPanelText`
* `labelTrackText`
* `meterPeak`
* `meterDisabledPen`
* `meterDisabledBrush`
* `meterInputPen`
* `meterInputBrush`
* `meterInputRMSBrush`
* `meterInputClipBrush`
* `meterInputLightPen`
* `meterInputDarkPen`
* `meterOutputPen`
* `meterOutputBrush`
* `meterOutputRMSBrush`
* `meterOutputClipBrush`
* `meterOutputLightPenl`
* `meterOutputDarkPen`
* `rulerBackground`
* `axisLines`
* `graphLines`
* `ResponseLines`
* `hzPlot`
* `wavelengthPlot`
* `envelope`
* `muteButtonActive`
* `muteButtonVetoed`
* `cursorPen`
* `recordingPen`
* `playbackPen`
* `recordingBrush`
* `playbackBrush`
* `rulerRecordingBrush`
* `rulerRecordingPen`
* `rulerPlaybackPen`
* `timeFont`
* `timeBack`
* `timeFontFocus`
* `timeBackFocus`
* `labelTextNormalBrush`
* `labelTextEditBrush`
* `labelUnselectedBrush`
* `labelSelectedBrush`
* `labelUnselectedPen`
* `labelSelectedPen`
* `labelSurroundPen`
* `trackFocus0`
* `trackFocus1`
* `trackFocus2`
* `snapGuide`
* `trackInfo`
* `trackInfoSelected`
* `light`
* `medium`
* `dark`
* `lightSelected`
* `mediumSelected`
* `darkSelected`
* `clipped`
* `muteClipped`
* `progressDone`
* `progressNotYet`
* `syncLockSe`
* `selTranslucent`
* `blankSelected`
* `sliderLight`
* `sliderMain`
* `sliderDark`
* `trackBackground`
* `placeHolder1`
* `graphLabels`
* `spectroBackground`
* `scrubRuler`
* `timeHours`
* `focusBox`
* `trackNameText`
* `midiZebra`
* `midiLines`
* `textNegativeNumbers`
* `clipAffordanceOutlinePen`
* `clipAffordanceInactiveBrush`
* `clipAffordanceActiveBrush`
* `clipAffordanceStroke`
* `clipLabelTrackTextSelection`
* `clipNameText`
* `clipNameTextSelection`

### Icons and Other Images

#### Transport Icons

* `pause` (16x16)
* `pauseDisabled` (16x16)
* `play` (16x16)
* `playDisabled` (16x16)
* `loop` (16x16)
* `loopDisabled` (16x16)
* `cutPreview` (16x16)
* `cutPreviewDisabled` (16x16)
* `stop` (16x16)
* `stopDisabled` (16x16)
* `rewind` (16x16)
* `rewindDisabled` (16x16)
* `ffwd` (16x16)
* `ffwdDisabled` (16x16)
* `record` (16x16)
* `recordDisabled` (16x16)
* `recordBelow` (16x16)
* `recordBelowDisabled` (16x16)
* `recordBeside` (16x16)
* `recordBesideDisabled` (16x16)
* `scrub` (18x16)
* `scrubDisabled` (18x16)
* `seek` (26x16)
* `seekDisabled` (26x16)

#### Tool Icons

* `IBeam` (27x27)
* `zoom` (27x27)
* `envelope` (27x27)
* `timeShift` (27x27)
* `draw` (27x27)
* `multi` (27x27)
* `mic` (25x25)
* `speaker` (25x25)

#### Edit Icons

* `zoomFit` (27x27)
* `zoomFitDisabled` (27x27)
* `zoomIn` (27x27)
* `zoomInDisabled` (27x27)
* `zoomOut` (27x27)
* `zoomOutDisabled` (27x27)
* `zoomSel` (27x27)
* `zoomSelDisabled` (27x27)
* `zoomToggle` (27x27)
* `zoomToggleDisabled` (27x27)
* `cut` (26x24)
* `cutDisabled` (26x24)
* `copy` (26x24)
* `copyDisabled` (26x24)
* `paste` (26x24)
* `pasteDisabled` (26x24)
* `trim` (26x24)
* `trimDisabled` (26x24)
* `silence` (26x24)
* `silenceDisabled` (26x24)
* `undo` (26x24)
* `undoDisabled` (26x24)
* `redo` (26x24)
* `redoDisabled` (26x24)

#### Sync Lock

* `syncLockSelected` (20x22)
* `syncLockTracksDown` (20x20)
* `syncLockTracksUp` (20x20)
* `syncLockTracksDisabled` (20x20)
* `syncLockIcon` (12x12)

#### Label Glyphs

* `labelGlyphLeft` (15x23)
* `labelGlyphLeftSelected1` (15x23)
* `labelGlyphLeftSelected2` (15x23)
* `labelGlyphRight` (15x23)
* `labelGlyphRightSelected1` (15x23)
* `labelGlyphRightSelected2` (15x23)
* `labelGlyphSingle` (15x23)
* `labelGlyphSingleSelected` (15x23)
* `labelGlyphSingleLeftSelected` (15x23)
* `labelGlyphSingleRightSelected` (15x23)

#### Play and Thumb Heads

* `playPointer` (20x20)
* `playPointerPinned` (20x20)
* `recordPointer` (20x20)
* `recordPointerPinned` (20x20)
* `grabberDropLoc` (20x20)
* `sliderThumb` (20x20)
* `sliderThumbHilited` (20x20)
* `sliderThumbRotated` (20x20)
* `sliderThumbRotatedHilited` (20x20)

#### Track Panel Buttons

* `upButtonExpand` (96x18)
* `upButtonExpandSel` (96x18)
* `downButtonExpand` (96x18)
* `downButtonExpandSel` (96x18)
* `hiliteUpButtonExpand` (96x18)
* `hiliteUpButtonExpandSel` (96x18)
* `hiliteButtonExpand` (96x18)
* `hiliteButtonExpandSel` (96x18)

#### Toolbar Buttons

* `upButtonLarge` (48x48)
* `upButtonSmall` (27x27)
* `downButtonLarge` (48x48)
* `downButtonSmall` (27x27)
* `hiliteUpButtonLarge` (48x48)
* `hiliteUpButtonSmall` (27x27)
* `hiliteButtonLarge` (48x48)
* `hiliteButtonSmall` (27x27)
* `recoloredUpLarge` (48x48)
* `recoloredDownLarge` (48x48)
* `recoloredUpHiliteLarge` (48x48)
* `recoloredHiliteLarge` (48x48)
* `recoloredUpSmall` (27x27)
* `recoloredDownSmall` (27x27)
* `recoloredUpHiliteSmall` (27x27)
* `recoloredHiliteSmall` (27x27)

## Changes from 1.3.x

There have been some changes from previous versions of Tenacity in what is
themable and what resources are used or not. Some of the changes include the following:

* Mac-specific resources were removed.
* Spectrogram resources were removed.
* Cursor resources were removed.
* Extra waveform colors were removed (they will be customizable by users instead).
* The app icon is no longer themable.

Most of these resources were not used in the first place. The only real impact
for theme developers is that there are less resources that you must add to your
theme.

## Future Changes
It is eventually planned to extend the new theme system in different ways. In
what we could add, we are not sure at the moment. However, there are a few
ideas to extend the new theme system.

### SVG Support
It would be nice to have SVG support so Tenacity has better HiDPI support.
Themes with SVG icons would be able to scale at any DPI and still look good
overall. Currently, the guaranteed target for this feature is version 2.0, but
it might be possible to implement support for SVGs in wx-based Tenacity using
NanoSVG.

### Support for Multi-Type Resources
Multi-type resources would allow a resource to be either specified as one
resource type or another resource type. For example, the pause resource could
be specified as either an image resource or a color resource. Multi-resource
types can help simplify theme development if a theme only needs to specify a
solid color for a certain theme resource. This would also compliment SVG
support in that these colors are also scalable at any DPI.