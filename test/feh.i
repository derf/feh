#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;

use Test::More tests => 3;
use X11::GUITest qw/
	FindWindowLike
	StartApp
	SendKeys
	WaitWindowViewable
/;

sub feh_start {
	my ($opts, $files) = @_;

	$opts //= q{};
	$files //= 'test/ok.png';

	StartApp("feh ${opts} ${files}");
	if (WaitWindowViewable(qr{^feh}) == 0) {
		BAIL_OUT("Unable to start feh ${opts} ${files}");
	}
}

sub test_no_win {
	my ($reason) = @_;

	for (1 .. 10) {
		sleep(0.1);
		if (FindWindowLike(qr{^feh}) == 0) {
			pass("Window closed ($reason)");
			return;
		}
	}
	fail("Window closed ($reason)");
	BAIL_OUT("unclosed window still open, cannot continue");
}

if (FindWindowLike(qr{^feh})) {
	BAIL_OUT('It appears you have an open feh window. Please close it.');
}

for my $key (qw/q x {ESC}/) {
	feh_start();
	SendKeys($key);
	test_no_win("$key pressed");
}
