.. index:: babel

Documents with text that is not in English
==========================================

If you make figures containing text objects in languages other than
English, you will need to enter accented characters, or characters
from other scripts such as Greek, Cyrillic, Farsi, Arabic, Hangul,
Kana, or Chinese characters.

Of course you can still use the LaTeX syntax ``K\"onig`` to enter the
German word "König", but for larger runs of text it's more convenient
to enter text in a script supported by your system.  When Ipe writes
the LaTeX source file, it writes the text in UTF-8 encoded Unicode.
You have to make sure that your LaTeX setup can handle this file.


.. index:: Russian text

Pdflatex for latin and cyrillic scripts
---------------------------------------

An easy solution, sufficient for German, French, and other languages
for which support is already in a standard LaTeX-setup, is to add
the line

.. code-block::

   \usepackage[utf8]{inputenc}

in your *Latex preamble* (set in the *Document properties*
dialog, available on the *Edit* menu).

In addition, you may need to setup your document language. So, to use
UTF-8 encoded Russian in LaTeX, your preamble would look like this:

.. code-block::

   \usepackage[utf8]{inputenc}
   \usepackage[russian]{babel}

.. texlive-lang-cyrillic

When setting this up, you have to keep in mind that Ipe can only
handle scalable fonts, such as Postscript Type1 fonts.  You'll have to
choose a setup that uses such scalable fonts.  Quite often it is
sufficient to install the package ``cm-super``, which contains
Type1 versions of standard fonts.

If your LaTeX setup does not contain scalable fonts for your
document, it will often fall back on Metafont fonts in bitmapped Type3
format.  Ipe will show an error message informing you about Type3
fonts in your LaTeX output.  In that case, try installing
``cm-super``, or use a package that explicitly uses Postscript
fonts.  For instance, for Russian you could install the *PsCyr*
package, and the following preamble:

.. code-block::

   \usepackage[utf8]{inputenc}
   \usepackage[russian]{babel}
   \usepackage{pscyr}


.. index:: Thai text
   
Using Xetex
-----------

If you set the LaTeX engine to ``xetex`` (you set this in the
*Document properties* dialog, available on the *Edit* menu), then Ipe
will use Xelatex to convert your text objects to PDF.  Xetex supports
Unicode natively, and will give you access to many scalable fonts on
your system.

For instance, to use Thai in your Ipe document, it suffices to set the
Latex engine to ``xetex``, and to select a Thai font in the
preamble:

.. code-block::

   \usepackage{fontspec}
   \setmainfont{Garuda}

(On a Unix-system, you can determine which fonts on your system
support Thai by saying ``fc-list :lang=th`` on the command line.)

Unfortunately, running Xetex is significantly slower than Pdflatex, so
you may want to turn off the automatic running of LaTeX in the *File*
menu.

If your script uses right-to-left writing, you will need :ref:`some
more effort <rtl>`.


.. index:: Korean text, Chinese text, Japanese text

Chinese, Japanese, and Korean
-----------------------------

Using Korean Hangul and Hanja in Ipe is quite easy, by placing this in your
preamble:

.. code-block::

   \usepackage[utf]{kotex}

.. texlive-lang-korean

Japanese LaTeX-users had long used a specialized version of TeX.
Fortunately, the new ``luatex`` implementation makes this unnecessary,
so we can now typeset Japanese by setting the *Latex engine* to
``luatex`` and including the ``luatexja`` package in the preamble:

.. code-block::

   \usepackage{luatexja}

.. texlive-lang-japanese

To typeset Chinese, the *xeCJK* package of CTeX also works with
Ipe.  Set the *Latex engine* to ``xetex``, and include the
package in your preamble:

.. code-block::

   \usepackage{xeCJK}

..
  texlive-lang-chinese
  Documentation can be found at \url{www.ctex.org}
  What does the no-math option do?
  \usepackage[no-math]{xeCJK}

.. _rtl:  
.. index:: Persian text, Arabic text

Right-to-left writing: Farsi, Hebrew, and Arabic
------------------------------------------------

Scripts with right-to-left writing require some extra care.  The main
document needs to be processed left-to-right for Ipe to work
correctly.  Only individual text objects can be translated using
right-to-left mode.

Here are solutions that work for Farsi (Persian),  Hebrew, and
Arabic.

Persian
^^^^^^^

