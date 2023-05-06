// --------------------------------------------------------------------
// IpePresenter for Qt
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

#include "ipepresenter_qt.h"

#include "ipethumbs.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QInputDialog>
#include <QDesktopServices>
#include <QUrl>

using namespace ipe;

// --------------------------------------------------------------------

inline QString QIpe(const String &str)
{
  return QString::fromUtf8(str.z());
}

// --------------------------------------------------------------------

IpeAction::IpeAction(int cmd, const QString &text, const char *shortcut,
                     MainWindow *parent):
  QAction(text, parent), iCommand{cmd}
{
  if (shortcut)
    setShortcut(QKeySequence(shortcut));
  connect(this, &QAction::triggered, [parent,this] () { parent->cmd(iCommand); });
}

// --------------------------------------------------------------------

BeamerView::BeamerView(Qt::WindowFlags f) : QMainWindow(nullptr, f)
{
  iView = new PdfView(this);
  iView->setBackground(Color(0, 0, 0));
  setCentralWidget(iView);
}

// --------------------------------------------------------------------

MainWindow::MainWindow(BeamerView* bv, Qt::WindowFlags f) :
  QMainWindow(nullptr, f), iScreen(bv)
{
  QWidget *centralwidget = new QWidget(this);
  QHBoxLayout *horizontalLayout = new QHBoxLayout(centralwidget);

  QSplitter *splitV = new QSplitter(centralwidget);
  splitV->setOrientation(Qt::Horizontal);

  iCurrent = new PdfView(splitV);

  QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(iCurrent->sizePolicy().hasHeightForWidth());
  iCurrent->setSizePolicy(sizePolicy);
  iCurrent->setMinimumSize(QSize(600, 0));
  splitV->addWidget(iCurrent);

  QSplitter *splitH = new QSplitter(splitV);
  splitH->setOrientation(Qt::Vertical);

  QWidget *clockNotes = new QWidget(splitH);
  QVBoxLayout *clockNotesLayout = new QVBoxLayout(clockNotes);
  clockNotesLayout->setContentsMargins(0, 0, 0, 0);

  iClock = new TimeLabel(clockNotes);
  QFont clockFont;
  clockFont.setPointSize(28);
  iClock->setFont(clockFont);

  clockNotesLayout->addWidget(iClock);

  QLabel *notesLabel = new QLabel(clockNotes);
  clockNotesLayout->addWidget(notesLabel);

  iNotes = new QPlainTextEdit(clockNotes);
  iNotes->setReadOnly(true);
  QFont notesFont;
  notesFont.setFamily(QStringLiteral("Monospace"));
  iNotes->setFont(notesFont);

  clockNotesLayout->addWidget(iNotes);

  splitH->addWidget(clockNotes);

  QWidget *nextView = new QWidget(splitH);
  QVBoxLayout *nextLayout = new QVBoxLayout(nextView);
  nextLayout->setContentsMargins(0, 0, 0, 0);
  QLabel *nextLabel = new QLabel(nextView);
  QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
  sizePolicy1.setHorizontalStretch(0);
  sizePolicy1.setVerticalStretch(0);
  sizePolicy1.setHeightForWidth(nextLabel->sizePolicy().hasHeightForWidth());
  nextLabel->setSizePolicy(sizePolicy1);

  nextLayout->addWidget(nextLabel);

  iNext = new PdfView(nextView);
  QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
  sizePolicy2.setHorizontalStretch(0);
  sizePolicy2.setVerticalStretch(0);
  sizePolicy2.setHeightForWidth(iNext->sizePolicy().hasHeightForWidth());
  iNext->setSizePolicy(sizePolicy2);

  nextLayout->addWidget(iNext);

  splitH->addWidget(nextView);
  splitV->addWidget(splitH);

  horizontalLayout->addWidget(splitV);

  setCentralWidget(centralwidget);

  iClock->setText("00:00:00");
  notesLabel->setText("Notes:");
  nextLabel->setText("Next view:");

  QMenuBar *menubar = new QMenuBar(this);
  setMenuBar(menubar);

  iViewMenu = menuBar()->addMenu(tr("&View"));
  iTimeMenu = menuBar()->addMenu(tr("&Time"));
  iMoveMenu = menuBar()->addMenu(tr("&Navigate"));
  iHelpMenu = menuBar()->addMenu(tr("&Help"));

  iShowPresentationAction = new IpeAction(EShowPresentation, "Show presentation", "F5", this);
  iShowPresentationAction->setCheckable(true);
  iViewMenu->addAction(iShowPresentationAction);

  iFullScreenAction = new IpeAction(EFullScreen, "Full screen", "F11", this);
  iFullScreenAction->setCheckable(true);
  iViewMenu->addAction(iFullScreenAction);

  iBlackoutAction = new IpeAction(EBlackout, "Blackout", "B", this);
  iBlackoutAction->setCheckable(true);
  iViewMenu->addAction(iBlackoutAction);

  connect(iViewMenu, &QMenu::aboutToShow,
	  [this] () {
	    iShowPresentationAction->setChecked(iScreen->isVisible());
	    iFullScreenAction->setChecked((iScreen->windowState() & Qt::WindowFullScreen) != 0);
	    iBlackoutAction->setChecked(iScreen->pdfView()->blackout());
	  });

  iTimeMenu->addAction(new IpeAction(ESetTime, "Set time", "", this));
  iTimeMenu->addAction(new IpeAction(EResetTime, "Reset time", "R", this));

  IpeAction *countDown = new IpeAction(ETimeCountdown, "Count down", "/", this);
  countDown->setCheckable(true);
  iTimeMenu->addAction(countDown);
  IpeAction* countTime = new IpeAction(EToggleTimeCounting, "Count time", "T", this);
  countTime->setCheckable(true);
  iTimeMenu->addAction(countTime);

  IpeAction *next = new IpeAction(ENextView, "Next view", nullptr, this);
  IpeAction *prev = new IpeAction(EPreviousView, "Previous view", nullptr, this);
  QList<QKeySequence> nextKeys { QKeySequence("Right"), QKeySequence("Down"), QKeySequence("PgDown"),};
  QList<QKeySequence> prevKeys { QKeySequence("Left"), QKeySequence("Up"), QKeySequence("PgUp") };
  next->setShortcuts(nextKeys);
  prev->setShortcuts(prevKeys);
  iMoveMenu->addAction(next);
  iMoveMenu->addAction(prev);
  iMoveMenu->addAction(new IpeAction(ENextPage, "Next page", "N", this));
  iMoveMenu->addAction(new IpeAction(EPreviousPage, "Previous page", "P", this));
  iMoveMenu->addAction(new IpeAction(EFirstView, "First view", "Home", this));
  iMoveMenu->addAction(new IpeAction(ELastView, "Last view", "End", this));
  iMoveMenu->addAction(new IpeAction(EJumpTo, "Jump to...", "J", this));
  iMoveMenu->addAction(new IpeAction(ESelectPage, "Select page...", "S", this));

  iHelpMenu->addAction(new IpeAction(EAbout, "About IpePresenter", nullptr, this));

  connect(iScreen->pdfView(), &PdfView::sizeChanged,
	  [this] () { fitBox(mediaBox(-1), iScreen->pdfView()); });
  connect(iCurrent, &PdfView::sizeChanged,
	  [this] () { fitBox(mediaBox(-1), iCurrent); });
  connect(iNext, &PdfView::sizeChanged,
	  [this] () { fitBox(mediaBox(-2), iNext); });

  connect(iCurrent, &PdfView::mouseButton,
	  [this] (int button, const Vector &pos) {
	    const PdfDict *action = findLink(pos);
	    if (action) {
	      interpretAction(action);
	      setView();
	    } else
	      cmd(button);
	  });

  connect(iScreen->pdfView(), &PdfView::mouseButton, this, &MainWindow::cmd);
}

