// --------------------------------------------------------------------
// A page of a document.
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2024 Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ipepage.h"
#include "ipegroup.h"
#include "ipereference.h"
#include "ipepainter.h"
#include "ipeiml.h"
#include "ipeutils.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Page
  \ingroup doc
  \brief An Ipe document page.

  Its main ingredients are a sequence of Objects (with selection
  state, layer, and a cached bounding box), a set of Layers, and
  a sequence of Views.

  Each object on a Page belongs to one of the layers of the page.
  Layers are orthogonal to the back-to-front ordering of objects (in
  particular, they are not "layered" - the word is a misnomer).  The
  "layer" is really just another attribute of the object.

  - Layers are either editable or locked.  Objects in a locked layer
    cannot be selected, and a locked layer cannot be made active in
    the UI.  This more or less means that the contents of such a layer
    cannot be modified---but that's a consequence of the UI, Ipelib
    contains no special handling of locked layers.

  - A layer may have snapping on or off---objects will behave
    magnetically only if their layer has snapping on.


  A Page is presented in a number of \e views.  Each view presents
  some of the layers of the page.  In addition, each view has an
  active layer (where objects are added when this view is shown in the
  UI), and possibly a transition effect (Acrobat Reader eye candy).

  A Page can be copied and assigned.  The operation takes time linear
  in the number of top-level object on the page.

*/

//! The default constructor creates a new empty page.
/*! This page still needs a layer and a view to be usable! */
Page::Page() : iTitle()
{
  iUseTitle[0] = iUseTitle[1] = false;
  iMarked = true;
}

//! Create a new empty page with standard settings.
/*! This is an empty page with layer 'alpha' and a single view. */
Page *Page::basic()
{
  // Create one empty page
  Page *page = new Page;
  page->addLayer("alpha");
  page->insertView(0, "alpha");
  page->setVisible(0, "alpha", true);
  return page;
}

// --------------------------------------------------------------------

//! save page in XML format.
void Page::saveAsXml(Stream &stream) const
{
  stream << "<page";
  if (!title().empty()) {
    stream << " title=\"";
    stream.putXmlString(title());
    stream << "\"";
  }
  if (iUseTitle[0]) {
    stream << " section=\"\"";
  } else if (!iSection[0].empty()) {
    stream << " section=\"";
    stream.putXmlString(iSection[0]);
    stream << "\"";
  }
  if (iUseTitle[1]) {
    stream << " subsection=\"\"";
  } else if (!iSection[1].empty()) {
    stream << " subsection=\"";
    stream.putXmlString(iSection[1]);
    stream << "\"";
  }
  if (!iMarked)
    stream << " marked=\"no\"";

  stream << ">\n";
  if (!iNotes.empty()) {
    stream << "<notes>";
    stream.putXmlString(iNotes);
    stream << "</notes>\n";
  }
  for (int i = 0; i < countLayers(); ++i) {
    stream << "<layer name=\"" << iLayers[i].iName << "\"";
    if (iLayers[i].locked)
      stream << " edit=\"no\"";
    if (iLayers[i].snapMode == SnapMode::Never)
      stream << " snap=\"never\"";
    else if (iLayers[i].snapMode == SnapMode::Always)
      stream << " snap=\"always\"";
    if (!iLayers[i].iData.empty()) {
      stream << " data=\"";
      stream.putXmlString(iLayers[i].iData);
      stream << "\"";
    }
    stream << "/>\n";
  }
  for (int i = 0; i < countViews(); ++i) {
    stream << "<view layers=\"";
    String sep;
    for (int l = 0; l < countLayers(); ++l) {
      if (visible(i, l)) {
	stream << sep << layer(l);
	sep = " ";
      }
    }
    stream << "\"";
    if (!active(i).empty())
      stream << " active=\"" << active(i) << "\"";
    if (!effect(i).isNormal())
      stream << " effect=\"" << effect(i).string() << "\"";
    if (markedView(i))
      stream << " marked=\"yes\"";
    if (!viewName(i).empty())
      stream << " name=\"" << viewName(i) << "\"";
    const AttributeMap &map = viewMap(i);
    if (map.count() == 0 && iViews[i].iLayerMatrices.empty()) {
      stream << "/>\n";
    } else {
      stream << ">\n";
      map.saveAsXml(stream);
      for (const auto &s : iViews[i].iLayerMatrices) {
	stream << "<transform layer=\"" << s.iLayer << "\" matrix=\""
	       << s.iMatrix << "\"/>\n";
      }
      stream << "</view>\n";
    }
  }
  int currentLayer = -1;
  for (ObjSeq::const_iterator it = iObjects.begin();
       it != iObjects.end(); ++it) {
    String l;
    if (it->iLayer != currentLayer) {
      currentLayer = it->iLayer;
      l = layer(currentLayer);
    }
    it->iObject->saveAsXml(stream, l);
  }
  stream << "</page>\n";
}

