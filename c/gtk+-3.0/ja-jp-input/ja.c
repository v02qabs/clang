#include <gtk/gtk.h>

static void
activate(GApplication *app, gpointer user_data)
{
    GtkWidget *win = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(win), "表示: 日本");
    gtk_window_set_default_size(GTK_WINDOW(win), 300, 100);

    GtkWidget *label = gtk_label_new("日本");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(win), label);

    gtk_widget_show_all(win);
}

int
main(int argc, char **argv)
{
    int status;
    GtkApplication *app = gtk_application_new("com.example.nihon", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

