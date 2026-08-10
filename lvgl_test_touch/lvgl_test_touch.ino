#include <PINS_JC4827W543.h>
#include <lvgl.h>
#include <XPT2046_Touchscreen.h> // Install resistive touch library

#define SCREEN_W 480
#define SCREEN_H 272
#define DRAW_ROWS 40

// About 38 KB: deliberately modest for a first test.
static lv_color_t drawBuffer[SCREEN_W * DRAW_ROWS];

#define TOUCH_CS 38

static SPIClass touchSPI{HSPI};
XPT2046_Touchscreen touch(TOUCH_CS, 255);
static lv_indev_t *touchInput;

void lvglFlush(lv_display_t *display, const lv_area_t *area, uint8_t *pxMap) {
  const int32_t width = area->x2 - area->x1 + 1;
  const int32_t height = area->y2 - area->y1 + 1;

  gfx->draw16bitRGBBitmap(
    area->x1,
    area->y1,
    reinterpret_cast<uint16_t *>(pxMap),
    width,
    height
  );

  lv_display_flush_ready(display);
}

//functions
static uint32_t lvglMillis() {
  return millis();
}

int median3(int16_t a, int16_t b, int16_t c) {
  if ((a >= b && a <= c) || (a >= c && a <= b)) return a;
  if ((b >= a && b <= c) || (b >= c && b <= a)) return b;
  return c;
}

bool convertTouch(int &outX, int &outY) {
  bool touchSuccess = false;
  int attempts = 0;

  while (!touchSuccess && touch.touched() && attempts < 6) {
    attempts++;

    TS_Point p0 = touch.getPoint();
    delayMicroseconds(500);
    TS_Point p1 = touch.getPoint();
    delayMicroseconds(500);
    TS_Point p2 = touch.getPoint();
    delayMicroseconds(500);
    TS_Point p3 = touch.getPoint();

    int16_t px = median3(p0.x, p1.x, p2.x);
    int16_t py = median3(p0.y, p1.y, p2.y);

    if (abs(px - p3.x) <= 50) {
      int rawX = map(py, 3775, 300, 0, 480);
      int rawY = map(px, 3880, 255, 0, 272);

      outX = constrain(rawX, 0, 479);
      outY = constrain(rawY, 0, 271);
      touchSuccess = true;
    }
    else {
      delay(10);
    }
  }

  return touchSuccess;
}

void lvglTouchRead(lv_indev_t *indev, lv_indev_data_t *data) {
  static int lastX = 0;
  static int lastY = 0;

  if (touch.touched() && convertTouch(lastX, lastY)) {
    data->point.x = lastX;
    data->point.y = lastY;
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void buttonPressed(lv_event_t *event) {
  lv_obj_t *label = static_cast<lv_obj_t *>(lv_event_get_user_data(event));
  lv_label_set_text(label, "TOUCH WORKS!");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // This is the known-working display setup from your GIF sketch.
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed");
    while (true) {}
  }

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
  gfx->fillScreen(RGB565_BLACK);
  touchSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  touch.begin(touchSPI);

  // Start LVGL and attach it to the display.
  lv_init();
  lv_tick_set_cb(lvglMillis);

  lv_display_t *display = lv_display_create(SCREEN_W, SCREEN_H);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(
    display,
    drawBuffer,
    NULL,
    sizeof(drawBuffer),
    LV_DISPLAY_RENDER_MODE_PARTIAL
  );
  lv_display_set_flush_cb(display, lvglFlush);
  touchInput = lv_indev_create();
  lv_indev_set_type(touchInput, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(touchInput, lvglTouchRead);


  // Make some unmistakably LVGL-made items.
  lv_obj_t *title = lv_label_create(lv_screen_active());
  lv_label_set_text(title, "LVGL IS ALIVE!");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x00E5FF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 28);

  lv_obj_t *panel = lv_obj_create(lv_screen_active());
  lv_obj_set_size(panel, 330, 90);
  lv_obj_align(panel, LV_ALIGN_CENTER, 0, -22);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x203050), 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(panel, 3, 0);
  lv_obj_set_style_radius(panel, 18, 0);

  lv_obj_t *message = lv_label_create(panel);
  lv_label_set_text(message, "Touch the button below");
  lv_obj_t *button = lv_button_create(lv_screen_active());
  lv_obj_set_size(button, 180, 52);
  lv_obj_align(button, LV_ALIGN_BOTTOM_MID, 0, -55);

  lv_obj_t *buttonLabel = lv_label_create(button);
  lv_label_set_text(buttonLabel, "TEST TOUCH");
  lv_obj_center(buttonLabel);
  lv_obj_add_event_cb(button, buttonPressed, LV_EVENT_CLICKED, buttonLabel);
  lv_obj_set_style_text_align(message, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(message);

  lv_obj_t *bar = lv_bar_create(lv_screen_active());
  lv_obj_set_size(bar, 260, 18);
  lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_bar_set_value(bar, 75, LV_ANIM_OFF);
}

void loop() {
  lv_timer_handler();
  delay(5);
}
