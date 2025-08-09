#!/bin/sh
# Run this script with locale as the current directory
set -o errexit
echo ";; Recreating tenacity.pot using .h, .cpp and .mm files"
for path in ../modules/mod-* ../libraries/lib-* ../src ; do
   find $path -name \*.h -o -name \*.cpp -o -name \*.mm
done | LANG=c sort | \
sed -E 's/\.\.\///g' |\
xargs xgettext \
--no-wrap \
--default-domain=tenacity \
--directory=.. \
--keyword=_ --keyword=XO --keyword=XC:1,2c --keyword=XXO --keyword=XXC:1,2c --keyword=XP:1,2 --keyword=XPC:1,2,4c \
--add-comments=" i18n" \
--add-location=file  \
--copyright-holder='Tenacity Team' \
--package-name="tenacity" \
--package-version='1.4' \
--msgid-bugs-address="~tenacity/tenacity-discuss@lists.sr.ht" \
--add-location=file -L C -o tenacity.pot 
echo ";; Adding nyquist files to tenacity.pot"
for path in ../plug-ins ; do find $path -name \*.ny -not -name rms.ny; done | LANG=c sort | \
sed -E 's/\.\.\///g' |\
xargs xgettext \
--no-wrap \
--default-domain=tenacity \
--directory=.. \
--keyword=_ --keyword=_C:1,2c --keyword=ngettext:1,2 --keyword=ngettextc:1,2,4c \
--add-comments=" i18n" \
--add-location=file  \
--copyright-holder='Tenacity Team' \
--package-name="tenacity" \
--package-version='1.4' \
--msgid-bugs-address="~tenacity/tenacity-discuss@lists.sr.ht" \
--add-location=file -L Lisp -j -o tenacity.pot 
if test "${TENACITY_ONLY_POT:-}" = 'y'; then
    return 0
fi

echo ";; Updating en.po"
msgmerge --lang=en en.po tenacity.pot -o en.po

echo ";; Removing '#~|' (which confuse Windows version of msgcat)"
sed '/^#~|/d' en.po > TEMP; mv TEMP en.po

echo ""
echo ";;Translation updated"
echo ""
head -n 11 tenacity.pot | tail -n 3
wc -l tenacity.pot
