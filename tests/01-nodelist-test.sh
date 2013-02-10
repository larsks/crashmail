#!/bin/sh

describe "nodelist"

. ./testcommon.sh

before () {
	setup_crashmail_env
}

after () {
	clean_crashmail_env
}

it_generates_nodelist_index () {
	$__tools__/crashlist nodelist && test -f nodelist/cmnodelist.index
}

it_finds_a_node () {
	$__tools__/crashlist nodelist
	$__tools__/crashgetnode 99:99/99 nodelist > crashgetnode.output
	diff crashgetnode.output $__topdir__/crashgetnode-node.expected
}

it_finds_a_point () {
	$__tools__/crashlist nodelist
	$__tools__/crashgetnode 99:99/1.1 nodelist > crashgetnode.output
	diff crashgetnode.output $__topdir__/crashgetnode-point.expected
}

