/***
 *          _
 *      ___(_)_ __  _ __  _   _
 *     / __| | '_ \| '_ \| | | |
 *    | (__| | | | | | | | |_| |
 *     \___|_|_| |_|_| |_|\__, |
 *                        |___/minimal
 *     Because FUCK React/Node.
 */

#include <string.h>
#include <gtk/gtk.h>
#include <curl/curl.h>
#include <sys/resource.h>
#include <webkit2/webkit2.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

// Includes
#include "cinny.c"

// Defines & Statics
#define RESTORE_LABEL "Restore"
#define CLOSE_LABEL "Close"
#define SEPARATOR_LABEL "-"
#define VERSION_LABEL "v%s"

static GtkStatusIcon *tray_icon;
static GtkWidget *window;

char localver[20] = "0.06"; // Current version, update this while building!

// Horrible logger
void hlog(const char *type, const char *format, ...)
{
  const char *prefix = "";
  const char *colorCode = "";

  switch (type[0])
  {
  case 'l':
    if (strcmp(type, "lfc") == 0)
    {
      prefix = "[Cinny] ";
      colorCode = "\033[38;5;214m";
    }
    else if (strcmp(type, "lfw") == 0)
    {
      prefix = "[WebKit] ";
      colorCode = "\033[38;5;45m";
    }
    else if (strcmp(type, "lfg") == 0)
    {
      prefix = "[Success] ";
      colorCode = "\033[38;5;76m";
    }
    else if (strcmp(type, "lfr") == 0)
    {
      prefix = "[Failure] ";
      colorCode = "\033[38;5;160m";
    }
    break;
  default:
    prefix = "";
    colorCode = "";
    break;
  }

  printf("%s%s%s", colorCode, prefix, "\033[0m");

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

// Notification (permissions) now work!
static void on_permission_request(WebKitWebView *web_view, WebKitPermissionRequest *request, gpointer user_data)
{
  if (WEBKIT_IS_NOTIFICATION_PERMISSION_REQUEST(request))
  {
    hlog("lfc", "Notification permission request allowed.\n");
    WebKitNotificationPermissionRequest *notification_request = WEBKIT_NOTIFICATION_PERMISSION_REQUEST(request);
    webkit_permission_request_allow(WEBKIT_PERMISSION_REQUEST(notification_request));
  }
}

static gboolean show_notification_callback(WebKitWebView *web_view, WebKitNotification *notification, gpointer user_data)
{
  const gchar *title = webkit_notification_get_title(notification);
  const gchar *body = webkit_notification_get_body(notification);

  // I know this is horrible.
  hlog("lfw", "Title: %s - Body: %s\n", title, body);
  gchar *cmd = g_strdup_printf("notify-send \"%s\" \"%s\"", title, body);
  system(cmd);
  g_free(cmd);

  return TRUE;
}

static void on_window_close(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  GtkWidget *window = GTK_WIDGET(data);
  gtk_widget_hide(window);
}

void on_version_item_activate(GtkMenuItem *menu_item, gpointer data)
{
  hlog("lfc", "Opening updates page... \n");
  GError *error = NULL;
  gboolean result = gtk_show_uri(NULL, "https://the-sauna.icu/matrix_client/", GDK_CURRENT_TIME, &error);
  if (!result)
  {
    hlog("lfr", "Error opening URL: %s", error->message);
    g_error_free(error);
  }
}

// most of these stats i got from stackoverflow, thansk peeps.
void memusage()
{
  struct rusage r_usage;
  getrusage(RUSAGE_SELF, &r_usage);
  hlog("lfc", "Memory usage: %ld MB\n", r_usage.ru_maxrss / 1024);
}

// Here's a much better way to handle this lmao
// You really shouldn't be coding sleep deprived ☠️

static gboolean reset_memusage_pressed(gpointer data)
{
  gboolean *memusage_pressed = (gboolean *)data;
  *memusage_pressed = FALSE;
  return G_SOURCE_REMOVE;
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  static gboolean memusage_pressed = FALSE;

  if (event->keyval == GDK_KEY_F12 && !memusage_pressed)
  {
    memusage();
    memusage_pressed = TRUE;
    g_timeout_add(100, (GSourceFunc)reset_memusage_pressed, &memusage_pressed);
  }

  return FALSE;
}

static void on_tray_icon_activate(GtkStatusIcon *icon, gpointer data)
{
  if (gtk_widget_get_visible(window))
  {
    gtk_widget_hide(window);
    hlog("lfc", "Window iconified! \n");
  }
  else
  {
    gtk_widget_show(window);
    gtk_window_deiconify(GTK_WINDOW(window));
    gtk_window_present(GTK_WINDOW(window));
    hlog("lfc", "Window deiconified! \n");
  }
}

static void on_tray_icon_popup_menu(GtkStatusIcon *icon, guint button, guint activate_time, gpointer data)
{
  hlog("lfc", "Popup menu action! \n");
  GtkWidget *menu = gtk_menu_new();

  // Create & add menu items to the right click menu.
  GtkWidget *restore_item = gtk_menu_item_new_with_label(RESTORE_LABEL);
  GtkWidget *close_item = gtk_menu_item_new_with_label(CLOSE_LABEL);
  GtkWidget *separator_item = gtk_separator_menu_item_new();

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), restore_item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), close_item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator_item);

  // If we have an update available, show the new version in the popup menu.
  char label[100];
  if (strcmp(localver, "Update available!") != 0)
  {
    sprintf(label, VERSION_LABEL, localver);
  }
  else
  {
    sprintf(label, "%s", localver);
  }
  GtkWidget *version_item = gtk_menu_item_new_with_label(label);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), version_item);
  g_signal_connect(version_item, "activate", G_CALLBACK(on_version_item_activate), NULL);

  // Connect signals & show right click menu.
  g_signal_connect(restore_item, "activate", G_CALLBACK(on_tray_icon_activate), NULL);
  g_signal_connect_swapped(close_item, "activate", G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show_all(menu);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, icon, button, activate_time);
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *userdata)
{
  char *data = (char *)ptr;
  size_t total_size = size * nmemb;

  char *response = (char *)malloc(total_size + 1);
  if (response == NULL)
  {
    hlog("lfr", "alloc memory failed in write_data() !\n");
    return 0;
  }

  memcpy(response, data, total_size);
  response[total_size] = '\0';

  hlog("lfc", "Server side version: %s\n", response);
  hlog("lfc", "Local version: %s\n", localver);

  if (strcmp(response, localver) == 0)
  {
    hlog("lfc", "No need to update.\n");
  }
  else
  {
    char cmd[256];                                                                    // this is a
    snprintf(cmd, sizeof(cmd), "notify-send 'Cinny Update' '%s' -i cinny", response); // really dumb way
    system(cmd);                                                                      // but it works
    strcpy(localver, "Update available!");                                            // ☠️
  }

  free(response);
  return total_size;
}

