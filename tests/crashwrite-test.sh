#!/bin/sh

describe "crashwrite tool"

before () {
	mkdir -p packets
}

after () {
	rm -rf packets
}

it_generates_a_packet () {
	../tools/crashwrite DIR packets \
		FROMNAME "Test Sysop" \
		FROMADDR "99:99/1" \
		TONAME "Test User" \
		TOADDR "99:99/99" \
		SUBJECT "Test Message"
}

