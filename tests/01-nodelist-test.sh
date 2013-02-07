#!/bin/sh

describe "nodelist tools"

. ./testcommon.sh

before () {
	setup_crashmail_env
}

after () {
	clean_crashmail_env
}

it_generates_nodelist_index () {
	../tools/crashlist nodelist && test -f nodelist/cmnodelist.index
}

it_finds_a_node () {
	../tools/crashlist nodelist
	../tools/crashgetnode 99:99/99 nodelist > crashgetnode.output
	diff crashgetnode.output crashgetnode-node.expected
	rm -f crashgetnode.output
}

it_finds_a_point () {
	../tools/crashlist nodelist
	../tools/crashgetnode 99:99/1.1 nodelist > crashgetnode.output
	diff crashgetnode.output crashgetnode-point.expected
	rm -f crashgetnode.output
}

