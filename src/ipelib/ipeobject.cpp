// --------------------------------------------------------------------
// The Ipe object type
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2023 Otfried Cheong

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

/*! \mainpage The Ipe library documentation

   The Ipe library ("Ipelib") provides the geometric primitives and
   implements all the geometric objects that appear in Ipe.  Many
   tasks related to modifying an Ipe document are actually performed
   by Ipelib.  For instance, the ipetoipe program consists of only a
   few calls to Ipelib.

   Ipelib can easily be used by C++ programs to read, write, and
   modify Ipe documents. Compiling Ipelib is easy, it requires only
   the standard C++ library (including the STL), and the zlib
   compression library.  Nearly all symbols in Ipelib are in the \ref
   ipe "ipe" namespace, those that aren't start with the letters
   "Ipe".

   Before using Ipelib in your own program, make sure to initialize
   the library by calling ipe::Platform::initLib().

   Many of the Ipelib classes are also made available as Lua objects.
   \subpage lua describes the Lua bindings to Ipelib, to ipeui, and to
   the Ipe program itself.  The Ipe program itself is mostly written
   in Lua and uses these Lua bindings.

   On Unix, all filenames passed to Ipelib are assumed to be in the
   local system's encoding.  On Windows, all filenames passed to
   Ipelib are assumed to be UTF-8.  All Lua strings are assumed to be
   UTF-8 (filenames are converted by the ipelua bindings).

   \subpage ipelets explains how to write ipelets, that is, extensions
   to Ipe.  Ipelets are either written in Lua or in C++ (using a small
   Lua wrapper to describe the ipelet).  C++ ipelets have to be linked
   with Ipelib to access and modify Ipe objects.

   The classes documented here are implemented in five different libraries:

   \li \e libipe is the core Ipelib library.  It implements geometric
   primitives, Ipe objects, and the Ipe document.  However, it doesn't
   know anything about drawing to the screen or about the Lua
   bindings.
   \li \e libipecairo implements the \ref cairo "Ipe cairo module".
   It provides drawing of Ipe objects using the Cairo library.
   \li \e libipelua implements the \ref lua "Lua bindings" for Ipelib.
   If installed properly, it can be loaded dynamically from Lua using
   \c require. It is also used by ipescript.
   \li \e libipecanvas implements the \ref qtcanvas "Ipe canvas module".
   It provides a widget for displaying and editing Ipe objects.
   \li \e libipeui implements Lua bindings for user interfaces. This
   library does not depend on any other Ipe component, and can be used
   for other Lua projects.

   Here is an annotated list of the modules:

   \li \ref base : Some basic datatypes: ipe::String, ipe::Buffer,
   ipe::Stream, ipe::Fixed
   \li \ref geo :  Geometric types and linear algebra: ipe::Vector,
   ipe::Matrix, ipe::Line, ipe::Segment, ipe::Arc, ipe::Bezier, ipe::Shape.
   \li \ref attr : Attributes such as ipe::Color, ipe::Kind, ipe::Attribute
   \li \ref obj : The five ipe::Object types: ipe::Group, ipe::Path,
   ipe::Text, ipe::Image, and ipe::Reference
   \li \ref doc : The Ipe document: ipe::Document, ipe::Page, and
   ipe::StyleSheet
   \li \ref high : Some utility classes: ipe::ImlParser, ipe::BitmapFinder, etc.
   \li \ref ipelet : The ipe::Ipelet interface
   \li \ref cairo : Classes to draw Ipe objects on a Cairo surface:
   ipe::CairoPainter, ipe::Fonts, ipe::Face
   \li \ref canvas : A widget ipe::Canvas to display Ipe objects,
   and tools for this canvas: ipe::PanTool, ipe::SelectTool,
   ipe::TransformTool.

   Finally, here is list of the pages describing Lua bindings:

   \li \ref luageo Lua bindings for basic geometric objects
   \li \ref luaobj Lua bindings for Ipe objects
   \li \ref luapage Lua bindings for documents, pages, and stylesheets
   \li \ref luaipeui Lua bindings for dialogs, menus, etc.
   \li \ref luaipe Lua bindings for the Ipe program itself.
*/

/* namespace ipe
  brief Ipe library namespace

  Nearly all symbols defined by the Ipe library and the Ipe-Cairo
  interface are in the namespace ipe.  (Other symbols all start with
  the letters "Ipe" (or "ipe" or "IPE").
*/

// --------------------------------------------------------------------

