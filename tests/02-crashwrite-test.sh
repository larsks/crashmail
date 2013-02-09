#!/bin/sh

describe "crashwrite"

. ./testcommon.sh

before () {
	setup_crashmail_env
}

after () {
	clean_crashmail_env
}

it_generates_a_packet () {
	../tools/crashwrite DIR spool/temp \
		FROMNAME "Test Sysop" \
		FROMADDR "99:99/1" \
		TONAME "Test User" \
		TOADDR "99:99/99" \
		SUBJECT "Test Message"
}

