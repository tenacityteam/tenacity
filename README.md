**We are currently undergoing a major rebase off Audacity 3.7.1. See PR https://codeberg.org/tenacityteam/tenacity/pulls/527 for progress. During this time, we will NOT accept pull requests against `main`. Please (re)base them off the `audacity-3.7-rebase` branch instead.**

For more information on the situation, please see https://fosstodon.org/@tenacity/113731514298404501. We are still accepting contributions elsewhere. We resume accepting contributions when the rebase is complete. Thank you for your understanding.

---

[![Tenacity](https://codeberg.org/tenacityteam/assets/raw/branch/master/PNG/tenacity-logo-dark-readme.png)](https://tenacityaudio.org)

[![Latest Release](https://img.shields.io/gitea/v/release/tenacityteam/tenacity?gitea_url=https://codeberg.org)](https://codeberg.org/tenacityteam/tenacity/releases)
[![Flathub Downloads](https://img.shields.io/flathub/downloads/org.tenacityaudio.Tenacity)](https://flathub.org/apps/org.tenacityaudio.Tenacity)
[![Chat on IRC](https://badgen.net/badge/irc/%23tenacity/green)](https://web.libera.chat/gamja/?channels=#tenacity)
[![Join our Lemmy community](https://img.shields.io/lemmy/tenacity@programming.dev?style=flat)](https://programming.dev/c/tenacity)
[![Follow our official Mastodon account](https://img.shields.io/mastodon/follow/114162723251170134?domain=floss.social&style=flat)](https://floss.social/@tenacity)
[![Translation status](https://hosted.weblate.org/widgets/tenacity/-/tenacity/svg-badge.svg)](https://hosted.weblate.org/engage/tenacity/)
[![License](https://badgen.net/badge/license/GPLv2/blue)](LICENSE.txt)

<!--[![builds.sr.ht](https://builds.sr.ht/~tenacity/tenacity/commits/.svg)](https://builds.sr.ht/~tenacity/tenacity/commits/?)-->

Please note that **we do NOT accept any pull requests or issues on GitHub anymore**. Please either open a pull request on Codeberg instead or submit a patch via email on SourceHut.

---

Tenacity is an easy-to-use multi-track audio editor and recorder for Windows, macOS, Linux and other operating systems. It is built on top of the widely popular [Audacity](https://audacityteam.org/) and is being developed by a wide, diverse group of volunteers.

**Are you coming from [Audacium](https://github.com/Audacium/audacium) or [Saucedacity](https://codeberg.org/tenacityteam/saucedacity-legacy)? You're in the right place!** We'd like to welcome all Audacium and Saucedacity users to Tenacity and our community. We've implemented new features from Audacity 3.1 to make editing easier. Additionally, we've preserved the themes of these two forks with only very slight modifications so they look better with our new editing features and slight track changes.

## Features

- **Recording** from audio devices (real or virtual)
- **Export / Import** a wide range of audio formats (extensible with FFmpeg)
- **High quality** including up to 32-bit float audio support
- **Plug-ins** providing support for VST, LV2, and AU plugins
- **Scripting** in the built-in scripting language Nyquist, or in Python, Perl and other languages with named pipes
- **Editing** arbitrary sampling and multi-track timeline
- **Accessibility** including editing via keyboard, screen reader support and narration support
- **Tools** useful in the analysis of signals, including audio

## Motivation

Our project initially started as a fork of [Audacity](https://audacityteam.org) as a result of multiple controversies and public relation crises, which you can find out more about here:

- [**Privacy policy which may violate the original project's GPL license**](https://github.com/audacity/audacity/issues/1213)
- [**Contributor's License Agreement (CLA) which may violate the same GPL license**](https://github.com/audacity/audacity/discussions/932)
- [**Attempts at adding telemetry using Google services for data collection**](https://github.com/audacity/audacity/pull/835)

Nevertheless, the goal of this project is to pick up the decades-long work by the original developers of Audacity and create an audio editor that is fresh, more modern, convenient and practical to use, with the help and the guidance of our users and our community.

## Differences from Audacity

One of the primary features of Tenacity is its removal of Audacity's networking portions, including telemetry, cloud storage support, and sharing to audio.com. However, while following upstream updates, we have been taking Tenacity into its own direction too. So far, we have done the following that isn't found in Audacity:

- New, modern themes.
- A powerful dynamic compressor.
- Improved support for more platforms such as [Haiku](https://haiku-os.org).
- Matroska importing and exporting without needing FFmpeg.
- Support for importing, editing, exporting Matroska chapters as label tracks.
- Sync-lock improvements, including the ability to temporarily override sync-lock.
- Horizontal scrolling in the Frequency Analysis window.
- Under-the-hood changes, such as a revamped build system allowing for modern upstream dependencies.

More changes are yet to come, big or small. We are always welcoming contributions to Tenacity, no matter how big or small, feature or fix. For more info, see the Contributing section.

## Download

### Tenacity

The latest release of Tenacity is always found [here on our Releases page](https://codeberg.org/tenacityteam/tenacity/releases). Additionally, for Windows, we provide FFmpeg libraries for you to use if you need to work with formats beyond what Tenacity natively supports.

Additionally, below is a matrix of where Tenacity is packaged. While these are unofficial packages, we work with packagers to upstream fixes and keep everything as upstream as possible.

[![Packaging status](https://repology.org/badge/vertical-allrepos/tenacity.svg?columns=3)](https://repology.org/project/tenacity/versions)

From this table, we'd like to highlight a few packages:

**Windows**:
- [Unofficial Tenacity Chocolatey Package](https://community.chocolatey.org/packages/tenacity)

**macOS**:
- [Tenacity MacPorts Package](https://ports.macports.org/port/tenacity)

**Linux**:
- [Arch Linux Package](https://archlinux.org/packages/extra/x86_64/tenacity/)
- [Unofficial Arch Linux (AUR) Git Package](https://aur.archlinux.org/packages/tenacity-git/)

If there are any issues with any of the above packages, you should report them to their respective maintainers. If you believe an issue is not specific to the package, please report the issue here.

### Audacity

The latest official version of Audacity that does not implement telemetry is `3.0.2`. Some Linux-based operating systems also ship Audacity with telemetry and networking features disabled by default.

Downloads for these versions can be found on the [Audacity website](https://www.audacityteam.org/download/). If you're looking for support regarding these versions, it may be worth consulting [Audacity's forum](https://forum.audacityteam.org/) or [Audacity's wiki](https://wiki.audacityteam.org/).

## Building

Build instructions for Tenacity are available in the [BUILDING.md file](BUILDING.md).

## Contributing

To start contributing, please consult the [CONTRIBUTING.md file](CONTRIBUTING.md). We accept contributions on both Codeberg (via pull requests) and SourceHut (via emails). We do a lot of work on Codeberg, but don't let that override your preferences.

If you are planning to make a big change or if you are otherwise hesitant as to whether we want to incorporate something you want to work on in Tenacity itself, simply open an issue about it in our [Codeberg issue tracker](https://codeberg.org/tenacityteam/tenacity/issues). We can discuss it and work together, so that neither our time or your time and hard effort goes to waste.

Note that we don't require a corresponding issue for each contribution.

### Translating

You can help us translate Tenacity and make it more accessible on [Weblate](https://hosted.weblate.org/projects/tenacity).

## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. For more information, see [LICENSE.txt](LICENSE.txt)
