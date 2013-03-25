#include "gui.h"
#include "revision.h"

enum
{
  COLUMN_STRING,
  COLUMN_INT,
  COLUMNS
};

pthread_mutex_t  mutex_guichange;

struct gui_res
{
    GtkTextBuffer *chat_buffer;
    GtkWidget *text_entry;
    GtkWidget *status_bar;
    
    GtkWidget *scrolledwindow_chat;
    GtkWidget *scrolledwindow_user_list;
    
    GtkToolItem *toolbar_connect;
} gres;


void push_status_bar(const gchar*);
void add_message_to_chat(gpointer data, gchar *str, gchar type);

void* GuiThread(void* arg)
{
    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    gui_res* gres = (gui_res*) arg;

    bool oldstatus = false;
    while(1)
    {
        if(c_core->IsConnected() && !oldstatus)
        {
            gtk_tool_button_set_label(
                GTK_TOOL_BUTTON(gres->toolbar_connect),
                "Disconnect");
            push_status_bar("Connected with server!");
            add_message_to_chat(gres->chat_buffer,
                                (gchar*) "<local> Connected!\n", 'e');
        }
        if(!c_core->IsConnected() && oldstatus)
        {
            gtk_tool_button_set_label(
                GTK_TOOL_BUTTON(gres->toolbar_connect),
                "Connect");
            push_status_bar("Disconnected from server!");
            add_message_to_chat(gres->chat_buffer,
                                (gchar*) "<local> Disconnected!\n", 'e');
        }
        oldstatus = c_core->IsConnected();

        if(c_core->IsConnected())
        {
            eventg ev = c_core->GetEvent();
            if (ev.who != "")
            {
                stringstream ss;
                ss << "<" << ev.who << "> " << ev.data << endl;
                add_message_to_chat(gres->chat_buffer,
                                    (gchar*) ss.str().c_str(), 'm');

            }
        }
        c_core->WaitEvent();
    }

    pthread_exit(NULL);
}

GdkPixbuf *create_pixbuf(const gchar * filename)
{
   GdkPixbuf *pixbuf;
   GError *error = NULL;
   pixbuf = gdk_pixbuf_new_from_file(filename, &error);

   if(!pixbuf)
   {
      fprintf(stderr, "%s\n", error->message);
      g_error_free(error);
   }

   return pixbuf;
}

void show_about()
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file("data/psc.png", NULL);

  GtkWidget *dialog = gtk_about_dialog_new();
  gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), "psc");
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), _REVISION);
  gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog),
      "(c) Alessandro Rosetti Daniele Lazzarini Alessandro Furlanetto");
  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),
      "project secure chat");
  gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog),
      "http://code.google.com/p/project-secure-chat/");
  gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pixbuf);
  g_object_unref(pixbuf), pixbuf = NULL;
  gtk_dialog_run(GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);
}

void show_message(gchar *message)
{
   GtkWidget *dialog, *label;

   dialog = gtk_dialog_new_with_buttons (_REVISION,
                                         0,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_NONE,
                                         NULL);
   label = gtk_label_new (message);

   g_signal_connect_swapped (dialog,
                             "response",
                             G_CALLBACK (gtk_widget_destroy),
                             dialog);
   gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
                      label);
   gtk_widget_show_all (dialog);
}

void destroy(GtkObject *object, gpointer user_data)
{
    gtk_main_quit();
}

void add_user_to_list(gpointer data, gchar *str, gint num)
{
  GtkTreeView *view = GTK_TREE_VIEW(data);
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

  pthread_mutex_lock(&mutex_guichange);
  gtk_list_store_append(GTK_LIST_STORE(model), &iter);
  gtk_list_store_set(GTK_LIST_STORE(model),
                     &iter,
                     COLUMN_STRING,
                     str,
                     COLUMN_INT,
                     num,
                     -1);
  pthread_mutex_unlock(&mutex_guichange);
}

void remove_user_from_list(/*gpointer data, gchar *str, gint num*/)
{

}

void scroll_down(GtkWidget *scrolled)
{
    assert(scrolled);
    GtkAdjustment* adjustment;
    adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled));
    gtk_adjustment_set_value(adjustment, gtk_adjustment_get_upper(adjustment));
}

