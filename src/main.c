#include <string.h>
#include <gtk/gtk.h>
#include <curl/curl.h>
#include <webkit2/webkit2.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "cinny.c" // our icon

static GtkStatusIcon *tray_icon;
static GtkWidget *window;

char localver[] = "0.02"; // Our version.

static void on_window_close(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_widget_hide(widget);
}

void on_version_item_activate(GtkMenuItem *menu_item, gpointer data)
{
  GError *error = NULL;
  gboolean result = gtk_show_uri(NULL, "https://the-sauna.icu/matrix_client/", GDK_CURRENT_TIME, &error);
  if (!result)
  {
    g_warning("Error opening URL: %s", error->message);
    g_error_free(error);
  }
}

static void on_tray_icon_activate(GtkStatusIcon *icon, gpointer data)
{
  if (gtk_widget_get_visible(window))
  {
    gtk_widget_hide(window);
  }
  else
  {
    gtk_widget_show(window);
    gtk_window_deiconify(GTK_WINDOW(window));
    gtk_window_present(GTK_WINDOW(window));
  }
}

static void on_tray_icon_popup_menu(GtkStatusIcon *icon, guint button, guint activate_time, gpointer data)
{
  GtkWidget *menu = gtk_menu_new();

  GtkWidget *restore_item = gtk_menu_item_new_with_label("Restore");
  g_signal_connect(restore_item, "activate", G_CALLBACK(on_tray_icon_activate), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), restore_item);

  GtkWidget *close_item = gtk_menu_item_new_with_label("Close");
  g_signal_connect_swapped(close_item, "activate", G_CALLBACK(gtk_main_quit), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), close_item);

  GtkWidget *separator_item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator_item);

  /* this is utterly idiotic */
  /* - kouts               */

  char label[100];
  if (strcmp(localver, "Update available!") != 0)
  {
    sprintf(label, "v%s", localver);
  }
  else
  {
    sprintf(label, "%s", localver);
  }
  GtkWidget *version_item = gtk_menu_item_new_with_label(label);
  g_signal_connect(version_item, "activate", G_CALLBACK(on_version_item_activate), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), version_item);

  gtk_widget_show_all(menu);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, icon, button, activate_time);
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  char *data = (char *)ptr;
  data[size * nmemb] = '\0';
  printf("Server side version: %s\n", data);
  printf("Local version: %s\n", localver);
  if (strcmp(data, localver) == 0)
  {
    printf("No need to update.\n");
  }
  else
  {
    printf("Update available!\n");
    strcpy(localver, "Update available!");
  }
  return size * nmemb;
}

int check_update()
{
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, "https://the-sauna.icu/matrix_client/update.php");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      return 1;
    }

    curl_easy_cleanup(curl);
  }
  return 1;
}

int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Cinny");
  gtk_window_set_icon(GTK_WINDOW(window), gdk_pixbuf_new_from_inline(-1, cinny, FALSE, NULL));
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 450); // Jarvis,
  gtk_widget_set_size_request(GTK_WIDGET(window), 800, 450); // make me immutable.
  g_signal_connect(window, "delete-event", G_CALLBACK(on_window_close), NULL);

  WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
  webkit_web_view_load_uri(web_view, "https://cinny.the-sauna.icu");

  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(web_view));

  gtk_container_add(GTK_CONTAINER(window), scrolled_window);
  gtk_widget_show_all(GTK_WIDGET(window));

  tray_icon = gtk_status_icon_new_from_pixbuf(gdk_pixbuf_new_from_inline(-1, cinny, FALSE, NULL));
  g_signal_connect(tray_icon, "activate", G_CALLBACK(on_tray_icon_activate), NULL);
  g_signal_connect(tray_icon, "popup-menu", G_CALLBACK(on_tray_icon_popup_menu), NULL);

  check_update();
  gtk_main();
  return 0;
}
