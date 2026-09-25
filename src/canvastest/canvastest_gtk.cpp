// --------------------------------------------------------------------
// Canvastest for GTK
// --------------------------------------------------------------------

#include "ipedoc.h"

#include "ipecanvas_gtk.h"
#include "ipetool.h"

#include <gtk/gtk.h>

using namespace ipe;

class AppUi : public CanvasObserver {
public:
    enum TAction {
	EOpen = 9001,
	EQuit,
	EGridVisible,
	EFitPage,
	EZoomIn,
	EZoomOut,
	ENextView,
	EPreviousView,
	ENumActions = EPreviousView - 9000
    };

    AppUi();
    virtual ~AppUi();

    void show();

    void load();
    bool load(const char * fn);

    void zoom(int delta);
    void nextView(int delta);
    void fitBox(const Rect & box);
    void updateLabel();

    void cmd(int action);

    GtkWidget * windowId() const { return iWindow; }

protected: // from CanvasObserver
    virtual void canvasObserverWheelMoved(int degrees);
    virtual void canvasObserverMouseAction(int button);

private:
    void addItem(int m, int action, const char * label, const char * shortcut = nullptr,
		 bool checkable = false);

    CanvasBase * iCanvas;
    Snap iSnap;

    bool iGridVisible;
    Document * iDoc;
    String iFileName;
    int iPageNo;
    int iViewNo;

    GtkWidget * iWindow;
    GMenu * iSubMenu[3];
    GtkWidget * vbox;
    GSimpleActionGroup * iActions;
    GtkShortcutController * iShortcuts;
};

static GMainLoop * main_loop;

static void action_response(GSimpleAction * action, GVariant * value, gpointer data) {
    AppUi * ui = (AppUi *)data;
    if (!value) {
	GVariant * state = g_action_get_state(G_ACTION(action));
	if (state) {
	    value = g_variant_new_boolean(!g_variant_get_boolean(state));
	    g_variant_unref(state);
	}
    }
    if (value) g_simple_action_set_state(action, value);
    ui->cmd(
	(AppUi::TAction)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "action")));
}

static gboolean close_request(GtkWindow *, gpointer) {
    g_main_loop_quit(main_loop);
    return FALSE;
}

void AppUi::addItem(int m, int action, const char * label, const char * shortcut,
		    bool checkable) {
    char action_name[32];
    sprintf(action_name, "action%d", action);
    GSimpleAction * a = checkable
			    ? g_simple_action_new_stateful(action_name, nullptr,
							   g_variant_new_boolean(false))
			    : g_simple_action_new(action_name, nullptr);
    g_object_set_data(G_OBJECT(a), "action", GINT_TO_POINTER(action));
    g_signal_connect(a, "activate", G_CALLBACK(action_response), this);
    g_action_map_add_action(G_ACTION_MAP(iActions), G_ACTION(a));

    GMenuItem * item = g_menu_item_new(label, action_name);
    char detailed_name[40];
    sprintf(detailed_name, "win.%s", action_name);
    g_menu_item_set_detailed_action(item, detailed_name);
    g_menu_append_item(iSubMenu[m], item);
    g_object_unref(item);

    if (shortcut) {
	GtkShortcutTrigger * trigger = gtk_shortcut_trigger_parse_string(shortcut);
	GtkShortcutAction * shortcut_action = gtk_named_action_new(detailed_name);
	gtk_shortcut_controller_add_shortcut(iShortcuts,
					     gtk_shortcut_new(trigger, shortcut_action));
    }
}

