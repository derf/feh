#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;
use autodie qw/:all/;

use Cwd;
use GD qw/:DEFAULT :cmp/;
use Test::More tests => 11;
use Time::HiRes qw/sleep/;
use X11::GUITest qw/:ALL/;

my ($pid_xnest, $pid_twm);
my $win;
my ($width, $height);
my $pwd = getcwd();

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
	$files //= 'test/ok/png';

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

sub same_files {
	my ($one, $two) = @_;

	my $img_one = GD::Image->new($one);
	my $img_two = GD::Image->new($two);

	return( ! ($img_one->compare($img_two) & GD_CMP_IMAGE));
}

sub check_scr {
	my ($file) = @_;

	system("import -silent -window root /tmp/feh_${$}.png");

	ok(
		same_files("test/scr/${file}", "/tmp/feh_${$}.png"),
		"X root window is test/scr/${file}"
	);
}

if (FindWindowLike(qr{^feh})) {
	BAIL_OUT('It appears you have an open feh window. Please close it.');
}

feh_start(
	"--draw-actions --draw-filename --info 'echo foo; echo bar' "
	. '--action quux --action5 baz --action8 "nrm \'%f\'"',
	'test/bg/exact/in test/bg/large/w/in test/bg/large/h/in'
);
check_scr('draw_all_multi');
feh_stop();

feh_start(
	"--draw-actions --draw-filename --info 'echo foo; echo bar' "
	. '--action quux --action5 baz --action8 "nrm \'%f\'"',
	'test/bg/exact/in'
);
check_scr('draw_all_one');
feh_stop();

feh_start(
	'--fullscreen',
	'test/bg/large/w/in'
);
check_scr('feh_full_lwi');
feh_stop();

feh_start(
	q{},
	'test/bg/large/w/in'
);
check_scr('feh_lwi');

SendKeys('^({RIG})');
check_scr('feh_lwi_scroll_r');

SendKeys('^({DOW})');
check_scr('feh_lwi_scroll_rd');

SendKeys('^({RIG})');
check_scr('feh_lwi_scroll_rdr');

SendKeys('^({UP})');
check_scr('feh_lwi_scroll_rdru');

SendKeys('^({LEF})');
check_scr('feh_lwi_scroll_rdrul');

feh_stop();

feh_start(
	'--scale-down',
	'test/bg/large/w/in'
);
check_scr('feh_scaledown_lwi');
feh_stop();

feh_start(
	'--thumbnails',
	'test/ok/gif test/ok/jpg test/ok/png test/ok/pnm'
);
check_scr('thumbnail_default');
feh_stop();

unlink("/tmp/feh_${$}.png");
