#!/usr/bin/env perl
use strict;
use warnings;
use 5.010;

no if $] >= 5.018, warnings => 'experimental::smartmatch';

use Cwd;
use Test::More tests => 103;
use Time::HiRes qw/sleep/;
use X11::GUITest qw/:ALL/;

my $win;
my ( $width, $height );
my $pwd = getcwd();

$ENV{HOME} = 'test';

sub waitfor(&) {
	my ($sub) = @_;
	my $out;
	for ( 1 .. 40 ) {
		sleep(0.05);
		$out = &{$sub};
		if ($out) {
			return $out;
		}
	}
	return 0;
}

sub feh_start {
	my ( $opts, $files ) = @_;
	my $id;

	$opts  //= q{};
	$files //= 'test/ok/png';

	StartApp("feh ${opts} ${files}");
	($id) = WaitWindowViewable(qr{^feh});

	if ( not $id ) {
		BAIL_OUT("Unable to start feh ${opts} ${files}");
	}

	if ( not SetInputFocus($id) ) {
		BAIL_OUT("Unable to focus window");
	}

	return $id;
}

sub feh_stop {
	SendKeys('{ESC}');
	if ( not waitfor { not FindWindowLike(qr{^feh}) } ) {
		BAIL_OUT("Unclosed feh window still open, cannot continue");
	}
}

sub test_no_win {
	my ($reason) = @_;

	if ( waitfor { not FindWindowLike(qr{^feh}) } ) {
		pass("Window closed ($reason)");
	}
	else {
		fail("Window closed ($reason)");
		BAIL_OUT("unclosed window still open, cannot continue");
	}
}

sub test_win_title {
	my ( $win, $wtitle ) = @_;
	my $rtitle;

	if ( waitfor { GetWindowName($win) eq $wtitle } ) {
		pass("Window has title: $wtitle");
	}
	else {
		$rtitle = GetWindowName($win);
		fail("Window has title: $wtitle");
		diag("expected: $wtitle");
		diag("     got: $rtitle");
	}
}

sub slurp {
	my ($file) = @_;
	my $ret;
	local $/ = undef;
	open( my $fh, '<', $file ) or die("Can't open $file: $!");
	$ret = <$fh>;
	close($fh) or die("Can't close $file: $!");
	return ($ret);
}

if ( FindWindowLike(qr{^feh}) ) {
	BAIL_OUT('It appears you have an open feh window. Please close it.');
}

for my $key (qw/q x {ESC}/) {
	feh_start();
	SendKeys($key);
	test_no_win("$key pressed");
}

$win = feh_start( q{}, 'test/ok/png' );
test_win_title( $win, 'feh [1 of 1] - test/ok/png' );
feh_stop();

$win = feh_start( q{}, 'test/ok/png test/ok/jpg test/ok/gif' );
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('{RIG}');
test_win_title( $win, 'feh [2 of 3] - test/ok/jpg' );
SendKeys('n');
test_win_title( $win, 'feh [3 of 3] - test/ok/gif' );
SendKeys('{SPA}');
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('{LEF}');
test_win_title( $win, 'feh [3 of 3] - test/ok/gif' );
SendKeys('p');
test_win_title( $win, 'feh [2 of 3] - test/ok/jpg' );
SendKeys('{BAC}');
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('p');
test_win_title( $win, 'feh [3 of 3] - test/ok/gif' );
SendKeys('{DEL}');
test_win_title( $win, 'feh [2 of 2] - test/ok/jpg' );
SendKeys('{DEL}');
test_win_title( $win, 'feh [1 of 1] - test/ok/png' );
SendKeys('{DEL}');
test_no_win("Removed all images from slideshow");

$win = feh_start( '--title \'feh %m %u/%l %n\'',
	'test/ok/png test/ok/jpg test/ok/gif' );
test_win_title( $win, 'feh slideshow 1/3 png' );
SendKeys('{RIG}');
test_win_title( $win, 'feh slideshow 2/3 jpg' );
feh_stop();

feh_start( '--on-last-slide=quit', 'test/ok/png test/ok/jpg' );
for ( 1 .. 2 ) {
	SendKeys('{RIG}');
}
test_no_win("--on-last-slide=quit -> window closed");

feh_start(
	'--on-last-slide=quit --slideshow-delay 0.5',
	'test/ok/png test/ok/jpg test/ok/gif'
);
sleep(1.5);
test_no_win('on-last-slide=quit + slideshow-delay -> window closed');

