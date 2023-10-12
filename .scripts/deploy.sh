#!/bin/bash -x

SERVER=web196@server43.webgo24.de
UPLOAD_DIR=$SERVER:/home/www/snapshots

DEPLOY_ARCHIVE="zip"

eval "$(ssh-agent -s)"

PROJECT_DIR="$PROJECT_NAME"
case $PROJECT_DIR in
	m68k-atari-mint-gcc) PROJECT_DIR=gcc ;;
	m68k-atari-mint-binutils-gdb) PROJECT_DIR=binutils ;;
esac

upload_file() {
	local from="$1"
	local to="$2"
	for i in 1 2 3
	do
		scp -o "StrictHostKeyChecking no" "$from" "$to"
		[ $? = 0 ] && return 0
		sleep 1
	done
	exit 1
}

link_file() {
	local from="$1"
	local to="$2"
	for i in 1 2 3
	do
		ssh -o "StrictHostKeyChecking no" $SERVER -- "cd www/snapshots/${PROJECT_DIR}; ln -sf $from $to"
		[ $? = 0 ] && return 0
		sleep 1
	done
	exit 1
}

OUT="${PWD}/out"
for CPU in 000 v4e 020; do
	ARCHIVE_NAME="${PROJECT_LOWER}${VERSION}-${SHORT_ID}-${CPU}.${DEPLOY_ARCHIVE}"
	ARCHIVE_PATH="${OUT}/$ARCHIVE_NAME"
	upload_file "$ARCHIVE_PATH" "${UPLOAD_DIR}/${PROJECT_DIR}/${ARCHIVE_NAME}"
	link_file "${ARCHIVE_NAME}" "${PROJECT_DIR}-${CPU}-latest.${DEPLOY_ARCHIVE}"
done
