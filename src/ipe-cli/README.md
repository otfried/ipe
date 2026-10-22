# ipe-cli

The command-line tools of [Ipe](https://ipe.otfried.org), compiled to
WebAssembly and runnable with Node.js (version 20 or later):

* `ipetoipe` – convert between Ipe XML and PDF
* `iperender` – render Ipe pages to PNG, SVG, PDF, EPS
* `ipeextract` – extract the Ipe XML from a PDF made by Ipe
* `ipescript` – run Lua scripts with the Ipe Lua API (the standard Ipe
  scripts such as `onepage` and `update-master` are included)

## Installation

    npm install -g @otfried/ipe-cli

or run without installing:

    npx -p @otfried/ipe-cli ipetoipe -pdf figure.ipe figure.pdf

## Notes

* Text in figures is typeset by running `pdflatex` (or `xelatex`,
  `lualatex`), so a TeX installation must be on the `PATH`, exactly as
  for the native Ipe.
* Personal scripts for `ipescript` are searched in `./` and
  `~/.local/share/ipe/scripts` (or `$XDG_DATA_HOME/ipe/scripts`),
  before the bundled ones.  Set `IPESCRIPTS` (colon-separated) to
  override.

## Building the package (maintainers)

Build the WebAssembly tools (`export IPEWASM=1; make` in `src/`, see
`../ipe-web/README.md`), then

    pnpm pack        # copies emscripten/bin/*.{js,wasm} and scripts/ in
    pnpm publish --access=public
    pnpm publish --access=public --tag beta  # for a pre-release
    pnpm publish --dry-run   # runs prepack (staging) and checks everything except the upload

Logging into npmjs registry:

    pnpm login
    pnpm whoami

**Pre-releases:** for a beta, set a version like `7.3.1-beta.1` and
  run `pnpm publish --tag beta`. Users then get it only with `pnpm
  install ipe-cli@beta`, and the default latest tag stays untouched.

**Mistakes:** you can run `pnpm deprecate ipe-cli@7.3.1 "reason"`. You
  can only unpublish within 72 hours of publishing, so prefer
  publishing a fixed version.

**Releases:** for each new Ipe release, rebuild the WASM tools, bump
  the version (`pnpm version 7.3.2 --no-git-tag-version`, or edit
  `package.json`), and run `pnpm publish`.

**Later:** once it works, you can switch to automated publishing from
  GitHub Actions using npm's "trusted publishing". That avoids
  long-lived tokens, but you can set it up after the first manual
  release, which has to be done by hand.