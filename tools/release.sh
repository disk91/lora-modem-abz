#!/bin/bash
set -Eeuo pipefail

basename=lora-modem-abz

bail() {
    echo $1
    exit 1
}

if [ $# -ne 1 ] ; then
    bail "Usage: $0 <version>"
fi

version="$1"

if [ -z "$GITHUB_TOKEN" ] ; then
    bail "Error: GITHUB_TOKEN environment variable is not set"
fi

make clean

# We only generate releases from a git repository clone that does not have any
# uncommitted modification or untracked files
if [ -n "$(git status --porcelain)" ] ; then
    bail "Error: Your git repository clone is not clean"
fi

previous_tag=$(git describe --tags --abbrev=0)
if [ -z "$previous_tag" ] ; then
    bail "Error: Could not determine the previous release tag"
fi

new_tag="v$version"
name="$basename-$version"

# Create the tag in the local git repository clone. Fail if the tag already
# exists.
git tag "$new_tag"

# Build both release and debug versions of the firmware binary
make release
make debug

# And copy the resulting biinary files into the current directory
cp -f out/release/firmware.bin "$name.bin"
cp -f out/debug/firmware.bin   "$name.debug.bin"
cp -f out/debug/firmware.map   "$name.debug.map"

# Compute SHA-256 checksums of the binary files
sums=$(sha256sum -b "$name.bin" "$name.debug.bin" "$name.debug.map")

# Push the newly created tag into the Github repository
git push origin "$new_tag"

# And create new draft release for the tag with all the generated files
# attached.
hub release create       \
    -d                   \
    -a "$name.bin"       \
    -a "$name.debug.bin" \
    -a "$name.debug.map" \
    -F - $new_tag << EOF
Release $version

**SHA256 checksums**:
\`\`\`txt
$sums
\`\`\`

**Full changelog**: https://github.com/hardwario/$basename/compare/$previous_tag...$new_tag
EOF
