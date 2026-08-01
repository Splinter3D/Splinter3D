# General Architecture

Splinter3D is organized into layers. The fundamental rule: **a layer depends only on the layers below it**, never the other way around.

In `src/`:
```
app/            orchestration, startup, assembly
   ↓
ui/             presentation (windows, panels, widgets)
   ↓
application/    logic orchestration (commands, controllers, services)
   ↓
core/           UI ↔ logic mediation, central state, events
   ↓
geometry/       business domain (mesh, split, transform...)
rendering/      2D/3D display
platform/       OS-related concerns (files, config, threads...)
```

`geometry/`, `rendering/`, and `platform/` are at the same level: they are independent domains, each depending only on `core/` (and nothing above it).

## Why this separation?

- **`app/`** contains *only* startup logic. If you put business logic or data managers there, the layer loses its purpose and becomes a catch-all.
- **`ui/`** must never call `geometry/` or `rendering/` directly. It goes through `application/` (triggering a command) and observes `core/` (state, events) to update itself.
- **`core/`** is the only layer allowed to be known by both `ui/` and `application/`/`geometry/`/`rendering/`. It is the mandatory gateway, not a catch-all folder either.
- **`platform/`** never knows anything about the layers above it. A `ConfigManager` must never include a UI header.

## Auxiliary folders

- `tests/`: `unit/`, `integration/`, `end_to_end/` — mirrors the `src/` structure but tests each layer in isolation, followed by their integration.
- `third_party/`: only if an external library is vendored directly into the repo (i.e., not via a package manager). - `plugins/`: intended for future extensibility; empty for now.

## Convention

Each top-level subdirectory has its own `README.md` explaining what goes there and why.

## Doxygen

### How to Generate Documentation

By using Doxygen, you need to use a specific commenting style in your code. Here are some examples of how to comment your code for Doxygen:

```cpp
/**
 * @brief This is a brief description of the function.
 *
 * This is a more detailed description of the function, explaining its purpose and usage.
 *
 * @param param1 Description of the first parameter.
 * @param param2 Description of the second parameter.
 * @return Description of the return value.
 * @code{.cpp}
 * // some code
 * @endcode
 */
```

Common Doxygen tags in C++:

| Tag              | Purpose |
|------------------|---------|
| @brief           | Short description |
| @param           | Function parameter |
| @return          | Return value |
| @code / @endcode | Code examples |
| @note            | Additional information |
| @warning         | Warning |
| @see             | Reference to another class/function |
| @ingroup         | Documentation grouping |

Then you can generate the documentation, running the following command:

```bash
doxygen Doxyfile
```

**Assure that the Doxyfile is properly configured for your project.**

Example of a Doxyfile configuration:
```Doxyfile
PROJECT_NAME = "Splinter3D"

INPUT = widgets

RECURSIVE = YES

FILE_PATTERNS = *.hpp *.cpp

GENERATE_HTML = YES

OUTPUT_DIRECTORY = docs

EXTRACT_PRIVATE = NO

EXTRACT_PUBLIC = YES

MARKDOWN_SUPPORT = YES

SOURCE_BROWSER = YES
```