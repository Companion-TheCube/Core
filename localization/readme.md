# Localization Guide

This document describes the translation workflow that exists in this repository today.

## Current Status

Translation support is currently gated in CMake behind:

* `UNIX`
* `TRANSLATE_ENABLED`

In the current `CMakeLists.txt`, `TRANSLATE_ENABLED` defaults to `ON`.

Current translation-related paths:

* source templates: `localization/po/*.pot`
* editable translations: `localization/po/<lang>.po`
* compiled translations: `localization/mo/<lang>/LC_MESSAGES/CubeCore.mo`
* runtime copy destination: `build/bin/localization/<lang>/LC_MESSAGES/CubeCore.mo`

Current languages declared in CMake:

* `en_US`
* `es`
* `fr`
* `de`
* `it`
* `ja`
* `ko`

Important detail:

* the repository currently contains translation templates, but not every language `.po` file is checked in yet
* there is also some mismatch today between checked-in template naming and the per-language `.pot` naming that the CMake merge targets expect

## Marking Strings In Code

The current codebase uses `gettext`-style macros such as `_()` and `N_()` when translation is enabled.

Pattern used in the codebase:

```cpp
#ifdef TRANSLATE_ENABLED
#include <libintl.h>
#include <locale.h>
#define _(String) gettext(String)
#define N_(String) String
#else
#define _(String) String
#define N_(String) String
#endif
```

Example:

```cpp
std::cout << _("Hello, World!") << std::endl;
```

If you introduce translatable UI strings in new code, make sure the translation macros are available in that compilation unit or header path.

## Prerequisites

You need the standard gettext tools:

* `xgettext`
* `msgmerge`
* `msgfmt`

On Debian or Ubuntu, installing `gettext` is sufficient.

## Practical Workflow

### 1. Mark new strings

Wrap user-facing strings with `_()` where appropriate.

### 2. Regenerate or update the `.pot` template

The current CMake file declares a POT-generation target named `GenrateTranslationPotFiles`.

Important detail:

* that target name is misspelled in CMake
* it is also annotated there as `# not working`

So the reliable workflow today is to use `xgettext` manually if the target does not behave as expected.

Example manual command pattern:

```bash
xgettext \
  --keyword=_ \
  --language=C++ \
  --sort-output \
  --from-code=UTF-8 \
  --package-name=CubeCore \
  --package-version=1.0.0 \
  -o localization/po/gui.cpp.pot \
  src/gui/gui.cpp
```

If you are touching multiple translation-bearing source files, regenerate the relevant template files the same way.

### 3. Create or update language `.po` files

If a language file does not exist yet, create it from the relevant template.

Example:

```bash
cp localization/po/gui.cpp.pot localization/po/es.po
```

If the `.po` file already exists, merge updated template entries into it:

```bash
msgmerge --update localization/po/es.po localization/po/gui.cpp.pot
```

The current CMake file also declares:

* `UpdateTranslationsPoFiles`

That target reflects the intended merge step, but the repo should still be treated as using a partially rough translation pipeline because the file naming and target wiring are not fully cleaned up yet.

### 4. Compile `.po` files into `.mo`

Compile the finished translation into the runtime binary format:

```bash
mkdir -p localization/mo/es/LC_MESSAGES
msgfmt -o localization/mo/es/LC_MESSAGES/CubeCore.mo localization/po/es.po
```

The current CMake file declares a target for this step:

* `GenerateTranslationMoFiles`

### 5. Make the runtime output available

When the build-side copy step runs, locale files are copied into:

```text
build/bin/localization/<lang>/LC_MESSAGES/CubeCore.mo
```

That is the runtime location `CubeCore` expects in the build output tree.

The current CMake file also declares an umbrella target:

* `update_translations`

Treat that as the intended combined target, not a guarantee that every translation workflow edge case is already polished.

## Testing

To test a locale, launch `CubeCore` with `LANG` set:

```bash
cd build/bin
LANG=es ./CubeCore
```

Use the locale code that matches your compiled `.mo` directory.

## Adding A New Language

When adding a new language:

1. create the new `.po` file from the appropriate template
2. compile a matching `.mo` file under `localization/mo/<lang>/LC_MESSAGES/`
3. add the language code to the `LANGUAGES` list in `CMakeLists.txt` if it is not already present

## Current Gaps

The translation pipeline exists, but it is not fully tidy yet:

* the POT target is currently named `GenrateTranslationPotFiles`
* the repository does not currently check in a full set of ready-to-edit `.po` files
* translation automation in CMake should be treated as implementation-in-progress rather than a polished localization toolchain