/*! \defgroup obj Ipe Objects
  \brief The Ipe object model.

  This module deals with the actual objects inside an Ipe document.
  All Ipe objects are derived from Object.

*/

#include "ipegeo.h"
#include "ipeobject.h"
#include "ipepainter.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Object
  \ingroup obj
  \brief Base class for all Ipe objects, composite or leaf.

  All objects are derived from this class.  It provides functionality
  common to all objects, and carries the standard attributes.

  All Object's provide a constant time copy constructor (and a virtual
  Object::clone() method).  Objects of non-constant size realize this
  by separating the implementation and using reference counting.  In
  particular, copying a composite object does not create new copies of
  the components.

  Object has only three attributes: the transformation matrix, the
  pinning status, and the allowed transformations.

  If an object is pinned, it cannot be moved at all (or only in the
  non-pinned direction) from the Ipe user interface.

  Restricting the allowed transformations works somewhat differently:
  It doesn't stop transformations being applied to the object, but
  they only effect the position of the reference point (the origin of
  the object coordinate system), and (if transformations() ==
  ETransformationsRigidMotions) the orientation of the object
  coordinate system.
*/

//! Construct from XML stream.
Object::Object(const XmlAttributes &attr)
{
  String str;
  if (attr.has("matrix", str))
    iMatrix = Matrix(str);
  iPinned = ENoPin;
  if (attr.has("pin", str)) {
    if (str == "yes")
      iPinned = EFixedPin;
    else if (str == "h")
      iPinned = EHorizontalPin;
    else if (str == "v")
      iPinned = EVerticalPin;
  }
  iTransformations = ETransformationsAffine;
  if (attr.has("transformations", str) && !str.empty()) {
    if (str == "rigid")
      iTransformations = ETransformationsRigidMotions;
    else if (str == "translations")
      iTransformations = ETransformationsTranslations;
  }
  iCustom = Attribute::UNDEFINED();
  if (attr.has("custom", str) && !str.empty())
    iCustom = Attribute(false, str);
}

/*! Create object by taking pinning/transforming from \a attr and
  setting identity matrix. */
Object::Object(const AllAttributes &attr)
{
  iPinned = attr.iPinned;
  iTransformations = attr.iTransformations;
  iCustom = Attribute::UNDEFINED();
}

/*! Create object with identity matrix, no pinning, all transformations. */
Object::Object()
{
  iPinned = ENoPin;
  iTransformations = ETransformationsAffine;
  iCustom = Attribute::UNDEFINED();
}

//! Copy constructor.
Object::Object(const Object &rhs)
{
  iMatrix = rhs.iMatrix;
  iPinned = rhs.iPinned;
  iTransformations = rhs.iTransformations;
  iCustom = rhs.iCustom;
}

//! Pure virtual destructor.
Object::~Object()
{
  // nothing
}

//! Write layer, pin, transformations, matrix to XML stream.
void Object::saveAttributesAsXml(Stream &stream, String layer) const
{
  if (!layer.empty())
    stream << " layer=\"" << layer << "\"";
  if (!iMatrix.isIdentity())
    stream << " matrix=\"" << iMatrix << "\"";
  switch (iPinned) {
  case EFixedPin:
    stream << " pin=\"yes\"";
    break;
  case EHorizontalPin:
    stream << " pin=\"h\"";
    break;
  case EVerticalPin:
    stream << " pin=\"v\"";
    break;
  case ENoPin:
  default:
    break;
  }
  if (iTransformations == ETransformationsTranslations)
    stream << " transformations=\"translations\"";
  else if (iTransformations == ETransformationsRigidMotions)
    stream << " transformations=\"rigid\"";
  if (iCustom != Attribute::UNDEFINED())
    stream << " custom=\"" << iCustom.string() << "\"";
}

//! Return pointer to this object if it is an Group, nullptr otherwise.
Group *Object::asGroup()
{
  return nullptr;
}

//! Return pointer to this object if it is an Group, nullptr otherwise.
const Group *Object::asGroup() const
{
  return nullptr;
}

//! Return pointer to this object if it is an Text, nullptr otherwise.
Text *Object::asText()
{
  return nullptr;
}

//! Return pointer to this object if it is an Path, nullptr otherwise.
Path *Object::asPath()
{
  return nullptr;
}

//! Return pointer to this object if it is an Image , nullptr otherwise.
Image *Object::asImage()
{
  return nullptr;
}