// --------------------------------------------------------------------

Page::SLayer::SLayer(String name)
{
  iName = name;
  locked = false;
  snapMode = SnapMode::Visible;
}

//! Set free data field of the layer.
void Page::setLayerData(int index, String data)
{
  iLayers[index].iData = data;
}

//! Set locking of layer \a i.
void Page::setLocked(int i, bool flag)
{
  iLayers[i].locked = flag;
}

//! Set snapping of layer \a i.
void Page::setSnapping(int i, SnapMode mode)
{
  iLayers[i].snapMode = mode;
}

//! Add a new layer.
void Page::addLayer(String name)
{
  iLayers.push_back(SLayer(name));
  iLayers.back().iVisible.resize(countViews());
  for (int i = 0; i < countViews(); ++i)
    iLayers.back().iVisible[i] = false;
}

//! Find layer with given name.
/*! Returns -1 if not found. */
int Page::findLayer(String name) const
{
  for (int i = 0; i < countLayers(); ++i)
    if (layer(i) == name)
      return i;
  return -1;
}

const char * const layerNames[] = {
  "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta",
  "theta", "iota", "kappa", "lambda", "mu", "nu", "xi",
  "omicron", "pi", "rho", "sigma", "tau", "upsilon", "phi",
  "chi", "psi", "omega" };

//! Add a new layer with unique name.
void Page::addLayer()
{
  for (int i = 0; i < int(sizeof(layerNames)/sizeof(const char *)); ++i) {
    if (findLayer(layerNames[i]) < 0) {
      addLayer(layerNames[i]);
      return;
    }
  }
  char name[20];
  int i = 1;
  for (;;) {
    std::sprintf(name, "alpha%d", i);
    if (findLayer(name) < 0) {
      addLayer(name);
      return;
    }
    i++;
  }
}

//! Moves the position of a layer in the layer list.
void Page::moveLayer(int index, int newIndex)
{
  assert(0 <= index && index < int(iLayers.size())
	 && 0 <= newIndex && newIndex < int(iLayers.size()));

  SLayer layer = iLayers[index];
  iLayers.erase(iLayers.begin() + index);
  iLayers.insert(iLayers.begin() + newIndex, layer);

  // Layer of object needs to be decreased by one if > index
  // Then increase by one if >= newIndex
  // If == index, then replace by newIndex
  for (ObjSeq::iterator it = iObjects.begin(); it != iObjects.end(); ++it) {
    int k = it->iLayer;
    if (k == index) {
      k = newIndex;
    } else {
      if (k > index)
	k -= 1;
      if (k >= newIndex)
	k += 1;
    }
    it->iLayer = k;
  }
}

