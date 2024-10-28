// -*- C++ -*-
// --------------------------------------------------------------------
// Wait dialog for QT
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

#ifndef WAITDLG_QT_H
#define WAITDLG_QT_H

#include "appui.h"

#include <QDialog>
#include <QMutex>

// --------------------------------------------------------------------

class Waiter : public QObject {
    Q_OBJECT

public:
    Waiter(const QString & cmd);

signals:
    void completed();
public slots:
    void process();

private:
    QString iCommand;
};

class WaitDialog : public QDialog {
    Q_OBJECT

public:
    WaitDialog(QString label, AppUiBase * observer);
    bool showDialog(); // returns true if dialog is showing now
    bool isRunning() const noexcept { return running; }
public slots:
    void completed();

protected:
    void keyPressEvent(QKeyEvent * e);
    void closeEvent(QCloseEvent * ev);

private:
    AppUiBase * observer;
    bool running; // the waiter has not yet signaled completed
    QMutex mutex; // locked when dialog is waiting modally
};

// --------------------------------------------------------------------
#endif
