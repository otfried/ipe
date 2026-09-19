# Ipe

The [extensible drawing editor Ipe](https://ipe.otfried.org) as a VS
Code extension.

## Usage

If you open an existing file with extension `.ipe`, the Ipe user
interface will appear inside VS Code.  You can edit Ipe documents in
the same way as in the Ipe desktop application or in [Ipe
Web](https://ipe-web.otfried.org).

To create a new Ipe document, create a new file with extension `.ipe`
in VS Code.

You can also work on Ipe documents in PDF format.  When you open for the first time, it may appear in a text edit - you can switch to the Ipe UI in the top right corner.

You can open several Ipe documents at the same time, but you cannot
open the same Ipe document in more than one tab.


## Installation instructions

```
code --install-extension Ipe-7.3.1-beta1.vsix 
```


## Caveats

VS Code reserves some shortcut keys, such as F1, F5, F11.  If some of them
do not work in Ipe, you will probably want to remap those Ipe actions.

The standard ipelets and style sheets are baked into the extension.
You cannot change them (a bad idea anyway), but you can override them
with your own setup.


## Not yet implemented

- Need to query host for list of available style sheets, and need to ask for a specific one.
- Some changes in Ipe do not mark the document dirty, e.g. setting a fill color from the properties
- auto export
- Cloud Latex
- Live Sharing
