#!/bin/sh

describe "crashstats"

it_generates_alpha_sorted_stats () {
	test -f crashmail.stats
	../tools/crashstats crashmail.stats sort a
}

it_generates_message_sorted_stats () {
	test -f crashmail.stats
	../tools/crashstats crashmail.stats sort t
}

it_generates_byday_sorted_stats () {
	test -f crashmail.stats
	../tools/crashstats crashmail.stats sort m
}

it_generates_byfirstimport_sorted_stats () {
	test -f crashmail.stats
	../tools/crashstats crashmail.stats sort d
}

it_generates_bylastimport_sorted_stats () {
	test -f crashmail.stats
	../tools/crashstats crashmail.stats sort l
}

it_generates_bydupes_sorted_stats () {
	test -f crashmail.stats
	../tools/crashstats crashmail.stats sort u
}

