#!/bin/bash
# package.sh: the release. Copies the committed tree (HEAD) into the Tiger apps' scratch guest (/www/tiger-kit;
# start it with its guest-boot.sh), builds it there with Xcode 2.5 (xcodebuild, Release), makes the disk image with
# Tiger's hdiutil (Arkanoid.app and "Read Me.txt"), and copies it to dist/ with its SHA-256. Set DIST to write
# somewhere else (a build to try before it is released). It refuses a tree with uncommitted changes.
#
# https://tiger.v2core.com serves the newest image in dist/ whose .sha256 matches, rebuilding every 2 minutes
# (/www/tiger/tools/build-site.py). So both files are renamed into place, the .sha256 last: a download never gets a
# half-written image.
set -euo pipefail
H=$(cd "$(dirname "$0")" && pwd)
KIT=$(cd "$H/../tiger-kit" && pwd)
DIST=${DIST:-$H/dist}
V=$(sed -n '/CFBundleShortVersionString/{n;s/.*<string>\(.*\)<\/string>.*/\1/p;}' "$H/Info.plist")
D=/tmp/arkanoid-release
cd "$H"
[ -z "$(git status --porcelain --untracked-files=no)" ] || { echo "commit first: the release is built from HEAD" >&2; exit 1; }
"$KIT/g" true || { echo "the scratch guest is not up: $KIT/guest-boot.sh" >&2; exit 1; }
git archive --format=tar HEAD | "$KIT/g" "rm -rf $D && mkdir -p $D/src $D/image && tar -xf - -C $D/src"
"$KIT/g" "set -e; cd $D/src
  xcodebuild -configuration Release > ../build.log 2>&1 || { tail -30 ../build.log; exit 1; }
  grep -iE 'warning|error' ../build.log || true
  ditto build/Release/Arkanoid.app $D/image/Arkanoid.app
  cp 'resources/Read Me.txt' '$D/image/Read Me.txt'
  hdiutil create -quiet -srcfolder $D/image -volname 'Arkanoid $V' -format UDZO -imagekey zlib-level=9 $D/Arkanoid-$V.dmg
  hdiutil verify -quiet $D/Arkanoid-$V.dmg"
mkdir -p "$DIST"
cd "$DIST"
"$KIT/g" "cat $D/Arkanoid-$V.dmg" > ".Arkanoid-$V.dmg.tmp"
sha256sum ".Arkanoid-$V.dmg.tmp" | sed "s/\.Arkanoid-$V\.dmg\.tmp\$/Arkanoid-$V.dmg/" > ".Arkanoid-$V.dmg.sha256.tmp"
mv ".Arkanoid-$V.dmg.tmp" "Arkanoid-$V.dmg"
mv ".Arkanoid-$V.dmg.sha256.tmp" "Arkanoid-$V.dmg.sha256"
cat "Arkanoid-$V.dmg.sha256" && ls -l "Arkanoid-$V.dmg"
