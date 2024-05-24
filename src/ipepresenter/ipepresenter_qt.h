// -*- C++ -*-
// --------------------------------------------------------------------
// IpePresenter for Qt
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

#ifndef IPEPRESENTER_QT_H
#define IPEPRESENTER_QT_H

#include "ipeselector_qt.h"
#include "ipepdfview_qt.h"
#include "timelabel_qt.h"
#include "ipepresenter.h"

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QAction>
#include <QTimer>
#include <QTime>

// --------------------------------------------------------------------

class MainWindow;

class IpeAction: public QAction
{
  Q_OBJECT
public:
  IpeAction(int cmd, const QString &text,
	    const char *shortcut, MainWindow *parent);
private:
  int iCommand;
};

// --------------------------------------------------------------------

class BeamerView: public QMainWindow
{
  Q_OBJECT
public:
  BeamerView(Qt::WindowFlags f = Qt::Widget);
  PdfView *pdfView() { return iView; }
private:
  PdfView *iView;
};

// --------------------------------------------------------------------

class MainWindow: public QMainWindow, public Presenter
{
  Q_OBJECT
public:
  MainWindow(BeamerView* bv, Qt::WindowFlags f = Qt::Widget);
  bool load(const char* fn);
  void cmd(int c);
  void aboutIpePresenter();

private:
  void closeEvent(QCloseEvent *event);
  void setPdf();
  void setView();
  void jumpTo();
  void selectPage();
  void showType3Warning(const char *s);
  void browseLaunch(bool launch, String dest);

private:
  QMenu *iViewMenu;
  QMenu *iTimeMenu;
  QMenu *iMoveMenu;
  QMenu *iHelpMenu;
  IpeAction *iShowPresentationAction;
  IpeAction *iFullScreenAction;
  IpeAction *iBlackoutAction;
  PdfView *iCurrent;
  PdfView *iNext;
  BeamerView *iScreen;
  QPlainTextEdit *iNotes;
  TimeLabel *iClock;

  std::vector<QPixmap> iPageIcons;
};

// --------------------------------------------------------------------
#endif
