# Tenacity Translations

I'm writing this README to keep track of how translations work in Tenacity.
Notoriously, I have messed up translations before, so I think it's best I
document everything that I've learned here about how translations are setup
in this project.

- gperson

## How Translations are Set Up
Tenacity is translated using Weblate over at
https://hosted.weblate.org/projects/tenacity/tenacity. You can sign up for an
account if you don't already have one and get to translating there.

Weblate supports multiple formats, including monolingual and bilingual formats.
Tenacity uses wxWidgets for translations, which uses gettext under the hood, so
natively, it'll use PO files. However, as I perceive, there is more than one
way gettext can be set up in Weblate.

For Tenacity's Weblate setup, we have a _monolingual base file_, which is the
file containing the English "translation", that stores all the strings that
need to be translated. As strings change in Tenacity, this file is also
modified to reflect that.

(TODO: perhaps we should introduce a contributor requirement that should a
translatable string be added, modified, or removed, the monolingual base
translation. That way, Weblate always has an accurate state of what strings
need to be translated).

## Updating Translations
Historically, in Audacity, you updated translations via `update_po_files.sh`.
Previously, this updated _all_ translations to contain the latest strings from
the source code. This way, all translations would contain the latest
strings from the source code.

From what I understand with Weblate in Tenacity, however, this doesn't appear
necessary. Weblate will already handle this if we just update the monolingual
base file and, maybe, translation template. Weblate should then be able to just
take care of the rest.

## Updates
This README will be updated as I learn more information about how Tenacity's
translations work.
