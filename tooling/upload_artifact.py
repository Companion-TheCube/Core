#!/usr/bin/env python3
"""Upload the CubeCore executable to the build server.

Usage: upload_artifact.py <project_root> <build_number_header>
"""

import argparse
import os
import re
import urllib.request

UPLOAD_URL = "http://developmenttracking.lan:9180/TheCube-Core/artifacts/upload"


def parse_build_number(header_path: str) -> str:
    """Extract the build number from the given header file."""
    try:
        text = open(header_path, "r", encoding="utf-8").read()
    except OSError as exc:
        raise SystemExit(f"Failed to read {header_path}: {exc}")

    match = re.search(r"#define\s+BUILD_NUMBER\s+(\d+)", text)
    if not match:
        raise SystemExit(f"BUILD_NUMBER not found in {header_path}")
    return match.group(1)


def create_multipart(filename: str, file_path: str, boundary: str) -> bytes:
    """Create multipart/form-data body for a single file field."""
    lines = [
        f"--{boundary}",
        f'Content-Disposition: form-data; name="artifact"; filename="{filename}"',
        "Content-Type: application/octet-stream",
        "",
    ]

    body = b"\r\n".join(s.encode() for s in lines)
    with open(file_path, "rb") as f:
        body += b"\r\n" + f.read()
    body += f"\r\n--{boundary}--\r\n".encode()
    return body


def upload_artifact(root_dir: str, header_path: str) -> None:
    if os.environ.get("CUBECORE_OFFLINE") == "1":
        print("CUBECORE_OFFLINE set; skipping artifact upload")
        return

    build_number = parse_build_number(header_path)
    artifact_path = os.path.join(root_dir, "build", "bin", "CubeCore")

    if not os.path.isfile(artifact_path):
        raise SystemExit(f"Executable not found: {artifact_path}")

    artifact_name = f"CubeCore-{build_number}"
    boundary = "----CubeUploadBoundary"
    data = create_multipart(artifact_name, artifact_path, boundary)

    request = urllib.request.Request(
        UPLOAD_URL,
        data=data,
        method="POST",
        headers={"Content-Type": f"multipart/form-data; boundary={boundary}"},
    )

    try:
        with urllib.request.urlopen(request) as response:
            print(response.read().decode())
    except Exception as exc:
        raise SystemExit(f"Upload failed: {exc}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Upload compiled artifact")
    parser.add_argument(
        "project_root",
        help="Absolute path of the project's root directory",
    )
    parser.add_argument(
        "build_header",
        help="Absolute path of build_number.h",
    )
    args = parser.parse_args()

    upload_artifact(args.project_root, args.build_header)


if __name__ == "__main__":
    main()
