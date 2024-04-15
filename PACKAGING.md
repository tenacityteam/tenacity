# Packaging Tenacity
This guide contains some general information about how you should package
Tenacity. If you have additional questions remain, you can either use your best
judgement or ask us questions. We highly recommend the latter so we can help
clarify any questions you have.

# Re-using Audacity Packages
Any existing Audacity packages, especially those for 3.0.0-3.1.3, should be a
good template to start off of. This might be the easiest way to package
Tenacity if there is an existing Audacity package for your platform.

Be aware that Tenacity has completely different build options from Audacity.
For example, instead of `-Daudacity_use_ffmpeg=off|loaded`, Tenacity has
`-DFFMPEG=On|Off`. Take care to modify the build options when re-using an
existing Audacity package for Tenacity.

There might also be different quirks between Audacity and Tenacity. You will
have to test them via trial and error.

## Installation Alongside Audacity
Tenacity is capable of running alongside Audacity. Therefore, you should allow
the package to install alongside Audacity. Do **NOT** make the package conflict
or replace Audacity. Users should be able to install and use both without
issue.

# Patches
While we try our best to make sure Tenacity works on every platform, it is
possible that Tenacity won't work without modifications for your platform. In
this case, it goes without saying that you will need to patch Tenacity. If in
the case Tenacity does need patches, we highly recommend you submit a pull
request so they are integrated upstream. The next version we release will most
likely contain the patches.

# Build options
Unless necessary, we recommend you use the default CMake configuration. Most
options automatically disable themselves if the appropriate dependencies
aren't found.

Some options are disabled by default for various reasons. For example,
`-DSBSMS` is disabled by default because libsbsms 2.1.0 isn't available on
every Linux distro. While this will get better as time progresses, we have to
keep it off to ensure Tenacity builds for everyone, sacrificing some features
in the process.

Below are a few build options we recommend you enable if possible. This list
will likely expand as time progresses, so check back here regularly.

## Options Disabled By Default That Should Be Enabled
If possible, you should enable these build options:

* `-DSBSMS`: Enables high quality stretching.