# HW1 Report
## Youssif Kenani – 212345532

---

# Task 1 – Framebuffer Background Pattern

## Goal
The goal of this task was to modify the raw framebuffer background by changing the RGB values of every pixel. Instead of using a simple solid color or a one-dimensional gradient, I created a two-dimensional background pattern using both the `x` and `y` pixel coordinates.

## Implementation
Inside `nanorender/src/main.cpp`, I modified the Scene Rendering loop.  
For each pixel, I calculated its `x` and `y` coordinates from the one-dimensional framebuffer index `i`.

First, I normalized the coordinates relative to the window size:

```cpp
float nx = (float)x / WIDTH;
float ny = (float)y / HEIGHT;
```

Then I generated a checker/grid pattern using integer division and modulo operations:

```cpp
int grid = ((x / 40) + (y / 40)) % 2;
```

Finally, I used these values to calculate the red, green, and blue channels:

```cpp  
uint8_t r = (uint8_t)(20 + 60 * nx + (grid ? 18 : 0));
uint8_t g = (uint8_t)(35 + 90 * ny + (grid ? 25 : 0));
uint8_t b = (uint8_t)(70 + 100 * (1.0f - nx) + (grid ? 30 : 0));
```

This produces a smooth two-dimensional gradient combined with a subtle checker-style pattern across the framebuffer.

## Result

The final result is a dark neon-style background rendered directly into the framebuffer. The pattern changes in both horizontal and vertical directions and creates a more visually interesting scene than a simple solid color.

![Task 1 Result](./assets/task1_pattern.png)

## Notes

This task helped me better understand how a framebuffer works as a one-dimensional array and how 2D pixel coordinates can be transformed into color values for rendering graphical patterns.


# Task 2 – Immediate Mode UI Widget

## Goal
The goal of this task was to add a new interactive MicroUI widget and demonstrate how Immediate Mode UI uses external state.

## Implementation
Inside `nanorender/src/main.cpp`, I added a new button inside the `mu_begin(ctx)` UI block. The button toggles a static boolean variable and prints a message to the console when clicked.

```cpp
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
```
# Result

Clicking the button toggles a text label inside the MicroUI window.
The button also prints a message to the console when clicked.

![Task 2 Result](./assets/task2_widget.png)

# Notes

This task helped me understand that in Immediate Mode UI, widgets are declared every frame, while persistent state must be stored externally.