$win = feh_start(
	'--on-last-slide=quit --slideshow-delay -0.01',
	'test/ok/png test/ok/jpg test/ok/gif'
);

test_win_title( $win, 'feh [1 of 3] - test/ok/png [Paused]' );

SendKeys('h');
test_no_win('on-last-slide=quit + negative delay + [h]');

$win = feh_start( q{}, 'test/ok/png test/ok/gif test/ok/gif test/ok/jpg' );
for ( 1 .. 2 ) {
	SendKeys('{END}');
	test_win_title( $win, 'feh [4 of 4] - test/ok/jpg' );
}
for ( 1 .. 2 ) {
	SendKeys('{HOM}');
	test_win_title( $win, 'feh [1 of 4] - test/ok/png' );
}

SendKeys('{PGU}');
test_win_title( $win, 'feh [4 of 4] - test/ok/jpg' );
SendKeys('{PGD}');
test_win_title( $win, 'feh [1 of 4] - test/ok/png' );
SendKeys('{PGD}');
test_win_title( $win, 'feh [2 of 4] - test/ok/gif' );

feh_stop();

$win
  = feh_start( '--slideshow-delay 1', 'test/ok/png test/ok/gif test/ok/jpg' );
sleep(1.7);
test_win_title( $win, 'feh [3 of 3] - test/ok/jpg' );
SendKeys('h');
test_win_title( $win, 'feh [3 of 3] - test/ok/jpg [Paused]' );
SendKeys('{RIG}');
test_win_title( $win, 'feh [1 of 3] - test/ok/png [Paused]' );
SendKeys('h');
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
sleep(0.8);
test_win_title( $win, 'feh [2 of 3] - test/ok/gif' );
feh_stop();

$win = feh_start(
	'--action3 ";echo foo" --action7 "echo foo" '
	  . '--action8 ";touch feh_test_%u_%l" --action9 "rm feh_test_%u_%l"',
	'test/ok/png test/ok/gif test/ok/jpg'
);
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('3');
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('7');
test_win_title( $win, 'feh [2 of 3] - test/ok/gif' );
SendKeys('8');
test_win_title( $win, 'feh [2 of 3] - test/ok/gif' );
ok( -e 'feh_test_2_3',
	'feh action created file with correct format specifiers' );
SendKeys('9');
ok( waitfor { not -e 'feh_test_2_3' }, 'feh action removed file' );
test_win_title( $win, 'feh [3 of 3] - test/ok/jpg' );
feh_stop();

# .config/feh/keys
# Action Unbinding + non-conflicting none/shift/control/meta modifiers

$ENV{XDG_CONFIG_HOME} = 'test/config/keys';

$win = feh_start(
	'--action1 "touch a1" --action2 "touch a2" '
	  . '--action3 "touch a3" --action4 "touch a4" '
	  . '--action5 "touch a5" --action6 "touch a6" ',
	'test/ok/png test/ok/jpg test/ok/pnm'
);
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('6');
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('{RIG}');
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('a');
test_win_title( $win, 'feh [2 of 3] - test/ok/jpg' );
SendKeys('b');
test_win_title( $win, 'feh [3 of 3] - test/ok/pnm' );
SendKeys('c');
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('d');
test_win_title( $win, 'feh [3 of 3] - test/ok/pnm' );
SendKeys('e');
test_win_title( $win, 'feh [2 of 3] - test/ok/jpg' );
SendKeys('f');
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('1');
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
SendKeys('x');
ok( waitfor { -e 'a1' }, 'action 1 = X ok' );
SendKeys('X');
ok( waitfor { -e 'a2' }, 'action 2 = Shift+X ok' );
SendKeys('^(x)');
ok( waitfor { -e 'a3' }, 'action 3 = Ctrl+X ok' );
SendKeys('^(X)');
ok( waitfor { -e 'a4' }, 'action 4 = Ctrl+Shift+X ok' );
SendKeys('%(x)');
ok( waitfor { -e 'a5' }, 'action 5 = Alt+X ok' );

for my $f (qw(a1 a2 a3 a4 a5)) {
	unlink($f);
}
feh_stop();

$ENV{XDG_CONFIG_HOME} = 'test/config/themes';

