#! /usr/bin/perl
#
# Create a coverage report
#
# The input to this is coverage files generated by gcov.
#
# Usage:
#
# coveragereport gcov-output-files
# gcov-output-files can include directories - in that case, all gcov files
# in the specified directory are included.
#
# For future expansion
#    --config=filename - Use this to provide detailed configuration information
#
# Defaults and initialization
@files = ();
$cleanGCOVoutput = 1;
$indexFileName = "index.html";
#
# This is what version 4.x GNU compilers/gcov generate as line leader
$lineLeader4 = '^\s*[0-9-]+:\s*\d+:\s*[\{\}]?\s*';

# Line colors
$missedColor = 'red';
$unknownColor = 'fuchsia';

# Annotations recognized, and the background color to use with them
%blockColor = (
  'ERRORHANDLING' => '#CFDCE6', # blue
  'DEBUG' => '#E6E3CF', # yellow
  'EXPERIMENTAL' => '#CFE6CF', # green
  'UNUSED' => '#E6E6E6' # gray
);

# Ignore missed/unknown lines
%blockIgnored = (
  'ERRORHANDLING' => 1,
  'EXPERIMENTAL' => 1,
  'DEBUG' => 1,
  'UNUSED' => 0
);

# Ignore missed/unknown lines and warn if block contains executed lines
%blockUnused = (
  'ERRORHANDLING' => 0,
  'EXPERIMENTAL' => 0,
  'DEBUG' => 0,
  'UNUSED' => 1
);

$annotationBeginPrefixC  = '\/\*\s*\[';
$annotationBeginPostfixC = '\]\s*\*\/';
$annotationEndPrefixC  = '\/\*\s*\[\/';
$annotationEndPostfixC = '\]\s*\*\/';
$annotationBeginPrefixF  = '!\s*\[';
$annotationBeginPostfixF = '\]\s*$';
$annotationEndPrefixF  = '!\s*\[\/';
$annotationEndPostfixF = '\]\s*$';

# Create a default block (simplifies code later on)
$blockColor{'DEFAULT'} = 'white';
$blockIgnored{'DEFAULT'} = 0;
$blockUnused{'DEFAULT'} = 0;