//! Removes an empty layer from the page.
/*! All objects are adjusted.  Panics if there are objects in the
  deleted layer, of if it is the only layer.
*/
void Page::removeLayer(String name)
{
  int index = findLayer(name);
  assert(iLayers.size() > 1 && index >= 0);
  for (ObjSeq::iterator it = iObjects.begin(); it != iObjects.end(); ++it) {
    int k = it->iLayer;
    assert(k != index);
    if (k > index)
      it->iLayer = k-1;
  }
  iLayers.erase(iLayers.begin() + index);
}

//! Return number of objects in each layer
void Page::objectsPerLayer(std::vector<int> &objcounts) const
{
  objcounts.clear();
  for (int i = 0; i < countLayers(); ++i)
    objcounts.push_back(0);
  for (const auto & obj : iObjects)
    objcounts[obj.iLayer] += 1;
}

//! Rename a layer.
void Page::renameLayer(String oldName, String newName)
{
  int l = findLayer(oldName);
  if (l < 0)
    return;
  iLayers[l].iName = newName;
}

//! Returns a precise bounding box for the artwork on the page.
/*! This is meant to be used as the bounding box in PDF output.  It is
  computed by rendering all objects on the page that are visible in at
  least one view, plus all objects in a layer named "BBOX" (even if
  that is not visible), using a BBoxPainter. */
Rect Page::pageBBox(const Cascade *sheet) const
{
  // make table with all layers to be rendered
  std::vector<bool> layers;
  for (int l = 0; l < countLayers(); ++l) {
    bool vis = false;
    if (layer(l) == "BBOX")
      vis = true;
    else
      for (int vno = 0; !vis && vno < countViews(); ++vno)
	vis = visible(vno, l);
    layers.push_back(vis);
  }

  BBoxPainter bboxPainter(sheet);
  for (int i = 0; i < count(); ++i) {
    if (layers[layerOf(i)])
      object(i)->draw(bboxPainter);
  }
  return bboxPainter.bbox();
}

//! Returns a precise bounding box for the artwork in the view.
/*! This is meant to be used as the bounding box in PDF and EPS
  output.  It is computed by rendering all objects in the page using a
  BBoxPainter. */
Rect Page::viewBBox(const Cascade *sheet, int view) const
{
  BBoxPainter bboxPainter(sheet);
  for (int i = 0; i < count(); ++i) {
    if (objectVisible(view, i))
      object(i)->draw(bboxPainter);
  }
  return bboxPainter.bbox();
}

//! Snapping occurs if the layer is visible and has snapping enabled.
bool Page::objSnapsInView(int objNo, int view) const noexcept
{
  int layer = layerOf(objNo);
  switch (snapping(layer)) {
  case SnapMode::Visible:
    return visible(view, layer);
  case SnapMode::Always:
    return true;
  case SnapMode::Never:
  default:
    return false;
  }
}

// --------------------------------------------------------------------

//! Set effect of view.
/*! Panics if \a sym is not symbolic. */
void Page::setEffect(int index, Attribute sym)
{
  assert(sym.isSymbolic());
  iViews[index].iEffect = sym;
}

//! Set active layer of view.
void Page::setActive(int index, String layer)
{
  assert(findLayer(layer) >= 0);
  iViews[index].iActive = layer;
}

//! Set visibility of layer \a layer in view \a view.
void Page::setVisible(int view, String layer, bool vis)
{
  int index = findLayer(layer);
  assert(index >= 0);
  iLayers[index].iVisible[view] = vis;
}

//! Insert a new view at index \a i.
void Page::insertView(int i, String active)
{
  iViews.insert(iViews.begin() + i, SView());
  iViews[i].iActive = active;
  iViews[i].iMarked = false;
  for (int l = 0; l < countLayers(); ++l)
    iLayers[l].iVisible.insert(iLayers[l].iVisible.begin() + i, false);
}

//! Remove the view at index \a i.
void Page::removeView(int i)
{
  iViews.erase(iViews.begin() + i);
  for (int l = 0; l < countLayers(); ++l)
    iLayers[l].iVisible.erase(iLayers[l].iVisible.begin() + i);
}

