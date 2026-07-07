# Prototype
A quick prototype of our slicer

## Building the project

Run `scripts/build.sh` on Unix-like systems, or `scripts/build.ps1` on Windows. This script will install the dependencies and build the `splinter3D` project for you.

You can also use the docker image on any platform by running `docker compose build --no-cache && docker compose up -d`, and then connect to `http://localhost:6080/vnc.html`

## Dependencies

The `gettext` package must be installed on your system if you build via a script.
