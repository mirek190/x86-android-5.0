#!/usr/bin/perl -w

use Getopt::Long;
use Term::ReadKey;
use RPC::XML::Client;

# Usage : perl update_bz_comment <filename.txt> <builder_name> <daily_build_version>
# e.g. : perl update_bz_comment info.txt mikaelx MFLD_R2_20110905_002
# This script reads a file that contains lines whose structure is as followed
# 1234 980989 0980989 098909
# 1234 being a BZ number and the others informations are one or more gerrit patches URL
# The file adds the following comment in the BZ:
# "Integrated by builder mdoninix in build MFLD_R2_AAAAMMJJ-XXX with the patche(s): patch1 patch2 patch3\n Reporters, please verify and close"

sub usage{
  print "Usage: update_bz_comment.pl [FILE] [BUILDER_NAME] [DAILY_BUILD_VERSION]\n";
  print "Adds the following comment:\n";
  print "    Integrated by builder [BUILDER_NAME] in build [DAILY_BUILD_VERSION] with the patche(s) PATCHx PATCHy ...\n";
  print "    Reporters, please verify and close.\n";
  print "for each BZs (first column) of FILE\n\n";
  print "EXAMPLE:\n";
  print "    update_bz_comment.pl bz.txt mdoninix R2_20111006-001\n";
  return 0;
}

sub parse_args {
  my %PAR;

  Getopt::Long::config("no_ignore_case");

  if (Getopt::Long::GetOptions(
        \%PAR,
        "help|h!"
        ) == 0 ) {
    print("Try `$0 --help' for more information.\n");
    exit 1;
  }

  $PAR{'help'} ||= 0;

  if ( $PAR{'help'} == 1) {
    &usage;
    exit 0;
  }

  return \%PAR;
}

my $params = &parse_args();

my $nb_arg = scalar(@ARGV);

if ($nb_arg != 3 and $nb_arg != 5) {
  print "update_bz_comment.pl: number of ARGs=$nb_arg is NOT EQUAL to EXPECTED ARG(s)=3 or 5\n";
  print "Try `update_bz_comment.pl --help' for more information.\n";
  exit 1;
}

my $inputfile = $ARGV[0];
my $buildername = $ARGV[1];
my $dailybuild = $ARGV[2];
my $username = "";
my $password = "";
if ($nb_arg == 5) {
  $username = $ARGV[3];
  $password = $ARGV[4]; 
}
else {
  ($username, $encrypted) = ( getpwuid $< )[0,1];

   print "Enter $username password: ";
   ReadMode 'noecho';
   $password = ReadLine 0;
   chomp $password;
   ReadMode 'normal';

   print "\n";

   if (crypt($password, $encrypted) ne $encrypted) {
       die "Wrong password for $username\n";
   }
}

my $cli = RPC::XML::Client->new("http://$username:$password\@umgbugzilla.sh.intel.com:41006/xmlrpc.cgi");
open (FILE, $inputfile) or die "Cannot open $inputfile";
while (<FILE>) {

  chomp;

  my $string = "";
  my @values = split(" ");
  $valueSize = scalar (@values);
  my $BZnumber = "";

  for my $p (0 .. ($valueSize-1)) {
      if ($p == 0) {
          $BZnumber = $values[$p];
          $string .= "Integrated by builder $buildername in build $dailybuild with the patche(s) ";
      }
      else {
          $string .= "$values[$p] ";
      }
  }

  my $tmp = $cli->send_request('Bug.add_comment', { id => $BZnumber, comment => $string});
}
close (FILE);
exit 0;
