#!/bin/bash
# Compresses every man page
APP=qelectrotech.1
MAN_DIR=$(dirname $0)/files

find ${MAN_DIR} -type f -name "${APP}" -exec gzip -9n {} \;

# correct files permissions
find ${MAN_DIR} -type d -exec chmod a+rx {} \;
find ${MAN_DIR} -type f -name "${APP}.gz" -exec chmod a+r {} \;
