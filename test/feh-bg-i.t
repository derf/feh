#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;
use autodie qw/:all/;

use GD qw/:DEFAULT :cmp/;
use Test::More tests => 70;
use Time::HiRes qw/sleep/;

my $scr_dir = '/tmp/feh-test-scr';
my ( $pid_xnest, $pid_twm );

$ENV{HOME} = 'test';

sub set_bg {
	my ( $mode, $file ) = @_;

	ok(
		system("feh --bg-${mode} test/bg/${file}") == 0,
		"Ran feh --bg-${mode} test/bg/${file}"
	);
}

sub same_files {
	my ( $one, $two ) = @_;

	my $img_one = GD::Image->new($one);
	my $img_two = GD::Image->new($two);

	return ( !( $img_one->compare($img_two) & GD_CMP_IMAGE ) );
}

sub check_bg {
	my ($file) = @_;

	system("import -silent -window root ${scr_dir}/feh_${$}.png");

	ok( same_files( "test/bg/${file}", "${scr_dir}/feh_${$}.png" ),
		"Wallpaper is test/bg/${file}" );
}

if ( not -d $scr_dir ) {
	mkdir($scr_dir);
}

if ( ( $pid_xnest = fork() ) == 0 ) {
	exec(qw( Xephyr -screen 500x500 :7 ));
}

sleep(0.5);

$ENV{'DISPLAY'} = ':7';

if ( ( $pid_twm = fork() ) == 0 ) {
	exec('twm');
}

sleep(0.5);

for my $mode (qw( center fill max scale tile )) {

	set_bg( $mode, 'exact/in' );
	check_bg('exact/out');

	for my $type (qw( exact small large )) {
		for my $orientation (qw( w h )) {

			set_bg( $mode, "${type}/${orientation}/in" );
			check_bg("${type}/${orientation}/${mode}");

		}
	}
}

kill( 15, $pid_twm );
sleep(0.2);
kill( 15, $pid_xnest );
sleep(0.2);

unlink("${scr_dir}/feh_${$}.png");
unlink('test/.fehbg');
