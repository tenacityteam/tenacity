# lib-basic-ui

**FIXME**: Maybe rename this to `lib-generic-ui` to avoid confusion.

lib-basic-ui in Tenacity is an abstract library that allows the use of a
single UI library with multiple different toolkits (wxWidgets, Qt, GTK+, etc).
It is designed to be a completely abstract library, requiring implementations
for what you have to use.

More documentation might arrive in a `doc/` folder or something. Documentation
is a priority too, and this will mostly be done with Doxygen for now.

Currently, anything introduced in our version of lib-basic-ui (namely GenericUI)
isn't built with Tenacity. GenericUI is not yet ready (even for very basic
things), and, in order to save time compiling, is not built.

Warning: some of this code is untested. If you see a bug, or if something
doesn't look right, please file an issue in our issue tracker.
