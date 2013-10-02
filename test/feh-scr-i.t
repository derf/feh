#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;
use autodie qw/:all/;

use Cwd;
use GD qw/:DEFAULT :cmp/;
use Test::More tests => 54;
use Time::HiRes qw/sleep/;
use X11::GUITest qw/:ALL/;

my ( $pid_xnest, $pid_twm );
my $win;
my ( $width, $height );
my $pwd     = getcwd();
my $test_id = 0;
my $scr_dir = '/tmp/feh-test-scr';

$ENV{HOME} = 'test';

sub waitfor(&) {
	my ($sub) = @_;
	my $out;
	for ( 1 .. 10 ) {
		sleep(0.05);
		$out = &{$sub};
		if ($out) {
			return $out;
		}
	}
	return 0;
}

sub feh_start {
	my ( $opts, $files ) = @_;
	my $id;

	$opts  //= q{};
	$files //= 'test/ok/png';

	StartApp("feh ${opts} ${files}");
	($id) = WaitWindowViewable(qr{^feh});

	if ( not $id ) {
		BAIL_OUT("Unable to start feh ${opts} ${files}");
	}

	if ( not SetInputFocus($id) ) {
		BAIL_OUT("Unable to focus window");
	}

	return $id;
}

sub feh_stop {
	SendKeys('{ESC}');
	if ( not waitfor { not FindWindowLike(qr{^feh}) } ) {
		BAIL_OUT("Unclosed feh window still open, cannot continue");
	}
}

sub same_files {
	my ( $one, $two ) = @_;

	my $img_one = GD::Image->new($one);
	my $img_two = GD::Image->new($two);

	if ( not defined $img_one or not defined $img_two ) {
		return 0;
	}

	return ( !( $img_one->compare($img_two) & GD_CMP_IMAGE ) );
}

sub check_scr {
	my ($file) = @_;

	system("import -silent -window root ${scr_dir}/feh_${$}.png");

	return same_files( "test/scr/${file}", "${scr_dir}/feh_${$}.png" );
}

sub test_scr {
	my ($file) = @_;
	my $msg = "X root window is test/scr/${file}";

	$test_id++;

	if ( waitfor { check_scr($file) } ) {
		pass($msg);
	}
	else {
		fail($msg);
		rename( "${scr_dir}/feh_${$}.png",
			"${scr_dir}/feh_${$}_${test_id}_${file}.png" );
	}
}

if ( FindWindowLike(qr{^feh}) ) {
	BAIL_OUT('It appears you have an open feh window. Please close it.');
}

