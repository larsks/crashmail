#!/bin/sh

describe "Index nodelist"

it_generates_nodelist_index () {
	rm -f nodelist/cmnodelist.index
	../tools/crashlist nodelist && test -f nodelist/cmnodelist.index
}

it_finds_a_node () {
	../tools/crashgetnode 1:322/761 nodelist > getnode.output
	generated_sha1=$(openssl sha1 getnode.output | cut -f2 -d' ')
	expected_sha1="f99ccc63a1a80803873a644536b5524de295b371"
	test "$generated_sha1" = "$expected_sha1"
}