$win = feh_start( '-Ttest_general', 'test/ok/png test/ok/jpg' );
SendKeys('1');
ok( waitfor { -e 'a1' }, 'theme: action 1 okay' );
unlink('a1');
feh_stop();

$win = feh_start( '-Ttest_general --action1 "touch c1"',
	'test/ok/png test/ok/jpg' );
SendKeys('1');
ok( waitfor { -e 'c1' }, 'theme: commandline overrides theme' );
unlink('c1');
feh_stop();

$win = feh_start( '-Ttest_multiline', 'test/ok/png test/ok/jpg' );
SendKeys('1');
ok( waitfor { -e 'a1' }, 'multiline theme: first line ok' );
SendKeys('2');
ok( waitfor { -e 'a2' }, 'multiline theme: second line ok' );
SendKeys('3');
ok( waitfor { -e 'a3' }, 'multiline theme: last line ok' );
for my $f (qw(a1 a2 a3)) {
	unlink($f);
}
feh_stop();

delete $ENV{XDG_CONFIG_HOME};

$win = feh_start( q{}, 'test/ok/png ' x 100 );
test_win_title( $win, 'feh [1 of 100] - test/ok/png' );
SendKeys('{PGD}');
test_win_title( $win, 'feh [6 of 100] - test/ok/png' );
SendKeys('{PGD}');
test_win_title( $win, 'feh [11 of 100] - test/ok/png' );
SendKeys('{HOM PGU}');
test_win_title( $win, 'feh [96 of 100] - test/ok/png' );
feh_stop();

$win = feh_start( '--thumbnails -H 300 -W 310 --thumb-title "feh [%l] %f"',
	'test/ok/png test/ok/gif test/ok/jpg' );
test_win_title( $win, 'feh [thumbnail mode]' );
( $width, $height ) = ( GetWindowPos($win) )[ 2, 3 ];
is( $width,  310, 'thumbnail win: Set correct width' );
is( $height, 300, 'thumbnail win: Set correct height' );
MoveMouseAbs( 30, 30 );
ClickMouseButton(M_BTN1);
($win) = WaitWindowViewable(qr{test/ok/png$});
ok( $win, 'Thumbnail mode: Window opened' );
test_win_title( $win, 'feh [3] test/ok/png' );
SetInputFocus($win);
SendKeys('x');
ok( waitfor { not FindWindowLike(qr{^ok/png$}) }, 'Thumbnail mode: closed' );

MoveMouseAbs( 90, 30 );
ClickMouseButton(M_BTN1);
($win) = WaitWindowViewable(qr{test/ok/gif$});
ok( $win, 'Thumbnail mode: Window opened' );
test_win_title( $win, 'feh [3] test/ok/gif' );

MoveMouseAbs( 150, 30 );
ClickMouseButton(M_BTN1);
($win) = WaitWindowViewable(qr{test/ok/jpg$});
ok( $win, 'Thumbnail mode: Other window opened' );
test_win_title( $win, 'feh [3] test/ok/jpg' );

feh_stop();

feh_start( '--multiwindow', 'test/ok/png test/ok/gif test/ok/jpg' );
ok( waitfor { FindWindowLike(qr{^feh - test/ok/png$}) }, 'multiwindow 1/3' );
ok( waitfor { FindWindowLike(qr{^feh - test/ok/gif$}) }, 'multiwindow 2/3' );
ok( waitfor { FindWindowLike(qr{^feh - test/ok/jpg$}) }, 'multiwindow 3/3' );

($win) = FindWindowLike(qr{^feh - test/ok/gif$});
SetInputFocus($win);
SendKeys('x');
ok( waitfor { not FindWindowLike(qr{^feh - test/ok/gif$}) }, 'win 1 closed' );
ok( FindWindowLike(qr{^feh - test/ok/png$}), 'multiwindow 1/2' );
ok( FindWindowLike(qr{^feh - test/ok/jpg$}), 'multiwindow 2/2' );

($win) = FindWindowLike(qr{^feh - test/ok/jpg$});
SetInputFocus($win);
SendKeys('x');
ok( waitfor { not FindWindowLike(qr{^feh - test/ok/jpg$}) }, 'win 2 closed' );

($win) = FindWindowLike(qr{^feh - test/ok/png$});
SetInputFocus($win);
SendKeys('x');
test_no_win('all multiwindows closed');