//! Return pointer to this object if it is an Ref, nullptr otherwise.
Reference *Object::asReference()
{
  return nullptr;
}

// --------------------------------------------------------------------

//! Set the transformation matrix.
/*! Don't use this on an Object in a Page, because it wouldn't
  invalidate its bounding box.  Call Page::transform instead. */
void Object::setMatrix(const Matrix &matrix)
{
  iMatrix = matrix;
}

//! Return pinning mode of the object.
TPinned Object::pinned() const
{
  return iPinned;
}

//! Set pinning mode of the object.
void Object::setPinned(TPinned pin)
{
  iPinned = pin;
}

//! Set allowed transformations of the object.
void Object::setTransformations(TTransformations trans)
{
  iTransformations = trans;
}

//! Set an attribute on this object.
/*! Returns true if an attribute was actually changed.  */
bool Object::setAttribute(Property prop, Attribute value)
{
  switch (prop) {
  case EPropPinned:
    assert(value.isEnum());
    if (value.pinned() != iPinned) {
      iPinned = value.pinned();
      return true;
    }
    break;
  case EPropTransformations:
    assert(value.isEnum());
    if (value.transformations() != iTransformations) {
      iTransformations = value.transformations();
      return true;
    }
    break;
  default:
    break;
  }
  return false;
}

//! Get setting of an attribute of this object.
/*! If object does not have this attribute, returnes "undefined"
  attribute. */
Attribute Object::getAttribute(Property prop) const noexcept
{
  switch (prop) {
  case EPropPinned:
    return Attribute(pinned());
  case EPropTransformations:
    return Attribute(iTransformations);
  default:
    return Attribute::UNDEFINED();
  }
}

//! Set the 'custom' attribute (not used by Ipe, for users and ipelets)
void Object::setCustom(Attribute value)
{
  assert(value.isString());
  iCustom = value;
}

//! Return value of the 'custom' attribute
Attribute Object::getCustom() const noexcept
{
  return iCustom;
}

// --------------------------------------------------------------------

//! Check all symbolic attributes.
void Object::checkStyle(const Cascade *, AttributeSeq &) const
{
  // nothing
}

/*! Check whether attribute \a is either absolute or defined in the
  style sheet cascade \a sheet.  Add \a attr to \a seq if this is not
  the case. */
void Object::checkSymbol(Kind kind, Attribute attr, const Cascade *sheet,
			 AttributeSeq &seq)
{
  if (attr.isSymbolic() && sheet->findDefinition(kind, attr) < 0) {
    AttributeSeq::const_iterator it =
      std::find(seq.begin(), seq.end(), attr);
    if (it == seq.end())
      seq.push_back(attr);
  }
}

//! Compute vertex snapping position for transformed object.
/*! Looks only for positions closer than \a bound.
  If successful, modify \a pos and \a bound.
  The default implementation does nothing.
*/
void Object::snapVtx(const Vector &mouse, const Matrix &m,
		     Vector &pos, double &bound) const
{
  // nothing
}

//! Compute control point snapping position for transformed object.
/*! Looks only for positions closer than \a bound.
  If successful, modify \a pos and \a bound.
  The default implementation does nothing.
*/
void Object::snapCtl(const Vector &mouse, const Matrix &m,
		     Vector &pos, double &bound) const
{
  // nothing
}

//! Compute boundary snapping position for transformed object.
/*! Looks only for positions closer than \a bound.
  If successful, modify \a pos and \a bound.
  The default implementation does nothing.
*/
void Object::snapBnd(const Vector &/* mouse */, const Matrix &/* m */,
		     Vector &/* pos */, double &/* bound */) const
{
  // nothing
}

// --------------------------------------------------------------------

/*! \class ipe::Visitor
  \ingroup high
  \brief Base class for visitors to Object.

  Many operations on Ipe Objects are implemented as visitors, all
  derived from Visitor.

  The default implementation of each visitXXX member does nothing.
*/

//! Pure virtual destructor.
Visitor::~Visitor()
{
  // void
}

//! Called on an Group object.
void Visitor::visitGroup(const Group *)
{
  // nothing
}

//! Called on an Path object.
void Visitor::visitPath(const Path *)
{
  // nothing
}

//! Called on an Image object.
void Visitor::visitImage(const Image * )
{
  // nothing
}

//! Called on an Text object.
void Visitor::visitText(const Text * )
{
  // nothing
}

//! Called on an Reference object.
void Visitor::visitReference(const Reference * )
{
  // nothing
}

// --------------------------------------------------------------------
