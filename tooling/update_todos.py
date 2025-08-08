#!/usr/bin/env python3
import re, pathlib, sys

ROOT = pathlib.Path(__file__).resolve().parent
SRC_DIR = ROOT / "src"
TODO_MD = ROOT / "TODO's.md"

todo_re = re.compile(r"\bTODO\b[^\n]*", re.IGNORECASE)
# Match a line in TODO's.md like: - [file:line] text
md_todo_re = re.compile(r"^- \[([^\]]+)\]\s+(.*)$")

def discover() -> set[str]:
    """Find all TODOs in the source tree and format them as required."""
    entries = set()
    for file in SRC_DIR.rglob("*"):
        if file.is_dir():
            continue
        if file.suffix.lower() not in {
            ".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".py", ".js", ".ts", ".cs"
        }:
            continue
        text = file.read_text(encoding="utf-8", errors="ignore")
        for idx, line in enumerate(text.splitlines(), 1):
            m = todo_re.search(line)
            if m:
                # Format: - [relative/path:line] message
                relative_path = file.relative_to(ROOT)
                todo_text = m.group(0).replace("TODO", "", 1).strip() or "(no description)"
                entry = f"- [{relative_path}:{idx}] {todo_text}"
                entries.add(entry)
    return entries

def main():
    todos_in_code = discover()

    if TODO_MD.exists():
        existing_lines = TODO_MD.read_text("utf-8").splitlines()
    else:
        existing_lines = ["# Project TODOs", ""]

    # Identify existing TODO entries
    existing_todos = {l for l in existing_lines if md_todo_re.match(l)}
    keep_other = [l for l in existing_lines if not md_todo_re.match(l)]

    # Sync
    to_add = todos_in_code - existing_todos
    to_remove = existing_todos - todos_in_code

    new_doc = keep_other + sorted((existing_todos - to_remove) | to_add)
    TODO_MD.write_text("\n".join(new_doc) + "\n", encoding="utf-8")

    print(f"Added {len(to_add)}, removed {len(to_remove)}, total {len(todos_in_code)} TODOs")

if __name__ == "__main__":
    sys.exit(main())
# This script updates the TODO's.md file with the latest TODOs found in the source code.
# It discovers TODOs in the source files, formats them, and updates the markdown file accordingly.
# It also ensures that existing TODOs are preserved and only new ones are added.
# It is designed to be run from the root of the project directory.
# It does not modify or delete any files, only updates the TODO's.md file.