// --------------------------------------------------------------------

void MainWindow::cmd(int c)
{
  // ipeDebug("Command %d", c);
  switch (c) {
  case EOpen:
    break;
  case EQuit:
    QApplication::exit();
    break;
    //
  case EShowPresentation:
    if (iScreen->isVisible())
      iScreen->hide();
    else
      iScreen->show();
    break;
  case EFullScreen:
    iScreen->setWindowState(iScreen->windowState() ^ Qt::WindowFullScreen);
    break;
  case EBlackout:
    iScreen->pdfView()->setBlackout(!iScreen->pdfView()->blackout());
    iScreen->pdfView()->updatePdf();
    break;
    //
  case EToggleTimeCounting:
    iClock->toggleCounting();
    break;
  case ETimeCountdown:
    iClock->toggleCountdown();
    break;
  case ESetTime:
    iClock->setTime();
    break;
  case EResetTime:
    iClock->resetTime();
    break;
    //
  case ELeftMouse:
  case ENextView:
    nextView(+1);
    setView();
    break;
  case EOtherMouse:
  case EPreviousView:
    nextView(-1);
    setView();
    break;
  case ENextPage:
    nextPage(+1);
    setView();
    break;
  case EPreviousPage:
    nextPage(-1);
    setView();
    break;
  case EFirstView:
    firstView();
    setView();
    break;
  case ELastView:
    lastView();
    setView();
    break;
  case EJumpTo:
    jumpTo();
    break;
  case ESelectPage:
    selectPage();
    break;
  case EAbout:
    aboutIpePresenter();
    break;
  default:
    // unknown action
    return;
  }
}

// --------------------------------------------------------------------

bool MainWindow::load(const char* fn)
{
  bool result = Presenter::load(fn);
  if (result) {
    setPdf();
    setView();
  }
  return result;
}

void MainWindow::jumpTo()
{
  auto str = QInputDialog::getText(this, tr("Jump to page"), tr("Enter page label:"));
  if (!str.isEmpty()) {
    jumpToPage(String(str.trimmed().toUtf8()));
    setView();
  }
}

