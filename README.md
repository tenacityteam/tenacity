**Saucedacity** is an easy-to-use, multi-track audio editor and recorder for Windows, GNU/Linux, and other operating systems*. Saucedacity is open source software licensed under GPL, version 2 or later.

*macOS is unofficially supported (it is buildable). We lack an actual Mac to test our CI builds on to make sure Saucedacity runs properly.

- **Recording** from any real, or virtual audio device that is available to the host system.
- **Export / Import** a wide range of audio formats, extendible with FFmpeg.
- **High quality** using 32-bit float audio processing.
- **Plug-ins** Support for multiple audio plug-in formats, including VST, LV2, AU.
- **Macros** for chaining commands and batch processing.
- **Scripting** in Python, Perl, or any language that supports named pipes.
- **Nyquist** Very powerful built-in scripting language that may also be used to create plug-ins.
- **Editing** multi-track editing with sample accuracy and arbitrary sample rates.
- **Accessibility** for VI users.
- **Analysis and visualization** tools to analyze audio, or other signal data.

## Cloning the Source
To clone the Saucedacity source tree, you will need to run the following command:
```
git clone --recurse-submodules https://github.com/saucedacity/saucedacity
```

The `--recurse-submodules` is very important because, unlike Audacity, Saucedacity uses submodules. If you forget the `--recurse-submodules`, you can run the following after `git clone`:

```
git submodule init
git submodule update
```

## Contributing and To Do

All contributions made to Saucedacity are available under the GNU General Public License, version 2 or later.

If you want to contribute to this project, we welcome your contributions. Please see the [issue tracker](https://github.com/saucedacity/saucedacity/issues) to see what needs to be done. You can also take a look at [our TODO page](https://github.com/saucedacity/saucedacity/wiki/TODO). There, you will find some things that we plan on doing or that need to be done.

Finally, **you do NOT need to know how to code in order to contribute to this program!** If you **do** know how to code, then we welcome your contributions.

## Getting Started
For end users, the latest Windows and Linux release version of Saucedacity is available from the [Saucedacity releases page](https://github.com/saucedacity/saucedacity).
Help with using Saucedacity is available from the Wiki (in progress.

Build instructions are available [here](BUILDING.md).
