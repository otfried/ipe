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
};

static void menuitem_response(GtkWidget * item, gchar * string) {
    AppUi * ui = (AppUi *)g_object_get_data(G_OBJECT(item), "appui");
    ui->cmd(string);
}

static gboolean button_cb(GtkWidget * widget, GdkEvent * event, gpointer data) {
    if (event->type == GDK_BUTTON_RELEASE) {
	GdkEventButton * bevent = (GdkEventButton *)event;
	// gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL,
	// bevent->button, bevent->time);
	AppUi * ui = (AppUi *)data;
	ui->showMenu(int(bevent->x), int(bevent->y));
	return TRUE;
    }
    return FALSE;
}

AppUi::AppUi(lua_State * L0) {
    L = L0;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(GTK_WIDGET(window), 200, 100);
    gtk_window_set_title(GTK_WINDOW(window), "UI Test");
    g_signal_connect(window, "delete-event", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget * menu1 = gtk_menu_new();
    GtkWidget * menu2 = gtk_menu_new();
    for (int i = 0; i < int(NUM_ACTIONS); ++i) {
	GtkWidget * item = gtk_menu_item_new_with_label(actions[i]);
	g_object_set_data(G_OBJECT(item), "appui", this);
	if (i == 0)
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu1), item);
	else
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu2), item);
	g_signal_connect(item, "activate", G_CALLBACK(menuitem_response),
			 gpointer(actions[i]));
	gtk_widget_show(item);
    }

    GtkWidget * root_menu1 = gtk_menu_item_new_with_label("File");
    gtk_widget_show(root_menu1);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(root_menu1), menu1);

    GtkWidget * root_menu2 = gtk_menu_item_new_with_label("Tests");
    gtk_widget_show(root_menu2);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(root_menu2), menu2);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    menu_bar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 2);
    gtk_widget_show(menu_bar);

    canvas = gtk_drawing_area_new();
    gtk_box_pack_end(GTK_BOX(vbox), canvas, TRUE, TRUE, 2);
    gtk_widget_add_events(canvas, GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(canvas), "button-release-event", G_CALLBACK(button_cb),
		     this);
    gtk_widget_show(canvas);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), root_menu1);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), root_menu2);
}

AppUi::~AppUi() { fprintf(stderr, "AppUi::~AppUi()\n"); }

void AppUi::show() { gtk_widget_show(window); }

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
	gtk_main_quit();
    } else {
	// all other actions call lua code
	lua_getglobal(L, "action");
	lua_pushstring(L, cmd);
	lua_call(L, 1, 0);
    }
}

// --------------------------------------------------------------------

int mainloop(lua_State * L) {
    gtk_main();
    return 0;
}

int main(int argc, char * argv[]) {
    gtk_init(&argc, &argv);
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
