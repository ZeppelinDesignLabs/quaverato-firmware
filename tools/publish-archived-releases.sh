#!/bin/bash
# Create or refresh the archived GitHub Releases (v1.1.3 / v2.3.6 / v2.4.2) from
# the assets under archive/. These ship the original field-verified hexes and are
# never rebuilt by CI -- see docs/releases.md.
#
# Requires gh authenticated with push access.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO"

if ! command -v gh >/dev/null 2>&1; then
  echo "gh is not installed" >&2
  exit 1
fi
gh auth status

# Historical tags point at commits on master and must never be moved: the
# archived hexes are only meaningful against those exact commits. Push a tag if
# it is missing, but stop rather than retarget one that already differs.
for tag in v1.1.3 v2.3.6 v2.4.2; do
  local_sha="$(git rev-parse -q --verify "refs/tags/$tag" || true)"
  if [ -z "$local_sha" ]; then
    echo "Local tag $tag is missing; see docs/releases.md for the intended commit" >&2
    exit 1
  fi
  remote_sha="$(git ls-remote --tags origin "refs/tags/$tag" | awk '{print $1}')"
  if [ -z "$remote_sha" ]; then
    echo "Pushing missing tag $tag"
    git push origin "$tag"
  elif [ "$remote_sha" != "$local_sha" ]; then
    echo "Tag $tag differs: local $local_sha, origin $remote_sha." >&2
    echo "Refusing to move a historical tag. Resolve this by hand." >&2
    exit 1
  fi
done

publish() {
  local ver="$1"
  local title="$2"
  local notes="$3"
  local dir="archive/v${ver}"

  if [ ! -d "$dir" ]; then
    echo "Missing $dir" >&2
    return 1
  fi

  if gh release view "v${ver}" >/dev/null 2>&1; then
    echo "Release v${ver} exists - refreshing assets"
    gh release upload "v${ver}" "${dir}"/* eeprom/quaverato-default-presets.hex --clobbe
  else
    echo "Creating release v${ver}"
    gh release create "v${ver}" "${dir}"/* eeprom/quaverato-default-presets.hex \
      --title "$title" \
      --notes "$notes"
  fi
}

publish 1.1.3 "v1.1.3 (archived field build)" \
  "Initial public firmware (Aug 2018), no MIDI. Original ISP hex, not rebuilt from the current toolchain. See docs/releases.md."

publish 2.3.6 "v2.3.6 (archived field build)" \
  "Full MIDI implementation and rate-knob rework (Dec 2018). Original ISP hex, not rebuilt from the current toolchain. See docs/releases.md."

publish 2.4.2 "v2.4.2 (archived field build)" \
  "Four-stage relay sequence with isolator on pin 11. Original ISP hex (15,642 bytes), not rebuilt from the current toolchain. See docs/releases.md."

echo "Done."
