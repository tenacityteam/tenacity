# Tenacity Theme Package Documentation

Tenacity theme packages come as ZIP files. Their structure looks something
like this:

```
TenacityDarkTheme.zip:
|
|--- theme.json
|--- colors.json
|--- bitmaps
     |--- bmpPause.png
     |--- bmpPlay.png
     |--- ...etc
```

## Package Precedence

When an archive is first opened, Tenacity first reads `theme.json`. If this
file is not found, this file is invalid, or contains invalid values of required
properties, Tenacity considers the entire theme package invalid.

After reading `theme.json`, Tenacity reads `colors.json` next and loads any
color resources. Tenacity will always continue parsing the rest of the package
even if `colors.json` is invalid or missing, although it will use the system's
colors, which might not look good with the icon set.

Finally, after

## `theme.json`

At the root of each package is a `theme.json` file. This file contains basic
information about the theme package, such as the name, minimum required app
version, and other information.

### Properties

- `name` (string): The name of the theme package. Required.
- `minAppVersion` (int array): The minimum required app version (e.g.,
  `[1, 4, 0]`). Optional, but strongly recommended. If not specified, a value
  of `[1, 4, 0]` is specified as the deafult.
- `themeType` (string): Either "dark" for dark themes, "light" for light
  themes, or "neutral" for neither.

## `colors.json`

This is where colors are specified.

### Properties

TODO.