void MainWindow::setPdf()
{
  iScreen->pdfView()->setPdf(iPdf.get(), iFonts.get());
  iCurrent->setPdf(iPdf.get(), iFonts.get());
  iNext->setPdf(iPdf.get(), iFonts.get());
}

void MainWindow::setView()
{
  setViewPage(iScreen->pdfView(), iPdfPageNo);
  setViewPage(iCurrent, iPdfPageNo);
  setViewPage(iNext, iPdfPageNo < iPdf->countPages() - 1 ? iPdfPageNo + 1 : iPdfPageNo);

  setWindowTitle(QIpe(currentLabel()));
  iNotes->setPlainText(QIpe(iAnnotations[iPdfPageNo]));
}

void MainWindow::selectPage()
{
  constexpr int iconWidth = 250;
  std::vector<String> labels;
  for (int i = 0; i < iPdf->countPages(); ++i)
    labels.push_back(pageLabel(i));

  if (iPageIcons.empty()) {
    PdfThumbnail r(iPdf.get(), iconWidth);
    for (int i = 0; i < iPdf->countPages(); ++i) {
      Buffer b = r.render(iPdf->page(i));
      QImage bits((const uchar *) b.data(), r.width(), r.height(), QImage::Format_RGB32);
      // need to copy bits since buffer b is temporary
      iPageIcons.push_back(QPixmap::fromImage(bits.copy()));
    }
  }

  QDialog *d = new QDialog();
  d->setWindowTitle("IpePresenter: Select page");

  QLayout *lo = new QVBoxLayout;

  PageSelector *p = new PageSelector(d);
  p->fill(iPageIcons, labels);
  p->setCurrentRow(iPdfPageNo);

  lo->addWidget(p);
  d->setLayout(lo);

  QWidget::connect(p, SIGNAL(selectionMade()), d, SLOT(accept()));

  d->setWindowState(Qt::WindowMaximized);

  int result = d->exec();
  int sel = p->selectedIndex();
  delete d;

  if (result == QDialog::Accepted) {
    iPdfPageNo = sel;
    setView();
  }
}

void MainWindow::browseLaunch(bool launch, String dest)
{
  QDesktopServices::openUrl(QUrl(dest.z()));
}

// --------------------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent *event)
{
  iScreen->close();
  QMainWindow::closeEvent(event);
}

// --------------------------------------------------------------------

static const char * const aboutText =
  "<qt><h1>IpePresenter %d.%d.%d</h1>"
  "<p>Copyright (c) 2020-2023 Otfried Cheong</p>"
  "<p>A presentation tool for giving PDF presentations "
  "created in Ipe or using beamer.</p>"
  "<p>Originally invented by Dmitriy Morozov, "
  "IpePresenter is now developed together with Ipe and released under the GNU Public License.</p>"
  "<p>See the <a href=\"http://ipepresenter.otfried.org\">IpePresenter homepage</a>"
  " for further information.</p>"
  "<p>If you are an IpePresenter fan and want to show others, have a look at the "
  "<a href=\"https://www.shirtee.com/en/store/ipe\">Ipe T-shirts</a>.</p>"
  "<h3>Platinum and gold sponsors</h3>"
  "<ul><li>Hee-Kap Ahn</li>"
  "<li>Günter Rote</li>"
  "<li>SCALGO</li>"
  "<li>Martin Ziegler</li></ul>"
  "<p>If you enjoy IpePresenter, feel free to treat the author on a cup of coffee at "
  "<a href=\"https://ko-fi.com/ipe7author\">Ko-fi</a>.</p>"
  "<p>You can also become a member of the exclusive community of "
  "<a href=\"http://patreon.com/otfried\">Ipe patrons</a>. "
  "For the price of a cup of coffee per month you can make a meaningful contribution "
  "to the continuing development of IpePresenter and Ipe.</p>"
  "</qt>";

void MainWindow::aboutIpePresenter()
{
  std::vector<char> buf(strlen(aboutText) + 100);
  sprintf(buf.data(), aboutText,
	  IPELIB_VERSION / 10000,
	  (IPELIB_VERSION / 100) % 100,
	  IPELIB_VERSION % 100);

  QMessageBox msgBox(this);
  msgBox.setWindowTitle("About IpePresenter");
  msgBox.setInformativeText(buf.data());
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
}

void MainWindow::showType3Warning(const char *s)
{
  QMessageBox msgBox(this);
  msgBox.setWindowTitle("Type3 font detected");
  msgBox.setInformativeText(s);
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
}

// --------------------------------------------------------------------

static void usage()
{
  fprintf(stderr, "Usage: ipepresenter <filename>\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  Platform::initLib(IPELIB_VERSION);
  QApplication a(argc, argv);

  if (argc != 2)
    usage();

  const char *load = argv[1];

  BeamerView *bv = new BeamerView();
  MainWindow *mw = new MainWindow(bv);

  if (!mw->load(load))
    exit(2);

  mw->show();

  QObject::connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
  return a.exec();
}

// --------------------------------------------------------------------
