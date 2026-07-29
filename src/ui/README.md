# Splinter3D ui

## Bootstrap

- Doxyfile (if you want to generate documentation)

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