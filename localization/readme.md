# Translation Update Guide

This document explains how to update and compile translations for this project.

### Code 

In order to enable translation in your code, you need to use the `gettext` library. This library provides functions to translate strings into different languages.
Place the following code at the beginning of your program to include the necessary headers:

```cpp
#ifdef __linux__
#ifdef TRANSLATE_ENABLED
#include <libintl.h>
#include <locale.h>
#define _(String) gettext(String)
#define N_(String) String
#else
#define _(String) String
#define N_(String) String
#endif
#else
#define _(String) String
#define N_(String) String
#endif
```
Use "_()" function to mark the strings that need to be translated. For example:

```cpp
std::cout << _("Hello, World!") << std::endl;
```
The macro will convert this to a call to the gettext function `gettext("Hello, World!")`.

### Prerequisites

Ensure that `gettext` is installed on your system. You may need the following utilities:
- `xgettext`: Extracts translatable strings to a `.pot` file.
- `msgfmt`: Compiles `.po` files into `.mo` files.

### Directory Structure

The translation files are organized as follows:
```
project-root/
    └── localization/
        ├── po/
        │   ├── en_US.po
        │   └── es.po
        └── mo/
            ├── en_US/
            │   └── LC_MESSAGES/
            │       └── yourapp.mo
            └── es/
                └── LC_MESSAGES/
                    └── yourapp.mo
```

### How to Update Translations

Follow these steps to update the translations whenever you add or modify translatable strings in the code:

## Step 1: Extract Translatable Strings

-  Run build command on the target GenerateTranslationPotFiles to generate the .pot files.
- This command looks for `_()` function calls in `yourapp.cpp` and extracts the strings into `yourapp.pot`.

## Step 2: Update Language Files (`.po`)

1. **Update Existing `.po` Files**: Merge any changes in the `.pot` file into each `.po` file to ensure they contain all current translatable strings.

     - Run build on the target UpdateTranslationsPoFiles to update the .po files.

2. **Translate New Strings**: Open each `.po` file (e.g., `locale/po/en_US.po` and `locale/po/es.po`) in a text editor, and fill in translations for any new or modified `msgid` entries.

## Step 3: Compile `.po` Files to `.mo`

1. **Compile**: Convert each updated `.po` file into a binary `.mo` file. The `.mo` files are used by the application at runtime.

    ```bash
    msgfmt -o locale/mo/en_US/LC_MESSAGES/yourapp.mo locale/po/en_US.po
    msgfmt -o locale/mo/es/LC_MESSAGES/yourapp.mo locale/po/es.po
    ```

## Step 4: Run the Application with Different Locales

To test the application in a specific language, set the `LANG` environment variable:

```bash
LANG=es ./yourapp
```

This will run the application in Spanish (if `es.po` has been translated and compiled).

## Alternative: Using CMake to Automate Translation Updates

1. If you prefer to update translations through CMake, use the `translations` target. Run the following commands:

    ```bash
    mkdir build
    cd build
    cmake ..
    make translations
    ```

2. This command will:
   - Extract translatable strings.
   - Update `.po` files.
   - Compile `.po` files into `.mo` files.

## Notes

- **Adding a New Language**: To add a new language, copy the `.pot` file to create a new `.po` file (e.g., `fr.po` for French), add translations, and follow Step 3 to compile.
- **Editing Translations**: When modifying translations, edit only the `.po` files. Re-run Step 3 to compile the changes.

By following these steps, you can keep translations up to date and test your application with multiple languages.