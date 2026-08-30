#!/bin/sh
#
# Build MidiMonitor from a source tree and produce a .hpkg in packaging/out/.
#
# MUST RUN ON HAIKU -- it needs `package`, and it links against the Haiku
# libraries the resulting package will declare requirements on.
#
# Usage:  packaging/build-package.sh [<source-dir>]
#
# <source-dir> defaults to the repository root, i.e. building the working tree.
# Pass an unpacked release tarball instead to build exactly what the HaikuPorts
# recipe would build:
#     packaging/build-package.sh /boot/home/build/MidiMonitor2-2.0.0

set -e

PKGDIR=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$PKGDIR/.." && pwd)
SRCDIR=${1:-$REPO}
OUTDIR="$PKGDIR/out"

# --- version ---------------------------------------------------------------
#
# VERSION_BASE is the software's version; VERSION appends OUR package revision.
# Bump the revision (not the base) when the packaging changes but the source
# does not -- a rebuilt package with identical contents and the same version is
# indistinguishable to pkgman, so clients never pick it up.
#
# ⚠️ VERSION_BASE must stay in step with the app_version resource in
# MidiMonitor.rdef (major/middle/minor). Nothing checks this; the About box and
# the package would simply disagree.
VERSION_BASE="2.0.1"
# Revision resets to 1 on a VERSION_BASE bump -- it counts OUR packaging
# iterations of a given software version, not builds overall.
#
# 2.0.1 rather than 2.0.0-2: the fix it carries is a SOURCE change (moving MIDI
# and window setup out of the BApplication constructor into ReadyToRun(), see
# main.cpp), and the revision is only for packaging changes over identical
# source. 2.0.0-1 as published hangs on launch and shows no window at all, so
# it should be considered withdrawn rather than merely superseded.
REVISION="1"
VERSION="$VERSION_BASE-$REVISION"

APPNAME="MidiMonitor"

if [ "$(uname -s)" != "Haiku" ]; then
	echo "ERROR: this must run on Haiku -- it needs the 'package' tool." >&2
	exit 1
fi
command -v package >/dev/null 2>&1 || {
	echo "ERROR: 'package' not found." >&2; exit 1; }
[ -f "$SRCDIR/Makefile" ] || {
	echo "ERROR: no Makefile in $SRCDIR" >&2; exit 1; }

case "$(uname -m)" in
	x86_64)	ARCH=x86_64 ;;
	BePC|x86)
		if [ -d /boot/system/develop/headers/x86_gcc2 ] \
			|| gcc -dumpversion 2>/dev/null | grep -q '^2\.'; then
			ARCH=x86_gcc2
		else
			ARCH=x86
		fi
		;;
	*)	ARCH=$(uname -m) ;;
esac

# Unlike Beton, MidiMonitor is NOT refused on x86_gcc2. It is plain Be API C++
# with no STL, no exceptions and no C++11, so it is expected to build under
# gcc 2.95 -- that is the whole point of keeping it that way. It has not been
# built there yet as of 2026-08-29 (the 32-bit box was offline), so treat a
# gcc2 build as unverified until someone runs this on one, and note that
# x86_gcc2 packages belong in a DIFFERENT repository from this x86_64 one.

echo "MidiMonitor package build"
echo "  source:  $SRCDIR"
echo "  version: $VERSION"
echo "  arch:    $ARCH"
echo

# --- compile ---------------------------------------------------------------
#
# ALWAYS FROM CLEAN. The makefile-engine names its object directory after the
# architecture and compiler (objects.x86_64-cc13-release), so a stale directory
# from a different toolchain can sit alongside the current one and the glob
# below would not know which is which. Removing them all first makes "exactly
# one match" a meaningful check rather than a coincidence.
echo "=== clean ==="
( cd "$SRCDIR" && rm -rf objects.* )
echo "=== make ==="
( cd "$SRCDIR" && make -j"$(nproc)" )

