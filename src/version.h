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

// These are equivalent to the protocol revision for MS-SHLLNK
#define _MAJOR  3
#define _MINOR  0
// This is my revision number
#define _BUILD  12 // Housekeeping
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
