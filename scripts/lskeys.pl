#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;

use autodie;

my @keys;

open(my $fh, '<', 'src/keyevents.c');
while (my $line = <$fh>) {
	chomp($line);
	if ($line =~ qr{
			if \s \( \! strcmp \( action , \s " (?<key> [^"]+ ) " \)\) }x) {
		push(@keys, $+{key});
	}
}
close($fh);

say join("\n", sort @keys);
