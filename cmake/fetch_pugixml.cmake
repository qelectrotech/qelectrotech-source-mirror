# Copyright 2006 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.

message(" - fetch_pugixml")

Include(FetchContent)

option(BUILD_PUGIXML "Build pugixml library, use system one otherwise" YES)

if(BUILD_PUGIXML)

  # Pinned to the commit v1.15 points at, not to the tag itself.
  #
  # A git tag is only a named pointer to a commit, and anyone with push access
  # to the upstream repository can move it (git push --force) to any other
  # commit. FetchContent fetches whatever the tag points at when the build
  # runs, so if a maintainer account or CI token is compromised, the attacker
  # can retarget a well-known release tag to malicious code: every fresh build
  # of QElectroTech then compiles it, while nothing changes in this repository
  # and the tag name still looks correct. A commit hash cannot be moved, because
  # it is derived from the content: different code always has a different hash.
  #
  # This attack has been used in the wild:
  # - March 2025, tj-actions/changed-files (CVE-2025-30066): tags v1 through
  #   v45.0.7 were retargeted to a commit that dumped CI secrets into build
  #   logs, affecting more than 23,000 repositories.
  # - March 2026, aquasecurity/trivy-action (CVE-2026-33634): 76 of 77
  #   version tags were force-pushed to a credential stealer and stayed
  #   malicious for about 12 hours.
  # Both were GitHub Actions rather than CMake dependencies, but the mechanism
  # is the same one FetchContent relies on here: resolving a git tag at build
  # time.
  #
  # To upgrade, look up the commit the new tag points at with
  # git ls-remote <repo> 'refs/tags/<tag>*', check that it is the release you
  # expect, and update both the hash and the trailing tag comment.
  #
  # How many lines that prints depends on which of the two kinds of tag
  # upstream created:
  # - A lightweight tag is nothing but a ref pointing straight at the commit,
  #   so ls-remote prints a single line, "refs/tags/<tag>", and its hash is
  #   the commit to pin. pugixml tags this way, which is why the v1.15 hash
  #   below is what "git ls-remote ... refs/tags/v1.15" reports directly;
  #   SingleApplication (v3.2.0) does the same.
  # - An annotated tag is a git object in its own right, carrying a tagger,
  #   a date, a message and optionally a GPG signature, and pointing at the
  #   commit. ls-remote then prints two lines: "refs/tags/<tag>" is the tag
  #   object and "refs/tags/<tag>^{}" is that object dereferenced, i.e. the
  #   commit. The KDE Frameworks modules tag this way, so for them it is the
  #   "^{}" hash that belongs in the pin; the other hash identifies the tag
  #   object itself, which is not the source revision and changes whenever
  #   upstream re-creates the tag, even over the very same commit.
  FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG        ee86beb30e4973f5feffe3ce63bfa4fbadf72f38) # v1.15
  set(PUGIXML_INSTALL OFF CACHE INTERNAL "")
  FetchContent_MakeAvailable(pugixml)
else()
  find_package(pugixml REQUIRED)
endif()
