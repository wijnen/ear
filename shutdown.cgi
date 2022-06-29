#!/bin/sh

# This script is called from the web server, to shut the system down.

exec sudo shutdown -h now
