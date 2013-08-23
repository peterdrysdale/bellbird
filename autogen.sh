#!/bin/sh
mkdir -p "m4" || exit 1
autoreconf --install $@ || exit 1
