# Notes about Tenacity's Builtin Effects
(This excludes things like LADSPA support, VST2/3 support, etc).

**WARNING**: This README may be incorrect. While I intend to convey accurate
and correct information here, you are cautioned that my description (and
consequently understanding) of these classes could be incorrect. PRs are
accepted to correct information found here.

First, a bit of context: Audacity refactored their effect classes and are
attempting to move towards stateless effects. Previously, all effects in
Audacity were stateful. Below, you will see a primary distinction between
stateful and stateless effects. Consequently, if you plan to add a new effect
to Tenacity, **we ask that you design your effect to be stateless**.

There is a prime distinction between different effect classes. Most likely, you
will be dealing with one of three classes among others:

- `StatefulEffect`
- `StatefulPerTrackEffect`
- `PerTrackEffect`

`StatefulEffect` is for when you have a basic stateful effect that came from an
older version of Tenacity. `StatefulPerTrackEffect` is where you are in the
process of migrating your effect to eventually be a stateless one (or so I
understand; it might not be exactly accurate, but it's the best way I can
understand it). `PerTrackEffect` is used in combination with
`EffectWithSettings` as one of two template parameters for a stateless effect.

A stateful effect in Tenacity contains all the effect's settings within the
class itself. A stateless effect, on the other hand, contains all those same
settings within a different _settings class_ in addition to using an _instance_
class for doing some effect processing (as I understand). Let's use the echo
effect as an example. In this effect, `EchoBase` is the actual effect class
(minus the GUI bits), while `EchoSettings` holds the effect's settings and
`EchoBase::Instance` is what does the actual processing. Compared to a stateful
effect, you'll realize that both the settings and instance classes would both
lie in the effect's class itself. Overall, for a stateless effect, the actual
effect class itself only provides the glue to make an effect actually work
while the actual work resides in separate classes for settings and processing,
respectively.

`StatefulPerTrackEffect` is a bit interesting because it uses an instance class
to maintain state and shims around this to make it seem like a normal stateful
effect. In other words, `StatefulPerTrackEffect` _somewhat_ wraps around an
`Instance` class that simply gets the effect and calls similarly named member
functions on the effect class to perform its duties. Having read
https://github.com/audacity/audacity/pull/2759#issue-1190737079, this class
appears to be some transitional class, though that interpretation could be
wrong. All stateful effect classes, except for very few, are derived from this
effect.

On the other hand, for _stateless_ effects, your primary class will be
`PerTrackEffect`. However, you do _not_ derive from this class directly.
Instead, you pass it as the second template parameter to `EffectWithSettings`
and derive from that class instead. (The first template parameter is your
settings class).

To touch very briefly upon GUI effect classes, `StatelessPerTrackEffect` is
simply an alias for effects that derive from both
`EffectWithSettings<..., PerTrackEffect>` and `StatelessEffectUIServices`.
Some effects do this, but most stateless effects often derive from their base
class and `StatelessEffectUIServices`

## Miscellaneous Notes
All effects by default are not realtime. Override the `RealtimeSupport()`
member function to return a different value to enable support.

Modern versions of Audacity, and consequently Tenacity 1.4 and up, have their
builtin effects split between two different places: `lib-builtin-effects` and
`src/effects`. The latter contains the GUI code for effects, while the latter
contains only the builtin logic. Consequently, the latter shouldn't use
anything wxWidgets related. Meanwhile, `lib-effects` does not contain any
builtin effects, but instead contains the actual effect interfaces that one can
make a new effect with.
