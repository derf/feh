#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;

my %opts;

for my $chr ('a' .. 'z', 'A' .. 'Z') {
	$opts{$chr} = q{};
}

open(my $fh, '<', 'src/options.c') or die("Can't open options.c: $!");
while (my $line = <$fh>) {
	chomp($line);
	if ($line =~ /\{"(?<long>[^"]+)"\s*,.+,.+, (?<short>...)/) {
		if (substr($+{'short'}, 0, 1) eq '\'') {
			$opts{substr($+{'short'}, 1, 1)} = $+{'long'};
		}
		else {
			$opts{$+{'short'}} = $+{'long'};
		}
	}
}
close($fh);

foreach my $short (sort keys %opts) {
	printf(
		"%s\t%s\n",
		$short,
		$opts{$short},
	);
}
