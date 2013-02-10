#!/bin/sh

describe "crashstats"

. ./testcommon.sh
. ./03-crashmail-test.sh

before () {
	setup_crashmail_env

	# Uses tests from 03-crashmail-tests.sh to generate
	# crashmail.stats before each test.
	it_tosses_netmail_successfully
	it_tosses_echos_successfully
}

after () {
	clean_crashmail_env
}

it_generates_alpha_sorted_stats () {
	test -f crashmail.stats
	$__tools__/crashstats crashmail.stats sort a
}

it_generates_message_sorted_stats () {
	test -f crashmail.stats
	$__tools__/crashstats crashmail.stats sort t
}

it_generates_byday_sorted_stats () {
	test -f crashmail.stats
	$__tools__/crashstats crashmail.stats sort m
}

it_generates_byfirstimport_sorted_stats () {
	test -f crashmail.stats
	$__tools__/crashstats crashmail.stats sort d
}

it_generates_bylastimport_sorted_stats () {
	test -f crashmail.stats
	$__tools__/crashstats crashmail.stats sort l
}

it_generates_bydupes_sorted_stats () {
	test -f crashmail.stats
	$__tools__/crashstats crashmail.stats sort u
}