//! Remove all views of this page.
void Page::clearViews()
{
  iViews.clear();
  for (LayerSeq::iterator it = iLayers.begin();
       it != iLayers.end(); ++it)
    it->iVisible.clear();
}

void Page::setMarkedView(int index, bool marked)
{
  iViews[index].iMarked = marked;
}

int Page::countMarkedViews() const
{
  int count = 0;
  for (int i = 0; i < countViews(); ++i) {
    if (markedView(i))
      ++count;
  }
  return (count == 0) ? 1 : count;
}

//! Return index of view with given number or name.
/*! Input numbers are one-based.
  Returns -1 if no such view exists.
*/
int Page::findView(String s) const
{
  if (s.empty())
    return -1;
  if ('0' <= s[0] && s[0] <= '9') {
    int no = Lex(s).getInt();
    if (no <= 0 || no > countViews())
      return -1;
    return no - 1;
  }
  for (int i = 0; i < countViews(); ++i) {
    if (s == viewName(i))
      return i;
    }
  return -1;
}

//! Return matrices for all layers.
std::vector<Matrix> Page::layerMatrices(int view) const
{
  std::vector<Matrix> m(countLayers());
  for (int i = 0; i < size(iViews[view].iLayerMatrices); ++i) {
    int l = findLayer(iViews[view].iLayerMatrices[i].iLayer);
    if (l >= 0)
      m[l] = iViews[view].iLayerMatrices[i].iMatrix;
  }
  return m;
}

//! Set matrix for the given layer in this view.
void Page::setLayerMatrix(int view, int layer, const Matrix &m)
{
  String name = iLayers[layer].iName;
  std::vector<SLayerMatrix> &ms = iViews[view].iLayerMatrices;
  std::vector<SLayerMatrix>::iterator it =
    std::find_if(ms.begin(), ms.end(), [&name](const SLayerMatrix &a) {return a.iLayer == name;});
  if (m.isIdentity()) {
    if (it != ms.end()) ms.erase(it);
  } else {
    if (it != ms.end())
      it->iMatrix = m;
    else
      ms.push_back({name, m});
  }
}

//! Set the attribute mapping for the view.
void Page::setViewMap(int index, const AttributeMap &map)
{
  iViews[index].iAttributeMap = map;
}

// --------------------------------------------------------------------

Page::SObject::SObject()
{
  iObject = nullptr;
  iLayer = 0;
  iSelect = ENotSelected;
}

Page::SObject::SObject(const SObject &rhs)
  : iSelect(rhs.iSelect), iLayer(rhs.iLayer)
{
  if (rhs.iObject)
    iObject = rhs.iObject->clone();
  else
    iObject = nullptr;
}

Page::SObject &Page::SObject::operator=(const SObject &rhs)
{
  if (this != &rhs) {
    delete iObject;
    iSelect = rhs.iSelect;
    iLayer = rhs.iLayer;
    if (rhs.iObject)
      iObject = rhs.iObject->clone();
    else
      iObject = nullptr;
    iBBox.clear(); // invalidate
  }
  return *this;
}

Page::SObject::~SObject()
{
  delete iObject;
}

// --------------------------------------------------------------------

//! Insert a new object at index \a i.
/*! Takes ownership of the object. */
void Page::insert(int i, TSelect select, int layer, Object *obj)
{
  iObjects.insert(iObjects.begin() + i, SObject());
  SObject &s = iObjects[i];
  s.iSelect = select;
  s.iLayer = layer;
  s.iObject = obj;
}

//! Append a new object.
/*! Takes ownership of the object. */
void Page::append(TSelect select, int layer, Object *obj)
{
  iObjects.push_back(SObject());
  SObject &s = iObjects.back();
  s.iSelect = select;
  s.iLayer = layer;
  s.iObject = obj;
}

