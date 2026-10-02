// --------------------------------------------------------------------
// uitest with Qt UI
// --------------------------------------------------------------------

#include <QApplication>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <cstring>

extern int luaopen_ipeui(lua_State * L);
extern void push_winid(lua_State * L, QWidget * w);

// --------------------------------------------------------------------

static int traceback(lua_State * L) {
    if (!lua_isstring(L, 1)) /* 'message' not a string? */
	return 1;            /* keep it intact */
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_getfield(L, -1, "debug");
    if (!lua_istable(L, -1)) {
	lua_pop(L, 2);
	return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
	lua_pop(L, 3);
	return 1;
    }
    lua_pushvalue(L, 1);   // pass error message
    lua_pushinteger(L, 2); // skip this function and traceback
    lua_call(L, 2, 1);     // call debug.traceback
    return 1;
}

// --------------------------------------------------------------------

static const char * const actions[] = {"quit",       "dialog 1",   "collect garbage",
				       "open",       "save",       "color",
				       "messagebox", "start timer"};

#define NUM_ACTIONS (sizeof(actions) / sizeof(const char *))

class AppUi;

class Canvas : public QWidget {
public:
    Canvas(AppUi * ui, QWidget * parent)
	: QWidget(parent)
	, iUi(ui) {}

protected:
    void mousePressEvent(QMouseEvent * e) override;

private:
    AppUi * iUi;
};

class AppUi {
public:
    AppUi(lua_State * L0);

    void show();

    void cmd(const char * cmd);
    void showMenu(int x, int y);

    QWidget * windowId() const { return window; }

private:
    lua_State * L;

    QMainWindow * window;
    Canvas * canvas;
};

AppUi::AppUi(lua_State * L0) {
    L = L0;

    window = new QMainWindow();
    window->resize(200, 100);
    window->setWindowTitle("UI Test");

    QMenu * menu1 = window->menuBar()->addMenu("&File");
    QMenu * menu2 = window->menuBar()->addMenu("&Tests");
    for (int i = 0; i < int(NUM_ACTIONS); ++i) {
	QMenu * menu = (i == 0) ? menu1 : menu2;
	QAction * action = menu->addAction(actions[i]);
	QObject::connect(action, &QAction::triggered, window,
			 [this, i]() { cmd(actions[i]); });
    }

    canvas = new Canvas(this, window);
    window->setCentralWidget(canvas);
}

void AppUi::show() { window->show(); }

// --------------------------------------------------------------------

void AppUi::showMenu(int x, int y) {
    lua_getglobal(L, "show_menu");
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    lua_call(L, 2, 0);
}

void AppUi::cmd(const char * cmd) {
    fprintf(stderr, "cmd: %s\n", cmd);
    if (!strcmp(cmd, "quit")) {
	window->close();
    } else {
	// all other actions call lua code
	lua_getglobal(L, "action");
	lua_pushstring(L, cmd);
	lua_call(L, 1, 0);
    }
}

void Canvas::mousePressEvent(QMouseEvent * e) {
    QPoint p = mapToGlobal(e->pos());
    iUi->showMenu(p.x(), p.y());
}

// --------------------------------------------------------------------

static int mainloop(lua_State * L) {
    qApp->exec();
    return 0;
}

int main(int argc, char * argv[]) {
    QApplication app(argc, argv);

    lua_State * L = luaL_newstate();
    luaL_openlibs(L);
    luaopen_ipeui(L);

    AppUi * ui = new AppUi(L);
    ui->show();

    push_winid(L, ui->windowId());
    lua_setglobal(L, "appui");

    lua_pushcfunction(L, traceback);

    int res = luaL_loadfile(L, "uitest.lua");
    if (res != 0) {
	fprintf(stderr, "Could not load uitest.lua: %d\n", res);
	return 1;
    }
    if (lua_pcall(L, 0, 0, -2)) {
	const char * errmsg = lua_tostring(L, -1);
	fprintf(stderr, "%s\n", errmsg);
	return 1;
    }

    lua_pushcfunction(L, mainloop);
    if (lua_pcall(L, 0, 0, -2)) {
	const char * errmsg = lua_tostring(L, -1);
	fprintf(stderr, "%s\n", errmsg);
	return 1;
    }

    lua_close(L);
    delete ui;
    return 0;
}

// --------------------------------------------------------------------
