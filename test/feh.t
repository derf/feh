#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;
use Test::Command tests => 73;

$ENV{HOME} = 'test';

my $feh         = "src/feh";
my $images_ok   = 'test/ok/gif test/ok/jpg test/ok/png test/ok/pnm';
my $images_fail = 'test/fail/gif test/fail/jpg test/fail/png test/fail/pnm';
my $images      = "${images_ok} ${images_fail}";
my $has_help    = 0;

my $feh_name = $ENV{'PACKAGE'};

# These tests are meant to run non-interactively and without X.
# make sure they are capable of doing so.
delete $ENV{'DISPLAY'};

my $err_no_env = <<'EOF';

Unable to determine feh PACKAGE.
This is most likely because you ran 'prove test' or 'perl test/feh.t'.
Since this test uses make variables and is therefore designed to be run from
the Makefile only, use 'make test' instead.

If you absolutely need to run it the other way, use
    PACKAGE=feh ${your_command}

EOF

if ( length($feh_name) == 0 ) {
	die($err_no_env);
}

# Imlib2 1.6+ reports JPEG file format as 'jpg', older versions use 'jpeg'.
# Determine the output format used in this version with a --customlist call.
my $list_dir = 'list';
if (qx{$feh --customlist %t test/ok/jpg} =~ m{jpg}) {
	$list_dir = 'list_imlib2_1.6';
}

my $version = qx{$feh --version};
if ( $version =~ m{ Compile-time \s switches : \s .* help }ox ) {
	$has_help = 1;
}

# Imlib2 1.8+ returns "Invalid image file" rather than "No Imlib2 loader".
# feh compiled with magic=1 returns "Does not look like an image (magic bytes missing)"
# Here, we accept all three.
my $re_warning
  = qr{${feh_name} WARNING: test/fail/... \- (Invalid image file|No Imlib2 loader for that file format|Does not look like an image \(magic bytes missing\))\n};
my $re_loadable    = qr{test/ok/...};
my $re_unloadable  = qr{test/fail/...};
my $re_list_action = qr{test/ok/... 16x16};

my $cmd = Test::Command->new( cmd => "$feh --version" );

$cmd->exit_is_num(0);
$cmd->stderr_is_eq('');

$cmd = Test::Command->new( cmd => "$feh --loadable $images" );

$cmd->exit_is_num(1);
$cmd->stdout_like($re_loadable);
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(
	cmd => "$feh --loadable --action 'echo touch %f' $images" );

$cmd->exit_is_num(1);
$cmd->stdout_is_file('test/nx_action/loadable_action');
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(
	cmd => "$feh --loadable --action ';echo touch %f' $images" );

$cmd->exit_is_num(1);
$cmd->stdout_is_file('test/nx_action/loadable_naction');
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(
	cmd => "$feh --unloadable --action 'echo rm %f' $images" );

$cmd->exit_is_num(1);
$cmd->stdout_is_file('test/nx_action/unloadable_action');
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(
	cmd => "$feh --unloadable --action ';echo rm %f' $images" );

$cmd->exit_is_num(1);
$cmd->stdout_is_file('test/nx_action/unloadable_naction');
$cmd->stderr_is_eq('');

$cmd = Test::Command->new( cmd => "$feh --unloadable $images" );

$cmd->exit_is_num(1);
$cmd->stdout_like($re_unloadable);
$cmd->stderr_is_eq('');

$cmd = Test::Command->new( cmd => "$feh --list $images" );

$cmd->exit_is_num(0);
$cmd->stdout_is_file("test/${list_dir}/default");
$cmd->stderr_like($re_warning);

for my $sort (qw/name filename width height pixels size format/) {
	$cmd = Test::Command->new( cmd => "$feh --list $images --sort $sort" );

	$cmd->exit_is_num(0);
	$cmd->stdout_is_file("test/${list_dir}/$sort");
	$cmd->stderr_like($re_warning);
}

$cmd
  = Test::Command->new( cmd => "$feh --list $images --sort format --reverse" );

$cmd->exit_is_num(0);
$cmd->stdout_is_file("test/${list_dir}/format_reverse");
$cmd->stderr_like($re_warning);

$cmd = Test::Command->new(
	cmd => "$feh --list --recursive --sort filename test/ok" );

$cmd->exit_is_num(0);

# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=813729
#$cmd->stdout_is_file("test/${list_dir}/filename_recursive");
#$cmd->stderr_is_eq('');
# dummy tests to match number of planned tests
$cmd->exit_is_num(0);
$cmd->exit_is_num(0);

$cmd = Test::Command->new( cmd => "$feh --customlist '%f; %h; %l; %m; %n; %p; "
	  . "%s; %t; %u; %w' $images" );

$cmd->exit_is_num(0);
$cmd->stdout_is_file("test/${list_dir}/custom");
$cmd->stderr_like($re_warning);

$cmd = Test::Command->new( cmd => "$feh --list --quiet $images" );
$cmd->exit_is_num(0);
$cmd->stdout_is_file("test/${list_dir}/default");
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(
	cmd => "$feh --quiet --list --action 'echo \"%f %wx%h\" >&2' $images" );

$cmd->exit_is_num(0);
$cmd->stdout_is_file("test/${list_dir}/default");
$cmd->stderr_like($re_list_action);

$cmd
  = Test::Command->new( cmd => "$feh --list --min-dimension 20x20 $images_ok" );

$cmd->exit_is_num(1);
$cmd->stdout_is_eq('');
if ($has_help) {
	$cmd->stderr_is_file('test/no-loadable-files.help');
}
else {
	$cmd->stderr_is_file('test/no-loadable-files');
}

$cmd
  = Test::Command->new( cmd => "$feh --list --max-dimension 10x10 $images_ok" );

$cmd->exit_is_num(1);
$cmd->stdout_is_eq('');
if ($has_help) {
	$cmd->stderr_is_file('test/no-loadable-files.help');
}
else {
	$cmd->stderr_is_file('test/no-loadable-files');
}

$cmd
  = Test::Command->new( cmd => "$feh --list --min-dimension 16x16 $images_ok" );

$cmd->exit_is_num(0);
$cmd->stdout_is_file("test/${list_dir}/default");
$cmd->stderr_is_eq('');

$cmd
  = Test::Command->new( cmd => "$feh --list --max-dimension 16x16 $images_ok" );

$cmd->exit_is_num(0);
$cmd->stdout_is_file("test/${list_dir}/default");
$cmd->stderr_is_eq('');

$cmd = Test::Command->new( cmd => "$feh --list test/tiny.pbm" );
$cmd->exit_is_num(0);
$cmd->stderr_is_eq('');
