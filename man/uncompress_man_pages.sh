#!/bin/bash
# Uncompresses every man page
APP=qelectrotech.1
MAN_DIR=$(dirname $0)/files

find ${MAN_DIR} -type f -name "${APP}.gz" -exec gzip -d {} \;

# correct files permissions
find ${MAN_DIR} -type d -exec chmod a+rx {} \;
find ${MAN_DIR} -type f -name "${APP}" -exec chmod a+r {} \;
