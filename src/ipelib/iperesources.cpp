// --------------------------------------------------------------------
// PDF Resources
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

#include "iperesources.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::PdfResourceBase
 * \ingroup base
 * \brief Base class providing access to PDF objects.
 */

PdfResourceBase::PdfResourceBase() : iPageResources(new PdfDict)
{
  // nothing
}

PdfResourceBase::~PdfResourceBase()
{
  // nothing
}

const PdfObj *PdfResourceBase::getDeep(const PdfDict *d, String key) const noexcept
{
  if (!d)
    return nullptr;
  const PdfObj *obj = d->get(key);
  if (obj && obj->ref())
    obj = object(obj->ref()->value());
  return obj;
}

const PdfDict *PdfResourceBase::getDict(const PdfDict *d, String key) const noexcept
{
  const PdfObj *obj = getDeep(d, key);
  if (obj)
    return obj->dict();
  return nullptr;
}

const PdfDict *PdfResourceBase::resourcesOfKind(String kind) const noexcept
{
  const PdfObj *obj = iPageResources->get(kind);
  if (!obj)
    return nullptr;
  else
    return obj->dict();
}

const PdfDict *PdfResourceBase::findResource(String kind, String name) const noexcept
{
  return getDict(resourcesOfKind(kind), name);
}

const PdfDict *PdfResourceBase::findResource(const PdfDict *xf, String kind,
					     String name) const noexcept
{
  const PdfDict *res = getDict(xf, "Resources");
  const PdfDict *kindd = getDict(res, kind);
  return getDict(kindd, name);
}

// --------------------------------------------------------------------

/*! \class ipe::PdfFileResource
 * \ingroup base
 * \brief PDF resources directly from a PdfFile.
 */

PdfFileResources::PdfFileResources(const PdfFile *file) : iPdf(file)
{
  // nothing
}

const PdfObj *PdfFileResources::object(int num) const noexcept
{
  return iPdf->object(num);
}

// --------------------------------------------------------------------

/*! \class ipe::PdfResources
 * \ingroup base
 * \brief All the resources needed by the text objects in the document.
 */

PdfResources::PdfResources()
{
  // nothing
}

bool PdfResources::isIpeXForm(int num) const
{
  return ipeXForms.find(num) != ipeXForms.end();
}

void PdfResources::setIpeXForm(int num)
{
  ipeXForms.insert(num);
}

void PdfResources::add(int num, PdfFile *file)
{
  if (object(num)) // already present
    return;
  std::unique_ptr<const PdfObj> obj = file->take(num);
  if (!obj)
    return;  // no such object
  const PdfObj *q = obj.get();
  iObjects[num] = std::move(obj);
  addIndirect(q, file);
  iEmbedSequence.push_back(num); // after all its dependencies!
}

void PdfResources::addIndirect(const PdfObj *q, PdfFile *file)
{
  if (q->array()) {
    const PdfArray *arr = q->array();
    for (int i = 0; i < arr->count(); ++i)
      addIndirect(arr->obj(i, nullptr), file);
  } else if (q->dict()) {
    const PdfDict *dict = q->dict();
    for (int i = 0; i < dict->count(); ++i)
      addIndirect(dict->value(i), file);
  } else if (q->ref())
    add(q->ref()->value(), file);
}

const PdfObj *PdfResources::object(int num) const noexcept
{
  auto got = iObjects.find(num);
  if (got != iObjects.end())
    return got->second.get();
  else
    return nullptr;
}

const PdfDict *PdfResources::baseResources() const noexcept
{
  return iPageResources.get();
}

//! Collect (recursively) all the given resources (of the one latex page).
//! Takes ownership of all the scanned objects.
bool PdfResources::collect(const PdfDict *resd, PdfFile *file)
{
  /* A resource is a dictionary, like this:
    /Font << /F8 9 0 R /F10 18 0 R >>
    /ProcSet [ /PDF /Text ]
  */
  for (int i = 0; i < resd->count(); ++i) {
    String key = resd->key(i);
    if (key == "Ipe" || key == "ProcSet")
      continue;
    const PdfObj *obj = resd->get(key, file);
    const PdfDict *rd = obj->dict();
    if (!rd) {
      ipeDebug("Resource %s is not a dictionary", key.z());
      return false;
    }
    PdfDict *d = new PdfDict;
    for (int j = 0; j < rd->count(); ++j) {
      if (!addToResource(d, rd->key(j), rd->value(j), file))
	return false;
    }
    iPageResources->add(key, d);
  }
  return true;
}

bool PdfResources::addToResource(PdfDict *d, String key,
				 const PdfObj *el, PdfFile *file)
{
  if (el->name())
    d->add(key, new PdfName(el->name()->value()));
  else if (el->number())
    d->add(key, new PdfNumber(el->number()->value()));
  else if (el->ref()) {
    int ref = el->ref()->value();
    d->add(key, new PdfRef(ref));
    add(ref, file);             // take all dependencies from file
  } else if (el->array()) {
    PdfArray *a = new PdfArray;
    for (int i = 0; i < el->array()->count(); ++i) {
      const PdfObj *al = el->array()->obj(i, nullptr);
      if (al->name())
	a->append(new PdfName(al->name()->value()));
      else if (al->number())
	a->append(new PdfNumber(al->number()->value()));
      else {
	ipeDebug("Surprising type in resource: %s", el->repr().z());
	return false;
      }
    }
    d->add(key, a);
  } else if (el->dict()) {
    const PdfDict *eld = el->dict();
    PdfDict *d1 = new PdfDict;
    for (int i = 0; i < eld->count(); ++i) {
      if (!addToResource(d1, eld->key(i), eld->value(i), file))
	return false;
    }
    d->add(key, d1);
  }
  return true;
}

void PdfResources::show() const noexcept
{
  String s;
  StringStream ss(s);
  ss << "Resources:  " << iPageResources->repr() << "\n";
  ss << "Ipe XForms: ";
  for (int num : ipeXForms)
    ss << num << " ";
  ss << "\n";
  ipeDebug("%s", s.z());
}

// --------------------------------------------------------------------

void PdfResources::addPageNumber(SPageNumber &pn) noexcept
{
  iPageNumbers.emplace_back(std::move(pn));
}

const Text *PdfResources::pageNumber(int page, int view) const noexcept
{
  auto it = std::find_if(iPageNumbers.begin(), iPageNumbers.end(),
			 [page, view](const SPageNumber &pn)
			 { return pn.page == page && pn.view == view; } );
  if (it == iPageNumbers.end())
    return nullptr;
  else
    return it->text.get();
}

// --------------------------------------------------------------------
