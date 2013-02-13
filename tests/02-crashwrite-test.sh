#!/bin/sh

describe "crashwrite"

. ./testcommon.sh

before () {
	setup_crashmail_env
	setup_tmpfile
}

after () {
	clean_crashmail_env
	clean_tmpfile
}

it_generates_a_packet () {
	$__tools__/crashwrite DIR spool/temp \
		FROMNAME "Test Sysop" \
		FROMADDR "99:99/1" \
		TONAME "Test User" \
		TOADDR "99:99/99" \
		SUBJECT "Test Message" | tee $tmpfile
	
	pkt=$(awk '/^Writing:/ {print $2}' $tmpfile)
	test -f $pkt
}

it_generates_named_packet () {
	$__tools__/crashwrite DIR spool/temp \
		FILENAME test.pkt \
		FROMNAME "Test Sysop" \
		FROMADDR "99:99/1" \
		TONAME "Test User" \
		TOADDR "99:99/99" \
		SUBJECT "Test Message" | tee $tmpfile
	
	test -f spool/temp/test.pkt
}