Include the stylesheet *right-to-left.isy* from the Ipe
stylesheet folder.  It defines a text style ``rtl`` for
right-to-left text objects.

In the document properties (that is, in the *Document properties*
dialog, available on the *Edit* menu), set ``Latex engine`` to
``xetex``, and the ``Latex preamble`` to

.. code-block::

   \usepackage[documentdirection=lefttoright]{xepersian}
   \settextfont{FreeFarsi}

I needed to install the packages ``texlive-lang-arabic`` and
``fonts-freefarsi`` on my Linux system to use this.  On a
Unix-system, you can determine which fonts on your system support
Farsi by saying ``fc-list :lang=fa`` on the command line.

It is important to set the option ``documentdirection=lefttoright``
for the *xepersian* package, to make sure the main document is
processed in left-to-right mode.

You can now have text objects with Latin script using the
``normal`` text style, and text objects with Persian script using
the ``rtl`` text style.

If you want, you can make ``rtl`` the default text style, with the
following :ref:`customization <customize>`:

.. code-block::

   prefs.initial_attributes.textstyle = "rtl"


If you do not use the *right-to-left.isy* stylesheet, then you
have to put one more line in the preamble:

.. code-block::

   \ipedefinecolors{}
   \usepackage[documentdirection=lefttoright]{xepersian}
   \settextfont{FreeFarsi}

This is necessary, because Ipe normally loads the ``xcolors`` package
after processing the document preamble.  Some packages (like ``bidi``
and ``xepersian``) require to be loaded *after* ``xcolors``, so you
need to use ``\ipedefinecolors{}`` to load ``xcolors`` early.  We
didn't need this above, because the stylesheet :file:`right-to-left.isy`
already contains the command.

Hebrew
^^^^^^

Include the stylesheet *right-to-left.isy* from the Ipe
stylesheet folder.  It defines a text style ``rtl`` for
right-to-left text objects.

In the document properties (that is, in the *Document properties*
dialog, available on the *Edit* menu), set ``Latex engine`` to
``xetex``, and the ``Latex preamble`` to

.. code-block::

   \ipedefinecolors{}
   \usepackage{fontspec}
   \setmainfont{Liberation Serif}
   \setmonofont{Liberation Mono}
   \setsansfont{Liberation Sans}
   \usepackage{bidi}

I needed to install the package ``texlive-lang-arabic`` to use the
``bidi`` package.  On a Unix-system, you can determine which fonts
on your system support Hebrew by saying ``fc-list :lang=he`` on the
command line.

You can now have text objects with Latin script using the
``normal`` text style, and text objects in Hebrew using the
``rtl`` text style.

If you want, you can make ``rtl`` the default text style, with the
following :ref:`customization <customize>`:

.. code-block::

   prefs.initial_attributes.textstyle = "rtl"


Arabic
^^^^^^

In the document properties (that is, in the *Document properties*
dialog, available on the *Edit* menu), set ``Latex engine`` to
``luatex``, and the ``Latex preamble`` to

.. code-block::

   \usepackage{arabluatex}

If you don't want to use the standard Amiri font, select another font
in the preamble:

.. code-block::

   \newfontfamily\arabicfont[Script=Arabic]{KacstLetter}

On a Unix-system, you can list the fonts on your system supporting
Arabic by saying ``fc-list :lang=ar`` on the command line.

You can now create text objects in Arabic using the macro \verb+\arb+
and the environment ``arab``.

The following stylesheet *arabic.isy* makes this more
comfortable:

.. code-block::

   <ipestyle name="arabic">
   <textstyle name="arabic" type="minipage" begin="\begin{arab}" end="\end{arab}"/>
   <textstyle name="arabic" type="label" begin="\arb{" end="}"/>
   </ipestyle>

If you add this stylesheet to your document, you can select the
``arabic`` style for text objects, and directly write in Arabic
inside these objects.

If you want, you can make ``arabic`` the default text style, with the
following :ref:`customization <customize>`:

.. code-block::

   prefs.initial_attributes.textstyle = "arabic"

.. 
  finding fonts on your system with fc-list
  fc-list :lang=xx   # search for fonts for given language
  fc-list -f "%{family}\n" :lang=xx  # print only family
  Arabic: ar, Thai: th, Hebrew: he, Farsi: fa
  zh, ko, ja
  Covers Arabic, Thai, etc.
  https://bboxtype.com/typefaces/FiraGO
