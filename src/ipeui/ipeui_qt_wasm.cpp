// --------------------------------------------------------------------
// Special Lua bindings for Qt dialogs on ipe-web
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

#include "ipeui_qt.h"
#include <emscripten/bind.h>

#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTextStream>
#include <QMessageBox>
#include <QThread>
#include <QSaveFile>

#include <utility>

class CancellableWaitDialog : public WaitDialog {
        Q_OBJECT

    public:
        CancellableWaitDialog(QString label, QWidget *parent = nullptr) : WaitDialog(std::move(label), parent) {
        }

    protected:
        void keyPressEvent(QKeyEvent *e) override {
            QDialog::keyPressEvent(e);
        }
        void closeEvent(QCloseEvent *ev) override {
            QDialog::closeEvent(ev);
        }
};

#define UPLOAD_DIR "/home/web_user"

int ipeui_fileDialog(lua_State *L) {
    static const char *const typenames[] = {"open", "save", nullptr};

    QWidget *parent = check_winid(L, 1);
    int type = luaL_checkoption(L, 2, nullptr, typenames);
    QString caption = checkqstring(L, 3);
    if (!lua_istable(L, 4))
        luaL_argerror(L, 4, "table expected for filters");
    QString filters = "";
    QString filtersNames = "";
    int nFilters = lua_rawlen(L, 4);
    for (int i = 1; i <= nFilters; i += 1) {
        lua_rawgeti(L, 4, i);
        luaL_argcheck(L, lua_isstring(L, -1), 4, "filter entry is not a string");
        if (i % 2 == 0) {
            // 1-based indexing, first named entry, second csv extensions
            if (!filters.isEmpty())
                filters += ",";
            filters += checkqstring(L, -1).replace("*", "").replace(";", ",");
        } else {
            if (!filtersNames.isEmpty())
                filtersNames += "\n";
            filtersNames += checkqstring(L, -1);
        }
        lua_pop(L, 1); // element i
    }

    QString dir;
    if (!lua_isnoneornil(L, 5))
        dir = checkqstring(L, 5);
    QString name;
    if (!lua_isnoneornil(L, 6))
        name = checkqstring(L, 6);
    /*
    // not implemented
    int selected = 0;
    if (!lua_isnoneornil(L, 7))
        selected = luaL_checkinteger(L, 7);
    */

    QString filePath;
    if (type == 0) { // open
        CancellableWaitDialog dialog("Waiting for file upload");

        bool success = false;
        auto fileContentReady = [&filePath, &success, &dialog](const QString &fileName, const QByteArray &fileContent) {
            if (!fileName.isEmpty()) {
                std::filesystem::create_directories(UPLOAD_DIR);
                filePath = UPLOAD_DIR + ("/" + fileName);
                QSaveFile file(filePath);
                file.open(QIODevice::WriteOnly);
                file.write(fileContent);
                success = file.commit();
            }
            dialog.completed(); // never called if upload is cancelled, so dialog is user-cancellable
        };

        QFileDialog::getOpenFileContent(filters, fileContentReady);
        // unused: parent, caption, selected (filter), dir, name
        dialog.startDialog();
    } else { // save
        if (name.contains("/")) {
            name = name.section("/", -1);
        }

        bool ok;
        QString text = QInputDialog::getText(
            parent,
            caption,
            "Please enter a file name. Possible extensions:\n" + filtersNames,
            QLineEdit::Normal,
            name,
            &ok);

        if (ok && !text.isEmpty()) {
            if (dir.isEmpty()) {
                dir = UPLOAD_DIR;
            }
            std::filesystem::create_directories(dir.toStdString());
            filePath = dir + "/" + text;
        } else {
            return 0;
        }
    }

    push_string(L, filePath);
    // QString f = dialog.selectedNameFilter();
    // int sf = 0;
    // while (sf < filters.size() && filters[sf] != f)	++sf;
    // lua_pushinteger(L, (sf < filters.size()) ? sf + 1 : 0);
    // TODO currently type cannot be changed when saving / error when no extension
    lua_pushinteger(L, 0); // selected type
    return 2;
}

int ipeui_downloadFileIfIpeWeb(lua_State *L) {
    QString path = checkqstring(L, 1);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox msgBox;
        msgBox.setText("Download failed!");
        msgBox.setInformativeText("Could not open file at " + path + " for downloading!");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return 0;
    }
    QString name = path;
    if (name.contains("/")) {
        name = name.section("/", -1);
    }
    QFileDialog::saveFileContent(file.readAll(), name);
    return 0;
}

int ipeui_startBrowser(lua_State *L) {
    const std::string url = luaL_checklstring(L, 1, nullptr);
    emscripten::val window = emscripten::val::global("window");
    window.call<emscripten::val>("open", url, std::string("_blank"))
      .call<void>("focus");
    lua_pushboolean(L, true);
    return 1;
}
