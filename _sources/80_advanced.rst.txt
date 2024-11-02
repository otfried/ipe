Advanced topics
===============

.. _multiple:

Multiple figures in one Ipe document
------------------------------------

When writing an article in Latex, you can store all the figures in a
single Ipe document, by placing each figure on its own page.  In
Latex, each figure can then be included by saying, for instance,

.. code-block:: latex

   \includegraphics[page=7]{figs}

to include the figure on page 7 of the Ipe document :file:`figs.pdf`.

To avoid having to remember which figure is on which page (and having
to renumber all figures when you insert or delete a figure), you can
give *names* to the pages (figures) in your Ipe document, using
:menuselection:`Pages --> Edit title & sections`. Enter the name in
the *Section* field, using only letters.

Then, run the *page-labels* script on your document, as follows:

.. code-block:: console

   ipescript page-labels figs.pdf

This reads the document :file:`figs.pdf` and writes a new file
:file:`figsLabels.tex`.

In your Latex document, include this file in the preamble:

.. code-block:: latex

   \input{figsLabels}

You can now include a figure using its symbolic name, like this:

.. code-block:: latex

   \includegraphics[page=\ipeFigXXX]{figs}

where ``XXX`` is the symbolic name of the figure.

Whenever you add or delete a figure to the document, just run the
*page-labels* script again, and your symbolic names will remain up to
date.

If you want to refer to a specific view of a page, give the view a
name (:menuselection:`Views --> Edit view`). The *page-labels* script
will then generate a label consisting of first the page name and the
view name.  For instance, for the view named *YYY* on the page named
*XXX*, the label will be ``\ipeFigXXXYYY``.


.. _sharing:

Sharing Latex definitions with your Latex document
--------------------------------------------------

When using Ipe figures in a Latex document, it is convenient to have
access to some of the definitions from the document.

Ipe comes with a Lua script *update-master* that makes this easy.

In your Latex document, say :file:`master.tex`, surround the
interesting definitions using ``%%BeginIpePreamble`` and
``%%EndIpePreamble``, for instance like this:

.. code-block:: latex

  %%BeginIpePreamble
  \usepackage{amsfonts}
  \newcommand{\R}{\mathbb{R}}
  %%EndIpePreamble


Running the script as 

.. code-block:: console

  ipescript update-master master.tex

extracts these definitions and saves them as a stylesheet
:file:`master-preamble.isy`. (This filename is fixed, and does not
depend on the document name.)

Running this script as 

