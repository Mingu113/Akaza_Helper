#include <curl/curl.h>
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>

#define _NUMBER_OF_BUTTONS 4
#define _TIMER_BUTTON 0
#define _INTERACT_BUTTON 1
#define _QUIT_BUTTON _NUMBER_OF_BUTTONS - 1
#define _WEATHER_INFO_BUTTON 2
#define WINDOW_SIZE_WIDTH 400
#define WINDOW_SIZE_HEIGHT 600
#define TEXT_VIEW_SIZE_HEIGHT 100
// Tạm thời : ở Nha Trang, timestampe unix, không hourly, chỉ daily và current
#define WEATHER_API                                                           \
  "https://api.open-meteo.com/v1/"                                            \
  "forecast?latitude=12.2451&longitude=109.1943&current=temperature_2m,"      \
  "relative_humidity_2m,apparent_temperature,is_day,rain&daily=temperature_"  \
  "2m_max,temperature_2m_min,sunrise,sunset&timeformat=unixtime&timezone="    \
  "Asia%2FBangkok&forecast_days=1"

const char *files[] = {
  "akaza.jpg",

};
const char *main_buttons[_NUMBER_OF_BUTTONS] = {
  "Hẹn giờ",
  "Tương tác",
  "Thời tiết",
  "Thoát",
};
const char *dialogue_string[] = {
  "Xin chào",
  "Cậu khoẻ không?",
  "Cậu đã ăn chưa?",
  "Hi hi",
  "Mình yêu cậu",
  "Mình yêu cậu lắm",
  "Mình rất vui vì được nói chuyện với cậu",
  "...Mình yêu cậu",
  "...",
  "Em yêu anh",
};

/*
  Phần dành cho kết nối với mạng để lấy thông tin thời tiết, sử dụng curl
  Code sưu tầm từ Artem ở Dev.to
*/
// size để ghi vào file
struct MemoryStruct
{
  char *memory;
  size_t size;
};

