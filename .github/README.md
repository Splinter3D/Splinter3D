# Prototype
A quick prototype of our slicer

## Building the project

Run `scripts/build.sh` on Unix-like systems, or `scripts/build.ps1` on Windows. This script will install the dependencies and build the `splinter3D` project for you.

You can also use the docker image on any platform.
You can build and run the version you are currently checked out on by running `docker compose build --no-cache && docker compose up -d`.
You can run the prebuilt version by running `docker compose -f docker-compose-prebuilt.yml up -d`
You can then then connect to `http://localhost:6080/vnc.html`
