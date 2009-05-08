#!/bin/bash
# Dependance : png2ico
PNG_DIR="../oxygen-icons"

if [ -z "$(which png2ico 2> /dev/null)" ]; then
	echo "Error: png2ico not found."
	exit 1
fi

function generateIcon {
	echo "Generating icon from files ${1}.png in ${PNG_DIR}."
	png2ico "${1}.ico" $(find "${PNG_DIR}" -name "${1}.png" | grep -v 2[25])
}

generateIcon qelectrotech
generateIcon application-x-qet-element
generateIcon application-x-qet-project

