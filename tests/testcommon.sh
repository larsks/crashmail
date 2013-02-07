#!/bin/sh

setup_crashmail_env () {
	cp crashmail.prefs.in crashmail.prefs

	rm -rf spool
	mkdir -p spool/{inbound,outbound,temp,packets}

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

setup_tmpfile () {
	tmpfile=$(mktemp outputXXXXXX)
}

clean_tmpfile () {
	rm -f $tmpfile
}

clean_crashmail_env () {
	rm -rf nodelist areas spool
	rm -f dupes
}

