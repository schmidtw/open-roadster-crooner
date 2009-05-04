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
    die "Usage:\nlistener.pl serialport [baudrate]\n\tBaudrates:\n\t\t4000000\n\t\t3500000\n\t\t3000000\n\t\t2500000\n\t\t2000000\n\t\t1152000\n\t\t1000000\n\t\t921600\n\t\t576000\n\t\t500000\n\t\t460800 - default\n\t\t230400\n\t\t115200\n\t\t57600 \n\t\t38400\n\t\t19200\n\t\t9600\n";
}

my $baudrate = 460800;

my $serial_port = $ARGV[0];

if( 2 == scalar(@ARGV) ) {
    $baudrate = $ARGV[1];
}

my $old_handle = select( STDOUT );
$| = 1;
select( $old_handle );

my $port = new Device::SerialPort( $serial_port, undef, undef )
    || die "Can't open $serial_port: $!\n";

$port->baudrate($baudrate);
$port->databits(8);
$port->parity("none");
$port->stopbits(1);
$port->read_char_time(0);
$port->read_const_time(100);
$| = 1;

print STDOUT "--------------------------------------------------------------------------------\n";

while( 1 ) {
    my @temp = $port->read( 1 );
    if( undef == @temp ) {
        die "Serial port disconnected.\n";
    }

    my ($count, $saw) = @temp;
    if( $count > 0 ) {
        print STDOUT "$saw";
    }
}
