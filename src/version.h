/*********************************************************
**                                                      **
**                    version.h                         **
**                                                      **
**          Version information for 'lifer'             **
**                                                      **
**         Copyright Paul Tew 2011 to 2017              **
**                                                      **
*********************************************************/
/*
    For version history details see below :)
*/
/*
This file is part of Lifer, a Windows link file analyser.

    Lifer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lifer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lifer.  If not, see <http://www.gnu.org/licenses/>.
*/

// These are equivalent to the protocol revision for MS-SHLLINK
#define _MAJOR  3
#define _MINOR  0
// This is my revision number
#define _BUILD  18 // IDLists PropertyStores now parsed using option '-i' or '-io txt'
// Just the current year
#define _YEAR   2017

/*     VERSION HISTORY

  A lot of projects maintain a VERSIONS or Version.txt file (or something
  similarly named). I thought it might be a good idea to do this to record
  the development of this tool but noted I already had a version.h file
  which would do the trick nicely even though, for the uninitiated you might
  have to view some horrible 'C' code first ;)

  The earliest entries are at the bottom of this comment section and the 
  later one's are at the top. My memory, especially of the early development
  is hazy (because I only developed the tool for my own use). After
  initiating git and posting the tool to github there is obviously a much
  better recollection ;)

  v3.0.18
  Code clean up

  v3.0.17
  Property Stores embedded within ItemID objects are now detected and parsed
  using the '-i' option (only when used with the '-o txt' option and not with
  the '-s' option). The command: 'lifer -i {link file(s)}' will do the trick.

  v3.0.16
  Begin to implement the extraction of property store data within IDLists

  v3.0.15
  Create a function to parse property stores (because property stores seem to 
  appear in a lot of IDLists. The Extradata Property Store functionality now
  uses this function.
  The function is made 'extern' because I need to use it in the lifer tool rather
  than the liblife library.

  v3.0.14
  Begin to implement the output for IDLists ('.txt' output only)

  v3.0.13
  A decent working version that includes most structures (with the exception of 
  ID Lists)
  
  v3.0.12
  Linux integration tweaks and fix for issue 0001
  
  v3.0.11
  ED PropertyStoreDataBlock fully implemented
  
  v3.0.9_1
  ED PropertyStoreDataBlock partially implemented
  
  v3.0.8
  ED ShimDataBlock added
  
  v3.0.7
  ED KnownFolderDataBlock added
  
  v3.0.6
  ED IconEnvironmentDataBlock added
  
  v3.0.5
  ED EnvironmentVariableDataBlock added
  
  v3.0.4
  ED DarwinDataBlock info added
  
  v3.0.3
  ED ConsoleDataBlock info added
  
  v3.0.2
  Just a version bump as a test.
  
  v3.0.1
    The first attempt to bring lifer (as it stands) in line with the current
    version of MS-SHLLINK. Actually not much changes except some of the names
  of previously capitalized constants are slightly changed and made
  mixed-case.
  
  v1.2.31
  README.md gets an update because I've started to figure out how github
  works.

  v1.2.30
  The first git commit to github. The first two integers in the version (1.2)
  just happen to align with the version of MS-SHLLINK that I had been working
  from, at this point I decide that these first two version numbers should
  always align with the MS-SHLLINK version number so that folks can see which
  version of that document we are working from (the obvious thing is that
  from this point I need to align with the then current version of 3.0) but I
  hadn't updated the code to this version.
  In truth, I	had made a number of versions that were significantly different
  but with the same version number (poor, I know!) This was a version bump to
  supercede all previous versions.

  2016 Q4 & 2017 Q1 v1.2.5
  Just tidying up the code so that it is fit to be viewed by the world at
  large. At this stage it is almost exclusively a Linux application. but has
  pretentions of moving to Windows too.

  2016 v1.1.0
  OK, so I've been employed by a forensic software company, got tired of
  being pushed around by an overbearing CEO who has no concept of people
  skills (other than being a 'pushy salesperson') (My heart goes out to you
  US employees with such bosses, in fact I don't think the US has a
  monopoly on this, I feel for everyone who has an egotist as a boss). The
  upshot is that I finally retire, properly, and start doing things I want
  to do! *One* of these is engaging brain and implementing some of the
  neglected parts of lifer. Mission 1: - learn git because it was written by
  my hero Linus Torvalds as a result of not having a suitable versioning
  system for the Linux kernel code. Result of mission: - Linus is WAY
  cleverer than me :(
  Some basic versioning is implemented

  2012 v1.0.2
  Development continued until Oct 2012 at which point I retired from the UK
  Police The tool lies dormant on my systems at this point.

  2011 Q4? v1.0.0
  You may have gathered that I hadn't incorporated versioning at this stage,
  it was just a tool to do a job (albeit a very specific job).
  Around this time I implemented the different output types (txt, csv & tsv)
  in lifer and cleaned up the code in terms of separating out the different
  functions into those that belonged in the library and those that belonged
  in the implementaton of a tool.

  2011 Q2-3? v1.0.0
  Some folks in the community became aware of my tool and started requesting
  additional features. The main one was the parsing of ItemID objects which
  seemed to contain some useful data, a brief investigation proved this to be
  in the 'too difficult' box (at least for me) and besides, I had no cases
  where content of an ItemID was an issue. I forged on with my passion and
  implemented the extrapolation of dates from UUID/GUID/CLSID objects. I also
  implemented the ability to parse a number of named files OR 1 named
  directory, again, this (admittedly somewhat weird) implementation was
  driven by my desire to parse the occasional file (or two) OR parse an
  entire directory filled with every link file I could extract from a given
  machine.


  2011 Q1? v1.0.0
  I was working as a police forensic examiner and was well aware of some of
  the data embedded within link files, particularly the dates (a passion
  of mine at the time). Most of the time, if this was important, I had to use
  a hex-editor to interpret these times. I needed a tool to extract the data
  from link files. I was aware that other data was useful forensically so I
  decided to implement a library (liblife) so that the data could be used
  within other applications. I had written tools in C# but as I was using
  Linux for a lot of my data crunching I decided to write in C (mainly
  because I was bored at the time and needed an intellectual challenge)
*/
