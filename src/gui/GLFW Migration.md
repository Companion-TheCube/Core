# GLFW Migration Notes

## Current State

- The GUI windowing and input path now uses `GLFW`.
- The renderer thread owns the `GLFWwindow*` and OpenGL context.
- The GUI event loop consumes backend-neutral `CubeEvent` values from a blocking thread-safe queue.
- Input dispatch no longer depends on backend event payloads or numeric key-code alignment.

## Event Model

- `CubeEventType` describes the incoming event category.
- `CubeKey` and `CubeMouseButton` carry backend-neutral key and button identifiers.
- `SpecificEventTypes` remains in place for GUI wiring such as `KEYPRESS_A` and `DRAG_Y`.
- `EventManager` now synthesizes drag events from pointer movement and button state instead of polling global backend input state.

## Renderer Behavior

- Window size: `720x720`
- Borderless window via `GLFW_DECORATED = false`
- Context hints: depth `24`, stencil `8`, samples `4`
- VSync enabled with `glfwSwapInterval(1)`
- Frame pacing kept near 30 FPS with explicit sleep-until timing
- Framebuffer resize callback updates `glViewport`

## Runtime Notes

- Linux session startup now preserves existing `DISPLAY` or `WAYLAND_DISPLAY`.
- `DISPLAY=:0` is only applied as a fallback when neither session variable is present.
- Touch input is expected to reach the application through the compositor's primary-pointer path used for mouse events.

## Validation Checklist

- Launch under X11
- Launch under Wayland
- Verify menu navigation, drag/scroll behavior, popup buttons, and slider dragging
- Verify cursor visibility behavior from `SHOW_MOUSE_POINTER`
- Verify clean shutdown from normal stop and window close
