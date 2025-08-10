#!/usr/bin/env python3
import sys
from datetime import datetime
import urllib.request

def main(output_file, env_file):
    print(f"Updating build number and build info on server")
    url = "http://developmenttracking.lan:9180/TheCube-Core/buildnumber/getNext"
    # set timeout to 10 seconds
    try:
        with urllib.request.urlopen(url, timeout=10) as response:
            build_number = response.read().decode('utf-8').strip()
    except Exception as e:
        print(f"Error fetching build number: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Write the build number to the output file
    with open(output_file, 'w') as f:
        f.write(f'#define BUILD_NUMBER {build_number}\n')
    
    print(f"Build number {build_number} written to {output_file}")

    # Get the current date and time
    now = datetime.now()
    build_date = now.strftime("%Y-%m-%d")
    build_time = now.strftime("%H:%M:%S")

    # Get the build author from the .env file
    build_author = ""
    with open(env_file + "/.env", "r") as f:
        for line in f:
            if line.startswith("BUILD_AUTHOR="):
                build_author = line.split("=")[1].strip()
                break
    if not build_author:
        print("BUILD_AUTHOR not found in .env file", file=sys.stderr)
        sys.exit(1)
    # convert spaces in build author to %20
    build_author = build_author.replace(" ", "%20")
    build_author = build_author.replace("\"", "")
    print(f"Build author: {build_author}")

    # Update the buildinfo endpoint
    url = f"http://developmenttracking.lan:9180/TheCube-Core/buildinfo/setInfo/{build_number}/{build_date}/{build_time}/{build_author}"
    try:
        with urllib.request.urlopen(url, timeout=10) as response:
            result = response.read()
            if(result.decode('utf-8') != "true"):
                print(f"Error updating build info: {result.decode('utf-8')}", file=sys.stderr)
    except Exception as e:
        print(f"Error updating build info: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"Build info updated: {build_date} {build_time} {build_author}")
    sys.exit(0)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: get_build_number.py <output_file> <.env file location>", file=sys.stderr)
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])

# TODO: need to upload the executable file to the build info server so that we can pull it later on in case we need a specific build.