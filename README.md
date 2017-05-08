# lifer
A forensic tool for Windows link file examinations (i.e. Windows shortcuts)

## SYNOPSIS

'lifer' is a Windows or *nix command-line tool inspired by the whitepaper 'The Meaning of Link Files in Forensic Examinations' by Harry Parsonage and available [**here**](http://computerforensics.parsonage.co.uk/downloads/TheMeaningofLIFE.pdf).
It started life as a lightweight tool that I wrote in order to extract certain information from link files to assist in enquiries I was making whilst working as a computer forensic analyst. Now I am retired but I am looking to expand it's usefulness and publish it so that others can benefit.

The information extracted is in accordance with the Microsoft Open Specification Document 'MS-SHLLNK' which can be found online [**here**](https://msdn.microsoft.com/en-us/library/dd871305.aspx).
At the time of writing only parts of specification version 3.0 are implemented. Over time however, I hope to bring the tool into line with the full current specification and also include other goodies such as:
* A full output conforming to all of the sections in the MS-SHLLINK documentation.
* Relevant output from IDList containers
* Recognition of, and parsing of link file data within jump list containers.

## EXAMPLE USAGE
Details of the files to be found in the Test directory and how to use them is given in the '.\Test\Tests.txt' file. What follows is a brief outline...

Once you have installed the tool open a command-line shell (e.g. bash or Powershell). Type:
```
lifer -s ./Test/Test.lnk
```
This should give the output:
```
LINK FILE -------------- .\src\Test\Test1.lnk
{**OPERATING SYSTEM (stat) DATA**}
  Last Accessed:       2017-04-18 20:28:19 (UTC)
  Last Modified:       2017-04-18 20:28:19 (UTC)
  Last Changed:        2017-04-18 20:28:19 (UTC)

{**LINK FILE EMBEDDED DATA**}
  {S_2.1 - ShellLinkHeader}
    Attributes:          0x00000020   FILE_ATTRIBUTE_ARCHIVE
    Creation Time:       2008-09-12 20:27:17 (UTC)
    Access Time:         2008-09-12 20:27:17 (UTC)
    Write Time:          2008-09-12 20:27:17 (UTC)
    Target Size:         0 bytes
  {S_2.3 - LinkInfo}
    {S_2.3.1 - LinkInfo - VolumeID}
      Drive Type:        DRIVE_FIXED
      Drive Serial No:   307A8A81
      Volume Label:      [EMPTY]
      Local Base Path:   C:\test\a.txt
  {S_2.4 - StringData}
    {S_2.4 - StringData - RELATIVE_PATH}
      Relative Path:     .\a.txt
    {S_2.4 - StringData - WORKING_DIR}
      Working Dir:       C:\test
  {S_2.5 - ExtraData}
    {S_2.5.10 - ExtraData - TrackerDataBlock}
      MachineID:         chris-xps
      Droid1:            {94C77840-FA47-46C7-B356-5C2DC6B6D115}
      Droid2:            {7BCD46EC-7F22-11DD-9499-00137216874A}
        UUID Sequence:     153
        UUID Time:         2008-09-10 10:23:17 (UTC)
        UUID Node (MAC):   00:13:72:16:87:4A
```
A more fulsome output (including more accurate timestamps) can be obtained by omitting the '-s' option.

All the link files in a directory (folder) can be parsed by just passing the name of the directory:
```
lifer ./Test/WinXP
```
(for brevity the output has not been shown).

The most useful output for a number of link files can be created by sending the output as a tab (or comma) separated list to a file that can then be imported into a spreadsheet for analysis at your leisure. This can be achieved like this:
```
lifer -o tsv ./Test/WinXP > WinXP.tsv
```
or
```
lifer -so tsv ./Test/WinXP > WinXP.tsv
```
for a file that has some of the superfluous and uninteresting data redacted.
### WARNING ABOUT COMMA SEPARATED OUTPUT!!
Strings within link files can sometimes contain commas. Because this causes a conflict with the field separator any commas within strings have been replaced with semi-colons (i.e. ',' replaced with ';'). This is only true for the '-o csv' option and not the default '-o txt' or the '-o tsv' options.

## MOTIVATION
Windows link files (shortcuts) can harbour a trove of information for a forensic analyst. For example, perhaps determining that a disk that is no longer attached to the machine may well have been attached sometime in the past or maybe an indication of the the names and location of folders that have since been deleted.
As a forensic analyst I was using tools to interpret this data for me but none of them were open-source and I had no idea that the information presented was correct. In particular I had a concern that dates and times were not being interpreted fully and accurately and so I wrote a tool to do the job.
Initially lifer just parsed the information I was after but as with these things, I needed more and more information until I was interpreting pretty much the whole link file so I separated the tool and the library. Around 2012 I retired from my role and development pretty much stopped. Now I have more time so I've started on the project anew...

## INSTALLATION
The first thing to do is to ensure you have git installed on your machine/device; in a command-line shell, change to your desired project root directory and issue the command:
```
git clone https://github.com/Paul-Tew/lifer.git
```
A new directory named 'lifer' will be created.

#### LINUX INSTALLATION (and other *nix platforms)
(This may work for Mac installations but I don't have the kind of money needed to test it out for sure...)
Because this tool is pretty basic, the dependencies are minimal, ensure you have the 'gcc' compiler and the relevant 'libc' development libraries installed, that's all.
Start a command-line terminal and navigate to the **./lifer/src** directory.
Issue the command:
```
gcc -Wall ./lifer.c ./liblife/liblife.c -o lifer
```
Provided no warnings or errors appeared, you should now have an executable file 'lifer' sitting in the directory, you might want to check this by issuing the command:
```
ls -la
```
If all is OK then you can test that lifer works by testing it out on the file specified in the Microsoft document which I included as part of the git repository you cloned and should be sitting in the ./Test/ directory. You can do this by issuing the command:
```
./lifer ./Test/Test.lnk
```
You can also test that lifer works on a bunch of link files sitting in a directory by issuing the command:
```
./lifer ./Test/WinXP/
```
Install the tool onto the OS by issuing the command:
```
sudo install ./lifer /usr/bin/
```
This will enable you to use lifer anywhere on your system without specifying the directory prefix (e.g. `lifer ./Test/Test.lnk` rather than `./lifer ./Test/Test.lnk`)

#### WINDOWS INSTALLATION
The lifer github project comes complete with a Visual Studio 2017 project solution so the easiest way to create a Windows executable is to install Visual Studio 2017 first. There is a free version (known as the 'community' version) available [here](https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15).
Once Visual Studio is installed:
* Left-click on **File->Open->Project/Solution** and browse to the **lifer.sln** file to load the solution into Visual Studio.
* On the Standard Toolbar, set the Solution Configuration options to those that suit your machine and preference (for example, I use: 'x64' and 'Debug')
* Build the solution from the 'Build' menu or simply use the key combination: **Ctrl+Shift+B**
* Provided there were no errors you should have an executable 'lifer.exe' file in the relevant sub-folder of your project.
* At this point I usually open a Powershell terminal and navigate to the folder containing the executable which for me is done by issuing the command:
```
cd "F:\\lifer\src\x64\Debug\"
```
* I then test the executable using the command:
```
.\lifer.exe ..\..\Test\Test.lnk
```
---
It is possible to make lifer in Windows without installing Visual Studio but you will still need to download and install the Visual C++ build tools available [here](http://landinghub.visualstudio.com/visual-cpp-build-tools)
Once installed, lifer can be built in the ./src/ directory by issuing the command:
```
CL lifer.c .\liblife\liblife.c .\Win\dirent.c .\Win\getopt.c
```
## ACKNOWLEDGEMENTS
'lifer' was originally a Linux/GNU only tool which was not really portable into Windows until I found solutions to the main stumbling blocks of navigating a directory and parsing the command-line options in the same way that GNU does. To this end I am deeply indebted to the following two projects:
1. [dirent](http://www.two-sdg.demon.co.uk/curbralan/code/dirent/dirent.html)     Kevlin Henney
2. [getopt](https://www.codeproject.com/articles/157001/full-getopt-port-for-unicode-and-multibyte-microso)     Ludvik Jerabek

## INTERPRETATION OF OUTPUT
Users are encouraged to read the [whitepaper](http://computerforensics.parsonage.co.uk/downloads/TheMeaningofLIFE.pdf) before assigning any meaning to results. No results should be ascribed to this tool without a FULL understanding of what the output represents; this particularly applies to matters of fact for determination in a court of law. In such cases it is incumbent on the user to understand both of the aformentioned documents fully as well as having a comprehensive grasp on how Windows and other OS's treat the creation, moving, deletion of such files. A working knowledge of how 'lifer' has interpreted and presented the data is also needed (this requires reading and **understanding** the code).

## INFORMAL DISCLAIMER
I am only a self-taught programmer so no doubt there are loads of errors and 'gotchas' in the code. To this end, I make absolutely NO promises that this tool won't harm your system. I tried hard not to bust your machine but the road to hell is paved with good intentions...
## FORMAL DISCLAIMER
THIS MATERIAL IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SOME JURISDICTIONS DO NOT ALLOW THE EXCLUSION OF IMPLIED WARRANTIES, SO THE ABOVE EXCLUSION MAY NOT APPLY TO YOU. IN NO EVENT WILL I BE LIABLE TO ANY PARTY FOR ANY DIRECT, INDIRECT, SPECIAL OR OTHER CONSEQUENTIAL DAMAGES FOR ANY USE OF THIS MATERIAL INCLUDING, WITHOUT LIMITATION, ANY LOST PROFITS, BUSINESS INTERRUPTION, LOSS OF PROGRAMS OR OTHER DATA ON YOUR INFORMATION HANDLING SYSTEM OR OTHERWISE, EVEN If WE ARE EXPRESSLY ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

Paul Tew - Apr 2017