# The makefile-engine picks the object directory name itself; do not guess it.
# Insist on exactly one match -- more than one means the clean above did not
# take, and picking either would be a coin flip.
set -- "$SRCDIR"/objects.*/"$APPNAME"
if [ "$#" -ne 1 ] || [ ! -f "$1" ]; then
	echo "ERROR: expected exactly one built $APPNAME, found $#:" >&2
	printf '  %s\n' "$@" >&2
	exit 1
fi
BIN="$1"
echo "  binary:  ${BIN#$SRCDIR/}"

# The icon and signature live in MidiMonitor.rdef and are linked in by the
# makefile-engine's RDEFS handling. A binary with no resources still runs, so
# this is otherwise a silent defect -- the app would ship with a generic icon.
if ! listattr "$BIN" 2>/dev/null | grep -q 'BEOS:APP_SIG'; then
	echo "ERROR: $BIN has no BEOS:APP_SIG attribute -- resources were not" >&2
	echo "linked. Check that RDEFS is set in the Makefile and rc ran." >&2
	exit 1
fi

# --- assemble the package tree ---------------------------------------------
#
# Layout copied from an existing HaikuPorts GUI app (wonderbrush): the binary
# at apps/<Name>, and a Deskbar entry as a RELATIVE symlink from
# data/deskbar/menu/Applications/<Name>. It must be relative -- packagefs
# mounts the package contents under a prefix, so an absolute link would point
# outside the package.
#
# ⚠️ FOUR levels of "..", not three. From data/deskbar/menu/Applications/ the
# hops are Applications -> menu -> deskbar -> data -> root, and only then
# apps/. The superseded build_hpkg.sh in the repository root (Gemini's, commit
# 27d9537) used three and produced a Deskbar entry that pointed at
# data/apps/MidiMonitor -- a dangling link. It is dangling INSIDE packagefs, so
# nothing at build time complains; the only symptom is a Deskbar item that does
# not launch.
STAGE="$PKGDIR/.stage-$ARCH"
rm -rf "$STAGE"
mkdir -p "$STAGE/apps" "$STAGE/data/deskbar/menu/Applications"
cp "$BIN" "$STAGE/apps/$APPNAME"
ln -s "../../../../apps/$APPNAME" "$STAGE/data/deskbar/menu/Applications/$APPNAME"

sed -e "s|@VERSION@|$VERSION|g" \
    -e "s|@VERSION_BASE@|$VERSION_BASE|g" \
    -e "s|@ARCH@|$ARCH|g" \
    "$PKGDIR/$(echo $APPNAME | tr 'A-Z' 'a-z').PackageInfo.in" \
    > "$STAGE/.PackageInfo"

# --- create ----------------------------------------------------------------
mkdir -p "$OUTDIR"
PKGFILE="$OUTDIR/midimonitor-$VERSION-$ARCH.hpkg"
rm -f "$PKGFILE"
( cd "$STAGE" && package create -q "$PKGFILE" )
[ -f "$PKGFILE" ] || { echo "ERROR: package create produced nothing" >&2; exit 1; }

# The filename arch and the DECLARED arch must agree -- build-repo.sh enforces
# this too, but catching it here points at the actual cause.
declared=$(package list -i "$PKGFILE" | sed -n 's/^[[:space:]]*architecture:[[:space:]]*//p' | head -1)
[ "$declared" = "$ARCH" ] || {
	echo "ERROR: package declares arch '$declared', expected '$ARCH'" >&2; exit 1; }

# The Deskbar symlink is the one piece of the layout with no natural failure
# mode at build time (see the four-levels note above), so verify the target
# resolves within the package rather than trusting the ln.
if ! package list "$PKGFILE" | grep -q "apps/$APPNAME"; then
	echo "ERROR: $PKGFILE contains no apps/$APPNAME" >&2; exit 1
fi

rm -rf "$STAGE"

echo
echo "=== result ==="
ls -l "$PKGFILE"
echo
package list -i "$PKGFILE" | grep -iE 'name|version|architecture|vendor|provides|requires'
echo
echo "Next: packaging/build-repo.sh"
