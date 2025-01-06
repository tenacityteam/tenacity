_The following is an in-progress specification for Tenacity's future theme packages format. See #288 for more details._

# Theme Package Specification

This is the official base specification of Tenacity's theme package format.
Note that it does NOT cover Tenacity-specific details. It only covers the base
theme package format.

> The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL
NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED",  "MAY", and
"OPTIONAL" in this document are to be interpreted as described in
RFC 2119.

# 1. Introduction

Audacity has had theming support for a long while, and even includes several
themes itself. Many custom themes are available for Audacity, and users can
choose a custom theme by selecting the "Custom" option in Audacity's
preferences.

As a derivative, Tenacity had these same options. Nothing changed dramatically
from Tenacity's theme system ever since its first beta release. However,
starting with 1.3 stable, Tenacity merged Audacium's themes. All of these new
themes needed modification because they were missing resources needed for
Tenacity that weren't available in Audacium. While this was completed, it
revealed the inflexibility of Audacity's theming system.

Audacity's theming system is fragile. It, and by extension Tenacity's, uses a
hard-coded list of theme resources; any removal or rearrangement of theme
resources can break all existing themes. Combined with the additional
limitations of the theme format used, which is merely a collection of images
and colors inside a single PNG image, the motive for developing a new theme
system was quite clear.

This specification aims to provide a base package format for shipping themes
in applications. Not every application will follow the same behavior, but there
should be enough similarities between applications.

# 2. Format

## 2.1 Package Format

All valid theme packages MUST be valid ZIP archives in their most basic form.
Applications MAY support ZIP extensions as necessary but MUST be able to open
any valid ZIP archive without extensions.

## 2.2 Layout

Theme packages have a file at the root of the archive that specifies
information about the theme package. This file MUST be named `info.json`. Valid
fields for this file are described in Section 4.

`info.json` MUST consist of valid standard JSON as defined in ECMA-404 Second
Edition. Applications MAY support non-standard features.

All theme resources MUST reside in a folder named `resources/` located at the
root of the archive. Resource types MAY reside in a subfolder.

## 2.3 Unsupported Archive Features

Some ZIP features might be inappropriate for applications, including
encryption. These features SHOULD NOT be present in theme packages and
applications MAY leave these features unsupported. All other ZIP features
MUST be supported by applications.

# 3. Theme Resources

## 3.1 Types

A theme package consists of a set of theme resources. A theme resource is a
component of a theme, such as an image or color. The format of the resource is
the resource type. Theme packages MUST contain at least one resource type.
Applications MAY define their own resource types.

## 3.2 Predefined Types

These resource types are two distinct examples that applications SHOULD follow
when creating their own resource types. Applications SHOULD use these resource
types as defined in the following sections bellow if appropriate.

For the purposes of this specification, there are two different resource type
formats: single-file and multi-file. A single-file resource type MUST consist
of resources specified in a single JSON file. A multi-file resource type
MUST consist of multiple files named after a valid resource names. Applications
MAY define their own resource type formats.

### 3.2.1 Color Resource Types

Color resources MUST be a single-file resource type. They are stored in
`colors.json`.

Colors MUST be specified as an HTML hexadecimal number. Applications MAY
support other ways to describe colors.

### 3.2.2 Image Resource Types

Image resources MUST be a multi-file resource type. They are stored in an
`images/` subfolder.

#### 3.2.2.1 Supported Image Formats

Applications MAY support any suitable bitmap format and MAY support multiple
bitmap formats. Alternatively, applications MAY support any vector formats. At
least one image format, vector or bitmap, MUST be supported by the application.

## 3.3 Resource Type Requirements

Custom resource types that use multiple files MUST contain files named after
valid resource names.

Custom resource types that use JSON files MUST contain valid JSON that conforms
preferrably to the latest edition of ECMA-404. Applications MAY support
non-standard features if desired.

# 4. `info.json` Informational Fields

## 4.1 Required Fields

A theme package's `info.json` MUST have the following fields:

- `name`
- `minAppVersion`

### 4.1.1 The `name` Field

This field describes the theme's name. The provided value MUST be a string.
Theme packages MUST specify this field. Applications MUST fail loading the
theme package if this field is absent or empty.

### 4.1.2 The `minAppVersion` Field