void add_message_to_chat(gpointer data, gchar *str, gchar type) // TODO utilizzare "..."
{
    GtkTextBuffer *text_view_buffer = GTK_TEXT_BUFFER(data);
    GtkTextIter textiter;
    //gtk_text_buffer_get_iter_at_offset(text_view_buffer, &textiter, 0);
    //int offset = gtk_text_iter_get_offset(&textiter);
    //gtk_text_buffer_get_start_iter(buffer, &textiter);
    //gtk_text_buffer_get_end_iter(buffer, &textiter);

    pthread_mutex_lock(&mutex_guichange);
    gtk_text_buffer_get_end_iter(text_view_buffer, &textiter);
    switch(type)
    {
        case 'j': //join
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "green_bg", "bold", NULL);
        break;
        case 'l': //leave
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "red_bg", "white_fg", "bold", NULL);
        break;
        case 'm': //message received
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "black_fg", NULL);
        break;
        case 'M': //message received
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "black_fg", "bold", NULL);
        break;
        case 'e': //event
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "magenta_bg", "white_fg", "bold", NULL);
        break;
        case 's': //server
            gtk_text_buffer_insert_with_tags_by_name (text_view_buffer,
                &textiter, str, -1, "lmarg", "yellow_bg", "bold", NULL);
        break;
        default:
        break;
    }
    
    scroll_down(gres.scrolledwindow_chat);
    
    pthread_mutex_unlock(&mutex_guichange);
}

void button_send_click(gpointer data, gchar *str, gchar type)
{
    stringstream ss, ss_h;
    gchar *text = (gchar*) gtk_entry_get_text(GTK_ENTRY(gres.text_entry));

    if (strlen(text) == 0) // TODO check max length
        return;

    ss_h << text;
    if (!c_core->HandleSend((char*)ss_h.str().c_str()))
    {
        add_message_to_chat(gres.chat_buffer, (gchar*) "<local> send failed!\n", 'e');
        return;
    }

    if ((text[0] != '\\') || strncmp(text, "\\send", 5))
    {
        ss << "<" << CFG_GET_STRING("nickname") << "> " << text << endl;
        add_message_to_chat(gres.chat_buffer, (gchar*) ss.str().c_str(), 'M');
    }

    gtk_entry_set_text (GTK_ENTRY(gres.text_entry), "");
}

void push_status_bar(const gchar *str)
{
    pthread_mutex_lock(&mutex_guichange);

    guint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(gres.status_bar), "info");
    gtk_statusbar_pop(GTK_STATUSBAR(gres.status_bar), id);
    gtk_statusbar_push(GTK_STATUSBAR(gres.status_bar), id, str);

    pthread_mutex_unlock(&mutex_guichange);
}

void toolbar_reset_click(gpointer data)
{
    GtkTextBuffer *text_view_buffer = GTK_TEXT_BUFFER(gres.chat_buffer);
    GtkTextIter textiter;

    pthread_mutex_lock(&mutex_guichange);
    gtk_text_buffer_get_end_iter(text_view_buffer, &textiter);
    gtk_text_buffer_set_text(text_view_buffer, "", 0);
    pthread_mutex_unlock(&mutex_guichange);
}

void toolbar_connect_click(gpointer data, gchar *str, gchar type)
{
    GtkToolButton *toolbar_connect = GTK_TOOL_BUTTON(data);
    if (!c_core->IsConnected())
    {
        if(!c_core->Connect())
            push_status_bar("Connection failed!");

        return;
    }

    if(c_core->IsConnected())
    {
        if(!c_core->Disconnect())
            push_status_bar("Disconnection failed!?");
    }
}

