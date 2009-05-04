#!/usr/bin/perl

use strict;
use Device::SerialPort;
use Fcntl;

if( $ARGV[0] eq "" ) {
    shift @ARGV;
}

if( @ARGV < 1 ) {
    die "Usage:\noutOnly.pl serialport\n";
}

#
# Global Values
#
my %idToDeviceMap = (   0x00    =>  "Broadcast",
                        0x18    =>  "CD-Player",
                        0x30    =>  "unknown",
                        0x3b    =>  "Navigation",
                        0x3f    =>  "unknown",
                        0x43    =>  "MenuScreen",
                        0x44    =>  "unknown",
                        0x50    =>  "Steering Wheel Buttons",
                        0x60    =>  "Park Distance Control",
                        0x68    =>  "Radio",
                        0x6a    =>  "DSP",
                        0x7f    =>  "unknown",
                        0x80    =>  "IKE",
                        0xa8    =>  "unknown",
                        0xbb    =>  "TV Module",
                        0xbf    =>  "Light Control Module",
                        0xc0    =>  "Multi-Information Display Buttons",
                        0xc8    =>  "Telephone",
                        0xd0    =>  "Navigation Location",
                        0xe7    =>  "On-Board Computer TextBar",
                        0xe8    =>  "unknown",
                        0xed    =>  "Lights, Wipers, Seat Memory",
                        0xf0    =>  "Board Monitor Buttons",
                        0xf1    =>  "My Debug Listener",
                        0xff    =>  "Broadcast"
                    );

#
# Start Main
#

my $serial_port = $ARGV[0];

my $port = new Device::SerialPort( $serial_port, undef, undef )
    || die "Can't open $serial_port: $!\n";

$port->baudrate(9600);
$port->databits(8);
$port->parity("even");
$port->stopbits(1);
$port->read_char_time(0);
$port->read_const_time(100);

print "--------------------------------------------------------------------------------\n";
$| = 1;

my $chars=0;
my $buffer="";

my $data_length=0;
my $source=0;
my $destination=0;
my $payload="";
my $checksum=0;

my $data_length_valid=0;
my $source_valid=0;
my $destination_valid=0;
my $payload_length=0;
my $checksum_valid=0;

my $buffer_char=0;
my $buffer_char_valid=0;
my $buffer_this_message="";
my $xor_checksum=0;
while( 1 )
{
# 8 bytes -- source
# 8 bytes -- length
# 8 bytes -- destination
# 8 bytes * length - 2 -- payload
# 8 bytes -- checksum
    my @temp = $port->read( 255 );
    if( undef == @temp ) {
        die "Serial port disconnected.\n";
    }

    my ($count, $saw) = @temp;
    if( $count > 0 ) {
        $chars += $count;
        $buffer .= $saw;
    }
    
    if( $chars > 0 ) {
#        $buffer_char = ord sprintf("%1s", $buffer);
#        print (sprintf("%1s", $buffer));
        $buffer_char = ord(substr($buffer, 0, 1));
        $buffer = substr($buffer, 1);
        $chars -= 1;
        $buffer_char_valid = 1;
        $buffer_this_message .= sprintf("0x%02x ", $buffer_char);
        $xor_checksum ^= $buffer_char;
    }
    
    if( 1 == $buffer_char_valid ) {
        if( 0 == $source_valid ) {
#           We are looking for source of the message
            $source = $buffer_char;
            $source_valid = 1;
            $buffer_char_valid = 0;
        }
    }
    if( 1 == $buffer_char_valid ) {
        if( 0 == $data_length_valid ) {
#           We are looking for the data/payload length
            $data_length = $buffer_char;
            $data_length_valid = 1;
            $buffer_char_valid = 0;
        }
    }
    if( 1 == $buffer_char_valid ) {
        if( 0 == $destination_valid ) {
#           We are looking for the destination
            $destination = $buffer_char;
            $destination_valid = 1;
            $buffer_char_valid = 0;
        }
    }
    if( 1 == $buffer_char_valid ) {
        if( 1 == $data_length_valid ) {
#           We are looking for payload
            if( $payload_length < $data_length - 2 ) {
                $payload .= sprintf("0x%02x ", $buffer_char);
                $payload_length += 1;
                $buffer_char_valid = 0;
            }
        }
    }
    if( 1 == $buffer_char_valid ) {
        if( 0 == $checksum_valid ) {
#           We are looking for the checksum
            $checksum = $buffer_char;
            $checksum_valid = 1;
            $buffer_char_valid = 0;
        }
    }
    
    if( 1 == $checksum_valid ) {
#       We have everything.  Now we print it out.
        my $src_name = "";
        my $dst_name = "";
        if( undef eq $idToDeviceMap{$source} ) {
            $src_name = sprintf("undef (0x%02x)\n", $source);
        }
        else {
            $src_name = $idToDeviceMap{$source};
        }
        if( undef eq $idToDeviceMap{$destination} ) {
            $dst_name = sprintf("undef (0x%02x)\n", $destination);
        }
        else {
            $dst_name = $idToDeviceMap{$destination};
        }
        printf("\n");
        if( 0 != $xor_checksum ) {
            printf("**************\nThis is a bad message as the checksum doesn't match\n");
        }
        printf("Message: %s\n",$buffer_this_message);
        printf("Src [%s] --> Dst [%s]\n", $src_name, $dst_name );
        printf("Payload [%d]\t%s\n", $payload_length, $payload);
        if( 0 != $xor_checksum ) {
            printf("**************\n");
        }
# re-init the variables
        $xor_checksum = 0;
        $source_valid = 0;
        $destination_valid = 0;
        $payload_length = 0;
        $checksum_valid = 0;
        $data_length_valid = 0;
        $payload = ""; 
        $buffer_this_message = "";
    }
}
