.. _scripts-and-ipelets:

Scripts and ipelets
===================

Most of the Ipe program is written in the scripting language `Lua
<https://www.lua.org>`_, and the same Lua library that Ipe uses to
read, write, and modify documents is available for your own programs.
There are two ways to use it:

* An **Ipe script** is a Lua program that you run from the command
  line using :program:`ipescript`.  A script reads an Ipe document,
  does something with it, and typically writes a new document or prints
  some information.  Scripts are good for batch processing and for
  tasks that you want to run on many files.

* An **ipelet** is an extension of the Ipe editor.  It is a Lua file
  that adds one or more entries to the *Ipelets* menu, and works on
  the document you are editing, usually on the objects you have
  selected.  Ipelets are good for functions that you want to use
  interactively.

Scripts and ipelets share most of what they need to know.  They use the
same classes for documents, pages, objects, and geometry, and the same
conventions.  The differences are in how they are started and how they
treat the document:

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * -
     - Script
     - Ipelet
   * - Started by
     - :program:`ipescript` on the command line
     - the *Ipelets* menu (or a keyboard shortcut) in the Ipe editor
   * - Document
     - loads and saves it itself
     - the document being edited, accessed through the *model*
   * - Changing the document
     - directly
     - **never directly**: it registers an undo item with the model
   * - User interface
     - command-line arguments, :code:`print`, files
     - dialogs, selection, mouse position, status bar
   * - Placed in
     - Ipe's scripts folder
     - Ipe's ipelets folder

The rest of this chapter first introduces the common Lua library, then
describes scripts and ipelets in turn.  The complete reference for the
Lua library is the `Ipelib documentation
<https://ipe.otfried.org/manual/>`_ (look for the pages *Lua bindings
for ...*, and *Ipelets*).  The best documentation of what is possible
are the existing scripts and ipelets that come with Ipe: the scripts
are in the :file:`scripts` folder, and the ipelets in the :file:`ipelets`
folder of the Ipe installation (and in the Ipe source distribution).

.. _lua-library:

The Lua library in a nutshell
-----------------------------

All of Ipe's Lua functions are in a table called :code:`ipe`.  Here are
the most important things you need to know.

