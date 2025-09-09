#include <gtk/gtk.h>
#include <stdio.h>
#include <time.h>

#define ENTRY_FOLDER "DiaryEntries"

GtkWidget *text_view;
GtkWidget *calendar;
GtkWidget *image;
char current_image_path[512] = "";

/* --- Save diary entry (text + image path) --- */
static void on_save_clicked(GtkWidget *widget, gpointer data)
{
    guint year, month, day;
    gtk_calendar_get_date(GTK_CALENDAR(calendar), &year, &month, &day);
    month++; // GTK months are 0-based

    char filename[256];
    snprintf(filename, sizeof(filename), ENTRY_FOLDER "/%04d%02d%02d.txt", year, month, day);

    // Get text from TextView
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    // Save diary entry with image path
    g_mkdir_with_parents(ENTRY_FOLDER, 0755);
    FILE *f = fopen(filename, "w");
    if (f)
    {
        fprintf(f, "TEXT:\n%s\n", text);
        fprintf(f, "IMAGE:%s\n", current_image_path);
        fclose(f);

        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                                   "Diary entry saved!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    g_free(text);
}

/* --- Load diary entry (text + image) --- */
static void on_load_clicked(GtkWidget *widget, gpointer data)
{
    guint year, month, day;
    gtk_calendar_get_date(GTK_CALENDAR(calendar), &year, &month, &day);
    month++;

    char filename[256];
    snprintf(filename, sizeof(filename), ENTRY_FOLDER "/%04d%02d%02d.txt", year, month, day);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    FILE *f = fopen(filename, "r");
    if (f)
    {
        char *line = NULL;
        size_t len = 0;
        ssize_t read;
        GString *content = g_string_new("");
        char image_path[512] = "";

        while ((read = getline(&line, &len, f)) != -1)
        {
            if (g_str_has_prefix(line, "IMAGE:"))
            {
                g_strlcpy(image_path, line + 6, sizeof(image_path));
                g_strstrip(image_path);
            }
            else if (!g_str_has_prefix(line, "TEXT:"))
            {
                g_string_append(content, line);
            }
        }
        fclose(f);
        if (line) free(line);

        gtk_text_buffer_set_text(buffer, content->str, -1);
        g_string_free(content, TRUE);

        if (image_path[0] != '\0' && g_file_test(image_path, G_FILE_TEST_EXISTS))
        {
            GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(image_path, 400, 400, TRUE, NULL);
            if (pixbuf)
            {
                gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
                g_object_unref(pixbuf);
            }
            g_strlcpy(current_image_path, image_path, sizeof(current_image_path));
        }
        else
        {
            gtk_image_clear(GTK_IMAGE(image));
            current_image_path[0] = '\0';
        }
    }
    else
    {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                   "No entry found for this date.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        gtk_text_buffer_set_text(buffer, "", -1);
        gtk_image_clear(GTK_IMAGE(image));
        current_image_path[0] = '\0';
    }
}

/* --- Select an image and resize it to 400x400 --- */
static void on_add_image_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Image",
                                                    NULL,
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pixbuf_formats(filter);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        // Load and scale the image to 400x400
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(filename, 100,100, TRUE, NULL);
        if (pixbuf)
        {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
            g_object_unref(pixbuf);

            g_strlcpy(current_image_path, filename, sizeof(current_image_path));
        }
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

/* --- Main Window --- */
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Diary App with Picture (GTK C)");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    calendar = gtk_calendar_new();
    gtk_box_pack_start(GTK_BOX(vbox), calendar, FALSE, FALSE, 0);

    text_view = gtk_text_view_new();
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    // Image area + Add button
    image = gtk_image_new();
    GtkWidget *btn_add_img = gtk_button_new_with_label("Add Image");
    g_signal_connect(btn_add_img, "clicked", G_CALLBACK(on_add_image_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), btn_add_img, FALSE, FALSE, 0);

    // Save + Load buttons
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
