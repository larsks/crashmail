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
	../crashmail/crashmail settings crashmail.prefs | tee $tmpfile
	grep '^CrashMail II .* started successfully' $tmpfile
	grep '^CrashMail end' $tmpfile
}

it_tosses_netmail_successfully () {
	echo This is a test netmail message. |
	../tools/crashwrite dir spool/inbound \
		fromname "Test User" fromaddr 99:99/99 \
		toname "Test Sysop" toaddr 99:99/1 \
		subject "Test netmail message" \
		text /dev/stdin

	../crashmail/crashmail settings crashmail.prefs toss

	test -f areas/netmail/2.msg
}

it_tosses_echos_successfully () {
	echo This is a test netmail message. |
	../tools/crashwrite dir spool/inbound \
		fromname "Test User" fromaddr 99:99/99 \
		toname "Test Sysop" toaddr 99:99/1 \
		subject "Test netmail message" \
		area testarea \
		text /dev/stdin

	../crashmail/crashmail settings crashmail.prefs toss

	test -f areas/testarea/2.msg
}

it_handles_bad_packets_successfully () {
	echo This is a test netmail message. |
	../tools/crashwrite dir spool/inbound \
		fromname "Test User" fromaddr 99:99/77 \
		toname "Test Sysop" toaddr 99:99/1 \
		subject "Test netmail message" \
		area testarea \
		text /dev/stdin

	../crashmail/crashmail settings crashmail.prefs toss

	test -f areas/bad/2.msg
}

it_detects_dupes_successfully () {
	test -f 15bba400.pkt

	cp 15bba400.pkt spool/inbound
	../crashmail/crashmail settings crashmail.prefs toss
	cp 15bba400.pkt spool/inbound
	../crashmail/crashmail settings crashmail.prefs toss | tee $tmpfile
	grep 'Duplicate message in testarea' $tmpfile
}

