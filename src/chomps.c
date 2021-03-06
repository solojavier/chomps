#include <pebble.h>

static Window *window;
static TextLayer *message_layer;
static char symbol[5];
static char message[10];

enum {
  QUOTE_KEY_MESSAGE = 0x1,
  QUOTE_KEY_FETCH = 0x2,
};

static void fetch_msg(void) {
  Tuplet fetch_tuple = TupletInteger(QUOTE_KEY_FETCH, 1);
  Tuplet message_tuple = TupletInteger(QUOTE_KEY_MESSAGE, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &fetch_tuple);
  dict_write_tuplet(iter, &message_tuple);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // refresh
  text_layer_set_text(message_layer, "Loading...");
  fetch_msg();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *message_tuple = dict_find(iter, QUOTE_KEY_MESSAGE);

  if (message_tuple) {
    text_layer_set_text(message_layer, message_tuple->value->cstring);
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
}

static void app_message_init(void) {
  // Register message handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_failed(out_failed_handler);
  // Init buffers
  app_message_open(64, 64);
  fetch_msg();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  message_layer = text_layer_create(GRect(0, 0, 150, 140));
  text_layer_set_text_color(message_layer, GColorBlack);
  text_layer_set_background_color(message_layer, GColorClear);
  text_layer_set_font(message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(message_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(message_layer));

  fetch_msg();
}

static void window_unload(Window *window) {
  text_layer_destroy(message_layer);
}

static void init(void) {
  window = window_create();
  app_message_init();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}

