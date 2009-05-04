#!/usr/bin/perl

#
# open-roadster/crooner
#
# The simple perl script I like to use to listen to the EVK1100 Debug
# board instead of using minicom.
#
# Copyright (c) Weston Schmidt 2008
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

use strict;
use Device::SerialPort;
use Fcntl;

if( @ARGV < 1 ) {
    die "Usage:\nibus.pl serialport\n";
}

my $serial_port = $ARGV[0];

my $port = new Device::SerialPort( $serial_port, undef, undef )
    || die "Can't open $serial_port: $!\n";

$port->baudrate(9600);
$port->databits(8);
$port->parity("even");
$port->stopbits(1);
$port->read_char_time(0);
$port->read_const_time(100);
$| = 1;

print STDOUT "--------------------------------------------------------------------------------\n";

my $crlf = 2;
while( 1 ) {
    my @temp = $port->read( 1 );
    if( undef == @temp ) {
        die "Serial port disconnected.\n";
    }

    my ($count, $saw) = @temp;
    if( 1 == $count ) {
        printf( STDOUT "%02x ", ord($saw) );
        $crlf = 2;
    }

    if( 0 < $crlf ) {
        $crlf--;
    } elsif( 0 == $crlf ) {
        printf( STDOUT "\n" );
        $crlf--;
    }
}