AppUi::AppUi() {
    iWindow = gtk_window_new();
    gtk_window_set_default_size(GTK_WINDOW(iWindow), 600, 400);
    gtk_window_set_title(GTK_WINDOW(iWindow), "Canvastest");
    g_signal_connect(iWindow, "close-request", G_CALLBACK(close_request), nullptr);

    iActions = g_simple_action_group_new();
    gtk_widget_insert_action_group(iWindow, "win", G_ACTION_GROUP(iActions));
    iShortcuts = GTK_SHORTCUT_CONTROLLER(gtk_shortcut_controller_new());
    gtk_shortcut_controller_set_scope(iShortcuts, GTK_SHORTCUT_SCOPE_GLOBAL);
    gtk_widget_add_controller(iWindow, GTK_EVENT_CONTROLLER(iShortcuts));

    for (int i = 0; i < 3; ++i) iSubMenu[i] = g_menu_new();

    addItem(0, EOpen, "Open", "<Control>O");
    addItem(0, EQuit, "Quit", "<Ctrl>Q");
    addItem(1, EGridVisible, "Grid visible", "F12", true);
    addItem(1, EFitPage, "Fit page", "<Ctrl>F");
    addItem(1, EZoomIn, "Zoom in", "<Ctrl>plus");
    addItem(1, EZoomOut, "Zoom out", "<Ctrl>minus");
    addItem(2, ENextView, "Next view", "Next");
    addItem(2, EPreviousView, "Previous view", "Prior");

    vbox = gtk_grid_new();
    gtk_window_set_child(GTK_WINDOW(iWindow), vbox);

    GMenu * menubar = g_menu_new();
    g_menu_append_submenu(menubar, "File", G_MENU_MODEL(iSubMenu[0]));
    g_menu_append_submenu(menubar, "View", G_MENU_MODEL(iSubMenu[1]));
    g_menu_append_submenu(menubar, "Move", G_MENU_MODEL(iSubMenu[2]));
    GtkWidget * menu_bar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menubar));
    g_object_unref(menubar);

    gtk_grid_attach(GTK_GRID(vbox), menu_bar, 0, 0, 1, 1);

    Canvas * canvas = new Canvas(iWindow);
    gtk_grid_attach_next_to(GTK_GRID(vbox), canvas->window(), menu_bar, GTK_POS_BOTTOM, 1,
			    1);

    iDoc = nullptr;
    iGridVisible = false;
    iPageNo = iViewNo = 0;

    iSnap.iSnap = Snap::ESnapGrid | Snap::ESnapVtx;
    iSnap.iGridVisible = false;
    iSnap.iGridSize = 8;
    iSnap.iAngleSize = M_PI / 6.0;
    iSnap.iSnapDistance = 10;
    iSnap.iWithAxes = false;
    iSnap.iOrigin = Vector::ZERO;
    iSnap.iDir = 0;

    iCanvas = canvas;
    iCanvas->setObserver(this);
    iCanvas->setSnap(iSnap);
    iCanvas->setFifiVisible(true);
}

AppUi::~AppUi() { fprintf(stderr, "AppUi::~AppUi()\n"); }

void AppUi::show() { gtk_window_present(GTK_WINDOW(iWindow)); }

// --------------------------------------------------------------------

void AppUi::load() {
    GtkFileDialog * dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Open file...");
    gtk_file_dialog_open(
	dialog, GTK_WINDOW(iWindow), nullptr,
	[](GObject * source, GAsyncResult * result, gpointer data) {
	    GError * error = nullptr;
	    GFile * file =
		gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &error);
	    if (file) {
		char * path = g_file_get_path(file);
		((AppUi *)data)->load(path);
		g_free(path);
		g_object_unref(file);
	    } else if (error) {
		g_error_free(error);
	    }
	},
	this);
    g_object_unref(dialog);
}

void AppUi::cmd(int cmd) {
    ipeDebug("Command %d", cmd);
    switch (cmd) {
    case EOpen: load(); break;
    case EQuit: gtk_window_close(GTK_WINDOW(iWindow)); break;
    case EGridVisible: {
	Snap snap = iCanvas->snap();
	snap.iGridVisible = !snap.iGridVisible;
	iCanvas->setSnap(snap);
	iCanvas->update();
	break;
    }
    case EFitPage: fitBox(iDoc->cascade()->findLayout()->paper()); break;
    case EZoomIn: zoom(+1); break;
    case EZoomOut: zoom(-1); break;
    case ENextView: nextView(+1); break;
    case EPreviousView: nextView(-1); break;
    default:
	// unknown action
	return;
    }
}

