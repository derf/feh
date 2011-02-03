#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;
use Test::Command tests => 60;

$ENV{HOME} = 'test';

my $feh = "src/feh";
my $images = 'test/ok/gif test/ok/jpg test/ok/png test/ok/pnm '
           . 'test/fail/gif test/fail/jpg test/fail/png test/fail/pnm';

my ($feh_name, $feh_version) = @ENV{'PACKAGE', 'VERSION'};

# These tests are meant to run non-interactively and without X.
# make sure they are capable of doing so.
delete $ENV{'DISPLAY'};

my $err_no_env = <<'EOF';

Unable to determine feh PACKAGE or VERSION.
This is most likely because you ran 'prove test' or 'perl test/feh.t'.
Sinc this test uses make variables and is therefore designed to be run from
the Makefile only, use 'make test' instead.

If you absolutely need to run it the other way, use
    PACKAGE=feh VERSION=1.5 ${your_command}
(with the appropiate values, of course).

EOF

if (length($feh_name) == 0 or length($feh_version) == 0) {
	die($err_no_env);
}

my $re_warning =
	qr{${feh_name} WARNING: test/fail/... \- No Imlib2 loader for that file format\n};
my $re_loadable = qr{test/ok/...};
my $re_unloadable = qr{test/fail/...};
my $re_list_action = qr{test/ok/... 16x16 \(${feh_name}\)};

my $cmd = Test::Command->new(cmd => $feh);

# Insufficient Arguments -> Usage should return failure
$cmd->exit_is_num(1, 'missing arguments return 1');
$cmd->stdout_is_eq('', 'missing arguments print usage (!stdout)');
$cmd->stderr_is_eq(<<"EOF", 'missing arguments print usage (stderr)');
${feh_name} - No loadable images specified.
Use ${feh_name} --help for detailed usage information
EOF

$cmd = Test::Command->new(cmd => "$feh --version");

$cmd->exit_is_num(0);
$cmd->stdout_is_eq("${feh_name} version ${feh_version}\n");
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(cmd => "$feh --loadable $images");

$cmd->exit_is_num(0);
$cmd->stdout_like($re_loadable);
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(
	cmd => "$feh --loadable --action 'echo touch %f' $images"
);

$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/nx_action/loadable_action');
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(
	cmd => "$feh --loadable --action ';echo touch %f' $images"
);

$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/nx_action/loadable_naction');
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(
	cmd => "$feh --unloadable --action 'echo rm %f' $images"
);

$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/nx_action/unloadable_action');
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(
	cmd => "$feh --unloadable --action ';echo rm %f' $images"
);

$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/nx_action/unloadable_naction');
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(cmd => "$feh --unloadable $images");

$cmd->exit_is_num(0);
$cmd->stdout_like($re_unloadable);
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(cmd => "$feh --list $images");

$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/list/default');
$cmd->stderr_like($re_warning);

for my $sort (qw/name filename width height pixels size format/) {
	$cmd = Test::Command->new(cmd => "$feh --list $images --sort $sort");

	$cmd->exit_is_num(0);
	$cmd->stdout_is_file("test/list/$sort");
	$cmd->stderr_like($re_warning);
}

$cmd = Test::Command->new(cmd => "$feh --list $images --sort format --reverse");

$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/list/format_reverse');
$cmd->stderr_like($re_warning);

$cmd = Test::Command->new(cmd => "$feh --customlist '%f; %h; %l; %m; %n; %p; "
                               . "%s; %t; %u; %w' $images");

$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/list/custom');
$cmd->stderr_like($re_warning);

$cmd = Test::Command->new(cmd => "$feh --list --quiet $images");
$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/list/default');
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(cmd =>
	"$feh --quiet --list --action 'echo \"%f %wx%h (%P)\" >&2' $images");

$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/list/default');
$cmd->stderr_like($re_list_action);
