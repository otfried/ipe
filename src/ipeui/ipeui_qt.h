// -*- C++ -*-
// --------------------------------------------------------------------
// A UI toolkit for Lua, QT version
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

#ifndef IPEUI_QT_H
#define IPEUI_QT_H

#include "ipeui_common.h"

#include <QDialog>
#include <QGridLayout>
#include <QAction>
#include <QMenu>
#include <QThread>

class QTimer;
class QTextEdit;

// --------------------------------------------------------------------

class MenuAction : public QAction {
  Q_OBJECT
public:
  MenuAction(const QString &name, int number,
	     const QString &item, const QString &text,
	     QWidget *parent);
  QString name() const { return iName; }
  QString itemName() const { return iItemName; }
  int number() const { return iNumber; }

private:
  QString iName;
  QString iItemName;
  int iNumber;
};

class PMenu : public Menu {
public:
  PMenu(WINID parent);
  virtual ~PMenu();
  virtual int add(lua_State *L);
  virtual int execute(lua_State *L);
private:
  QMenu *iMenu;
};

// --------------------------------------------------------------------

class PTimer : public QObject, public Timer {
  Q_OBJECT
public:
  PTimer(lua_State *L0, int lua_object, const char *method);
  virtual ~PTimer();

  virtual int setInterval(lua_State *L);
  virtual int active(lua_State *L);
  virtual int start(lua_State *L);
  virtual int stop(lua_State *L);
private:
  QTimer *iTimer;
};

// --------------------------------------------------------------------

class PDialog : public QDialog, public Dialog {
  Q_OBJECT

public:
  PDialog(lua_State *L0, WINID parent, const char *caption, const char * language);
  ~PDialog();

  QGridLayout *gridlayout() { return iGrid; }

protected:
  virtual void setMapped(lua_State *L, int idx);
  virtual bool buildAndRun(int w, int h);
  virtual void retrieveValues();
  virtual void enableItem(int idx, bool value);
  virtual void acceptDialog(lua_State *L);

protected:
  virtual void keyPressEvent(QKeyEvent *e);

private:
  std::vector<QWidget *> iWidgets;
  QGridLayout *iGrid;
  QHBoxLayout *iButtonArea;
};

// --------------------------------------------------------------------

class EditorThread : public QThread
{
  Q_OBJECT

public:
  EditorThread(lua_State *L, const QString &cmd);

signals:
  void done();
protected:
  void run() override;
private:
  lua_State *L;
  QString iCommand;
};

// --------------------------------------------------------------------
#endif