.. code-block:: lua

   local doc = assert(ipe.Document("figure.ipe"))  -- load a document
   print(#doc)                    -- number of pages
   local page = doc[1]            -- pages are numbered from 1
   for i, obj, selected, layer in page:objects() do
     print(i, obj:type(), layer)  -- "path", "text", "group", ...
   end
   local v = ipe.Vector(100, 200)
   local m = ipe.Translation(v) * ipe.Rotation(math.pi / 2)
   doc:save("result.ipe")

* Pages, views, objects, and layers are **numbered from 1**, as usual
  in Lua.  Layers are identified by their name, not by a number.

* The coordinate system is the one of the Ipe file: units are
  PostScript points (1/72 inch), the origin is at the lower left of
  the paper, and the y-axis points *up*.

* The geometric classes :code:`ipe.Vector`, :code:`ipe.Matrix`,
  :code:`ipe.Line`, :code:`ipe.Segment`, :code:`ipe.Bezier`, and
  :code:`ipe.Arc` are immutable values: operations create new values.
  Only :code:`ipe.Rect` can be changed (:code:`r:add(v)` extends the
  rectangle).  Vectors and matrices have operators:
  :code:`v + w`, :code:`2 * v`, :code:`m * v`, :code:`m1 * m2`.

* The objects you get from a page (:code:`page[i]` or
  :code:`page:objects()`) are the page's own objects, not copies.
  Changing such an object changes the page.  Use :code:`obj:clone()` to
  make a copy, and :code:`page:clone()` to make a copy of the entire
  page.

* Attributes of objects are set and read using :code:`obj:set(name,
  value)` and :code:`obj:get(name)`.  The attribute names are those
  of the Ipe editor's property boxes, for instance :code:`stroke`,
  :code:`fill`, :code:`pen`, and :code:`dashstyle`.  The value
  can be an absolute value (a number, or a color given as
  :code:`{r=0.5, g=0.5, b=0.5}`), or a symbolic name from the stylesheet such as
  :code:`"red"` or :code:`"normal"`.

* Functions that read files return :code:`nil` and an error message
  when they fail.  Calling a function with illegal arguments (such as
  a page number that does not exist) raises a Lua error.

* New objects are created with :code:`ipe.Path`, :code:`ipe.Text`,
  :code:`ipe.Reference` (a symbol), :code:`ipe.Group`, and
  :code:`ipe.Image`.  The first three take a table with the attributes
  of the object (in an ipelet, :code:`model.attributes` is such a
  table) and the data of the object.  A path's geometry is described by a
  *shape*, a Lua table.  For instance, a circle with radius *r* and
  center *c* is :code:`{ { type="ellipse"; ipe.Matrix(r, 0, 0, r, c.x,
  c.y) } }`.  The reference documentation explains shapes in detail.

.. _writing-scripts:

Writing scripts
---------------

A script is a Lua file in one of Ipe's script folders.  The script
:file:`count-objects.lua` is run by saying

.. code-block:: console

   ipescript count-objects figure.ipe

(Do not write the extension :file:`.lua`.)  The command-line arguments
are available in the global table :code:`argv`.  Ipe looks for scripts
in the current directory, in the user's script folder
(:file:`~/.local/share/ipe/scripts` on Linux,
:file:`~/Library/Ipe/Scripts` on MacOS), and in the system-wide script
folder.  The environment variable ``IPESCRIPTS`` changes this, see
:ref:`environment`.

Here is a script that prints how many objects of each type there are on
every page of a document:

.. literalinclude:: examples/count-objects.lua
   :language: lua

:download:`count-objects.lua <examples/count-objects.lua>`

A script can do anything that Lua can do, and has the entire document
at its disposal, so it can modify it directly.  Here is a script that
makes all paths of a document red, and saves the result in a new file:

.. literalinclude:: examples/red-paths.lua
   :language: lua

:download:`red-paths.lua <examples/red-paths.lua>`

Ipe comes with several scripts that are good starting points for your
own scripts.  For instance, *onepage* collapses all pages of a document
into the layers of a single page, *scratchpad* prepares a presentation
for annotating during the talk, and *update-styles* updates the
stylesheets in many documents.

.. _writing-ipelets-intro:

Writing ipelets
---------------

An ipelet is a Lua file in one of Ipe's ipelet folders.  Ipe loads all
ipelets when it starts.  The ipelet folders are the system-wide folder
that comes with Ipe, and the user's own folder
(:file:`~/.local/share/ipe/ipelets` on Linux,
:file:`~/Library/Ipe/Ipelets` on MacOS; you can find the exact location
under :menuselection:`Help --> Show configuration`, and change it
using the environment variable ``IPELETPATH``, see :ref:`environment`).

An ipelet defines some global variables that tell Ipe about it:

``label``
  the name of the ipelet in the *Ipelets* menu.  A file that does not
  define a label is not shown in the menu (this is how you can write
  customizations of Ipe).

``about``
  a string shown in :menuselection:`Help --> Ipelet information`.

``run``
  the function to call when the ipelet is invoked.

``methods``
  for an ipelet with several functions, a table with one entry for each
  function.  Each entry is a table with a ``label`` (the text of its
  menu entry) and optionally a ``run`` function (if it has none, the
  global ``run`` is called).

``category``
  optional: the submenu of the *Ipelets* menu in which the ipelet
  appears.  (The menu is set up by :code:`prefs.ipelet_menu`.)

When the user invokes the ipelet, Ipe calls the ``run`` function with two
arguments: the **model**, which represents the document window, and the
number of the method (always 1 for an ipelet with a single function).

The simplest possible ipelet draws a circle at the mouse position:

.. literalinclude:: examples/ipelet-circle.lua
   :language: lua

:download:`ipelet-circle.lua <examples/ipelet-circle.lua>`

Save this in your ipelet folder as :file:`circle.lua`, restart Ipe, and
you will find *Circle* in the *Ipelets* menu.  If you set
:code:`prefs.developer = true` in your :file:`customization.lua` (see
:ref:`customize`), the *Help* menu has a *Developer* submenu with an
entry :menuselection:`Help --> Developer --> Reload ipelets`, so you do
not need to restart Ipe after every change.

Keyboard shortcuts can be assigned in :file:`customization.lua` using the
name of the ipelet's file (without the extension) and the number of the
method, for instance :code:`shortcuts.ipelet_1_circle = "Ctrl+Alt+C"`.

The ipelet's environment
""""""""""""""""""""""""

Each ipelet runs in a private environment, so you do not have to worry
about polluting the global namespace.  The following names are
available inside an ipelet: :code:`ipe`, :code:`ipeui`, :code:`math`,
:code:`string`, :code:`table`, :code:`assert`, :code:`ipairs`,
:code:`pairs`, :code:`print`, :code:`tonumber`, :code:`tostring`, and
the Ipe tables :code:`prefs`, :code:`config`, :code:`shortcuts`, and
:code:`mouse`.  Everything else, such as :code:`type`, :code:`pcall`,
:code:`io`, :code:`os`, or the Lua code of Ipe, must be accessed
through :code:`_G`, for instance :code:`_G.type(x)` or
:code:`_G.revertOriginal`.  (Writing :code:`type(x)` gives the error
"attempt to call a nil value (global 'type')".)

Text printed with :code:`print` appears on the terminal from which you
started Ipe.

.. _ipelet-model:

The model, and why you must not touch the document
""""""""""""""""""""""""""""""""""""""""""""""""""

The model is a Lua object with these members:

:code:`model.doc`
  the document being edited.

:code:`model.pno`, :code:`model.vno`
  the number of the current page and view.

:code:`model:page()`
  the current page.

:code:`model:selection()`
  a table with the numbers of the selected objects of the current
  page.

:code:`model.attributes`
  the attributes currently set in the property boxes of the user
  interface (a table you can pass to :code:`ipe.Path` and friends).

:code:`model.snap`
  the current snapping settings.

:code:`model.ui`
  the user interface, with for instance :code:`model.ui:pos()` (the
  mouse position after snapping), :code:`model.ui:win()` (the window,
  to be used as the parent of dialogs), and :code:`model.ui:explain(text)`
  (shows text in the status bar).

:code:`model:warning(text, details)`
  shows a warning box.

:code:`model:getString(prompt, caption, initial)`
  asks the user for a string (returns :code:`nil` if the user cancels).

An ipelet has access to the live document, but it must **not modify
it directly**.  The Ipe editor keeps an undo stack, and it does not
notice if the document changes behind its back.  An ipelet that changes
the page directly breaks undo and redo, may corrupt the document, and
does not mark the document as modified.  Instead, the ipelet makes its
changes through an **undo item**.  This is a table with at least these
fields:

``label``
  a text for the status bar and the undo history (:guilabel:`Undo
  'label'`).

``pno``, ``vno``
  the page and view to show after an undo or redo.

``undo``, ``redo``
  functions that are called with the undo item and the document.
  :code:`redo` makes the change, :code:`undo` reverts it.

The ipelet creates the item and passes it to :code:`model:register(t)`, which
calls :code:`t.redo(t, model.doc)` to perform the change, remembers the
selection, and puts the item on the undo stack.  The ipelet should
**not** change the document itself, and neither should it keep any
reference to the objects of the document: the item's :code:`redo` and
:code:`undo` functions are the only code that may touch the document
after the item has been registered.

There are three ways to create an undo item, from the simplest to the most
general one.

**1. Use one of the model's helper functions.**  :code:`model:creation(label,
obj)` inserts a new object on the current page (as in the circle
ipelet above), :code:`model:transformation(label, matrix)` transforms the
selected objects, and :code:`model:setAttribute(name, value)` sets an
attribute of the selected objects.  These functions create and
register the undo item for you.  The next ipelet uses
:code:`model:transformation`, and also shows how to ask the user for
a value in a dialog:

.. literalinclude:: examples/ipelet-scale.lua
   :language: lua

:download:`ipelet-scale.lua <examples/ipelet-scale.lua>`

**2. Work on a copy of the page.**  When the ipelet changes several
objects, the easiest approach is to make two copies of the page: the
*original*, and the *final* page in which the ipelet makes all its
changes.  The undo item then simply swaps the page in the document
between these two copies, using the functions
:code:`_G.revertOriginal` and :code:`_G.revertFinal`.  This is the
pattern used by most of the ipelets that come with Ipe:

.. literalinclude:: examples/ipelet-red-paths.lua
   :language: lua

:download:`ipelet-red-paths.lua <examples/ipelet-red-paths.lua>`

The same pattern also works to create several objects.  Here the
ipelet collects what it needs to know about the selected objects first,
and only then inserts new objects, since inserting objects while
iterating over the page would also visit the new objects:

.. literalinclude:: examples/ipelet-mark-centers.lua
   :language: lua

:download:`ipelet-mark-centers.lua <examples/ipelet-mark-centers.lua>`

**3. Write your own undo and redo functions.**  Copying a page is easy, but
wasteful if the ipelet only changes a small thing, such as the
properties of a layer.  Then the undo item can remember what it
needs to revert its action.  The next example also shows an ipelet
with several methods:

.. literalinclude:: examples/ipelet-lock-layers.lua
   :language: lua

:download:`ipelet-lock-layers.lua <examples/ipelet-lock-layers.lua>`

If your ipelet changes the stylesheets of the document, set
:code:`t.style_sheets_changed = true` in the undo item, so that Ipe updates the
lists of symbolic names in the user interface after an undo or redo.

Dialogs
"""""""

Ipelets can show dialogs using :code:`ipeui.Dialog`, as in the scale example
above.  Dialogs can contain labels, input fields, text boxes, lists,
combo boxes, check boxes, and image previews, arranged in a grid.  See the
Ipelib documentation (*Lua bindings for dialogs and menus*) for
all the details, and the ipelets *search-replace* and *selectby* that
come with Ipe for more examples.

.. _testing-ipelets:

Testing an ipelet without the editor
""""""""""""""""""""""""""""""""""""

It is tedious to test an ipelet by starting the Ipe editor, loading a
document, selecting objects, and invoking the ipelet.  The script
*run-ipelet* that comes with Ipe runs an ipelet on a document from the
command line, with a model that behaves like the one in the editor
(it uses the real model of the Ipe editor, including its undo stack):

.. code-block:: console

   ipescript run-ipelet circle.lua figure.ipe result.ipe

The ipelet is loaded in the same restricted environment as in the editor.
It is run with all objects of the first page selected (use the options
``--select none`` or ``--select 1,3`` to change this, ``--page``,
``--view``, and ``--method`` to change the page, the view, and the number of
the method to run).  The result is saved as :file:`result.ipe`.

The script then undoes and redoes the changes made by the ipelet, using
Ipe's own undo and redo, and checks that the page is restored exactly.
This finds the most common mistake in ipelets, namely
not registering an undo item or not reverting everything in the
:code:`undo` function:

.. code-block:: console

   $ ipescript run-ipelet scale.lua figure.ipe result.ipe factor=3
   Running scale on page 1, view 1 with 3 selected objects
   [dialog] Scale selection
   [dialog]   factor = 3
   [explain] scale selection
   1 undo item(s) registered: scale selection
   [explain] Undo 'scale selection'
   Undo restores the original page: OK
   [explain] Redo 'scale selection'
   Redo restores the result: OK
   Saved result.ipe

Dialogs cannot be shown by this script.  Instead, a dialog behaves as if
the user had accepted it (or canceled it, if you pass ``--cancel``), and
with every element having the value set by the ipelet.  You can give a
different value by adding arguments of the form *name=value*, as in the
example above, where :code:`factor=3` sets the value of the dialog
element :code:`factor`.  The exit status of *run-ipelet* is not zero
if the ipelet fails or the undo check finds a problem.

*run-ipelet* takes the Lua code of the Ipe editor from your Ipe
installation.  If you want to test with a different version (for
instance a source tree), set the environment variable ``IPELUAPATH`` to a Lua
path such as :file:`/path/to/ipe/src/ipe/lua/?.lua`.

*run-ipelet* has no canvas and does not run LaTeX, so it cannot test
ipelets that depend on them.  It is very useful for everything else,
and also makes it possible to write automatic tests for your ipelets.

C++ ipelets
"""""""""""

Ipelets can also be written in C++, which is useful if you already have
code for some geometric computation, or want to use a library such as
CGAL.  Such an ipelet consists of a shared library, and a small Lua file
that declares its label and its methods, and calls the C++ code.
The details are described in the Ipelib documentation (see the page *Ipelets*).

.. _ipelets-ai:

Writing scripts and ipelets with an AI assistant
------------------------------------------------

AI assistants (large language models) can write simple scripts and
ipelets for you.  The result will be much better if the assistant has
the right information.  We suggest to give it:

* this chapter of the manual, and the reference of the Lua library (the
  page *Lua bindings* of the Ipelib documentation, which lists all
  functions, and the page about dialogs if your ipelet has a dialog),

* one or two existing scripts or ipelets that are similar to what you
  need (the examples in this chapter, or the ones that come with Ipe).

Tell the assistant that it must write a script or an ipelet (it
matters, see above), what the objects it works on are, and
what should happen.  Then check the result carefully before using it
on a precious document.

If you use an assistant that can run commands, tell it about
*run-ipelet*: it can then test the ipelet on a sample document,
see the error messages and the result of the undo check, and fix its code
without your help.  Ipelets that modify the document without registering
an undo item are the most common problem of ipelets written without
testing.  Here is the output of *run-ipelet* for the example
:file:`ipelet-wrong.lua`, which changes the page directly:

.. literalinclude:: examples/ipelet-wrong.lua
   :language: lua

.. code-block:: console

   $ ipescript run-ipelet ipelet-wrong.lua figure.ipe result.ipe
   Running ipelet-wrong on page 1, view 1 with 3 selected objects
   The ipelet did not register an undo item.
   WARNING: the page has been modified without registering an undo item!
   Saved result.ipe
