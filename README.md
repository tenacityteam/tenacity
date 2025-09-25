[![Tenacity](https://codeberg.org/tenacityteam/assets/raw/branch/master/PNG/tenacity-logo-dark-readme.png)](https://tenacityaudio.org)

[![Chat on IRC](https://badgen.net/badge/irc/%23tenacity/green)](https://web.libera.chat/gamja/?channels=#tenacity)
[![License](https://badgen.net/badge/license/GPLv2/blue)](LICENSE.txt)
[![Translation status](https://hosted.weblate.org/widgets/tenacity/-/tenacity/svg-badge.svg)](https://hosted.weblate.org/engage/tenacity/)
<!--[![builds.sr.ht](https://builds.sr.ht/~tenacity/tenacity/commits/.svg)](https://builds.sr.ht/~tenacity/tenacity/commits/?)-->

Tenacity is an easy-to-use multi-track audio editor and recorder for Windows, macOS, Linux and other operating systems. It is built on top of the widely popular [Audacity](https://audacityteam.org/) and is being developed by a wide, diverse group of volunteers.

## Features

- **Recording** from audio devices (real or virtual)
- **Export / Import** a wide range of audio formats (extensible with FFmpeg)
- **High quality** including up to 32-bit float audio support
- **Plug-ins** providing support for VST, LV2, and AU plugins
- **Scripting** in the built-in scripting language Nyquist, or in Python, Perl and other languages with named pipes
- **Editing** arbitrary sampling and multi-track timeline
- **Accessibility** including editing via keyboard, screen reader support and narration support
- **Tools** useful in the analysis of signals, including audio

## Download

Head over to our releases at our repository page at https://codeberg.org/tenacityteam/tenacity/releases. You may also wish to try some unofficial packages:

**Windows**:
- [Unofficial Tenacity Chocolatey Package](https://community.chocolatey.org/packages/tenacity)

**Linux**:
- [Arch Linux Package](https://archlinux.org/packages/extra/x86_64/tenacity/)
- [Unofficial Arch Linux (AUR) Git Package](https://aur.archlinux.org/packages/tenacity-git/)

If there are any issues with any of the above packages, you should report them to their respective maintainers. However, if you believe an issue is not specific to the package, please report the issue here.

### Package Matrix
[![Packaging status](https://repology.org/badge/vertical-allrepos/tenacity.svg?columns=3)](https://repology.org/project/tenacity/versions)

If you are a packager, we accept patches from packages and encourage you to make as many PRs as much as possible! We want to keep Tenacity packages minimally patched so everyone gets the same consistent experience across many different packages and repositories.

## Motivation

Our project initially started as a fork of [Audacity](https://audacityteam.org) as a result of multiple controversies and public relation crises, which you can find out more about here:

- [**Privacy policy which may violate the original project's GPL license**](https://github.com/audacity/audacity/issues/1213)
- [**Contributor's License Agreement (CLA) which may violate the same GPL license**](https://github.com/audacity/audacity/discussions/932)
- [**Attempts at adding telemetry using Google services for data collection**](https://github.com/audacity/audacity/pull/835)

Nevertheless, the goal of this project is to pick up the decades-long work by the original developers of Audacity and create an audio editor that is fresh, more modern, convenient and practical to use, with the help and the guidance of our users and our community.

## Differences from Audacity

On the surface, Tenacity might seem like an Audacity fork that merely removes error reporting and update checking. However, Tenacity is more than that, as we have been hard at work implementing our own features and fixes and want to take Tenacity in a direction our users and community like. So far, we have fulfilled part of this endless goal by implementing the following:

- New, modern themes.
- Improved support for more platforms such as [Haiku](https://haiku-os.org).
- Matroska importing and exporting without needing FFmpeg.
- Support for importing, editing, exporting Matroska chapters as label tracks.
- Sync-lock improvements, including the ability to temporarily override sync-lock.
- Horizontal scrolling in the Frequency Analysis window.
- Under-the-hood changes, such as a revamped build system allowing for modern upstream dependencies.

More changes are yet to come, big or small. We are always welcoming contributions to Tenacity, no matter how big or small, feature or fix. For more info, see the Contributing section.

## Differences from Other Forks 

Much has changed since the original Audacity controversies and forks happened. Tenacity is now the result of collaboration between our own contributors, past and present; Audacium contributors; and Saucedacity contributors. As a result, Tenacity combines features from both Audacium and Saucedacity into one fork, plus newer features from upstream Audacity too. If you are a user of either Audacium or Saucedacity, we invite you to try Tenacity and give us you feedback! We welcome you to the community as well!

## Latest Version of Audacity Without Telemetry or Networking Features

The last official version of Audacity that does not implement any telemetry, cloud support, or networking features is `3.0.2`. Some Linux-based operating systems also ship Audacity with telemetry and networking features disabled by default.

Downloads for these versions can be found on the [Audacity website](https://www.audacityteam.org/download/). If you're looking for support regarding these versions, it may be worth consulting [Audacity's forum](https://forum.audacityteam.org/) or [Audacity's wiki](https://wiki.audacityteam.org/).

## Building

Build instructions for Tenacity are available in the [BUILDING.md file](BUILDING.md).

## Contributing

**PRs on GitHub are no longer accepted. Please visit https://codeberg.org/tenacityteam/tenacity/pulls to make a pull request instead**.

To start contributing, please consult the [CONTRIBUTING.md file](CONTRIBUTING.md).

If you are planning to make a big change or if you are otherwise hesitant as to whether we want to incorporate something you want to work on in Tenacity itself, simply open an issue about it in our [Codeberg issue tracker](https://codeberg.org/tenacityteam/tenacity/issues). We can discuss it and work together, so that neither our time or your time and hard effort goes to waste. However, pull requests are not required to address any particular issue.

### Translating

You can help us translate Tenacity and make it more accessible on [Weblate](https://hosted.weblate.org/projects/tenacity).

## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. For more information, see [LICENSE.txt](LICENSE.txt)
