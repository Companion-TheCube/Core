#!/usr/bin/env python3
"""
Adds an ANSI Shadow ASCII art filename header and MIT license to every .cpp and
.h file under src/.  For files that already have the header, the copyright year
is updated to the current year when it is out of date.

Usage:
    python3 tooling/add_file_headers.py [--dry-run]
"""

import re
import sys
import subprocess
import pathlib
import argparse
from datetime import datetime

# ---------------------------------------------------------------------------
# Dependency bootstrap
# ---------------------------------------------------------------------------
try:
    import pyfiglet  # type: ignore
except ImportError:
    print("pyfiglet not found — installing via pip...")
    # Try plain install first; fall back to --break-system-packages for
    # Debian/Ubuntu externally-managed environments.
    for extra in ([], ["--break-system-packages"]):
        result = subprocess.run(
            [sys.executable, "-m", "pip", "install", "pyfiglet", "--quiet"] + extra,
            capture_output=True,
        )
        if result.returncode == 0:
            break
    else:
        print("Could not install pyfiglet automatically.")
        print("Run:  pip install pyfiglet --break-system-packages")
        sys.exit(1)
    import pyfiglet  # type: ignore  # noqa: E402

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
CURRENT_YEAR = datetime.now().year
COPYRIGHT_HOLDER = "A-McD Technology LLC"

MIT_LICENSE_TEMPLATE = """\
/*
MIT License

Copyright (c) {year} A-McD Technology LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/"""

# Box-drawing and block-element characters used by ANSI Shadow
_ART_CHARS_RE = re.compile(r"[\u2500-\u259F\u2580-\u25FF]")
# Matches the opening comment block (non-greedy, stops at first */)
_COMMENT_BLOCK_RE = re.compile(r"/\*.*?\*/", re.DOTALL)
# Matches the copyright year
_COPYRIGHT_YEAR_RE = re.compile(r"(Copyright \(c\) )(\d{4})( )")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_art_block(filename: str) -> str:
    """Return a /* ... */ comment block with ANSI Shadow art of the filename."""
    raw = pyfiglet.figlet_format(filename.upper(), font="ansi_shadow", width=10000)
    lines = [line.rstrip() for line in raw.rstrip().splitlines()]
    return "/*\n" + "\n".join(lines) + "\n*/"


def is_art_comment(block: str) -> bool:
    """Return True if a comment block contains ANSI Shadow art characters."""
    return bool(_ART_CHARS_RE.search(block))


def has_art_header(content: str) -> bool:
    """Return True when the file's first comment block is an ASCII art header."""
    m = _COMMENT_BLOCK_RE.match(content)
    return bool(m and is_art_comment(m.group(0)))


def has_mit_license(content: str) -> bool:
    return "MIT License" in content and "Copyright (c)" in content


def update_copyright_year(content: str) -> tuple[str, bool]:
    """
    Replace an outdated copyright year with CURRENT_YEAR.
    Returns (updated_content, was_changed).
    """
    m = _COPYRIGHT_YEAR_RE.search(content)
    if not m:
        return content, False
    year = int(m.group(2))
    if year >= CURRENT_YEAR:
        return content, False
    updated = content[: m.start(2)] + str(CURRENT_YEAR) + content[m.end(2) :]
    return updated, True


def peel_plain_lead_comment(content: str) -> tuple[str, str]:
    """
    If the file begins with a plain /* ... */ block (no art characters), return
    (that block, remaining content).  Otherwise return ("", content).
    """
    m = _COMMENT_BLOCK_RE.match(content)
    if not m or is_art_comment(m.group(0)):
        return "", content
    return m.group(0).rstrip("\n"), content[m.end() :].lstrip("\n")


# ---------------------------------------------------------------------------
# Per-file logic
# ---------------------------------------------------------------------------

def process_file(path: pathlib.Path, dry_run: bool) -> str:
    content = path.read_text(encoding="utf-8", errors="ignore")

    art_present = has_art_header(content)
    lic_present = has_mit_license(content)

    expected_art = make_art_block(path.name)
    lic_block = MIT_LICENSE_TEMPLATE.format(year=CURRENT_YEAR)
    changes: list[str] = []

    # --- Fix or inject the art block -----------------------------------------
    if art_present:
        m = _COMMENT_BLOCK_RE.match(content)
        current_art = m.group(0)  # type: ignore[union-attr]
        if current_art != expected_art:
            content = expected_art + content[m.end():]  # type: ignore[union-attr]
            changes.append("updated art")
    else:
        lead, body = peel_plain_lead_comment(content)
        if lic_present:
            # License already exists further in — just prepend art
            content = expected_art + "\n\n" + content
        else:
            # Neither present — build the full header, preserving any plain lead comment
            parts = [expected_art, lic_block]
            if lead:
                parts.append(lead)
            parts.append(body)
            content = "\n\n".join(p for p in parts if p)
            if not content.endswith("\n"):
                content += "\n"
            lic_present = True  # we just embedded it above
        changes.append("added art")

    # --- Fix or inject the license block -------------------------------------
    if not lic_present:
        m = _COMMENT_BLOCK_RE.match(content)
        insert_at = m.end()  # type: ignore[union-attr]
        content = content[:insert_at] + "\n\n" + lic_block + content[insert_at:]
        changes.append("added license")
    else:
        updated, year_changed = update_copyright_year(content)
        if year_changed:
            content = updated
            changes.append(f"year → {CURRENT_YEAR}")

    if not changes:
        return "ok"
    if not dry_run:
        path.write_text(content, encoding="utf-8")
    return ", ".join(changes)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Report what would change without writing any files.",
    )
    args = parser.parse_args()

    files = sorted(f for f in SRC_DIR.rglob("*") if f.suffix in (".cpp", ".h"))

    tag = " [DRY RUN]" if args.dry_run else ""
    print(f"Scanning {len(files)} files under {SRC_DIR.relative_to(ROOT)}{tag}")
    print(f"Current year: {CURRENT_YEAR}\n")

    modified = 0
    for f in files:
        result = process_file(f, dry_run=args.dry_run)
        rel = f.relative_to(ROOT)
        marker = " " if result == "ok" else "*"
        print(f"  {marker} {rel}: {result}")
        if result != "ok":
            modified += 1

    unchanged = len(files) - modified
    print(f"\nDone — {modified} modified, {unchanged} already correct.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
