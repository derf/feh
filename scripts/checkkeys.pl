#!/usr/bin/env perl
## Copyright © 2011 by Birte Kristina Friesel <derf@finalrewind.org>
## License: WTFPL:
##   0. You just DO WHAT THE FUCK YOU WANT TO.
use strict;
use warnings;
use 5.010;

use autodie;

my $fh;
my $keys;

my $re_struct = qr{
	struct \s __fehkey \s (?<action> [^;]+ ) ;
}x;

my $re_set = qr{
	feh_set_kb \( \& keys \. (?<action> [^ ,]+ ) \s* ,
	\s* (?<mod1> \d+ ) ,
	\s* (?<key1> [^ ,]+ ) \s* ,
	\s* (?<mod2> \d+ ) ,
	\s* (?<key2> [^ ,]+ ) \s* ,
	\s* (?<mod3> \d+ ) ,
	\s* (?<key3> [^ ,]+ ) \s* \) ;
}x;

my $re_parse_action = qr{
	if \s \( \! strcmp \( action , \s " (?<action> [^"]+ ) " \) \)
}x;

my $re_parse_conf = qr{
	cur_kb \s = \s \& keys \. (?<str> [^;]+ ) ;
}x;

my $re_man = qr{
	^ \. It \s (?<keys> .+ ) \s Bq \s (?<action> .+ ) $
}x;

my $re_skip = qr{
	^ ( action | orient ) _
}x;

open($fh, '<', 'src/options.h');
while (my $line = <$fh>) {
	if ($line =~ $re_struct) {
		$keys->{ $+{action} }->{struct} = 1;
	}
}
close($fh);

open($fh, '<', 'src/keyevents.c');
while (my $line = <$fh>) {
	if ($line =~ $re_set) {
		$keys->{ $+{action} }->{default}
			= [@+{'mod1', 'key1', 'mod2', 'key2', 'mod3', 'key3'}];
	}
	elsif ($line =~ $re_parse_action) {
		$keys->{ $+{action} }->{parse} = 1;
	}
}
close($fh);

open($fh, '<', 'man/feh.pre');
while (my $line = <$fh>) {
	if ($line =~ $re_man) {
		$keys->{ $+{action} }->{man} = 1;
	}
}
close($fh);

for my $action (sort keys %{$keys}) {
	my $k = $keys->{$action};

	if ($action =~ $re_skip) {
		next;
	}

	if (not defined $k->{struct}) {
		say "$action missing in struct";
	}
	if (not defined $k->{default}) {
		say "$action missing in defaults";
	}
	if (not defined $k->{parse}) {
		say "$action missing in parser";
	}
	if (not defined $k->{man}) {
		say "$action missing in manual";
	}
}
