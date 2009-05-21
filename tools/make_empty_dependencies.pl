#!/usr/bin/perl

$numArgs = $#ARGV + 1;

if( $numArgs != 1 ) {
    print "\n\nusage: \n\n";
    print "\tfile name\n\n\n";
    exit (0);
}

$filename = $ARGV[0];

open( FILE, "<$filename" ) or die "Can't open $filename for reading.\n";
@f = <FILE>;
close(FILE);

open( FILE, ">>$filename" ) or die "Can't open $filename for writing.\n";

print FILE "\n\n# Everything below this line was auto generated from $0\n\n"; 
$processed_targets = false;
for $line (@f) {
    if( false == $processed_targets ) {
	    if( $line =~ m/^[^:]*:(.*)/ ) {
	        $processed_targets = true;
	        $line = $1;
	    }
    }
    $line =~ s/^\s*//;
    $line =~ s/\s*$//;
    if( true == $processed_targets ) {
        @dependencies = split /\s+/,$line;
        for $dep (@dependencies) {
            if(    ( '\\' ne $dep )
                && ( ' ' ne $dep )
                && ( $dep !~ m/\.c\s*$/ ) )
            {
                print FILE "$dep : \n";
            }
        }
    }
}
print FILE "\n\n";
close(FILE);

exit(0);
