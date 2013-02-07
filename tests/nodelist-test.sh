#!/bin/sh

describe "nodelist tools"

before () {
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
}

after () {
	rm -rf nodelist
	rm -f crashgetnode.output
}

it_generates_nodelist_index () {
	../tools/crashlist nodelist && test -f nodelist/cmnodelist.index
}

it_finds_a_node () {
	../tools/crashlist nodelist
	../tools/crashgetnode 99:99/99 nodelist > crashgetnode.output
	diff crashgetnode.output crashgetnode.expected
}