$win = feh_start( '--start-at test/ok/jpg',
	'test/ok/png test/ok/gif test/ok/jpg' );
test_win_title( $win, 'feh [3 of 3] - test/ok/jpg' );
SendKeys('{RIG}');
test_win_title( $win, 'feh [1 of 3] - test/ok/png' );
feh_stop();

feh_start( '--caption-path .captions', 'test/ok/png' );
SendKeys('cFoo Bar Quux Moep~');
feh_stop();
ok( -d 'test/ok/.captions', 'autocreated captions directory' );
is(
	slurp('test/ok/.captions/png.txt'),
	'Foo Bar Quux Moep',
	'Correct caption saved'
);

feh_start( '--caption-path .captions', 'test/ok/png' );
SendKeys('c');
SendKeys( '{BKS}' x length('Foo Bar Quux Moep') );
SendKeys('Foo Bar^(~)miep~');
feh_stop();
is(
	slurp('test/ok/.captions/png.txt'),
	"Foo Bar\nmiep",
	'Caption with newline + correct backspace'
);

unlink('test/ok/.captions/png.txt');
rmdir('test/ok/.captions');

$win = feh_start( '--filelist test/filelist',
	'test/ok/png test/ok/gif test/ok/png test/ok/jpg' );
SendKeys('{DEL}');
test_win_title( $win, "feh [1 of 3] - ${pwd}/test/ok/gif" );
feh_stop();

is( slurp('test/filelist'), <<"EOF", 'Filelist saved' );
${pwd}/test/ok/gif
${pwd}/test/ok/png
${pwd}/test/ok/jpg
EOF

$win = feh_start( '--filelist test/filelist', q{} );
test_win_title( $win, "feh [1 of 3] - ${pwd}/test/ok/gif" );
feh_stop();
unlink('test/filelist');

$win = feh_start('--geometry 423x232');
( undef, undef, $width, $height ) = GetWindowPos($win);
is( $width,  423, '--geometry: correct width' );
is( $height, 232, '--geometry: correct height' );
feh_stop();

$win = feh_start('--fullscreen');
( undef, undef, $width, $height ) = GetWindowPos($win);
ok( [ ( GetWindowPos($win) )[ 2, 3 ] ] ~~ [ GetScreenRes() ],
	'fullscreen uses full screen size' );
feh_stop();

$win = feh_start( q{}, 'test/ok/png ' . 'test/fail/png ' x 7 . 'test/ok/gif' );
test_win_title( $win, 'feh [1 of 9] - test/ok/png' );
SendKeys('{RIG}');
test_win_title( $win, 'feh [2 of 2] - test/ok/gif' );
SendKeys('{LEF}');
test_win_title( $win, 'feh [1 of 2] - test/ok/png' );
SendKeys('{LEF}');
test_win_title( $win, 'feh [2 of 2] - test/ok/gif' );
feh_stop();

$win = feh_start();
( undef, undef, $width, $height ) = GetWindowPos($win);
is( $width,  16, 'correct default window width' );
is( $height, 16, 'correct default window height' );

ResizeWindow( $win, 25, 30 );
( undef, undef, $width, $height ) = GetWindowPos($win);

SKIP: {
	if ( not( [ $width, $height ] ~~ [ 25, 30 ] ) ) {
		skip( 'ResizeWindow failed', 2 );
	}
	PressKey('w');
	ok( waitfor { [ ( GetWindowPos($win) )[ 2, 3 ] ] ~~ [ 16, 16 ] },
		'w key resizes correctly' );
}
feh_stop();

$win = feh_start( q{}, 'test/huge.png' );
ok(
	waitfor {
		( GetWindowPos($win) )[2] == ( GetScreenRes() )[0]
		  || ( GetWindowPos($win) )[3] == ( GetScreenRes() )[1];
	},
	'Large window limited to screen size'
);
feh_stop();

$win = feh_start( '--no-screen-clip', 'test/huge.png' );
ok(
	waitfor {
		[ ( GetWindowPos($win) )[ 2, 3 ] ] ~~ [ 4000, 3000 ];
	},
	'disabled screen clip'
);
feh_stop();

# GH-35 "Custom window title crashes feh on unloadable files"
$win = feh_start( '--title "feh %h"', 'test/ok/png test/fail/png test/ok/jpg' );
SendKeys('{RIG}');
test_win_title( $win, 'feh 16' );
feh_stop();
