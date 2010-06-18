#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;

use Test::More tests => 42;
use Time::HiRes qw/sleep/;
use X11::GUITest qw/:ALL/;

my $win;
my ($width, $height);

sub waitfor(&) {
	my ($sub) = @_;
	my $out;
	for (1 .. 40) {
		sleep(0.05);
		$out = &{$sub};
		if ($out) {
			return $out;
		}
	}
	return 0;
}

sub feh_start {
	my ($opts, $files) = @_;
	my $id;

	$opts //= q{};
	$files //= 'test/ok.png';

	StartApp("feh ${opts} ${files}");
	($id) = WaitWindowViewable(qr{^feh});

	if (not $id) {
		BAIL_OUT("Unable to start feh ${opts} ${files}");
	}

	if (not SetInputFocus($id)) {
		BAIL_OUT("Unable to focus window");
	}

	return $id;
}

sub feh_stop {
	SendKeys('{ESC}');
	if (not waitfor { not FindWindowLike(qr{^feh}) }) {
		BAIL_OUT("Unclosed feh window still open, cannot continue");
	}
}

sub test_no_win {
	my ($reason) = @_;

	if (waitfor { not FindWindowLike(qr{^feh}) }) {
		pass("Window closed ($reason)");
	}
	else {
		fail("Window closed ($reason)");
		BAIL_OUT("unclosed window still open, cannot continue");
	}
}

sub test_win_title {
	my ($win, $wtitle) = @_;
	my $rtitle;

	if (waitfor { GetWindowName($win) eq $wtitle }) {
		pass("Window has title: $wtitle");
	}
	else {
		$rtitle = GetWindowName($win);
		fail("Window has title: $wtitle");
		diag("expected: $wtitle");
		diag("     got: $rtitle");
	}
}

if (FindWindowLike(qr{^feh})) {
	BAIL_OUT('It appears you have an open feh window. Please close it.');
}

for my $key (qw/q x {ESC}/) {
	feh_start();
	SendKeys($key);
	test_no_win("$key pressed");
}

$win = feh_start(q{}, 'test/ok.png');
test_win_title($win, 'feh [1 of 1] - test/ok.png');
feh_stop();

$win = feh_start(q{}, 'test/ok.png test/ok.jpg test/ok.gif');
test_win_title($win, 'feh [1 of 3] - test/ok.png');
SendKeys('{RIG}');
test_win_title($win, 'feh [2 of 3] - test/ok.jpg');
SendKeys('n');
test_win_title($win, 'feh [3 of 3] - test/ok.gif');
SendKeys('{SPA}');
test_win_title($win, 'feh [1 of 3] - test/ok.png');
SendKeys('{LEF}');
test_win_title($win, 'feh [3 of 3] - test/ok.gif');
SendKeys('p');
test_win_title($win, 'feh [2 of 3] - test/ok.jpg');
SendKeys('{BAC}');
test_win_title($win, 'feh [1 of 3] - test/ok.png');
SendKeys('p');
test_win_title($win, 'feh [3 of 3] - test/ok.gif');
SendKeys('{DEL}');
test_win_title($win, 'feh [1 of 2] - test/ok.png');
SendKeys('{DEL}');
test_win_title($win, 'feh [1 of 1] - test/ok.jpg');
SendKeys('{DEL}');
test_no_win("Removed all images from slideshow");

feh_start('--cycle-once', 'test/ok.png test/ok.jpg');
for (1 .. 2) {
	SendKeys('{RIG}');
}
test_no_win("--cycle-once -> window closed");

feh_start('--cycle-once --slideshow-delay 0.5',
	'test/ok.png test/ok.jpg test/ok.gif');
sleep(1.5);
test_no_win('cycle-once + slideshow-delay -> window closed');

$win = feh_start('--cycle-once --slideshow-delay -0.01',
	'test/ok.png test/ok.jpg test/ok.gif');

TODO: {
	local $TODO = '"Paused" with negative delay broken for first window';
	test_win_title($win, 'feh [1 of 3] - test/ok.png [Paused]');
}
test_win_title($win, 'feh [1 of 3] - test/ok.png');

SendKeys('h');
test_no_win('cycle-once + negative delay + [h]');

$win = feh_start(q{}, 'test/ok.png test/ok.gif test/ok.gif test/ok.jpg');
for (1 .. 2) {
	SendKeys('{END}');
	test_win_title($win, 'feh [4 of 4] - test/ok.jpg');
}
for (1 .. 2) {
	SendKeys('{HOM}');
	test_win_title($win, 'feh [1 of 4] - test/ok.png');
}

SendKeys('{PGU}');
test_win_title($win, 'feh [4 of 4] - test/ok.jpg');
SendKeys('{PGD}');
test_win_title($win, 'feh [1 of 4] - test/ok.png');
SendKeys('{PGD}');
test_win_title($win, 'feh [2 of 4] - test/ok.gif');

feh_stop();

$win = feh_start('--slideshow-delay 1', 'test/ok.png test/ok.gif test/ok.jpg');
sleep(1.7);
test_win_title($win, 'feh [3 of 3] - test/ok.jpg');
SendKeys('h');
test_win_title($win, 'feh [3 of 3] - test/ok.jpg [Paused]');
SendKeys('{RIG}');
test_win_title($win, 'feh [1 of 3] - test/ok.png [Paused]');
SendKeys('h');
test_win_title($win, 'feh [1 of 3] - test/ok.png');
sleep(0.8);
test_win_title($win, 'feh [2 of 3] - test/ok.gif');
feh_stop();

$win = feh_start(q{}, 'test/ok.png ' x 100);
test_win_title($win, 'feh [1 of 100] - test/ok.png');
SendKeys('{PGD}');
test_win_title($win, 'feh [6 of 100] - test/ok.png');
SendKeys('{PGD}');
test_win_title($win, 'feh [11 of 100] - test/ok.png');
SendKeys('{HOM PGU}');
test_win_title($win, 'feh [96 of 100] - test/ok.png');
feh_stop();

$win = feh_start('--thumbnails', 'test/ok.png test/ok.gif test/ok.jpg');
test_win_title($win, 'feh [thumbnail mode]');
$width = (GetWindowPos($win))[2];
is($width, 640, 'thumbnail win: Correct default size');
MoveMouseAbs(30, 30);
ClickMouseButton(M_BTN1);
($win) = WaitWindowViewable(qr{^ok\.png$});
ok($win, 'Thumbnail mode: Window opened');
SetInputFocus($win);
SendKeys('x');
ok(waitfor { not FindWindowLike(qr{^ok\.png$}) }, 'Thumbnail mode: closed');

MoveMouseAbs(90, 30);
ClickMouseButton(M_BTN1);
($win) = WaitWindowViewable(qr{^ok\.gif$});
ok($win, 'Thumbnail mode: Window opened');

MoveMouseAbs(150,30);
ClickMouseButton(M_BTN1);
($win) = WaitWindowViewable(qr{^ok\.jpg$});
ok($win, 'Thumbnail mode: Other window opened');

feh_stop();
