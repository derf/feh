#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;

use GD qw/:DEFAULT :cmp/;
use Test::More tests => 30;
use Time::HiRes qw/sleep/;

sub set_bg {
	my ($mode, $file) = @_;

	$file //= 'bg.png';

	ok(
		system("feh --bg-${mode} test/${file}") == 0,
		"Ran feh --bg-${mode} test/${file}"
	);
}

sub same_files {
	my ($one, $two) = @_;

	my $img_one = GD::Image->new($one);
	my $img_two = GD::Image->new($two);

	return( ! ($img_one->compare($img_two) & GD_CMP_IMAGE));
}

sub check_bg {
	my ($file) = @_;

	system("import -silent -window root /tmp/feh_${$}.png");

	ok(
		same_files("test/${file}", "/tmp/feh_${$}.png"),
		"Wallpaper is test/${file}"
	);
}

for my $mode (qw( center fill max scale tile )) {
	set_bg($mode);
	check_bg('bg_all.png');

	set_bg($mode, 'bg_500x333.png');
	check_bg("bg_500x333_${mode}.png");

	set_bg($mode, 'bg_451x500.png');
	check_bg("bg_451x500_${mode}.png");
}

unlink("/tmp/feh_${$}.png");
