#!/bin/sh

describe "crashmail"

before () {
	rm -f spool/inbound/*
	rm -f spool/outbound/*
	rm -rf areas
	mkdir -p areas/netmail areas/testarea areas/bad

	mkdir -p nodelist
	cat > nodelist/testlist.txt <<-EOF
	Zone,99,Test_Zone,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN
	Host,99,Test_Net,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN
	,1,Test_Host_1,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN
	,99,Test_Host_1,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN
	EOF

	cat > nodelist/cmnodelist.prefs <<-EOF
	testlist.txt
	EOF

	../tools/crashlist nodelist
}

after () {
	rm -rf nodelist
	rm -rf areas
	rm -f dupes
}


it_runs_successfully () {
	../crashmail/crashmail settings crashmail.prefs
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
		fromname "Test User" fromaddr 99:99/88 \
		toname "Test Sysop" toaddr 99:99/1 \
		subject "Test netmail message" \
		area testarea \
		text /dev/stdin

	../crashmail/crashmail settings crashmail.prefs toss

	test -f areas/bad/2.msg
}