int check_update()
{
  hlog("lfc", "Checking for updates...\n");
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, "https://the-sauna.icu/matrix_client/update.php");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
      hlog("lfr", "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      return -1;
    }

    curl_easy_cleanup(curl);
  }
  else
  {
    hlog("lfr", "curl_easy_init() failed\n");
    curl_global_cleanup();
    return -1;
  }

  curl_global_cleanup();
  return 0;
}

int main(int argc, char *argv[])
{
  if (!gtk_init_check(&argc, &argv))
  {
    hlog("lfr", "GTK Initialization failed!?\n");
    return 1;
  }

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  if (!window)
  {
    hlog("lfr", "Window creation failed...?\n");
    return 1;
  }
  gtk_window_set_title(GTK_WINDOW(window), "Cinny");
  gtk_window_set_icon(GTK_WINDOW(window), gdk_pixbuf_new_from_inline(-1, cinny, FALSE, NULL));
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 450);
  gtk_widget_set_size_request(GTK_WIDGET(window), 800, 450);
  g_signal_connect(window, "delete-event", G_CALLBACK(on_window_close), window);

  WebKitWebContext *context = webkit_web_context_new();
  WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(context));
  if (!web_view)
  {
    hlog("lfr", "WebKit view creation failed!\n");
    return 1;
  }

  // Various webkit options, and a pretty dumb way to check they're set:
  WebKitSettings *settings = webkit_settings_new();
  g_object_set(settings,
               "enable-page-cache", TRUE,
               "enable-encrypted-media", TRUE,
               "enable-smooth-scrolling", TRUE,
               "enable-developer-extras", FALSE,
               "enable-accelerated-2d-canvas", TRUE,
               "javascript-can-access-clipboard", TRUE,
               "enable-offline-web-application-cache", TRUE,
               "enable-write-console-messages-to-stdout", FALSE,
               NULL);
  const char *properties[] = {
      "enable-encrypted-media",
      "enable-page-cache",
      "enable-smooth-scrolling",
      "enable-developer-extras",
      "enable-accelerated-2d-canvas",
      "javascript-can-access-clipboard",
      "enable-offline-web-application-cache",
      "enable-write-console-messages-to-stdout"};
  const int num_properties = sizeof(properties) / sizeof(properties[0]);

  for (int i = 0; i < num_properties; i++)
  {
    gboolean value = FALSE;
    g_object_get(settings, properties[i], &value, NULL);
    hlog("lfw", "%s: %s\n", properties[i], value ? "TRUE" : "FALSE");
  }

  // Use the settings object to configure a WebKitWebView...
  webkit_web_view_set_settings(web_view, settings);

  webkit_web_view_load_uri(web_view, "https://cinny.the-sauna.icu/");

  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  if (!scrolled_window)
  {
    hlog("lfg", "Scrollwindow creation failed!??\n");
    return 1;
  }
  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(web_view));

  gtk_container_add(GTK_CONTAINER(window), scrolled_window);
  gtk_widget_show_all(window);

  tray_icon = gtk_status_icon_new_from_pixbuf(gdk_pixbuf_new_from_inline(-1, cinny, FALSE, NULL));
  g_signal_connect(tray_icon, "activate", G_CALLBACK(on_tray_icon_activate), NULL);
  g_signal_connect(tray_icon, "popup-menu", G_CALLBACK(on_tray_icon_popup_menu), NULL);
  g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);
  g_signal_connect(web_view, "permission-request", G_CALLBACK(on_permission_request), NULL);
  g_signal_connect(web_view, "show-notification", G_CALLBACK(show_notification_callback), NULL);

  // Check updates, set off a fire in GTK & return 0 if everything went to hell!
  check_update();
  hlog("lfg", "gkt_main() - we're up and running!\n");
  gtk_main();
  // unnecessary
  gtk_widget_destroy(window);
  return 0;
}
