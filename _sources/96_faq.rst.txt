
Frequently asked questions
==========================



Where can I get support for Ipe?
--------------------------------

You are already looking at the FAQ - good!

Have you looked at the `manual <https://otfried.github.io/ipe>`_?

If you need help compiling or using Ipe, please post on the `Ipe
discussion list
<https://mailman.science.uu.nl/mailman/listinfo/ipe-discuss>`_.

If you have found a bug in Ipe, please go to `Ipe issues
<https://github.com/otfried/ipe/issues>`_. Verify that the bug has not
been reported before, and file a bug report. (If the bug has been
reported before but is not fixed yet, you can add additional
information to the existing bug report that would help solve the
problem.)

If you want to say how much you love Ipe, you can send email to the
author. But please note that he hardly ever replies directly to Email
about Ipe (he has a day job).



MacOS says "the developer cannot be verified"
---------------------------------------------

This is the normal behaviour of macOS when you install an app by a
developer who is not a registered Apple developer.  Apple shamelessly
asks for $99 per year just so you can sign your application, and this
is simply not realistic for an open-source application that is
distributed for free.

Apple explains how to start an application anyway at
https://support.apple.com/en-au/guide/mac-help/mh40616/mac

The other alternative is to compile Ipe yourself.



I want more/other choices for colors!
-------------------------------------

The colors available in the drop-down box are taken from your
document's :doc:`stylesheets <60_stylesheets>` (like any other
:ref:`symbolic attribute <symbolic-and-absolute-attributes>`).

To have more colors available, you can either add a new stylesheet, or
modify the basic stylesheet added to new documents by Ipe. 


Attributes do not update
------------------------

When you click on a line, the line attributes (thickness, arrow head
etc.) don't update in the Properties - they remain as they were set.

In Microsoft software like Word the user interface automatically picks
up the settings from the current spot in the document. I don't like
this behavior, and Ipe won't adopt this. It would cause users to
inadvertently reuse the properties of objects they had previously
selected, instead of the standard settings they started with (and
wanted).

Note that this is not a problem in, say, Microsoft Word: When you
click somewhere to start typing text, the toolbar will revert to the
settings active at this spot of the document. This 'reverting'
wouldn't be possible in Ipe.

You can, however, copy object properties from an object to the user
interface and from there to other objects with the operations
:menuselection:`Edit --> Pick properties` and
:menuselection:`Edit --> Apply properties`.


How do I use Ipe figures in beamer?
-----------------------------------

If you make an animated figure in Ipe (that is, an Ipe page with
several views), you can use it in beamer like this:

.. code-block:: latex

  \includegraphics<1>[page=1]{ipefig}
  \includegraphics<2>[page=2]{ipefig}

You may also like `Suresh's tip
<http://geomblog.blogspot.com/2009/09/beamer-ipe-views.html>`_ on this
topic.


When Ipe refuses to open a PDF file you created with Ipe
--------------------------------------------------------

This usually happens because you have been previewing the PDF
document, and the previewer took the liberty to overwrite the original
file.

Ipe PDF files contain Ipe markup hidden inside the PDF structure. When
the file is modified by any other tool, this markup is lost, and Ipe
will no longer be able to read it.

So please be careful with PDF viewers that have the capability to
overwrite the file being viewed, in particular the Preview.app on
recent version of MacOS.

Fortunately, the Preview.app has inbuilt versioning. So from within
Preview.app, try :menuselection:`File --> Revert To --> Browse All
Versions`, select the earliest available version, hit "Restore" and
close the document. And voila, Ipe should be able to open the file
again.


Online Latex-compilation does not work
--------------------------------------

If you get authentication errors for the online Latex compilation
service, try using the HTTP protocol instead of HTTPS. You would need
change the preference

.. code-block:: lua

  prefs.latex_service_url = "http://latexonline.cc"

(So change https to http - see :ref:`customizing Ipe <customize>`.)

You will need to disable and again enable online compilation from the
Help menu for the change to take effect.)