# process arguments
@paths = ();
for (@ARGV) {
  if (/^--?config=(.*)/) {
    print STDERR "--config=file not supported\n";
    $configfile = $1;
  } elsif (-d $_) {
    $paths[$#paths + 1] = $_;
  } elsif (-f $_) {
    $paths[$#paths + 1] = $_;
  } else {
    print STDERR "Unrecognized argument $_\n";
    &printHelp;
    exit(1);
  }
}

# give an error if no paths were provided
if ($#paths + 1 == 0) {
  print STDERR "Must provide at least one file or directory\n";
  &printHelp;
  exit(1);
}

for (@paths) {
  if (-d $_) {
    @files = (@files, &ExpandDir($_));
  } elsif ($_ =~ /\.gcov$/) {
    $files[$#files + 1] = $_;
  }
}

# give an error if no gcov files found
if ($#files + 1 == 0) {
  print STDERR "Unable to find .gcov files\n";
  exit(1);
}

# ---------------------------------------------------------------------------
# Process each file.  Accumulate information on each file that will
# be used to generate an index
%coverageIndex = ();
for my $file (@files) {
  $outfile = $file;
  $outfile =~ s/\.gcov/.html/;
  ($linecount, $executableLines, $missedLines) = &annotateFile($file, $outfile);
  $coverageIndex{$file} = "$linecount:$executableLines:$missedLines";
}

&createIndex($indexFileName);
exit(0);
# ---------------------------------------------------------------------------
# 
sub annotateFile {

  my $infile = $_[0];
  my $outfile = $_[1];
  my $linecount = 0;
  my $executableLines = 0;
  my $missedLines = 0;
  my @blockStack = ('DEFAULT');
  my $state = 'INIT';
  my $beginstate = "";
  my $endstate = "";

  open(INFD,"<$infile") || die "Could not open $infile";
  open(OUTFD,">$outfile")  || die "Could not open $outfile";

  if ($infile =~ /\.(c|cpp|h|hpp|inl)\.gcov$/) {
    $annotationBeginPrefix  = $annotationBeginPrefixC;
    $annotationBeginPostfix = $annotationBeginPostfixC;
    $annotationEndPrefix    = $annotationEndPrefixC;
    $annotationEndPostfix   = $annotationEndPostfixC;
  } elsif ($infile =~ /\.[fF]9?0?\.gcov$/) {
    $annotationBeginPrefix  = $annotationBeginPrefixF;
    $annotationBeginPostfix = $annotationBeginPostfixF;
    $annotationEndPrefix    = $annotationEndPrefixF;
    $annotationEndPostfix   = $annotationEndPostfixF;
  } else {
    print STDERR "Unrecognized language for source file.  No annotations recognized\n";
    # define unlikely prefixes to suppress matches
    $annotationBeginPrefix  = "^^^^####";
    $annotationBeginPostfix = "####^^^^";
    $annotationEndPrefix    = "::::####";
    $annotationEndPostfix   = "####::::";
  }

  print OUTFD &HTMLHeader($infile);
  print OUTFD "<pre>\n";
  print OUTFD $beginstate;

  while (<INFD>) {

    $linecount++;

    if (/$lineLeader4$annotationBeginPrefix(\w+)$annotationBeginPostfix/) {
      # Named block start
      if (defined($blockColor{$1})) {
        if (!$blockIgnored{$blockStack[-1]} && !$blockUnused{$blockStack[-1]}) {
          print OUTFD $endstate;
        }
        push(@blockStack, $1);
        print OUTFD "<span style=\"background-color: $blockColor{$blockStack[-1]};\">";
        if (!$blockIgnored{$blockStack[-1]} && !$blockUnused{$blockStack[-1]}) {
          print OUTFD $beginstate;
        }
      }
    } elsif (/$lineLeader4$annotationEndPrefix$blockStack[-1]$annotationEndPostfix/) {
      # Named block end
      if (!$blockIgnored{$blockStack[-1]} && !$blockUnused{$blockStack[-1]}) {
        print OUTFD $endstate;
      }
      print OUTFD "</span>";
      pop(@blockStack);
      if (!$blockIgnored{$blockStack[-1]} && !$blockUnused{$blockStack[-1]}) {
        print OUTFD $beginstate;
      }
    } elsif (/^\s*function/) {
      # Start of a function
      if (!$blockIgnored{$blockStack[-1]} && !$blockUnused{$blockStack[-1]}) {
        print OUTFD $endstate;
      }
      print OUTFD "<span style=\"background-color: $blockColor{'DEFAULT'}; font-weight: bold;\">";
      print OUTFD &HTMLify($_);
      print OUTFD "</span>";
      $state = 'INIT';
      $beginstate = "";
      $endstate = "";
      if (!$blockIgnored{$blockStack[-1]} && !$blockUnused{$blockStack[-1]}) {
        print OUTFD $beginstate;
      }
    } elsif (/^\s+\-:/) {
      # Non-executable line
      print OUTFD &HTMLify($_);
    } elsif (/^\s*######?/) {
      # An unexecuted line starts with #####
      if (!$blockIgnored{$blockStack[-1]} && !$blockUnused{$blockStack[-1]}) {
        $missedLines++;
        $executableLines++;
        if ($state ne 'MISSED') {
          print OUTFD $endstate;
          $state = 'MISSED';
          $beginstate = "<span style=\"background-color: $missedColor;\">";
          $endstate = "</span>";
          print OUTFD $beginstate;
        }
        print OUTFD &HTMLify($_);
      } else {
        print OUTFD &HTMLify($_);
      }
    } elsif (/^\s*\d+:/) {
      # An executed line starts with a number
      if (!$blockIgnored{$blockStack[-1]} && !$blockUnused{$blockStack[-1]}) {
        $executableLines++;
        if ($state ne 'EXECUTED') {
          print OUTFD $endstate;
          $state = 'EXECUTED';
          $beginstate = "";
          $endstate = "";
          print OUTFD $beginstate;
        }
        print OUTFD &HTMLify($_);
      } else {
        if ($blockUnused{$blockStack[-1]}) {
          print STDERR "Found executed code marked as unused at $infile:$linecount.\n";
        }
        print OUTFD &HTMLify($_);
      }
    } else {
      # Some versions of gcov generate additional statements
      # about calls and branches.
      if ($cleanGCOVoutput) {
        if (/^branch\s/ || /^call\s/) { next; }
      }
      # Unknown line
      if (!$blockIgnored{$blockStack[-1]} && !$blockUnused{$blockStack[-1]}) {
        if ($state ne 'UNKNOWN') {
          print OUTFD $endstate;
          $state = 'UNKNOWN';
          $beginstate = "<span style=\"background-color: $unknownColor;\">";
          $endstate = "</span>";
          print OUTFD $beginstate;
        }
        print OUTFD &HTMLify($_);
      } else {
        print OUTFD &HTMLify($_);
      }
    }

  }

  close(INFD);

  print OUTFD $endstate;
  print OUTFD "</pre>\n";
  print OUTFD "<hr>\n";
  print OUTFD "Code Lines<ul>\n<li> Total = $linecount\n";
  print OUTFD "<li> Executable = $executableLines\n";
  print OUTFD "<li> Missed = $missedLines\n";
  print OUTFD "</ul>\n";
  print OUTFD &HTMLTail();

  close(OUTFD);
  return ($linecount, $executableLines, $missedLines);

}

sub createIndex {

  my $outfile = $_[0];

  open (OUTFD, ">$outfile") || die "Could not open $outfile";
  print OUTFD &HTMLHeader("Coverage Index");
  # Create a list of files by the total number of missed lines
  print OUTFD "Files, in order of number of executable lines missed<br>\n";
  @filenames = keys(%coverageIndex);
  my $totalLines = 0;
  my $totalExecutable = 0;
  my $totalMissed = 0;
  @linesMissed = ();
  @percentCovered = ();

  for my $i (0 .. $#filenames) {
    $info = $coverageIndex{$filenames[$i]};
    # totallines:executable:missed
    my ($linecount,$executableLines,$missedLines) = split(/:/,$info);
    $linesMissed[$i] = $missedLines;
    if ($executableLines > 0) {
      $percentCovered[$i] = int (100 * ($executableLines - $missedLines) / $executableLines);
    } else {
      $percentCovered[$i] = 100;
    }
    $totalLines += $linecount;
    $totalExecutable += $executableLines;
    $totalMissed += $missedLines;
  }

  my $totalPercent = int(100 * ($totalExecutable - $totalMissed) / $totalExecutable);
  @permutation = sort { $linesMissed[$b] <=> $linesMissed[$a] } (0 .. $#linesMissed);

  print OUTFD "<table>\n";
  print OUTFD "<tr>";
  print OUTFD "<th class=\"file\">Name</th>";
  print OUTFD "<th>Missed Lines</th>";
  print OUTFD "<th>% Covered</th>";
  print OUTFD "</tr>\n";

  for my $i (@permutation) {
    my $file = $filenames[$i];
    my $filenamehtml = $file;
    my $lines = $linesMissed[$i];
    my $percent = $percentCovered[$i];
    $file         =~ s/\.gcov//;
    $filenamehtml =~ s/\.gcov/.html/;
    print OUTFD "<tr>";
    print OUTFD "<td class=\"file\"><a href=\"$filenamehtml\">$file</a></td>";
    print OUTFD "<td>$lines</td>";
    print OUTFD "<td>$percent</td>";
    print OUTFD "</tr>\n";
  }

  print OUTFD "<tr>";
  print OUTFD "<th class=\"totals\">Totals:</th>";
  print OUTFD "<td>$totalMissed</td>";
  print OUTFD "<td>$totalPercent</td>";
  print OUTFD "</tr>\n";
  print OUTFD "</table>\n";

  print OUTFD "<hr>\nFiles, in alphabetical order<br>\n";
  print OUTFD "<table>\n";
  print OUTFD "<tr>";
  print OUTFD "<th class=\"file\">Name</th>";
  print OUTFD "<th>Total Lines</th>";
  print OUTFD "<th>Executable Lines</th>";
  print OUTFD "<th>Missed Lines</th>";
  print OUTFD "<th>% Covered</th>";
  print OUTFD "</tr>\n";

  for my $filename (sort(keys(%coverageIndex))) {
    my $filenamehtml = $filename;
    $filenamehtml =~ s/\.gcov/.html/;
    $info = $coverageIndex{$filename};
    # totallines:executable:missed
    my ($linecount,$executableLines,$missedLines) = split(/:/,$info);
    $filename =~ s/\.gcov//;
    if ($executableLines > 0) {
      $percentCovered = int (100 * ($executableLines - $missedLines) / $executableLines);
    } else {
      $percentCovered = 100
    }
    print OUTFD "<tr>";
    print OUTFD "<td class=\"file\"><a href=\"$filenamehtml\">$filename</a></td>";
    print OUTFD "<td>$linecount</td>";
    print OUTFD "<td>$executableLines</td>";
    print OUTFD "<td>$missedLines</td>";
    print OUTFD "<td>$percentCovered</td>";
    print OUTFD "</tr>\n";
  }

  print OUTFD "<tr>";
  print OUTFD "<th class=\"totals\">Totals:</th>";
  print OUTFD "<td>$totalLines</td>";
  print OUTFD "<td>$totalExecutable</td>";
  print OUTFD "<td>$totalMissed</td>";
  print OUTFD "<td>$totalPercent</td>";
  print OUTFD "</tr>\n";
  print OUTFD "</table>\n";
  print OUTFD &HTMLTail();

  close(OUTFD);

}

# ---------------------------------------------------------------------------
sub printHelp {
  print STDERR "coveragereport file-or-directory ...\n\
Create a coverage report using the files (with name xxx.gcov) or directories\n\
(reading all files with name xxx.gcov in the named directory) as HTML files\n\
An index.html file is created in the current directory.  That index file\n\
provides a summary of the coverage and links to the HTML files\n";
}

# ---------------------------------------------------------------------------
# Get all of the .gcov files from the named directory, including any subdirs
# If there is a "merge" version of the gcov file, prefer that.  These are
# used when the same source file is compiled for both the MPI and PMPI 
# interfaces, 
sub ExpandDir {
  my $dir = $_[0];
  my @otherdirs = ();
  my @files = ();
  opendir DIR, "$dir";
  while ($filename = readdir DIR) {
    if ($filename =~ /^\./ || $filename eq ".svn") {
      next;
    } elsif (-d "$dir/$filename") {
      # Skip pmpi directories used for mergeing gcov output
      if ($filename =~ /\-pmpi$/) { next; }
      $otherdirs[$#otherdirs+1] = "$dir/$filename";
    } elsif ($filename =~ /(.*\.gcov)$/) {
      # Check for the presense of a "merged" gcov file and use instead
      if (-f "$dir/$filename.merge") {
        $files[$#files + 1] = "$dir/$filename.merge";
      } else {
        $files[$#files + 1] = "$dir/$filename";
      }
    }
  }
  closedir DIR;
    # (almost) tail recurse on otherdirs (we've closed the directory handle,
    # so we don't need to worry about it anymore)
  foreach $dir (@otherdirs) {
    @files = (@files, &ExpandDir( $dir ) );
  }
  return @files;
}
# --------------------------------------------------------------------------
# HTMLify
# Take an input line and make it value HTML
sub HTMLify {
  my $line = $_[0];
  $line =~ s/\&/--AMP--/g;
  $line =~ s/>/&gt;/g;
  $line =~ s/</&lt;/g;
  $line =~ s/--AMP--/&amp;/g;
  return $line;
}
sub HTMLHeader {
  my $title = $_[0];
  my $str = "";
  $str = "<!DOCTYPE html>\n<html lang=en>\n<head>\n";
  $str .= "<meta http-equiv=\"Content-Style-Type\" content=\"text/css\">\n";
  $str .= "<title>$title</title>\n</head>\n";
  $str .= "<style>\n";
  $str .= "table { padding: 0.5em; }\n";
  $str .= "th, td { text-align: center; padding: 0.2em 1em; }\n";
  $str .= "th { font-weight: bold; }\n";
  $str .= "td:first-child, th:first-child { padding-left: 0.2em; }\n";
  $str .= "td:last-child, th:last-child { padding-right: 0.2em; }\n";
  $str .= ".file { text-align: left; }\n";
  $str .= ".totals { text-align: right; }\n";
  $str .= "</style>\n";
  $str .= "<body style=\"background-color: #FFFFFF\";>\n";
  return $str;
}
sub HTMLTail {
  my $str = "</body>\n</html>\n";
  return $str;
}
