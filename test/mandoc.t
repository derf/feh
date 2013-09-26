#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;

use Test::More tests => 3;

SKIP: {
	qx{mandoc -V};

	if ( $? != 0 ) {
		diag('mandoc not installed, test skipped. This is NOT fatal.');
		skip( 'mandoc not installed', 3 );
	}

	for my $file ( 'feh', 'feh-cam', 'gen-cam-menu' ) {
		qx{mandoc -Tlint man/${file}.1};
		is( $?, 0, "${file}.1: Valid mdoc syntax" );
	}
}
