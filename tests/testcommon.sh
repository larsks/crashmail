#!/bin/sh

setup_sandbox () {
	__topdir__=$PWD
	__testdir__=$(mktemp -d testXXXXXX)
	__tools__=$PWD/../tools
	__crashmail__=$PWD/../crashmail/crashmail

	cd $__testdir__
}

clean_sandbox () {
	cd $__topdir__
	rm -rf $__testdir__
}

setup_crashmail_env () {
	setup_sandbox

	mkdir -p spool/{inbound,outbound,temp,packets}
	mkdir -p areas/netmail areas/testarea areas/bad
	mkdir -p nodelist

	cp $__topdir__/crashmail.prefs crashmail.prefs
	cp $__topdir__/{nodelist.txt,pointlist.txt,cmnodelist.prefs} nodelist/

	$__topdir__/../tools/crashlist nodelist
}

setup_tmpfile () {
	tmpfile=$(mktemp outputXXXXXX)
}

clean_tmpfile () {
	rm -f $tmpfile
}

clean_crashmail_env () {
	clean_sandbox
}

