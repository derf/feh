#!/usr/bin/env perl
## Copyright © 2010 by Birte Kristina Friesel <derf@finalrewind.org>
## License: WTFPL <http://sam.zoy.org/wtfpl>
use strict;
use warnings;

my $options;

open(my $c_fh, '<', 'src/options.c') or die("Can't read options.c: $!");
while (my $line = <$c_fh>) {

	if ($line =~ /\{"(?<long>[\w-]+)"\s*,.*,\s*(?:'(?<short>.)'|(?<short>\d+))\}/o) {
		push(@{$options->{$+{long}}}, ['source', $+{short}]);
	}
}
close($c_fh);

open(my $h_fh, '<', 'src/help.raw') or die("Can't read help.raw: $!");
while (my $line = <$h_fh>) {

	if ($line =~ /^ (?:\-(?<short>.), |\s*)--(?<long>[\w-]+) /) {
		push(@{$options->{$+{long}}}, ['help', $+{short}]);
	}
}
close($h_fh);

open(my $man_fh, '<', 'man/feh.1') or die("Can't read feh.1: $!");
while (my $line = <$man_fh>) {

	if ($line =~ /^\.It Cm (?:-(?<short>.) , )?--(?<long>[\w-]+)/) {
		push(@{$options->{$+{long}}}, ['manual', $+{short}]);
	}

}
close($man_fh);

foreach my $option (keys %{$options}) {
	my $last;
	my $count = 0;

	if ($option =~ / ^ action\d | help /x) {
		next;
	}

	foreach my $source (@{$options->{$option}}) {
		my $name = $source->[0];
		my $short = $source->[1] // '';
		$short = '' if ($short =~ /^\d+$/);

		if (not defined $last) {
			$last = $short;
		}

		if ($last ne $short) {
			last;
		}

		$last = $short;
		$count++;
	}

	if ($count == 3) {
		next;
	}

	foreach my $source (@{$options->{$option}}) {
		my $name = $source->[0];
		my $short = $source->[1] // '';
		print "$option: $name ($short)\n";
	}
}
