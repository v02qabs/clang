#include <gtk/gtk.h>
#include <stdio.h>
#include <time.h>

#define ENTRY_FOLDER "DiaryEntries"

GtkWidget *text_view;
GtkWidget *calendar;

static void on_save_clicked(GtkWidget *widget, gpointer data)
{
    guint year, month, day;
    gtk_calendar_get_date(GTK_CALENDAR(calendar), &year, &month, &day);
    month++; // GTK months are 0-based

    char filename[256];
    snprintf(filename, sizeof(filename), ENTRY_FOLDER "/%04d%02d%02d.txt", year, month, day);

    // get text from TextView
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    // save to file
    g_mkdir_with_parents(ENTRY_FOLDER, 0755);
    FILE *f = fopen(filename, "w");
    if (f)
    {
        fputs(text, f);
        fclose(f);
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                                   "Diary entry saved!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    g_free(text);
}

static void on_load_clicked(GtkWidget *widget, gpointer data)
{
    guint year, month, day;
    gtk_calendar_get_date(GTK_CALENDAR(calendar), &year, &month, &day);
    month++; // GTK months are 0-based

    char filename[256];
    snprintf(filename, sizeof(filename), ENTRY_FOLDER "/%04d%02d%02d.txt", year, month, day);

    FILE *f = fopen(filename, "r");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    if (f)
    {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);

        char *content = g_malloc(size + 1);
        fread(content, 1, size, f);
        content[size] = '\0';
        fclose(f);

        gtk_text_buffer_set_text(buffer, content, -1);
        g_free(content);
    }
    else
    {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                   "No entry found for this date.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        gtk_text_buffer_set_text(buffer, "", -1);
    }
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Diary App (GTK C)");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    calendar = gtk_calendar_new();
    gtk_box_pack_start(GTK_BOX(vbox), calendar, FALSE, FALSE, 0);

    text_view = gtk_text_view_new();
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *btn_save = gtk_button_new_with_label("Save Entry");
    GtkWidget *btn_load = gtk_button_new_with_label("Load Entry");

    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_clicked), NULL);
    g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(hbox), btn_save, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), btn_load, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}