.. code-block:: console

  ipescript update-master master.tex figures/*.ipe

creates the stylesheet :file:`master-preamble.isy` as above.  In
addition, it looks at all the Ipe figures mentioned on the command
line. The script adds the new stylesheet to each figure, or updates
the stylesheet to the newest version (if the figure already contains a
stylesheet named ``master-preamble``).

.. _writing-ipelets:

Writing ipelets
---------------

An ipelet is an extension to Ipe.  Ipe 7 uses the scripting language
`Lua <http://www.lua.org>`_ (in fact, most of the Ipe program itself
is written in Lua), and loads ipelets written in Lua when it starts
up.  It is also possible to write ipelets in C++, using a small Lua
wrapper that declares the methods available inside the ipelet.

Documentation about writing ipelets can be found in the `Ipelib
documentation <https://ipe.otfried.org/manual/>`_.

.. _troubleshooting:

Troubleshooting the \LaTeX-conversion
-------------------------------------

Ipe converts text objects from their Latex source representation to a
representation that can be rendered and included the PDF output by
creating a Latex source file and running Pdflatex.  This happens in a
dedicated directory, which Ipe creates the first time it is used.  The
Latex source and output files are left in that directory and not
deleted even when you close Ipe, to make it easy to solve problems
with the Latex conversion process.

You can determine the directory used by Ipe using :menuselection:`Help
--> Show configuration`.  If you'd prefer to use a different
directory, set the environment variable ``IPELATEXDIR`` before
starting Ipe.

If Ipe fails to translate your text objects, and you cannot find the
problem by looking at the log file displayed by Ipe (or Ipe doesn't
even display the log file), you can terminate Ipe, go to the
conversion directory, and run Pdflatex manually:

.. code-block:: console

   pdflatex ipetemp.tex


.. _customize:

Customizing Ipe
---------------

Since most of Ipe is writing in Lua, an interpreted language, much of
Ipe's behavior can be changed without recompilation. 

The main customization options are in the files :file:`prefs.lua`
(general settings), :file:`shortcuts.lua` (keyboard shortcuts), and
:file:`mouse.lua` (mouse shortcuts).  (Check the Lua code path in
:menuselection:`Help --> Show configuration` if you can't locate the
files.)

If you have installed Ipe for your personal use only (for instance
under Windows), you can simply modify the original Lua file.  In all
other cases, you need to provide a small Lua ipelet that will change
the setting you wish to change.

A small example is the following ipelet that changes a keyboard
shortcut and the maximum zoom:

.. code-block::

   ----------------------------------------------------------------------
   -- My customization ipelet: customize.lua
   ----------------------------------------------------------------------
   prefs.max_zoom = 100
   shortcuts.insert_text_box = "I"
   shortcuts.mode_splines = "Alt+Ctrl+I"

The ipelet needs to be placed with the extension ``.lua`` somewhere
on the ipelet path (check *Show configuration* again).  On Unix,
the directory :file:`~/.local/share/ipe/ipelets` will do nicely.  On Windows,
you will have to set the environment variable IPELETPATH, see the
next section.

.. _environment:

Environment variables
---------------------

Ipe, :program:`ipetoipe`, :program:`iperender`, and
:program:`ipescript` respect the following environment variables:

IPELATEXDIR
  the directory where Ipe runs Latex.

IPELATEXPATH
  the directory that contains the *pdflatex*, *xelatex*, and
  *lualatex* commands.  If not set, Ipe assumes the commands are on
  your path.

IPEDEBUG
  set to 1 for debugging output.

IPETEXFORMAT
  if set, Ipe will not call pdflatex but pdftex requesting the
  pdflatex format (and similarly for xetex and luatex).

The Ipe program uses several additional environment variables:

EDITOR
  external editor to use for editing text objects.

IPESTYLES
  a list of directories, separated by semicolons on Windows and colons
  otherwise, where Ipe looks for stylesheets, for instance for the
  standard stylesheet :file:`basic.isy`. You can write ``_`` (a single
  underscore) for the system-wide stylesheet directory.  If this
  variable is not set, the default consists of the system-wide
  stylesheet directory, plus :file:`~/.local/share/ipe/styles` on
  Linux and :file:`~/Library/Ipe/Styles` on MacOS.

IPELETPATH
  a list of directories, separated by semicolons on Windows and colons
  otherwise, containing ipelets. You can write ``_`` (a single
  underscore) for the system-wide ipelet directory. If this variable
  is not set, the default consists of the system-wide ipelet
  directory, plus :file:`~/.local/share/ipe/ipelets` on Unix or
  :file:`~/Library/Ipe/Ipelets` on MacOS.

IPEICONDIR
  directory containing icons for the Ipe user interface.

IPEDOCDIR
  directory containing Ipe documentation.

IPELUAPATH
  path for searching for Ipe Lua code.

The :program:`ipescript` program uses the following environment
variable:

IPESCRIPTS
  a list of directories, separated by semicolons on Windows and colons
  otherwise, where *ipescript* looks for scripts. You can write ``_``
  (a single underscore) for the system-wide script directory.  If this
  variable is not set, the default consists of the current directory
  and the system-wide script directory, plus
  :file:`~/.local/share/ipe/scripts` on Linux or
  :file:`~/Library/Ipe/Scripts` on OS X.

On Windows, you can use the special drive "letter" ``ipe:`` inside
environment variables.  Ipe translates it into the drive letter for
the drive containing your Ipe executables.

.. _ipe-conf:

ipe.conf
""""""""

Ipe allows you to set environment variables by writing the definitions
in a file *ipe.conf*.  On Windows, the file has to be in the top
level of the Ipe directory (the same place that contains the
*readme.txt* and *gpl.txt* files), on Linux it is
*~/.config/ipe/ipe.conf*, on MacOS it is 
*~/Library/Ipe/ipe.conf*.
Each line of the
file contains a setting for one environment variable, for instance
like this:

.. code-block::

   IPEDEBUG=1
   IPELATEXDIR=C:\latexrun

.. _usb-stick:

Ipe on a USB-stick
------------------

Ipe for Windows is entirely self-contained---you can unzip the package
anywhere, including on a USB-stick, and run Ipe from there.  However,
Ipe needs access to a LaTeX installation.  One option is to use LaTeX
online---you can enable this from the *Help* menu.

The other option is to install LaTeX as a portable installation on
the same USB-stick:

* You can use the `MiKTeX portable edition <https://miktex.org/howto/portable-edition>`_.
* The alternative is to use `texlive
  <https://www.tug.org/texlive/>`_. Download *install-tl-windows.exe*
  and run it, selecting the option for *Custom install*.  When the
  full installer starts, change *Portable setup* to *Yes*, and change
  the *TEXDIR* directory to ``X:\texlive``, where *X* is the drive
  letter of the USB-stick.  You can also decrease the size of your
  installation by unselecting unnecessary packages and languages.

Installing LaTeX on the USB-stick will take quite a while.  When it is
done, unzip the Ipe package for Windows onto the root of the
USB-stick.  You should have the Ipe binary in
``X:\ipe-7.x.y\bin\ipe.exe``.

Now create a directory ``X:\latexrun`` (it will be used by Ipe to run
Latex).

Finally, you need to create a small :ref:`configuration file
<ipe-conf>` as ``X:\ipe-7.x.y\ipe.conf``.  The contents of the file
should be (for MiKTeX):

.. code-block::

   IPELATEXPATH=ipe:\miktex-portable\texmfs\install\miktex\bin\x64
   IPELATEXDIR=ipe:\latexrun

or (for texlive):

.. code-block::

   IPELATEXPATH=ipe:\texlive\bin\win32
   IPELATEXDIR=ipe:\latexrun

The ``ipe:`` part is translated by Ipe into the drive letter for
the drive that Ipe is executed from, so it will correctly point to
your USB-stick.

Double-check the setting for the ``IPELATEXPATH``---it should be the
directory that contains the ``pdflatex.exe`` program.  Open Ipe and
look :menuselection:`Help --> Show configuration`, and check that all
settings are correct.

It should now work to run Ipe from the USB-stick, whenever you plug it
into a Windows computer.

If you want to make further :ref:`customization settings <customize>`,
you can use the variable ``config.ipedir`` in your Lua code to refer
to the USB-stick drive.

.. _wine:

Running Ipe under Wine on Linux
-------------------------------

Ipe itself works fine under Wine, but there is an issue: We don't want
to create a new tex installation for Windows under Wine, we want to
reuse the Linux tex installation!

So first we need to make the pdflatex program available to Wine, by
putting a symbolic link in the simulated ``C:`` drive.

.. code-block:: console

   $ cd  /.wine/drive_c/windows/
   $ ln -s /usr/bin/pdflatex pdflatex.exe

The second problem is that Ipe is not able to wait for the completion
of the pdflatex call---it starts pdflatex, and then immediately tries
to read the pdflatex output, which of course fails.  The solution is
to make Ipe wait for a specified number of milliseconds before trying
to read the pdflatex output.

You achieve this by creating a small text file called :ref:`ipe.conf
<ipe-conf>` and placing it in the top-level Ipe directory (that is,
the directory that contains *readme.txt* and *gpl.txt*).  The contents
of the file should be:

.. code-block::

   IPEWINE=1000
   IPELATEXPATH=c:\windows

(You can define any other environment variable in the same file.)