//! Remove the object at index \a i.
void Page::remove(int i)
{
  iObjects.erase(iObjects.begin() + i);
}

//! Replace the object at index \a i.
/*! Takes ownership of \a obj. */
void Page::replace(int i, Object *obj)
{
  delete iObjects[i].iObject;
  iObjects[i].iObject = obj;
  invalidateBBox(i);
}

//! Return distance between object at index \a i and \a v.
/*! But may just return \a bound if the distance is larger.
  This function uses the cached bounded box, and is faster than
  calling Object::distance directly. */
double Page::distance(int i, const Vector &v, double bound) const
{
  if (bbox(i).certainClearance(v, bound))
    return bound;
  return object(i)->distance(v, Matrix(), bound);
}

//! Transform the object at index \a i.
/*! Use this function instead of calling Object::setMatrix directly,
  as the latter doesn't invalidate the cached bounding box. */
void Page::transform(int i, const Matrix &m)
{
  invalidateBBox(i);
  object(i)->setMatrix(m * object(i)->matrix());
}

//! Invalidate the bounding box at index \a i (the object is somehow changed).
void Page::invalidateBBox(int i) const
{
  iObjects[i].iBBox.clear();
}

//! Return a bounding box for the object at index \a i.
/*! This is a bounding box including the control points of the object.
  If you need a tight bounding box, you'll need to use the Object
  directly.

  The Page caches the box the first time it is computed.

  Make sure you call Page::transform instead of Object::setMatrix,
  as the latter would not invalidate the bounding box.
*/

Rect Page::bbox(int i) const
{
  if (iObjects[i].iBBox.isEmpty())
    iObjects[i].iObject->addToBBox(iObjects[i].iBBox, Matrix(), true);
  return iObjects[i].iBBox;
}

//! Compute possible vertex snapping position for object at index \a i.
/*! Looks only for positions closer than \a bound.
  If successful, modifies \a pos and \a bound. */
void Page::snapVtx(int i, const Vector &mouse,
		   Vector &pos, double &bound) const
{
  if (bbox(i).certainClearance(mouse, bound))
    return;
  object(i)->snapVtx(mouse, Matrix(), pos, bound);
}

//! Compute possible control point snapping position for object at index \a i.
/*! Looks only for positions closer than \a bound.
  If successful, modifies \a pos and \a bound. */
void Page::snapCtl(int i, const Vector &mouse,
		   Vector &pos, double &bound) const
{
  if (bbox(i).certainClearance(mouse, bound))
    return;
  object(i)->snapCtl(mouse, Matrix(), pos, bound);
}

//! Compute possible boundary snapping position for object at index \a i.
/*! Looks only for positions closer than \a bound.
  If successful, modifies \a pos and \a bound. */
void Page::snapBnd(int i, const Vector &mouse,
		   Vector &pos, double &bound) const
{
  if (bbox(i).certainClearance(mouse, bound))
    return;
  object(i)->snapBnd(mouse, Matrix(), pos, bound);
}

//! Set attribute \a prop of object at index \a i to \a value.
/*! This method automatically invalidates the bounding box if a
  ETextSize property is actually changed. */
bool Page::setAttribute(int i, Property prop, Attribute value)
{
  bool changed = object(i)->setAttribute(prop, value);
  if (changed && (prop == EPropTextSize || prop == EPropTransformations))
    invalidateBBox(i);
  return changed;
}

// --------------------------------------------------------------------

//! Return section title at \a level.
/*! Level 0 is the section, level 1 the subsection. */
String Page::section(int level) const
{
  if (iUseTitle[level])
    return title();
  else
    return iSection[level];
}

//! Set the section title at \a level.
/*! Level 0 is the section, level 1 the subsection.

  If \a useTitle is \c true, then \a name is ignored, and the section
  title will be copied from the page title (and further changes to the
  page title are automatically reflected). */