void AppUi::canvasObserverWheelMoved(int degrees) {
    if (degrees > 0)
	zoom(+1);
    else
	zoom(-1);
}

void AppUi::canvasObserverMouseAction(int button) {
    Tool * tool = new PanTool(iCanvas, iDoc->page(iPageNo), iViewNo);
    iCanvas->setTool(tool);
}

// --------------------------------------------------------------------

bool AppUi::load(const char * fname) {
    Document * doc = Document::loadWithErrorReport(fname);
    if (!doc) return false;

    iFileName = String(fname);
    doc->runLatex(fname);

    delete iDoc;
    iDoc = doc;
    iPageNo = 0;
    iViewNo = 0;

    iCanvas->setResources(iDoc->resources());
    iCanvas->setPage(iDoc->page(iPageNo), iPageNo, iViewNo, iDoc->cascade());
    iCanvas->setPan(Vector(300, 400));
    iCanvas->update();
    iCanvas->setFifiVisible(true);

    updateLabel();
    return true;
}

// --------------------------------------------------------------------

void AppUi::updateLabel() {
    String s = iFileName;
    if (iFileName.rfind('/') >= 0) s = iFileName.substr(iFileName.rfind('/') + 1);
    if (iDoc->countTotalViews() > 1) {
	ipe::StringStream ss(s);
	ss << " (" << iPageNo + 1 << "-" << iViewNo + 1 << ")";
    }
    gtk_window_set_title(GTK_WINDOW(iWindow), s.z());
}

void AppUi::fitBox(const Rect & box) {
    if (box.isEmpty()) return;
    double xfactor = box.width() > 0.0 ? (iCanvas->canvasWidth() / box.width()) : 20.0;
    double yfactor = box.height() > 0.0 ? (iCanvas->canvasHeight() / box.height()) : 20.0;
    double zoom = (xfactor > yfactor) ? yfactor : xfactor;
    iCanvas->setPan(0.5 * (box.bottomLeft() + box.topRight()));
    iCanvas->setZoom(zoom);
    iCanvas->update();
}

void AppUi::zoom(int delta) {
    double zoom = iCanvas->zoom();
    while (delta > 0) {
	zoom *= 1.3;
	--delta;
    }
    while (delta < 0) {
	zoom /= 1.3;
	++delta;
    }
    iCanvas->setZoom(zoom);
    iCanvas->update();
}

void AppUi::nextView(int delta) {
    const Page * page = iDoc->page(iPageNo);
    if (0 <= iViewNo + delta && iViewNo + delta < page->countViews()) {
	iViewNo += delta;
    } else if (0 <= iPageNo + delta && iPageNo + delta < iDoc->countPages()) {
	iPageNo += delta;
	if (delta > 0)
	    iViewNo = 0;
	else
	    iViewNo = iDoc->page(iPageNo)->countViews() - 1;
    } else
	// at beginning or end of sequence
	return;
    iCanvas->setPage(iDoc->page(iPageNo), iPageNo, iViewNo, iDoc->cascade());
    iCanvas->update();
    updateLabel();
}

// --------------------------------------------------------------------

static void usage() {
    fprintf(stderr, "Usage: canvastest <filename>\n");
    exit(1);
}

int main(int argc, char * argv[]) {
    Platform::initLib(IPELIB_VERSION);

    gtk_init();

    if (argc != 2) usage();

    AppUi * ui = new AppUi();
    if (!ui->load(argv[1])) exit(2);

    ui->show();

    main_loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(main_loop);
    g_main_loop_unref(main_loop);
    main_loop = nullptr;
    delete ui;
    return 0;
}

// --------------------------------------------------------------------
