# Building and releasing

How to build Mage Bros. Native on Windows and publish a new version, step by step.

## Quick reference

| You want to | Run (in the repository folder) | Version in `CMakeLists.txt` |
|---|---|---|
| Build the current version (to test, or to rebuild a release) | double-click `make_release.bat`, or `make_release.bat` | unchanged |
| Build the next small release (fixes, a chunk of work) | `make_release.bat patch` | 0.4.21 -> 0.4.22 |
| Build the first release of a new stage | `make_release.bat minor` | 0.4.21 -> 0.5.0 |
| Build 1.0 or a major release | `make_release.bat major` | 0.4.21 -> 1.0.0 |
| Build an exact version (to skip one) | `make_release.bat 0.4.23` | set to 0.4.23 |

**Building never publishes anything.** A release goes public only when you **push** to `main`
with a version that has no GitHub release yet (see step 6).

## What you need (once)

- **Visual Studio 2022 or newer** with the **"Desktop development with C++"** workload (the
  Community edition is fine). The script finds it on its own; CMake comes with it.
- **Git**, with this repository cloned (for example to
  `C:\Users\ExoJag\Documents\MageBros-Project\MageBrosNative`).
- Optional: **`MageBros.exe` in the folder above the repository**
  (`MageBros-Project\MageBros.exe`). Then the map and hero tests also run on all 40 levels of
  your copy. Without it only the tests that need no game files run.

The first build downloads SDL3 and takes a few minutes; later ones are much quicker.

## Running the script

Double-click `make_release.bat`, or open a Command Prompt or PowerShell in the repository folder:

```bat
make_release.bat patch
```

It then:

1. changes the version in `CMakeLists.txt`, if you asked for that;
2. configures and builds the Release build in `build\`;
3. runs the tests;
4. packages `build\MageBrosNative-<version>-windows-x64.zip`;
5. prints `Done: ...` and waits for a key.

The game itself is also at `build\Release\MageBrosNative.exe`, the same program as in the zip.

**If something fails**, the script prints `Release failed.` and puts a raised version back, so
`CMakeLists.txt` is as it was. Scroll up to see the build error or the failed test.

A line saying `'vswhere.exe' is not recognized` is harmless.

## Making a new release, step by step

### 1. Start from an up-to-date `main`

```bat
git checkout main
git pull
git status
```

`git status` should say "nothing to commit, working tree clean" (apart from the changes you're
about to release).

### 2. Pick the new version number

- **patch** (0.4.21 -> 0.4.22): fixes and normal chunks of work. Almost every release.
- **minor** (0.4.x -> 0.5.0): the start of a new stage in `docs/ROADMAP.md`.
- **major** (-> 1.0.0): the big one.

**Skip any version a side branch has used**, even if that branch was never merged: its builds
went out under that number. Today `dxgi-present` uses **0.4.22**, so `main`'s next release is
**0.4.23** (`make_release.bat 0.4.23`). Check `CLAUDE.md` ("Where we are") and `docs/ROADMAP.md`
for any others.

To see which versions are already released:

```bat
git fetch --tags
git tag --list "v0.4.*"
```

### 3. Write the changelog

At the top of `CHANGELOG.md`, under `## [Unreleased]`, add a section for the new version in
**exactly** this form (the release workflow looks for it, and its text becomes the release
notes on GitHub):

```markdown
## [0.4.23] - 2026-11-02

- What changed, in plain words for players.
```

Move anything that was under `## [Unreleased]` into it, and leave `## [Unreleased]` empty above
it. Also bring `README.md` ("Status" line), `CLAUDE.md` ("Where we are") and `docs/ROADMAP.md`
up to the new version.

### 4. Build and test

```bat
make_release.bat 0.4.23
```

or `make_release.bat patch` when no version needs skipping. Check that it ends with
`Done: build\MageBrosNative-0.4.23-windows-x64.zip`, then play `build\Release\MageBrosNative.exe`
for a bit (copy it next to `MageBros.exe`, or run it with `--game <path to MageBros.exe>`).

### 5. Commit

```bat
git status
git add CMakeLists.txt CHANGELOG.md README.md CLAUDE.md docs
git commit -m "Short description (version 0.4.23)"
```

Check `git status` first: **never commit `MageBros.exe`, `MageBros.pdb`, the `.mid` files,
screenshots or anything exported from the game** (`.gitignore` covers them, but look anyway).
The `build\` folder is ignored too: the zip is not committed.

### 6. Push: this publishes the release

```bat
git push
```

Every push to `main` runs the **Release** workflow on GitHub
(`.github/workflows/release.yml`). It reads the version from `CMakeLists.txt`, and:

- if there is **no** release `v<version>` yet, it builds the zip again on GitHub's own Windows
  machine, runs the tests that need no game files, and publishes it as the release
  `v<version>`, with that version's `CHANGELOG.md` section as its notes;
- if that release **already exists** (a docs-only push, say), it does nothing.

Watch it under the repository's **Actions** tab if you like; it takes several minutes. The
release then shows on the **Releases** page with its zip.

## Common situations

- **I only changed docs.** Commit and push. Don't change the version: no release is made.
- **I want to rebuild a version that's already released** (to test it, or after a fresh clone).
  Run `make_release.bat` with nothing after it. The version stays as it is.
- **The build failed after a bump.** The version was put back automatically. Fix the error, then
  run the same command again.
- **I bumped the version by mistake** (the build worked, not committed yet). Put the file back
  with `git checkout -- CMakeLists.txt`, or edit the number on line 3 of `CMakeLists.txt`
  (`project(MageBrosNative VERSION x.y.z ...)`) by hand.
- **A push should have made a release but didn't.** The version in `CMakeLists.txt` probably
  wasn't raised, or that release already existed. Check the Actions tab for the run's log.
- **Releasing again under the same number.** Not possible: the workflow skips versions that are
  already released. Raise the version instead.

## Where the version lives

Line 3 of `CMakeLists.txt`, `project(MageBrosNative VERSION x.y.z LANGUAGES C CXX)`, is the only
place the number is kept. The build turns it into `build\generated\version.h` (from
`res\version.h.in`) for the program, and into the zip's name; the GitHub workflow reads the same
line. Don't type the version anywhere else in the code.
