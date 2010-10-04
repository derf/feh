#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;
use autodie qw/:all/;

use Cwd;
use GD qw/:DEFAULT :cmp/;
use Test::More tests => 19;
use Time::HiRes qw/sleep/;
use X11::GUITest qw/:ALL/;

my ($pid_xnest, $pid_twm);
my $win;
my ($width, $height);
my $pwd = getcwd();

sub waitfor(&) {
	my ($sub) = @_;
	my $out;
	for (1 .. 10) {
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

	return same_files("test/scr/${file}", "/tmp/feh_${$}.png");
}

sub test_scr {
	my ($file) = @_;

	ok(
		waitfor { check_scr($file) },
		"X root window is test/scr/${file}",
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
test_scr('draw_all_multi');
feh_stop();

feh_start(
	"--draw-actions --draw-filename --info 'echo foo; echo bar' "
	. '--action quux --action5 baz --action8 "nrm \'%f\'"',
	'test/bg/exact/in'
);
test_scr('draw_all_one');
feh_stop();

feh_start(
	'--fullscreen',
	'test/bg/large/w/in'
);
test_scr('feh_full_lwi');
feh_stop();

feh_start(
	q{},
	'test/bg/large/w/in'
);
test_scr('feh_lwi');

SendKeys('^({RIG})');
test_scr('feh_lwi_scroll_r');

SendKeys('^({DOW})');
test_scr('feh_lwi_scroll_rd');

SendKeys('^({RIG})');
test_scr('feh_lwi_scroll_rdr');

SendKeys('^({UP})');
test_scr('feh_lwi_scroll_rdru');

SendKeys('^({LEF})');
test_scr('feh_lwi_scroll_rdrul');

feh_stop();

feh_start(
	'--scale-down',
	'test/bg/large/w/in'
);
test_scr('feh_scaledown_lwi');
feh_stop();

feh_start(
	'--thumbnails',
	'test/ok/gif test/ok/jpg test/ok/png test/ok/pnm'
);
test_scr('thumbnail_default');
feh_stop();

feh_start(
	'--caption-path .tc',
	'test/bg/exact/in'
);
test_scr('caption_none');

SendKeys('c');
test_scr('caption_new');

SendKeys('Picknick im Zenit metaphysischen Wiederscheins der astralen Kuhglocke');
test_scr('caption_while');

SendKeys('~');
test_scr('caption_done');

SendKeys('c');
test_scr('caption_while');

SendKeys('{BKS}' x 80);
test_scr('caption_new');

SendKeys('~');
test_scr('caption_none');

SendKeys('cfoobar{ESC}');
test_scr('caption_none');

feh_stop();

unlink('test/bg/exact/.tc/in.txt');
rmdir('test/bg/exact/.tc');
unlink("/tmp/feh_${$}.png");
