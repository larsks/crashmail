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

# Verify that crashmail runs without error.
it_runs_successfully () {
	$__crashmail__ | tee $tmpfile
	grep '^CrashMail II .* started successfully' $tmpfile
	grep '^CrashMail end' $tmpfile
}

# Verify that inbound mail is tossed into the
# netmail area.
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

# Verify that inbound echomail is tossed into
# the appropriate area.
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

# Verify that echomail from unlinked nodes is
# tossed into the BAD area.
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

# Verify that crashmail detects and filters
# out duplicate messages.
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


# This catches a problem in which appending new messages to a packet
# resulted in an invalid packet size.
it_merges_packets () {
	$__crashmail__ sendinfo 99:99/99
	$__crashmail__ sendlist 99:99/99

	size=$(wc -c < spool/outbound/00630063.out)
	[ "$size" -eq 971 ]
}

# This checks that crashmail can HUB route packets correctly.
# The node 88:99/99 matches the following ROUTE line in the
# test configuration:
#
#  ROUTE "88:*/*" HUB 99:99/1
#
# Mail to 88:99/1 should go to the network HOST (88:99/0)
# but mail to 88:99/99 should go to the HUB, 88:99/2.
it_routes_packets () {
	$__crashmail__ sendinfo 88:99/1
	test -f spool/outbound.058/00630000.out

	$__crashmail__ sendinfo 88:99/99
	test -f spool/outbound.058/00630002.out
}

