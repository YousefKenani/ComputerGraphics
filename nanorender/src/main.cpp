#include <algorithm>
#include <vector>
#include <math.h>
#include "MiniFB.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "microui.h"
}
#include "ui_bridge.h"
#include "ui_renderer.h"

#define WIDTH 1600
#define HEIGHT 1200


static uint32_t g_buffer[WIDTH * HEIGHT];
static bool g_alt_background = false;

static float pattern_intensity = 40.0f;
static float color_shift = 60.0f;
static int enable_grid = 1;

struct Line {
  int x0, y0, x1, y1;
  uint32_t color;
};

static std::vector<Line> g_lines;
static bool g_is_drawing = false;
static int g_start_x = 0;
static int g_start_y = 0;
static int g_preview_x = 0;
static int g_preview_y = 0;
static int g_eraser_mode = 0;

static void draw_line_to_buffer(int x0, int y0, int x1, int y1, uint32_t color) {
  int dx = abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;

  while (true) {
    if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) {
      g_buffer[y0 * WIDTH + x0] = color;
    }

    if (x0 == x1 && y0 == y1) break;

    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

static float distance_to_line(int px, int py, const Line& line) {
  float ax = line.x0;
  float ay = line.y0;
  float bx = line.x1;
  float by = line.y1;

  float dx = bx - ax;
  float dy = by - ay;

  float length_sq = dx * dx + dy * dy;
  if (length_sq == 0) {
    float ex = px - ax;
    float ey = py - ay;
    return sqrtf(ex * ex + ey * ey);
  }

  float t = ((px - ax) * dx + (py - ay) * dy) / length_sq;
  t = std::max(0.0f, std::min(1.0f, t));

  float closest_x = ax + t * dx;
  float closest_y = ay + t * dy;

  float ex = px - closest_x;
  float ey = py - closest_y;

  return sqrtf(ex * ex + ey * ey);
}

static void erase_lines_near_point(int x, int y) {
  const float erase_radius = 15.0f;

  g_lines.erase(
      std::remove_if(g_lines.begin(), g_lines.end(),
                     [x, y, erase_radius](const Line& line) {
                        return distance_to_line(x, y, line) < erase_radius;
                      }),
      g_lines.end());
}


int main() {
  static float line_r = 255.0f;
  static float line_g = 255.0f;
  static float line_b = 255.0f;

  struct mfb_window *window =
    mfb_open_ex("MiniGUI Platform", WIDTH, HEIGHT, 0);
  if (!window)
    return 1;

  mu_Context *ctx = (mu_Context *)malloc(sizeof(mu_Context));
  mu_init(ctx);

  // Set font callbacks for microui
  ctx->text_width = [](mu_Font font, const char *str, int len) {
    return (len < 0 ? (int)strlen(str) : len) * 8;
  };
  ctx->text_height = [](mu_Font font) { return 8; };

  UIRenderer renderer(WIDTH, HEIGHT);

  // Set up char input callback for textbox input
  mfb_set_char_input_callback(
    [](struct mfb_window *w, unsigned int c) {
      extern void ui_bridge_char_input(struct mfb_window *, unsigned int);

      if (c == 'b' || c == 'B') {
        g_alt_background = !g_alt_background;
        printf("Background effect toggled: %s\n",
               g_alt_background ? "ON" : "OFF");
        return; // consume the event
      }

      ui_bridge_char_input(w, c); // pass other keys to the UI normally
    },
    window);

  while (mfb_update_events(window) != MFB_STATE_EXIT) {
    // 1. Input
    ui_bridge_input(ctx, window);

    uint32_t current_line_color =
    MFB_RGB((uint8_t)line_r, (uint8_t)line_g, (uint8_t)line_b);

    bool mouse_down = ctx->mouse_down == MU_MOUSE_LEFT;
    int mouse_x = ctx->mouse_pos.x;
    int mouse_y = ctx->mouse_pos.y;

    // Avoid drawing when mouse is over the UI panel
    bool mouse_over_ui = mouse_x < 420 && mouse_y < 620;

    if (!mouse_over_ui) {
      if (g_eraser_mode) {
        if (mouse_down) {
          erase_lines_near_point(mouse_x, mouse_y);
        }
        g_is_drawing = false;
      } else {
        if (mouse_down && !g_is_drawing) {
          g_is_drawing = true;
          g_start_x = mouse_x;
          g_start_y = mouse_y;
          g_preview_x = mouse_x;
          g_preview_y = mouse_y;
        } else if (mouse_down && g_is_drawing) {
          g_preview_x = mouse_x;
          g_preview_y = mouse_y;
        } else if (!mouse_down && g_is_drawing) {
          g_lines.push_back({g_start_x, g_start_y, g_preview_x, g_preview_y, current_line_color});
          g_is_drawing = false;
        }
      }
    }

    // 2. Scene Rendering (Background)
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
      int x = i % WIDTH;
      int y = i / WIDTH;

      float nx = (float)x / WIDTH;
      float ny = (float)y / HEIGHT;

      int cell_size = (int)(20 + pattern_intensity);
      int grid = enable_grid ? (((x / cell_size) + (y / cell_size)) % 2) : 0;

      uint8_t r, g, b;

      if (g_alt_background) {
        r = (uint8_t)(90 + color_shift * ny + (grid ? 35 : 0));
        g = (uint8_t)(25 + color_shift * (1.0f - nx) + (grid ? 20 : 0));
        b = (uint8_t)(120 + color_shift * nx + (grid ? 25 : 0));
      } else {
        r = (uint8_t)(20 + color_shift * nx + (grid ? 18 : 0));
        g = (uint8_t)(35 + color_shift * ny + (grid ? 25 : 0));
        b = (uint8_t)(70 + color_shift * (1.0f - nx) + (grid ? 30 : 0));
      }

      g_buffer[i] = MFB_RGB(r, g, b);
    }

    for (const Line& line : g_lines) {
      draw_line_to_buffer(line.x0, line.y0, line.x1, line.y1, line.color);
    }

    if (g_is_drawing) {
      draw_line_to_buffer(g_start_x, g_start_y, g_preview_x, g_preview_y, current_line_color);
    }

    // 3. UI Logic
    static float slider_val = 50.0f;
    static float number_val = 3.14f;
    static int checkbox_a = 0;
    static int checkbox_b = 1;
    static char textbox_buf[128] = "edit me";
    static bool quit_requested = false;

    mu_begin(ctx);

    // --- Widgets window ---
    if (mu_begin_window(ctx, "Widgets", mu_rect(20, 20, 360, 540))) {
      int w1[] = {-1};

      // label / text
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_label: plain static text");
      mu_text(ctx, "mu_text: word-wrapped longer text that will reflow inside "
                   "the window width automatically.");

      // button
      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "mu_button: click me")) {
        quit_requested = false; // just a reaction
      }

      // Task 2 custom widget
      static bool show_message = false;

      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "Toggle Task 2 Message")) {
        show_message = !show_message;
        printf("Task 2 button clicked!\n");
      }

      if (show_message) {
        mu_layout_row(ctx, 1, w1, 0);
        mu_label(ctx, "Task 2 message is now visible!");
      }

      // Task 5 controls: bind UI widgets to background state
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Task 5: Background Controls");

      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Pattern intensity:");
      mu_slider(ctx, &pattern_intensity, 0, 80);

      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Color shift:");
      mu_slider(ctx, &color_shift, 20, 140);

      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "Enable grid pattern", &enable_grid);


      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Task 6: Line Drawing");

      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Line Red:");
      mu_slider(ctx, &line_r, 0, 255);

      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Line Green:");
      mu_slider(ctx, &line_g, 0, 255);

      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Line Blue:");
      mu_slider(ctx, &line_b, 0, 255);

      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Current line color:");

      mu_Rect color_preview_rect = mu_layout_next(ctx);
      mu_draw_rect(ctx, color_preview_rect,
                  mu_color((int)line_r, (int)line_g, (int)line_b, 255));

      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "Eraser mode", &g_eraser_mode);

      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "Clear Lines")) {
        g_lines.clear();
      }

      

      // checkbox
      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "mu_checkbox A (off)", &checkbox_a);
      mu_checkbox(ctx, "mu_checkbox B (on)", &checkbox_b);

      // textbox
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_textbox:");
      mu_textbox(ctx, textbox_buf, sizeof(textbox_buf));

      // slider
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_slider (0-100):");
      mu_slider(ctx, &slider_val, 0, 100);

      // number
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_number (step 0.1):");
      mu_number(ctx, &number_val, 0.1f);

      // header (collapsible section)
      if (mu_header(ctx, "mu_header: collapsible section")) {
        mu_layout_row(ctx, 1, w1, 0);
        mu_label(ctx, "Content inside the header.");
      }

      // treenode
      if (mu_begin_treenode(ctx, "mu_treenode: root")) {
        mu_layout_row(ctx, 1, w1, 0);
        mu_label(ctx, "child item A");
        if (mu_begin_treenode(ctx, "nested node")) {
          mu_layout_row(ctx, 1, w1, 0);
          mu_label(ctx, "deeply nested item");
          mu_end_treenode(ctx);
        }
        mu_end_treenode(ctx);
      }

      // quit button
      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "Quit")) {
        quit_requested = true;
      }

      mu_end_window(ctx);
    }

    // --- Panel window ---
    if (mu_begin_window(ctx, "Panel Demo", mu_rect(395, 20, 380, 200))) {
      int w2[] = {-1};
      mu_layout_row(ctx, 1, w2, 120);
      mu_begin_panel(ctx, "scrollable panel");
      int wp[] = {-1};
      for (int i = 1; i <= 12; i++) {
        mu_layout_row(ctx, 1, wp, 0);
        char line[32];
        snprintf(line, sizeof(line), "Panel row %d", i);
        mu_label(ctx, line);
      }
      mu_end_panel(ctx);
      mu_end_window(ctx);
    }

    // --- Popup demo window ---
    if (mu_begin_window(ctx, "Popup Demo", mu_rect(395, 235, 380, 80))) {
      int w3[] = {-1};
      mu_layout_row(ctx, 1, w3, 0);
      if (mu_button(ctx, "Open popup")) {
        mu_Container *popup = mu_get_container(ctx, "my popup");
        popup->rect = mu_rect(ctx->mouse_pos.x, ctx->mouse_pos.y, 260, 84);
        popup->open = 1;
        ctx->hover_root = ctx->next_hover_root = popup;
        mu_bring_to_front(ctx, popup);
      }
      int popup_opt = MU_OPT_POPUP | MU_OPT_NORESIZE | MU_OPT_NOSCROLL |
                      MU_OPT_NOTITLE | MU_OPT_CLOSED;
      if (mu_begin_window_ex(ctx, "my popup", mu_rect(0, 0, 260, 84),
                             popup_opt)) {
        int wp[] = {-1};
        mu_layout_row(ctx, 1, wp, 0);
        mu_label(ctx, "mu_popup: click outside to close");
        if (mu_button(ctx, "Close")) {
          mu_get_current_container(ctx)->open = 0;
        }
        mu_end_window(ctx);
      }
      mu_end_window(ctx);
    }

    mu_end(ctx);

    if (quit_requested) {
      mfb_close(window);
      break;
    }

    // 4. UI Rendering
    renderer.render(ctx, g_buffer);

    // 5. Display
    mfb_update_state state = mfb_update_ex(window, g_buffer, WIDTH, HEIGHT);
    if (state < 0)
      break;

    // Cap FPS (optional, minifb has built-in sync)
    mfb_wait_sync(window);
  }

  mfb_close(window);
  free(ctx);
  return 0;
}