void main_gui(int argc, char **argv)
{
    GtkWidget *window;
    GtkWidget *vbox_main;

    /* menubar */
    GtkWidget *menubar;
    GtkWidget *filemenu,*helpmenu;
    GtkWidget *file;
    GtkWidget *connect,*open,*sep,*quit;
    GtkWidget *help;
    GtkWidget *about;
    GtkAccelGroup *accel_group = NULL;

    /* toolbar */
    GtkWidget *toolbar;
    GtkToolItem *toolbar_connect;
    GtkToolItem *toolbar_reset;
    GtkToolItem *toolbar_separator;
    GtkToolItem *toolbar_exit;

    /* chat */
    GtkWidget *hbox_chat;
    GtkWidget *view_chat;
    GtkTextBuffer *view_chat_buffer;

    /* lista utenti */

    GtkListStore *model_user_list;
    GtkWidget *view_user_list;
    GtkCellRenderer *renderer_user_list;
    GtkTreeSelection *selection_user_list;

    /* input della chat */
    GtkWidget *hbox_inputs;
    GtkWidget *vbox_inputs;
    GtkWidget *entry_command;
    GtkWidget *button_send;

    GtkWidget *dialog;

    GdkColor color;
    gdk_color_parse ("red", &color);

    /* inits */
    gtk_init (&argc, &argv);
    pthread_mutex_init(&mutex_guichange, NULL);

    /* window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(window),0);
    gtk_window_set_urgency_hint (GTK_WINDOW(window), TRUE);
    gtk_window_set_title (GTK_WINDOW (window), _PROJECTNAME);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    //setting window icon
    GdkPixbuf *pixbuf = create_pixbuf("data/psc.png");
    gtk_window_set_icon(GTK_WINDOW(window), pixbuf);
    g_object_unref(pixbuf);
    pixbuf = NULL;

    gtk_widget_show(window);

    g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(destroy), NULL);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

    /* vbox principale */
    vbox_main = gtk_vbox_new (FALSE, 1);
    gtk_container_add(GTK_CONTAINER(window), vbox_main);
    gtk_container_set_border_width(GTK_CONTAINER(vbox_main),0);

    /* accellgroup */
    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

    /* menubar */
    menubar = gtk_menu_bar_new();
    filemenu = gtk_menu_new();
    helpmenu = gtk_menu_new();

    file = gtk_menu_item_new_with_label("File");
    connect = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
    open = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
    sep = gtk_separator_menu_item_new();
    quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);
    help = gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP, NULL);
    about = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), filemenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), connect);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), quit);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help), helpmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpmenu), about);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help);

    gtk_box_pack_start(GTK_BOX(vbox_main), menubar, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(about), "activate", G_CALLBACK(show_about), NULL);

    /* toolbar */
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);

    gtk_container_set_border_width(GTK_CONTAINER(toolbar), 2);

    toolbar_connect = gtk_tool_button_new_from_stock(GTK_STOCK_NETWORK);
    if (!c_core->IsConnected())
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_connect), "Connect");
    else
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolbar_connect), "Disconnect");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbar_connect, -1);
    g_signal_connect(G_OBJECT(toolbar_connect), "clicked", G_CALLBACK(toolbar_connect_click), NULL);

    toolbar_reset = gtk_tool_button_new_from_stock(GTK_STOCK_CLEAR);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbar_reset, -1);
    g_signal_connect(G_OBJECT(toolbar_reset), "clicked", G_CALLBACK(toolbar_reset_click), NULL);

    toolbar_separator = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbar_separator, -1);

    toolbar_exit = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbar_exit, -1);
    g_signal_connect(G_OBJECT(toolbar_exit), "clicked", G_CALLBACK(gtk_main_quit), NULL);

    gtk_box_pack_start(GTK_BOX(vbox_main), toolbar, FALSE, FALSE, 0);

    /* CHAT */
    hbox_chat = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_main), hbox_chat, TRUE, TRUE, 0);

    gres.scrolledwindow_chat = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX (hbox_chat), gres.scrolledwindow_chat, TRUE, TRUE, 0);

    gtk_container_set_border_width (GTK_CONTAINER (gres.scrolledwindow_chat), 2);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (gres.scrolledwindow_chat),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);

    view_chat = gtk_text_view_new();
    
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view_chat), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view_chat), false);
    gtk_container_add (GTK_CONTAINER (gres.scrolledwindow_chat), view_chat);
    view_chat_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view_chat));
    gtk_text_buffer_create_tag(view_chat_buffer, "gap", "pixels_above_lines", 30, NULL);
    gtk_text_buffer_create_tag(view_chat_buffer, "lmarg", "left_margin", 5, NULL);
    gtk_text_buffer_create_tag(view_chat_buffer, "black_fg", "foreground", "black", NULL);
    gtk_text_buffer_create_tag(view_chat_buffer, "white_fg", "foreground", "white", NULL);
    gtk_text_buffer_create_tag(view_chat_buffer, "green_bg", "background", "lime", NULL);
    gtk_text_buffer_create_tag(view_chat_buffer, "blue_bg", "background", "blue", NULL);
    gtk_text_buffer_create_tag(view_chat_buffer, "red_bg", "background", "red", NULL);
    gtk_text_buffer_create_tag(view_chat_buffer, "yellow_bg", "background", "yellow", NULL);
    gtk_text_buffer_create_tag(view_chat_buffer, "magenta_bg", "background", "magenta", NULL);
    gtk_text_buffer_create_tag(view_chat_buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);
    gtk_text_buffer_create_tag(view_chat_buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);

    /*############################################## message test */
    add_message_to_chat(view_chat_buffer, (gchar*) "<gufo> has joined the chat\n", (gchar)'j');
    add_message_to_chat(view_chat_buffer, (gchar*) "<gufo> salve buonuomo\n", (gchar)'m');
    add_message_to_chat(view_chat_buffer, (gchar*) "<server> reboot scheduled in 5 minutes!\n", (gchar)'s');
    add_message_to_chat(view_chat_buffer, (gchar*) "<alec> ave!\n", (gchar)'m');
    add_message_to_chat(view_chat_buffer, (gchar*) "<furla> ...\n", (gchar)'m');
    add_message_to_chat(view_chat_buffer, (gchar*) "<alec> \"furla\" has been kicked out!\n", (gchar)'l');
    /*##############################################*/

    gres.scrolledwindow_user_list = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX (hbox_chat), gres.scrolledwindow_user_list, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (gres.scrolledwindow_user_list), 2);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (gres.scrolledwindow_user_list),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_widget_show (gres.scrolledwindow_user_list);

    model_user_list     = gtk_list_store_new(COLUMNS, G_TYPE_STRING, G_TYPE_INT);
    view_user_list      = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model_user_list));
    selection_user_list = gtk_tree_view_get_selection(GTK_TREE_VIEW(view_user_list));

    gtk_tree_selection_set_mode(selection_user_list, GTK_SELECTION_SINGLE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view_user_list), TRUE);

    renderer_user_list = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view_user_list), /* vista */
                       -1,                  /* posizione della colonna */
                       "Name",  /* titolo della colonna */
                       renderer_user_list,            /* cella inserita nella colonna */
                       "text",              /* attributo colonna */
                       COLUMN_STRING,    /* colonna inserita  */
                       NULL);               /* fine ;-) */

    renderer_user_list = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view_user_list), -1,
                          "?", renderer_user_list,
                          "text", COLUMN_INT,
                          NULL);

    gtk_widget_show (view_user_list);
    g_object_unref(model_user_list);

    gtk_container_add (GTK_CONTAINER (gres.scrolledwindow_user_list), view_user_list);
    gtk_container_set_border_width (GTK_CONTAINER (view_user_list), 0);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view_user_list), TRUE);
    /*############################################## user add test */
    add_user_to_list(view_user_list, (gchar*) "alec", 0);
    add_user_to_list(view_user_list, (gchar*) "furla", 1);
    add_user_to_list(view_user_list, (gchar*) "gufo", 2);
    /*##############################################*/

    /* INPUTS */
    hbox_inputs = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start(GTK_BOX (vbox_main), hbox_inputs, FALSE, FALSE, 0);

    vbox_inputs = gtk_vbox_new (FALSE, 0);
    gtk_container_add(GTK_CONTAINER(hbox_inputs), vbox_inputs);
    entry_command = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(vbox_inputs), entry_command);
    button_send = gtk_button_new_with_label("Send");
    gtk_widget_set_size_request (GTK_WIDGET (button_send), 70, 30);
    gtk_box_pack_start(GTK_BOX (hbox_inputs), button_send, FALSE, FALSE, 0);

    gres.text_entry = entry_command;
    gres.chat_buffer = view_chat_buffer;
    g_signal_connect(G_OBJECT(entry_command), "activate", G_CALLBACK(button_send_click), NULL);
    g_signal_connect(G_OBJECT(button_send), "clicked", G_CALLBACK(button_send_click), NULL);

    /* status_bar */
    gres.status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX (vbox_main), gres.status_bar, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(gres.status_bar), "info", (gpointer)"1");
    g_object_set_data(G_OBJECT(gres.status_bar), "info", (gpointer) "2");
    g_object_set_data(G_OBJECT(gres.status_bar), "info", (gpointer) "3");

    g_object_set_data(G_OBJECT(gres.status_bar), "warning", (gpointer) "A");
    g_object_set_data(G_OBJECT(gres.status_bar), "warning", (gpointer) "B");
    g_object_set_data(G_OBJECT(gres.status_bar), "warning", (gpointer) "C");

    //guint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar), "info");
    //gtk_statusbar_push(GTK_STATUSBAR(status_bar), id, "* uninitialized");

    /* end_widgets */
    gtk_widget_show_all(window);
    gres.toolbar_connect = toolbar_connect;

    // TODO avviare il thread in modo umano
    pthread_t tid;
    pthread_attr_t tattr;
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &tattr, GuiThread, (void*)&gres);
    pthread_attr_destroy(&tattr);

    g_print ("* starting gtk\n");
    gtk_main();

    pthread_mutex_destroy(&mutex_guichange);

    return;
}
