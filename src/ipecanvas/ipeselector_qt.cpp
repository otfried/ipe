// --------------------------------------------------------------------
// PageSelector for QT
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

#include "ipeselector_qt.h"

#include "ipethumbs.h"

#include "ipecanvas.h"

#include <QDialog>
#include <QVBoxLayout>

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::PageSelector
  \ingroup canvas
  \brief A Qt widget that displays a list of Ipe pages.
*/

//! Construct the widget.
/*! If \a page is negative, the last view of each page is shown.
  Otherwise, all views of this page are shown.
  \a itemWidth is the width of the page thumbnails (the height
  is computed automatically).
*/

PageSelector::PageSelector(QWidget *parent)
  : QListWidget(parent)
{
  setViewMode(QListView::IconMode);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setResizeMode(QListView::Adjust);
  setWrapping(true);
  setUniformItemSizes(true);
  setFlow(QListView::LeftToRight);
  setSpacing(10);
  setMovement(QListView::Static);

  connect(this, SIGNAL(itemActivated(QListWidgetItem *)),
	  SLOT(pageSelected(QListWidgetItem *)));
}

void PageSelector::pageSelected(QListWidgetItem *item)
{
  emit selectionMade();
}

void PageSelector::fill(std::vector<QPixmap> &icons, std::vector<String> &labels)
{
  int maxWidth = 0;
  int maxHeight = 0;
  for (const auto & icon : icons) {
    if (icon.width() > maxWidth)
      maxWidth = icon.width();
    if (icon.height() > maxHeight)
      maxHeight = icon.height();
  }
  setGridSize(QSize(maxWidth + 10, maxHeight + 50));
  setIconSize(QSize(maxWidth, maxHeight));

  for (size_t i = 0; i < icons.size(); ++i) {
    QString s = QString::fromUtf8(labels[i].z());
    QListWidgetItem *item = new QListWidgetItem(QIcon(icons[i]), s);
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setToolTip(s);
    addItem(item);
  }
}

// --------------------------------------------------------------------

static void fillWithPages(PageSelector *sel, Document *doc, int page, int itemWidth)
{
  Thumbnail r(doc, itemWidth);
  std::vector<QPixmap> icons;
  std::vector<String> labels;
  if (page >= 0) {
    Page *p = doc->page(page);
    for (int i = 0; i < p->countViews(); ++i) {
      Buffer b = r.render(p, i);
      QImage bits((const uchar *) b.data(), itemWidth, r.height(),
		  QImage::Format_RGB32);
      // need to copy bits since buffer b is temporary
      icons.push_back(QPixmap::fromImage(bits.copy()));
      String s;
      StringStream ss(s);
      if (!p->viewName(i).empty())
	ss << i+1 << ": " << p->viewName(i);
      else
	ss << "View " << i+1;
      labels.push_back(s);
    }
  } else {
    for (int i = 0; i < doc->countPages(); ++i) {
      Page *p = doc->page(i);
      Buffer b = r.render(p, p->countViews() - 1);
      QImage bits((const uchar *) b.data(), itemWidth, r.height(),
		  QImage::Format_RGB32);
      // need to copy bits since buffer b is temporary
      icons.push_back(QPixmap::fromImage(bits.copy()));

      String s;
      StringStream ss(s);
      if (!p->title().empty())
	ss << i+1 << ": " << p->title();
      else
	ss << "Page " << i+1;
      labels.push_back(s);
    }
  }
  sel->fill(icons, labels);
}

// --------------------------------------------------------------------

//! Show dialog to select a page or a view.
/*! If \a page is negative (the default), shows thumbnails of all
    pages of the document in a dialog.  If the user selects a page,
    the page number is returned. If the dialog is canceled, -1 is
    returned.

    If \a page is non-negative, all views of this page are shown, and
    the selected view number is returned. */
int CanvasBase::selectPageOrView(Document *doc, int page, int startIndex,
				 int pageWidth, int width, int height)
{
  QDialog *d = new QDialog();
  d->setWindowTitle((page >= 0) ? "Ipe: Select view" :
		    "Ipe: Select page");

  QLayout *lo = new QVBoxLayout;
  PageSelector *p = new PageSelector(d);
  fillWithPages(p, doc, page, pageWidth);

  lo->addWidget(p);
  d->setLayout(lo);

  QWidget::connect(p, SIGNAL(selectionMade()), d, SLOT(accept()));

  d->resize(width, height);
  p->setCurrentRow(startIndex);
  int result = d->exec();
  int sel = p->selectedIndex();
  delete d;

  return (result == QDialog::Rejected) ? -1 : sel;
}

// --------------------------------------------------------------------
