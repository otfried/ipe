// --------------------------------------------------------------------
// timelabel.cpp
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

#include "timelabel_qt.h"

#include <QInputDialog>

TimeLabel::TimeLabel(QWidget* parent):
  QLabel(parent), time(0,0,0), counting(false), countingDown(false)
{
  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(countTime()));
  timer->start(1000);       // one second
}

void TimeLabel::countTime()
{
  if (!counting) return;

  if (countingDown && !(time.hour() == 0 && time.minute() == 0 && time.second() == 0))
    time = time.addSecs(-1);

  if (!countingDown)
    time = time.addSecs(1);

  setText(time.toString("hh:mm:ss"));
}

void TimeLabel::mouseDoubleClickEvent(QMouseEvent* event)
{
  setTime();
}

void TimeLabel::setTime()
{
  bool counting_state = counting;
  counting = false;

  bool ok;
  int minutes = QInputDialog::getInt(this, tr("Minutes"),
				     tr("Minutes to count down:"),
				     0, 0, 10000, 1, &ok);
  if (ok && minutes >= 0)
    time.setHMS(minutes/60,minutes%60,0);

  counting = counting_state;

  setText(time.toString("hh:mm:ss"));
}

void TimeLabel::resetTime()
{
  time.setHMS(0, 0, 0);
  setText(time.toString("hh:mm:ss"));
}

void TimeLabel::toggleCounting()
{
  counting = !counting;
}

void TimeLabel::toggleCountdown()
{
  countingDown = !countingDown;
}

// --------------------------------------------------------------------
