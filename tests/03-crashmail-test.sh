#!/bin/sh

describe "crashmail"

. ./testcommon.sh

before () {
	setup_crashmail_env
	setup_tmpfile
}

after () {
	clean_crashmail_env
	clean_tmpfile
}

it_runs_successfully () {
	$__crashmail__ | tee $tmpfile
	grep '^CrashMail II .* started successfully' $tmpfile
	grep '^CrashMail end' $tmpfile
}

it_tosses_netmail_successfully () {
	echo This is a test netmail message. |
	$__tools__/crashwrite dir spool/inbound \
		fromname "Test User" fromaddr 99:99/999 \
		toname "Test Sysop" toaddr 99:99/1 \
		subject "Test netmail message" \
		text /dev/stdin

	$__crashmail__ toss

	test -f areas/netmail/2.msg
}

it_tosses_echos_successfully () {
	echo This is a test netmail message. |
	$__tools__/crashwrite dir spool/inbound \
		fromname "Test User" fromaddr 99:99/999 \
		toname "Test Sysop" toaddr 99:99/1 \
		subject "Test netmail message" \
		area testarea \
		text /dev/stdin

	$__crashmail__ toss

	test -f areas/testarea/2.msg
}

it_handles_bad_packets_successfully () {
	echo This is a test netmail message. |
	$__tools__/crashwrite dir spool/inbound \
		fromname "Test User" fromaddr 99:99/77 \
		toname "Test Sysop" toaddr 99:99/1 \
		subject "Test netmail message" \
		area testarea \
		text /dev/stdin

	$__crashmail__ toss

	test -f areas/bad/2.msg
}

it_detects_dupes_successfully () {
	mkdir spool/temp/newpacket

	echo 'This is a test.' |
	$__tools__/crashwrite dir spool/temp/newpacket \
		fromname "Test User" fromaddr 99:99/99 \
		toname "Test Sysop" toaddr 99:99/1 \
		subject "Test netmail message" \
		area testarea \
		text /dev/stdin

	pkt=$(find spool/temp/newpacket -type f)

	cp $pkt spool/inbound
	$__crashmail__ toss
	cp $pkt spool/inbound
	$__crashmail__ toss | tee $tmpfile
	grep 'Duplicate message in testarea' $tmpfile
}

