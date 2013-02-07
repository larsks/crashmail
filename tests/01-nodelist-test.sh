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

	cat > nodelist/pointlist.txt <<-EOF
	Boss,1:99:99/1
	Point,1,Test_Point,Test_Locale,Test_Sysop,-Unpublished-,300,IBN
	EOF

	cat > nodelist/cmnodelist.prefs <<-EOF
	testlist.txt
	pointlist.txt
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
	diff crashgetnode.output crashgetnode-node.expected
}

it_finds_a_point () {
	../tools/crashlist nodelist
	../tools/crashgetnode 99:99/1.1 nodelist > crashgetnode.output
	diff crashgetnode.output crashgetnode-point.expected
}

