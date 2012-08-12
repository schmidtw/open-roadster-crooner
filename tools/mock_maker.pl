#!/usr/bin/perl
##
#   Limitations:
#       All function pointer definitions must be typedef'ed, this doesn't work:
#           int foo( void(*bar)() )
#
#       All argument lists must not be empty
#
use strict;
use warnings;
use Getopt::Long;

# Only increment this when the contents of the files generated
# change in some way.
my $version = "1.3";

my $header_name = "";
my $output_path = "";
my $library_name = "";
my $extra_header_name = "";
my $add_std_impl = 0;

my $result = GetOptions( "header=s"  => \$header_name,
                         "library=s" => \$library_name,
                         "extra-header=s" => \$extra_header_name,
                         "add-std-impl" => \$add_std_impl,
                         "output=s"  => \$output_path );

if( ($header_name eq "") or ($output_path eq "") or ($library_name eq "") ) {
    die "mock_maker.pl --header=input_header.h --output=path --library=lib-name [--add-std-impl] [--extra-header=header-name]\n";
}

my $just_header_name = $header_name;
$just_header_name =~ s/^.*\///;

my $mock_header_name = $just_header_name;
$mock_header_name =~ s/\.h/-mock.h/;

my $impl_header_name = $just_header_name;
$impl_header_name =~ s/\.h/-mock-impl.h/;

my $mock_c_file_name = $just_header_name;
$mock_c_file_name =~ s/\.h/-mock.c/;

my $struct_name = $just_header_name;
$struct_name =~ s/\.h$//g;
$struct_name =~ s/\-/_/g;

open FH, $header_name or die $!;
my @lines = <FH>;
close FH;

chomp( @lines );

my @list = parse_header( \@lines );

if( 0 == scalar(@list) ) {
    # no functions, exit.
    exit 0;
}

my @functions_list = ();
for( my $i = 0; $i < scalar(@list); $i++ ) {
    push( @functions_list, $list[$i]{'NAME'} );
}

open FH, ">".$output_path."/".$mock_c_file_name or die $!;
print FH generate_c_file( $mock_header_name, $impl_header_name, $add_std_impl, \@list );
close FH;

print ">".$output_path."/".$mock_header_name."\n";

open FH, ">".$output_path."/".$mock_header_name or die $!;
print FH generate_header( $library_name, $just_header_name, $extra_header_name, \@functions_list );
close FH;