void Page::setSection(int level, bool useTitle, String name)
{
  iUseTitle[level] = useTitle;
  iSection[level] = useTitle ? String() : name;
}

//! Set the title of this page.
/*! An empty title is not displayed. */
void Page::setTitle(String title)
{
  iTitle = title;
  iTitleObject.setText(String("\\PageTitle{") + title + "}");
}

//! Return title of this page.
String Page::title() const
{
  return iTitle;
}

//! Set the notes of this page.
void Page::setNotes(String notes)
{
  iNotes = notes;
}

//! Set if page is marked for printing.
void Page::setMarked(bool marked)
{
  iMarked = marked;
}

//! Return Text object representing the title text.
/*! Return 0 if no title is set.
  Ownership of object remains with Page.
*/
const Text *Page::titleText() const
{
  if (title().empty())
    return nullptr;
  return &iTitleObject;
}

//! Apply styling to title text object.
void Page::applyTitleStyle(const Cascade *sheet)
{
  if (title().empty())
    return;
  const StyleSheet::TitleStyle *ts = sheet->findTitleStyle();
  if (!ts)
    return;
  iTitleObject.setMatrix(Matrix(ts->iPos));
  iTitleObject.setSize(ts->iSize);
  iTitleObject.setStroke(ts->iColor);
  iTitleObject.setHorizontalAlignment(ts->iHorizontalAlignment);
  iTitleObject.setVerticalAlignment(ts->iVerticalAlignment);
}

// --------------------------------------------------------------------

//! Return index of primary selection.
/*! Returns -1 if there is no primary selection. */
int Page::primarySelection() const
{
  for (int i = 0; i < count(); ++i)
    if (select(i) == EPrimarySelected)
      return i;
  return -1;
}

//! Returns true iff any object on the page is selected.
bool Page::hasSelection() const
{
  for (int i = 0; i < count(); ++i)
    if (select(i))
      return true;
  return false;
}

//! Deselect all objects.
void Page::deselectAll()
{
  for (int i = 0; i < count(); ++i)
    setSelect(i, ENotSelected);
}

/*! If no object is the primary selection, make the topmost secondary
  selection the primary one. */
void Page::ensurePrimarySelection()
{
  for (int i = 0; i < count(); ++i)
    if (select(i) == EPrimarySelected)
      return;
  for (int i = count() - 1; i >= 0; --i) {
    if (select(i) == ESecondarySelected) {
      setSelect(i, EPrimarySelected);
      return;
    }
  }
}

//! Copy whole page with bitmaps as <ipepage> into the stream.
void Page::saveAsIpePage(Stream &stream) const
{
  BitmapFinder bmFinder;
  bmFinder.scanPage(this);
  stream << "<ipepage>\n";
  int id = 1;
  for (std::vector<Bitmap>::const_iterator it = bmFinder.iBitmaps.begin();
       it != bmFinder.iBitmaps.end(); ++it) {
    Bitmap bm = *it;
    bm.saveAsXml(stream, id);
    bm.setObjNum(id);
    ++id;
  }
  saveAsXml(stream);
  stream << "</ipepage>\n";
}

//! Copy selected objects as <ipeselection> into the stream.
void Page::saveSelection(Stream &stream) const
{
  BitmapFinder bmFinder;
  for (int i = 0; i < count(); ++i) {
    if (select(i))
      object(i)->accept(bmFinder);
  }
  stream << "<ipeselection>\n";
  int id = 1;
  for (std::vector<Bitmap>::const_iterator it = bmFinder.iBitmaps.begin();
       it != bmFinder.iBitmaps.end(); ++it) {
    Bitmap bm = *it;
    bm.saveAsXml(stream, id);
    bm.setObjNum(id);
    ++id;
  }
  for (int i = 0; i < count(); ++i) {
    if (select(i))
      object(i)->saveAsXml(stream, layer(layerOf(i)));
  }
  stream << "</ipeselection>\n";
}

// --------------------------------------------------------------------