size_t
WriteMemoryCallback (void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc (mem->memory, mem->size + realsize + 1);
  if (ptr == NULL)
    {
      printf ("error: not enough memory\n");
      return 0;
    }

  mem->memory = ptr;
  memcpy (&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}
// HẠN CHẾ SỬ DỤNG HÀM NÀY, OPEN METEO CHO XÀI FREE THÌ KHÔNG NÊN PHÁ
char *
get_weather_info (gchar *source)
{
  CURL *curl_handle;
  CURLcode res;

  struct MemoryStruct chunk;
  chunk.memory = malloc (1);
  chunk.size = 0;

  curl_handle = curl_easy_init ();
  if (curl_handle)
    {
      curl_easy_setopt (curl_handle, CURLOPT_URL, source);
      curl_easy_setopt (curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION,
                        WriteMemoryCallback);
      curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
      curl_easy_setopt (curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
      res = curl_easy_perform (curl_handle);

      if (res != CURLE_OK)
        {
          fprintf (stderr, "error: %s\n", curl_easy_strerror (res));
          curl_easy_cleanup (curl_handle);
          return NULL;
        }
    }
  curl_global_cleanup ();
  return chunk.memory;
}

/*
  Kết thúc phần dành cho curl
*/
// Lấy thông tin thời tiết
const gchar *
weather_info ()
{
  char *weather_info = get_weather_info (WEATHER_API);
  if (weather_info == NULL)
    return "Em xin lỗi, em không lấy được thông tin thời tiết rồi.";
  JsonParser *parser = json_parser_new ();
  JsonObject *root;
  GError *error;
  if (json_parser_load_from_data (parser, weather_info, -1, &error))
    g_print ("Parse thành công\n");
  else
    g_print ("Parse không thành công. Lỗi: %s\n", error->message);
  free (weather_info);
  root = json_node_get_object (json_parser_get_root (parser));
  JsonObject *current = json_object_get_object_member (root, "current");
  JsonObject *daily = json_object_get_object_member (root, "daily");
  JsonNode *current_temperature
      = json_object_get_member (current, "temperature_2m");
  JsonNode *current_is_day = json_object_get_member (current, "is_day");
  JsonArray *daily_temperature_max
      = json_object_get_array_member (daily, "temperature_2m_max");
  JsonArray *daily_temperature_min
      = json_object_get_array_member (daily, "temperature_2m_min");

  JsonObject *unit = json_object_get_object_member (root, "current_units");
  JsonNode *temperature_unit = json_object_get_member (unit, "temperature_2m");
  const gchar *temperature_unit_str = json_node_get_string (temperature_unit);

  double current_temperature_double
      = json_node_get_double (current_temperature);
  gboolean current_is_day_bool = json_node_get_boolean (current_is_day);
  const gchar *current_day_night_str
      = current_is_day_bool ? "ban ngày" : "buổi tối";
  double daily_temperature_max_double = json_node_get_double (
      json_array_get_element (daily_temperature_max, 0));
  double daily_temperature_min_double = json_node_get_double (
      json_array_get_element (daily_temperature_min, 0));

  // Bởi vì sprintf chỉ lấy 2 arg
  const gchar *weather = g_strdup_printf ("Nhiệt độ hôm nay là %.1f",
                                          current_temperature_double);
  const gchar *weather_part2
      = g_strdup_printf (", hiện tại đang là %s", current_day_night_str);
  const gchar *weather_part3
      = g_strdup_printf (".\nNgày hôm nay, nhiệt độ cao nhất là %.1f",
                         daily_temperature_max_double);
  const gchar *weather_part4 = g_strdup_printf (
      ", còn nhiệt độ thấp nhất là %.1f", daily_temperature_min_double);

  const gchar *final_weather = g_strconcat (
      weather, temperature_unit_str, weather_part2, weather_part3,
      temperature_unit_str, weather_part4, temperature_unit_str, NULL);

  return final_weather;
}
/*
  Kết thúc phần liên quan đến mạng
*/
static void
weather_info_clicked (GtkWidget *button, gpointer data)
{
  GtkWidget *text_view = GTK_WIDGET (data);
  GtkTextBuffer *text_buffer
      = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  GtkTextIter start, end;
  gtk_text_buffer_set_text (text_buffer, "Đang lấy thông tin thời tiết", -1);
  gtk_text_buffer_get_start_iter (text_buffer, &start);
  gtk_text_buffer_get_end_iter (text_buffer, &end);
  gtk_text_buffer_apply_tag_by_name (text_buffer, "font", &start, &end);
  const char *weather_info_str = weather_info ();
  gtk_text_buffer_set_text (text_buffer, weather_info_str, -1);
  gtk_text_buffer_get_start_iter (text_buffer, &start);
  gtk_text_buffer_get_end_iter (text_buffer, &end);
  gtk_text_buffer_apply_tag_by_name (text_buffer, "font", &start, &end);
}
static gboolean
timer_on_clicked (GtkWidget *button, gpointer data)
{
  GtkWidget *label = GTK_WIDGET (((gpointer *)data)[0]);
  const char *label_text = gtk_label_get_label (GTK_LABEL (label));
  int index = GPOINTER_TO_INT (((gpointer *)data)[1]);
  guint value = strtol (label_text, NULL, 10);
  char update_value[3];
  const char *button_label = gtk_button_get_label (GTK_BUTTON (button));
  guint max_value = (index == 0) ? 23 : 59;
  if (g_strcmp0 (button_label, "+") == 0)
    {
      if (value < max_value)
        sprintf (update_value, "%d", ++value);
      else
        sprintf (update_value, "%d", value);
    }
  else
    {
      if (value > 0)
        sprintf (update_value, "%d", --value);
      else
        sprintf (update_value, "%d", value);
    }
  gtk_label_set_text (GTK_LABEL (label), update_value);
  return G_SOURCE_CONTINUE;
}
gboolean
show_alert_window (gpointer data)
{
  GtkWidget *alert_window = gtk_window_new ();
  GtkWidget *label = gtk_label_new ("Hẹn giờ đã kết thúc");
  GtkWidget *button = gtk_button_new_with_label ("Được rồi");
  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_window_destroy),
                            alert_window);

  gchar *time_string = g_date_time_format (data, "%H:%M:%S");
  gchar *label_string = g_strconcat (gtk_label_get_text (GTK_LABEL (label)),
                                     " vào lúc ", time_string, NULL);
  gtk_label_set_text (GTK_LABEL (label), label_string);
  gtk_label_set_wrap_mode (GTK_LABEL (label), GTK_WRAP_WORD);

  gtk_box_append (GTK_BOX (vbox), label);
  gtk_box_append (GTK_BOX (vbox), button);

  /* Thiết lập cửa sổ */
  gtk_window_set_title (GTK_WINDOW (alert_window), "Hẹn giờ kết thúc");
  gtk_window_set_default_size (GTK_WINDOW (alert_window), 250, 150);
  gtk_window_set_resizable (GTK_WINDOW (alert_window), FALSE);
  gtk_window_set_child (GTK_WINDOW (alert_window), vbox);
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_window_set_focus (GTK_WINDOW (alert_window), label);

  gtk_widget_set_size_request (label, 250, 150);

  gtk_window_present (GTK_WINDOW (alert_window));

  return FALSE;
}
void
set_timer (GDateTime *time_final)
{
  GDateTime *time_now = g_date_time_new_now_local ();

  while (g_date_time_compare (time_now, time_final) == -1)
    {
      g_usleep (500000);
      g_date_time_unref (time_now);
      time_now = g_date_time_new_now_local ();
    }

  /*Hiển thị cửa sổ thông báo ra màn hình*/
  g_idle_add (G_SOURCE_FUNC (show_alert_window), time_final);

  g_date_time_unref (time_final);
  g_date_time_unref (time_now);
}

