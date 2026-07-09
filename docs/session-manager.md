# Session Manager — Turtle

## Purpose

The Session Manager is the central coordinator for the Turtle desktop session.
It is responsible for creating, validating, running and shutting down core
desktop services such as the Window Manager and the Background Manager. The
Session Manager itself does not perform rendering; it manages lifecycle and
orchestration.

## Architecture

- The Session Manager composes independent modules (Window Manager,
  Background Renderer) and owns their lifetime via `unique_ptr`.
- Initialization and runtime responsibilities are separated: `initialize()`
  sets up services, `run()` starts the main loop, and `shutdown()` cleans up.
- Services are initialized in a deterministic order; shutdown reverses that
  order to permit safe dependency teardown.

## Startup sequence

1. Construct `SessionManager`.
2. Call `SessionManager::initialize()`.
   - Initialize `WindowManager` and validate startup.
   - Initialize `BackgroundRenderer` and validate startup.
3. On success call `SessionManager::run()` which delegates to the Window
   Manager's event loop.

## Shutdown sequence

- `SessionManager::shutdown()` tears services down in reverse initialization
  order. It ensures that partial initialization is cleaned up on failure.

## Module responsibilities

- Session Manager:
  - Orchestrates startup/shutdown and monitors component health.
  - Owns subsystem instances and is responsible for cleanup.
- Window Manager:
  - Owns X11 display/window resources and runs the primary event loop.
  - Handles drawing the wallpaper and processing input events.
- Background Renderer:
  - Loads and renders wallpaper images (Imlib2).

## Interaction with the Window Manager

- The Session Manager constructs the `WindowManager` and calls
  `initialize()` on it.
- The `WindowManager` currently owns its own rendering logic but the Session
  Manager can later be modified to supply shared services (e.g., a single
  `BackgroundRenderer` instance) to avoid duplicated state.

## Interaction with the Background Manager

- The Session Manager demonstrates background management by creating a
  `BackgroundRenderer` and verifying its ability to load the default
  wallpaper. This is currently informational; the Window Manager continues to
  render the wallpaper into the desktop window.

## Planned future services

The architecture is designed to allow easy addition of services such as:
- Panel
- Launcher
- Notification daemon
- Clipboard manager
- File manager daemon
- Network manager
- Audio manager
- Power manager
- Wallpaper service
- Autostart applications

Each service should implement a minimal lifecycle API (initialize, run,
shutdown) so the Session Manager can orchestrate them uniformly.

## Design decisions

- Composition over inheritance to keep services decoupled and testable.
- Small functions for clear behavior and easier unit testing.
- Deterministic initialization/shutdown ordering to avoid resource leaks and
  dangling services.

## Extension guide

To add a new service:
1. Add a new `unique_ptr` member to `SessionManager` for the service.
2. Implement `init_<service>()` that constructs and initializes the service.
3. Call `init_<service>()` from `initialize()` in the appropriate order.
4. Ensure `shutdown()` releases the service in reverse order.

## Documentation

This file serves as developer-facing documentation for the Session Manager.
If you want, I can also generate Doxygen-friendly comments and a more
detailed design document mapping interfaces and initialization error paths.
