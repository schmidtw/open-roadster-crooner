#!/usr/bin/perl

#
# A simple ihex text file to h file converter
#
# Copyright (c) Weston Schmidt 2010
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

if( @ARGV != 2 ) {
    die "Usage:\nihex2h.pl in out\n";
}

my $in_file = $ARGV[0];
my $out_file = $ARGV[1];

open( IN, "<", $in_file ) or die $!;
open( OUT, ">", $out_file ) or die $!;

my $count = 0;
printf( OUT "#include <stdint.h>\n\n" );

printf( OUT "const uint8_t binary_data[] = {\n", 0x01 );
while( <IN> ) {
    my @chars = split( //, $_ );

    print OUT "    ";
    foreach my $c (@chars) {
        $count ++;
        printf( OUT "0x%02x, ", ord($c) );
    }
    $count ++;
    printf( OUT "0x%02x,\n", 0x01 );
}
print OUT "    ";
$count ++;
printf( OUT "0x%02x };\n\n", 0x02 );

printf( OUT "const uint32_t binary_data_length = %d;\n", $count );

close( OUT );
close( IN );
