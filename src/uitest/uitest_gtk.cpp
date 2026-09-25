// --------------------------------------------------------------------
// uitest with GTK UI
// --------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <gtk/gtk.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <vector>

extern int luaopen_ipeui(lua_State * L);
extern void push_winid(lua_State * L, GtkWidget * w);

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
#define IDBASE 9001

class AppUi {
public:
    AppUi(lua_State * L0);
    ~AppUi();

    void show();

    void cmd(const char * cmd);
    void showMenu(int x, int y);
    void showDialog();

    GtkWidget * windowId() const { return window; }

private:
    lua_State * L;

    GtkWidget * window;
    GtkWidget * menu_bar;
    GtkWidget * vbox;
    GtkWidget * canvas;
    GSimpleActionGroup * actions_group;
};

static GMainLoop * main_loop;

static void menuitem_response(GSimpleAction *, GVariant *, gpointer data) {
    auto * action = (GSimpleAction *)data;
    AppUi * ui = (AppUi *)g_object_get_data(G_OBJECT(action), "appui");
    ui->cmd((const char *)g_object_get_data(G_OBJECT(action), "command"));
}

static void button_cb(GtkGestureClick *, int, double x, double y, gpointer data) {
    AppUi * ui = (AppUi *)data;
    ui->showMenu(int(x), int(y));
}

static gboolean close_request(GtkWindow *, gpointer) {
    if (main_loop) g_main_loop_quit(main_loop);
    return FALSE;
}

AppUi::AppUi(lua_State * L0) {
    L = L0;

    window = gtk_window_new();
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 100);
    gtk_window_set_title(GTK_WINDOW(window), "UI Test");
    g_signal_connect(window, "close-request", G_CALLBACK(close_request), nullptr);

    actions_group = g_simple_action_group_new();
    gtk_widget_insert_action_group(window, "win", G_ACTION_GROUP(actions_group));
    GMenu * menu1 = g_menu_new();
    GMenu * menu2 = g_menu_new();
    for (int i = 0; i < int(NUM_ACTIONS); ++i) {
	char name[32];
	sprintf(name, "action%d", i);
	GSimpleAction * action = g_simple_action_new(name, nullptr);
	g_object_set_data(G_OBJECT(action), "appui", this);
	g_object_set_data(G_OBJECT(action), "command", (gpointer)actions[i]);
	g_signal_connect(action, "activate", G_CALLBACK(menuitem_response), action);
	g_action_map_add_action(G_ACTION_MAP(actions_group), G_ACTION(action));
	char detailed[40];
	sprintf(detailed, "win.%s", name);
	GMenuItem * item = g_menu_item_new(actions[i], detailed);
	if (i == 0)
	    g_menu_append_item(menu1, item);
	else
	    g_menu_append_item(menu2, item);
	g_object_unref(item);
    }

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    GMenu * menubar = g_menu_new();
    g_menu_append_submenu(menubar, "File", G_MENU_MODEL(menu1));
    g_menu_append_submenu(menubar, "Tests", G_MENU_MODEL(menu2));
    menu_bar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menubar));
    gtk_box_append(GTK_BOX(vbox), menu_bar);
    g_object_unref(menubar);

    canvas = gtk_drawing_area_new();
    gtk_widget_set_vexpand(canvas, TRUE);
    gtk_box_append(GTK_BOX(vbox), canvas);
    GtkGesture * click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), 0);
    g_signal_connect(click, "released", G_CALLBACK(button_cb), this);
    gtk_widget_add_controller(canvas, GTK_EVENT_CONTROLLER(click));
}

AppUi::~AppUi() { fprintf(stderr, "AppUi::~AppUi()\n"); }

void AppUi::show() { gtk_window_present(GTK_WINDOW(window)); }

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
	gtk_window_close(GTK_WINDOW(window));
    } else {
	// all other actions call lua code
	lua_getglobal(L, "action");
	lua_pushstring(L, cmd);
	lua_call(L, 1, 0);
    }
}

// --------------------------------------------------------------------

int mainloop(lua_State * L) {
    main_loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(main_loop);
    g_main_loop_unref(main_loop);
    main_loop = nullptr;
    return 0;
}

int main(int argc, char * argv[]) {
    gtk_init();
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