if ( not -d $scr_dir ) {
	mkdir($scr_dir);
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

feh_start( '--fullscreen', 'test/bg/large/w/in' );
test_scr('feh_full_lwi');
feh_stop();

feh_start( q{}, 'test/bg/large/w/in' );
test_scr('feh_lwi');

SendKeys('^({RIG})');
test_scr('feh_lwi_scroll_r');

SendKeys('^({DOWN})');
test_scr('feh_lwi_scroll_rd');

SendKeys('^({RIG})');
test_scr('feh_lwi_scroll_rdr');

SendKeys('^({UP})');
test_scr('feh_lwi_scroll_rdru');

SendKeys('^({LEF})');
test_scr('feh_lwi_scroll_rdrul');

feh_stop();

feh_start( '--scale-down', 'test/bg/large/w/in' );
test_scr('feh_scaledown_lwi');
feh_stop();

feh_start( '--thumbnails', 'test/ok/gif test/ok/jpg test/ok/png test/ok/pnm' );
test_scr('thumbnail_default');
feh_stop();

feh_start( '--index --limit-width 400',
	'test/ok/gif test/ok/jpg test/ok/png test/ok/pnm' );
test_scr('index_w400');
feh_stop();

feh_start( '--fullindex --limit-width 400',
	'test/ok/gif test/ok/jpg test/ok/png test/ok/pnm' );
test_scr('index_full_w400');
feh_stop();

feh_start(
	'--index --limit-width 400 --index-info "%n\n%S\n%wx%h"',
	'test/ok/gif test/ok/jpg test/ok/png test/ok/pnm'
);
test_scr('index_full_w400');
feh_stop();

feh_start( '--index --limit-height 400',
	'test/ok/gif test/ok/jpg test/ok/png test/ok/pnm' );
test_scr('index_h400');
feh_stop();

feh_start( '--fullindex --limit-height 400',
	'test/ok/gif test/ok/jpg test/ok/png test/ok/pnm' );
test_scr('index_full_h400');
feh_stop();

feh_start( '--geometry +10+20', 'test/ok/png' );
test_scr('geometry_offset_only');
feh_stop();

feh_start( '--caption-path .tc', 'test/bg/exact/in' );
test_scr('caption_none');

SendKeys('c');
test_scr('caption_new');

SendKeys(
	'Picknick im Zenit metaphysischen Wiederscheins der astralen Kuhglocke');
test_scr('caption_while');

SendKeys('~');
test_scr('caption_done');

SendKeys('c');
test_scr('caption_while');

SendKeys( '{BKS}' x 80 );
test_scr('caption_new');

SendKeys('~');
test_scr('caption_none');

SendKeys('cfoobar{ESC}');
test_scr('caption_none');

feh_stop();

feh_start( '--info "echo \'%f\n%wx%h\'"', 'test/bg/exact/in' );
test_scr('draw_info');
feh_stop();

feh_start( '--info "echo \'%f\n%wx%h\'" --draw-tinted', 'test/bg/exact/in' );
test_scr('draw_info_tinted');
feh_stop();

feh_start( '--draw-actions --action8 "nrm \'%f\'"', 'test/bg/exact/in' );
test_scr('draw_action');
feh_stop();

feh_start( '--draw-actions --action8 "nrm \'%f\'" --draw-tinted',
	'test/bg/exact/in' );
test_scr('draw_action_tinted');
feh_stop();

feh_start( '--draw-filename', 'test/bg/exact/in' );
test_scr('draw_filename');
feh_stop();

feh_start( '--draw-filename --draw-tinted', 'test/bg/exact/in' );
test_scr('draw_filename_tinted');
feh_stop();

feh_start( '--draw-filename --draw-actions --action8 "nrm \'%f\'"',
	'test/bg/exact/in' );
test_scr('draw_filename_action');
feh_stop();

feh_start(
	'--draw-filename --draw-actions --action8 "nrm \'%f\'" --draw-tinted',
	'test/bg/exact/in' );
test_scr('draw_filename_action_tinted');
feh_stop();

feh_start( '--action8 "nrm \'%f\'"', 'test/bg/exact/in' );
test_scr('draw_nothing');

SendKeys('d');
test_scr('draw_filename');

SendKeys('da');
test_scr('draw_action');

SendKeys('d');
test_scr('draw_filename_action');

SendKeys('da');
test_scr('draw_nothing');

feh_stop();

feh_start( '--draw-tinted', 'test/bg/exact/in' );
test_scr('draw_nothing');
feh_stop();

feh_start( q{}, 'test/bg/large/h/in' );
test_scr('feh_lhi');

SendKeys('{UP}');
test_scr('feh_lhi_i');

SendKeys('{UP}');
test_scr('feh_lhi_ii');

SendKeys('^({RIG})');
test_scr('feh_lhi_iir');

SendKeys('^({RIG})');
test_scr('feh_lhi_iirr');

SendKeys('{UP}');
test_scr('feh_lhi_iirri');

SendKeys('{DOWN}');
test_scr('feh_lhi_iirrio');

feh_stop();

feh_start( q{}, 'test/bg/large/h/in' );
test_scr('feh_lhi');

SendKeys('{DOWN}');
test_scr('feh_lhi_o');

SendKeys('{DOWN}');
test_scr('feh_lhi_oo');

SendKeys('{DOWN}');
test_scr('feh_lhi_ooo');

feh_stop();

feh_start( q{}, 'test/bg/transparency' );
test_scr('feh_ibg_default');
feh_stop();

feh_start( '--image-bg checks', 'test/bg/transparency' );
test_scr('feh_ibg_default');
feh_stop();

feh_start( '--image-bg black', 'test/bg/transparency' );
test_scr('feh_ibg_black');
feh_stop();

feh_start( '--image-bg white', 'test/bg/transparency' );
test_scr('feh_ibg_white');
feh_stop();

unlink('test/bg/exact/.tc/in.txt');
rmdir('test/bg/exact/.tc');
unlink("${scr_dir}/feh_${$}.png");