This field describes the minimum app version required to load the theme
package. The provided value MUST be a string. Theme packages MUST specify this
field. If a package does not specify this field, it is up to the application to
define what behavior is present.

The provided string is application-specific. Applications are free to define
the version string format used for this field.

## 4.2 Optional Fields

### 4.2.1 The `subthemes` Field

This field lists paths to directories in the archive that contain valid
subthemes. The provided value MUST be an array of strings and MUST have at
least one element. For more information on this field, see Section 5.

## 4.3 Application-Specific Fields

Applications MAY define optional fields for other purposes. The use of these
application-specific fields SHOULD be documented by the application.

# 5. Subthemes

## 5.1 Definitions

A subtheme is a set of resources applied over a base theme. A base theme is a
set of resources used across one or more subthemes. Theme packages MAY contain
multiple subthemes. All subthemes MUST use only one base theme.

## 5.2 Layout

The layout of a subtheme MUST follow the layout described in Section 2.
Subthemes MUST NOT specify further subthemes. Applications MUST ignore any
`subtheme` section specified in a subtheme.

## 5.3 Required Informational Fields

A special field named `subthemes` MUST be present in order to specify one or
more subthemes. Each string represents a path specifying where the subtheme
resides. Subthemes MUST NOT contain this field in their `info.json`.

The required fields as specified in Section 3 MUST be present in each
subtheme's `info.json`. If any of these required fields are missing,
applications MAY use information specified in the base theme's `info.json`.
If fields are missing across multiple subthemes, applications SHOULD modify the
values provided to the required fields to prevent subthemes from sharing the
same information. If any required field is not specified anywhere, applications
MUST reject the theme package.

### 5.3.1 Special Exception with `minAppVersion`

The `minAppVersion` MUST NOT be specified in subthemes. For consistency, all
subthemes MUST share the same `minAppVersion`, and thus it MUST be specified in
the base theme.

## 5.4 Theme Resource Loading Order

When loading a subtheme, applications MUST load base theme resources first
before the subtheme's resources. Loading of base theme resources and subtheme
resources follows the same order as single-theme packages as described in
Section 6.

If a subtheme specifies a theme resource twice, applications MUST used the last
resource specified by the subtheme.

## 5.5 Error Handling

If any error occurs in a subtheme or a subtheme is invalid, applications MUST
stop loading the current subtheme but MUST continue loading other subthemes.

If any error occurs in a base theme, applications MUST stop loading the entire
theme package and raise an error.

# 6. Resource Loading Order

Applications MUST load resource types in the following order:

1. Color Resources
2. Image Resources
3. Any custom resources

## 6.1 Custom Resource Types

Applications with multiple custom resource types MAY specify any loading order.

# 7. Miscellaneous Clarifications

## 7.1 Paths

All paths MUST be relative to the root of the archive. Path MUST only use a
forward slash ('/') as the path separator.

## 7.2 Valid Resource Names

Valid resource names MAY contains alphanumerical characters, dashes ('-'), and
underscores ('_'). Resource names MUST NOT contain quotes, forward slashes
('/') or, if the resource is a part of a multi-file resource type, dots ('.').
For multi-file resource types, resources MAY contain a single dot for the file
extension in the archive.

## 7.3 Valid JSON Fields

A field is considered valid if during parsing it matches its described type and
other requirements as found in this specification. For exapmle, `subthemes` is
considered valid if it contains at least 1 string during parsing. If
`subthemes` is a different type, or it does not contain at least 1 string, it
is considered invalid.

## 7.4 Anything Not Specified by This Specification

This specification intentionally leaves some aspects of theme packages
undefined. Anything not defined in this specification is left up to
applications to implement.

Some parts of this specification are intentionally vague. It is also up to
applications to specify these parts. However, not all parts of the
specification are meant to be vague. Please see the section below for reporting
issues.

# Reporting Issues and Suggestions to this Specification

If you want to propose an amendment to this specification, such as correcting a
typo, adding a new feature, or clarifying an existing section, please fork
Tenacity's main repository, commit your changes, and open a pull request at
https://codeberg.org/tenacityteam/tenacity/pulls. If you are unsure about
making a change, feel free to open an issue in Tenacity's issue tracker over at
https://codeberg.org/tenacityteam/tenacity/issues.
