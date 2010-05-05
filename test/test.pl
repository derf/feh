#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;
use Test::Command tests => 18;

my $feh = 'src/feh';
my $images = 'test/ok.* test/fail.*';

my ($feh_name, $feh_version) = @ARGV;

my $re_warning =
	qr{${feh_name} WARNING: test/fail\.... \- No Imlib2 loader for that file format\n};
my $re_loadable = qr{test/ok\....};
my $re_unloadable = qr{test/fail\....};

my $cmd = Test::Command->new(cmd => $feh);

# Insufficient Arguments -> Usage should return failure
$cmd->exit_is_num(1, 'missing arguments return 1');
$cmd->stdout_is_eq('', 'missing arguments print usage (!stdout)');
$cmd->stderr_is_eq(<<"EOF", 'missing arguments print usage (stderr)');
${feh_name} - No loadable images specified.
Use ${feh_name} --help for detailed usage information
EOF

# feh --version / feh -v
$cmd = Test::Command->new(cmd => "$feh --version");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("${feh_name} version ${feh_version}\n");
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(cmd => "$feh --loadable $images");
$cmd->exit_is_num(0);
$cmd->stdout_like($re_loadable);
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(cmd => "$feh --unloadable $images");
$cmd->exit_is_num(0);
$cmd->stdout_like($re_unloadable);
$cmd->stderr_is_eq('');

$cmd = Test::Command->new(cmd => "$feh --list $images");
$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/list');
$cmd->stderr_like($re_warning);

$cmd = Test::Command->new(cmd => "$feh --customlist '%f; %h; %l; %m; %n; %p; "
                               . "%s; %t; %u; %w' $images");
$cmd->exit_is_num(0);
$cmd->stdout_is_file('test/customlist');
$cmd->stderr_like($re_warning);