static gboolean
get_timer_time (GtkWidget *button, gpointer data)
{
  GtkWidget **labels = (GtkWidget **)data;
  GDateTime *time_now = g_date_time_new_now_local ();

  GtkRoot *dialog = gtk_widget_get_root (labels[0]);

  const char *hour_text = gtk_label_get_text (GTK_LABEL (labels[0]));
  const char *minute_text = gtk_label_get_text (GTK_LABEL (labels[1]));
  const char *second_text = gtk_label_get_text ((GTK_LABEL (labels[2])));

  int hour = strtol (hour_text, NULL, 10);
  int min = strtol (minute_text, NULL, 10);
  int sec = strtol (second_text, NULL, 10);
  if (hour == min && min == sec && sec == 0)
    {
      return G_SOURCE_REMOVE;
    }

  long long total_seconds = hour * 3600 + min * 60 + sec;
  GDateTime *final_time = g_date_time_add_seconds (time_now, total_seconds);
  g_date_time_unref (time_now);

  GThread *timer
      = g_thread_new ("Timer thread", (gpointer)set_timer, final_time);
  g_thread_unref (timer);
  gtk_window_destroy (GTK_WINDOW (dialog));

  return G_SOURCE_CONTINUE;
}
static void
timer_dialog (GtkWidget *main, gpointer data)
{
  GtkWidget *dialog = gtk_window_new ();
  GtkWidget *grid_timer = gtk_grid_new ();
  GtkWidget *button;
  GtkWidget *label_timer;
  gpointer *label_data = g_new (gpointer, 3);
  gtk_window_set_title (GTK_WINDOW (dialog), "Hẹn giờ");
  for (size_t i = 0; i < 3; i++)
    {
      gpointer *callback_data = g_new (gpointer, 3);
      button = gtk_button_new_with_label ("+");
      gtk_grid_attach (GTK_GRID (grid_timer), button, i, 0, 1, 1);
      label_timer = gtk_label_new ("0");

      callback_data[0] = label_timer;
      callback_data[1] = GINT_TO_POINTER (i);

      g_signal_connect (button, "clicked", G_CALLBACK (timer_on_clicked),
                        callback_data);

      gtk_grid_attach (GTK_GRID (grid_timer), label_timer, i, 1, 1, 1);

      button = gtk_button_new_with_label ("-");
      g_signal_connect (button, "clicked", G_CALLBACK (timer_on_clicked),
                        callback_data);
      gtk_grid_attach (GTK_GRID (grid_timer), button, i, 2, 1, 1);

      label_data[i] = label_timer;
    }

  button = gtk_button_new_with_label ("Hẹn giờ");
  gtk_grid_attach (GTK_GRID (grid_timer), button, 0, 3, 3, 1);

  g_signal_connect (button, "clicked", G_CALLBACK (get_timer_time),
                    label_data);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_child (GTK_WINDOW (dialog), grid_timer);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 200, 200);
  gtk_widget_set_halign (grid_timer, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (grid_timer, GTK_ALIGN_CENTER);
  gtk_window_present (GTK_WINDOW (dialog));
}
static void
change_dialogue (GtkWidget *button, gpointer data)
{
  GtkWidget *text_view = GTK_WIDGET (data);
  GtkTextBuffer *text_buffer
      = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter (text_buffer, &start);
  gtk_text_buffer_get_end_iter (text_buffer, &end);
  const char *current_string
      = gtk_text_buffer_get_text (text_buffer, &start, &end, TRUE);
  srand (time (NULL));
  int r = 0;
  while (TRUE)
    {
      r = rand () % (sizeof (dialogue_string) / sizeof (dialogue_string[0]));
      if (g_strcmp0 (current_string, dialogue_string[r]) == 0)
        {
          continue;
        }
      else
        {
          gtk_text_buffer_set_text (text_buffer, dialogue_string[r], -1);
          break;
        }
    }
  gtk_text_buffer_get_start_iter (text_buffer, &start);
  gtk_text_buffer_get_end_iter (text_buffer, &end);
  gtk_text_buffer_apply_tag_by_name (text_buffer, "font", &start, &end);
}
static void
app_activate (GtkApplication *app)
{
  GtkWidget *window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Helper");
  gtk_window_set_default_size (GTK_WINDOW (window), WINDOW_SIZE_WIDTH,
                               WINDOW_SIZE_HEIGHT);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_window_set_child (GTK_WINDOW (window), vbox);
  gtk_widget_set_margin_bottom (vbox, 20);

  GtkWidget *grid = gtk_grid_new ();
  GtkWidget *button;

  GtkWidget *picture = gtk_picture_new_for_filename (files[0]);
  gtk_widget_set_size_request (picture, WINDOW_SIZE_WIDTH,
                               WINDOW_SIZE_HEIGHT - 200);

  GtkWidget *text_view = gtk_text_view_new ();
  GtkTextIter start, end;
  GtkTextBuffer *text_buffer
      = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

  gtk_text_buffer_create_tag (text_buffer, "font", "font", "Sans 10", NULL);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (text_view), FALSE);
  gtk_widget_set_size_request (text_view, WINDOW_SIZE_WIDTH - 50,
                               TEXT_VIEW_SIZE_HEIGHT);
  gtk_text_view_set_top_margin (GTK_TEXT_VIEW (text_view),
                                TEXT_VIEW_SIZE_HEIGHT * 1 / 3);
  gtk_text_view_set_bottom_margin (GTK_TEXT_VIEW (text_view),
                                   TEXT_VIEW_SIZE_HEIGHT * 1 / 3);
  gtk_text_buffer_set_text (text_buffer, dialogue_string[0],
                            strlen (dialogue_string[0]));

  gtk_text_buffer_get_start_iter (text_buffer, &start);
  gtk_text_buffer_get_end_iter (text_buffer, &end);
  gtk_text_view_set_justification (GTK_TEXT_VIEW (text_view),
                                   GTK_JUSTIFY_CENTER);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_text_buffer_apply_tag_by_name (text_buffer, "font", &start, &end);

  gtk_box_append (GTK_BOX (vbox), picture);
  gtk_box_append (GTK_BOX (vbox), text_view);
  gtk_box_append (GTK_BOX (vbox), grid);
  gtk_widget_set_valign (grid, GTK_ALIGN_CENTER);
  gtk_widget_set_halign (grid, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top (grid, 10);
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);

  for (size_t i = 0; i < _NUMBER_OF_BUTTONS; i++)
    {
      button = gtk_button_new_with_label (main_buttons[i]);
      gtk_grid_attach (GTK_GRID (grid), button, (i % 2) * 2, i / 2, 2, 1);
      switch (i)
        {
        case _TIMER_BUTTON:
          g_signal_connect (button, "clicked", G_CALLBACK (timer_dialog),
                            NULL);
          break;
        case _QUIT_BUTTON:
          g_signal_connect_swapped (button, "clicked",
                                    G_CALLBACK (gtk_window_destroy), window);
          break;
        case _INTERACT_BUTTON:
          g_signal_connect (button, "clicked", G_CALLBACK (change_dialogue),
                            text_view);
          break;
        case _WEATHER_INFO_BUTTON:
          g_signal_connect (button, "clicked",
                            G_CALLBACK (weather_info_clicked), text_view);
          break;
        default:
          break;
        }
    }

  gtk_window_present (GTK_WINDOW (window));
}
int
main (int argc, char **argv)
{
  GtkApplication *app = gtk_application_new ("vn.edu.ntu.minh.nh",
                                             G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (app_activate), NULL);
  int status;
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return status;
}