##
#   Parses the specified header into the following:
#       function names, function return type, function arguments
#
#   (array) of hashes
#       'NAME'     => function names
#       'RV_TYPE'  => function return type
#       'ARG_LIST' => array of function arguments
#               
#   @param data the file data read out of the file and put into an array
#               (1 line :: 1 entry)
#
#   @return the array of hashes defined above
#
sub parse_header {
    my @rv = ();
    my ($data_ref) = @_;
    my @data = @$data_ref;

    for( my $i = 0; $i < scalar(@data); $i++ ) {
        # Remove // comments
        $data[$i] =~ s/\/\/.*//;

        # Remove single-line /* */ comments
        $data[$i] =~ s/\/\*.*?\*\///;

        # Eat duplicate whitespace
        $data[$i] =~ s/\s+/ /g;
    }

    # Remove multi-line /* */ comments
    for( my $i = 0; $i < scalar(@data); $i++ ) {
        if( $data[$i] =~ m/\/\*/ ) {
            $data[$i] =~ s/\/\*.*//;
            $i++;
            while( $data[$i] !~ m/\*\// ) {
                $data[$i] = "";
                $i++;
            }
            $data[$i] =~ s/.*//;
        }
    }

    # Macro line(s)
    for( my $i = 0; $i < scalar(@data); $i++ ) {
        if( $data[$i] =~ m/^\s*\#\s*/ ) {
            while( $data[$i] =~ m/\\$/ ) {
                $data[$i] = "";
                $i++;
            }
            $data[$i] = "";
        }
    }

    # Merge the remaining declarations and functions into 1 line
    my $tmp = "";
    foreach my $line (@data) {
        if( $line !~ m/^\s*$/ ) {
            $tmp .= $line;
        }
    }

    # Eat all the internal {and data inside}
    $tmp =~ s/\{.*?\}//g;

    # Make everything a space & not a tab
    $tmp =~ s/\t/ /g;

    # Chomp duplicate whitespace
    $tmp =~ s/\s+/ /g;

    # Make all pointers uint32_t* foo
    $tmp =~ s/\s+\*\s*/\* /g;

    @data = split( ';', $tmp );

    for( my $i = 0; $i < scalar(@data); $i++ ) {
        # Get rid of typedefs
        if( $data[$i] =~ m/(typedef)/ ) {
            $data[$i] = "";
        }

        # Get rid of anything that doesn't have a parenthesis
        if( $data[$i] !~ m/\(/ ) {
            $data[$i] = "";
        }

        $data[$i] =~ s/extern\s+\"C\"\s+//g;
        $data[$i] =~ s/extern\s+//g;
    }

    foreach my $line (@data) {
        if( $line !~ m/^\s*$/ ) {
            if( $line =~ m/(.*) (.*?)\(\s*(.*)\s*\)$/ ) {
                my @args = split( ',', $3 );
                for( my $i = 0; $i < scalar(@args); $i++ ) {
                    $args[$i] =~ s/\s+\S*\s*$//;
                    $args[$i] =~ s/^\s+//;
                }

                my %entry = ('NAME' => $2,
                             'RV_TYPE' => $1,
                             'ARG_LIST' => \@args );

                push( @rv, \%entry );
            }
        }
    }

    return @rv;
}


##
#   Generates the mock function code for a function.
#
#   @param header_name the name of the header file being generated from
#   @param rv_type the return value type of the function to mock
#   @param fn_name the function name of the function to mock
#   @param arg_type_list_ref the reference to the array of argument types for
#                            the function
#   @param add_std_impl 0 if no std impl is added, 1 causes a std impl to be
#                       generated.
#
#   @return a string of generated 'c' code.
#
sub generate_function {
    my $rv = "";
    my ($header_name, $rv_type, $fn_name, $arg_type_list_ref, $add_std_impl) = @_;
    my @arg_type_list = @$arg_type_list_ref;

    $header_name =~ s/\.h$//g;
    $header_name =~ s/\-/_/g;

    $rv .= $rv_type." ".$fn_name."(";

    if( "void" eq $arg_type_list[0] ) {
        $rv .= " void";
    } else {
        my $comma = "";
        my $argname = 0;
        foreach my $arg (@arg_type_list) {
            $rv .= $comma." ".$arg." arg".$argname;
            # set this last
            $comma = ",";
            $argname++;
        }
    }

    $rv .= " )\n";
    $rv .= "{\n";
    $rv .= "    mock_obj_t *obj = \&mock_".$header_name.".".$fn_name.";\n";
    $rv .= "\n";
    $rv .= "    mock_test_assert( get_is_expecting(obj) );\n";
    $rv .= "\n";
    $rv .= "    if( NULL != obj->do_stuff_fct ) {\n";
    $rv .= "        ";
    if( "void" ne $rv_type ) {
        $rv .= "return ";
    }
    $rv .= "(($rv_type (*)(mock_obj_t*";

    if( "void" ne $arg_type_list[0] ) {
        foreach my $arg (@arg_type_list) {
            $rv .= ", ".$arg;
        }
    }

    $rv .= " ))(obj->do_stuff_fct))( obj";

    if( "void" ne $arg_type_list[0] ) {
        my $argname = 0;
        foreach my $arg (@arg_type_list) {
            $rv .= ", arg".$argname;
            $argname++;
        }
    }
    $rv .= " );\n";

    if( 0 != $add_std_impl ) {
        if( "void" eq $rv_type ) {
            $rv .= "        return;\n";
        }
    }
    $rv .= "    }\n";

    if( 0 != $add_std_impl ) {
        $rv .= "    if( obj->call_std_fct ) {\n";
        if( "void" ne $rv_type ) {
            $rv .= "        return ";
        } else {
            $rv .= "        ";
        }
        my $comma = "";
        $rv .= $fn_name."_std(";
        if( "void" ne $arg_type_list[0] ) {
            my $argname = 0;
            foreach my $arg (@arg_type_list) {
                $rv .= $comma." arg".$argname;
                $argname++;
                $comma = ",";
            }
        }
        $rv .= " );\n";
        $rv .= "    }\n";

        if( "void" ne $rv_type ) {
            $rv .= "\n    return (".$rv_type.") get_return_value( obj );\n";
        }
    } else {
        if( "void" ne $rv_type ) {
            $rv .= "\n    return (".$rv_type.") get_return_value( obj );\n";
        }
    }

    $rv .= "}\n";

    $rv =~ s/\(\s+\)/\(\)/g;

    return $rv;
}


##
#   Generates the mock function code for a function.
#
#   @param library the library the header file being generated from belongs to
#   @param header_name the name of the header file being generated from
#   @param extra_header_name the name of the extra header file to include for
#                            support functions
#   @param functions_ref the reference to the array of function names to
#                        generate code for
#
#   @return a string of generated 'c' code.
#
sub generate_header {
    my $rv = "";
    my ($library, $header_name, $extra_header_name, $functions_ref) = @_;

    my @functions = @$functions_ref;

    my $structname = $header_name;
    $structname =~ s/\.h$//g;
    $structname =~ s/\-/_/g;

    my $uppername = uc( $structname );

    # Start of file
    $rv .= generate_infostamp();
    $rv .= "\n";
    $rv .= "#ifndef __".$uppername."_MOCK_H__\n";
    $rv .= "#define __".$uppername."_MOCK_H__\n";
    $rv .= "\n";

    # Handle includes
    $rv .= "#include <".$library."/".$header_name.">\n";
    if( $extra_header_name =~ m/\.h/ ) {
        $rv .= "#include \"".$extra_header_name."\"\n";
    }
    $rv .= "#include <mock/mock.h>\n";
    $rv .= "\n";
    
    # C++ wrappers
    $rv .= "#ifdef __cplusplus\n";
    $rv .= "extern \"C\" {\n";
    $rv .= "#endif\n";
    $rv .= "\n";


    # Generate the do stuff
    $rv .= "void MOCK_reset__".$structname."( void );\n";
    foreach my $fn (@functions) {
        $rv .= "void MOCK_set_do_stuff__".$fn."( void *func );\n";
        $rv .= "bool MOCK_get_is_expecting__".$fn."( void );\n";
        $rv .= "void MOCK_set_is_expecting__".$fn."( const bool expecting );\n";
        $rv .= "uint64_t MOCK_get_rv__".$fn."( void );\n";
        $rv .= "void MOCK_set_rv__".$fn."( const uint64_t rv );\n";
        $rv .= "\n";
    }

    # C++ wrappers
    $rv .= "#ifdef __cplusplus\n";
    $rv .= "}\n";
    $rv .= "#endif\n";
    $rv .= "\n";

    # End of file
    $rv .= "#endif\n";

    return $rv;
}


##
#
sub generate_c_file
{
    my $rv = "";
    my ($mock_header_name, $impl_header_name, $add_std_impl, $list_ref) = @_;
    my @list = @$list_ref;

    # Start of file
    $rv .= generate_infostamp();
    $rv .= "\n";
    $rv .= "\#include <stdbool.h>\n";
    $rv .= "\#include <stdint.h>\n";
    $rv .= "\n";
    $rv .= "\#include \"".$mock_header_name."\"\n";
    if( 0 != $add_std_impl ) {
        $rv .= "\#include \"".$impl_header_name."\"\n";
    }
    $rv .= "\n";

    # Generate structure and support
    $rv .= "struct ".$struct_name."_mock {\n";
    for( my $i = 0; $i < scalar(@list); $i++ ) {
        $rv .= "    mock_obj_t ".$list[$i]{'NAME'}.";\n";
    }
    $rv .= "};\n";
    $rv .= "\n";
    $rv .= "static struct ".$struct_name."_mock mock_".$struct_name.";\n";
    $rv .= "\n";
    $rv .= "void MOCK_reset__".$struct_name."( void )\n";
    $rv .= "{\n";
    $rv .= "    mock_reset( (mock_obj_t*) \&mock_".$struct_name.", (sizeof(mock_".$struct_name.")/sizeof(mock_obj_t)) );\n";
    $rv .= "}\n";
    $rv .= "\n";
    for( my $i = 0; $i < scalar(@list); $i++ ) {
        my $fn = $list[$i]{'NAME'};

        $rv .= "void MOCK_set_do_stuff__$fn( void *func )\n";
        $rv .= "{\n";
        $rv .= "    set_do_stuff_function( \&mock_$struct_name.$fn, func );\n";
        $rv .= "}\n\n";

        $rv .= "bool MOCK_get_is_expecting__$fn( void )\n";
        $rv .= "{\n";
        $rv .= "    return get_is_expecting( \&mock_$struct_name.$fn );\n";
        $rv .= "}\n\n";

        $rv .= "void MOCK_set_is_expecting__$fn( const bool expecting )\n";
        $rv .= "{\n";
        $rv .= "    set_is_expecting( \&mock_$struct_name.$fn, expecting );\n";
        $rv .= "}\n\n";

        $rv .= "uint64_t MOCK_get_rv__$fn( void )\n";
        $rv .= "{\n";
        $rv .= "    return get_return_value( \&mock_$struct_name.$fn );\n";
        $rv .= "}\n\n";

        $rv .= "void MOCK_set_rv__$fn( const uint64_t rv )\n";
        $rv .= "{\n";
        $rv .= "    return set_return_value( \&mock_$struct_name.$fn, rv );\n";
        $rv .= "}\n\n";

        if( 0 != $add_std_impl ) {
            $rv .= "void MOCK_set_use_std_fct__$fn( void )\n";
            $rv .= "{\n";
            $rv .= "    set_use_standard_fct( \&mock_$struct_name.$fn );\n";
            $rv .= "}\n\n";
        }
    }

    for( my $i = 0; $i < scalar(@list); $i++ ) {
        $rv .= generate_function( $just_header_name,
                                  $list[$i]{'RV_TYPE'},
                                  $list[$i]{'NAME'},
                                  $list[$i]{'ARG_LIST'},
                                  $add_std_impl );

        $rv .= "\n";
    }

    return $rv;
}


##
#   Generates the boilerplate header information about the script and version
#
#   @return a string of generated 'c' code.
#
sub generate_infostamp
{
    return "/* Auto-generated by mock_maker.pl Version: ".$version." */\n";
}
