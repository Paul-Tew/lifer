'lifer' is a command-line tool inspired by the whitepaper 'The Meaning of Link
Files in Forensic Examinations' by Harry Parsonage and available from:
http://computerforensics.parsonage.co.uk/downloads/TheMeaningofLIFE.pdf
It started life as a lightweight tool that I wrote in order to extract certain
information from link files to assist in enquiries I was making whilst working
as a computer forensic analyst. Now I am retired I am looking to expand it's 
usefulness and publish it so that others can benefit.

The tool uses a static library (liblife) which parses the file in accordance
with the Microsoft Open Specification Document 'MS-SHLLNK' which can be 
found online at:
https://msdn.microsoft.com/en-us/library/dd871305.aspx
At the time of writing only parts of specification version 1.2 are implemented,
over time I hope to bring the tool into line with the full current 
specification (version 3.0).

The tool is designed to extract as much information as is available from a 
Windows shortcut file (or collection of such files). Output is to the terminal
screen but a simple redirection can create a text file or a comma/tab separated
file that can easily be imported into a spreadsheet.

Users are encouraged to read the whitepaper before assigning any meaning to 
results. No results should be ascribed to this tool without a FULL 
understanding of what the output represents; this particularly applies to
matters of fact for determination in a court of law. In such cases it is
encumbent on a user to understand the two documents fully as well as having a
full grasp on how Windows and other OS's treat the creation, moving, deletion
of such files. A working knowledge of how 'lifer' has interpreted and 
presented the data is also needed.

I am only a self-taught programmer so no doubt there are loads of errors and
'gotchas' in the code. To this end, I make NO promises that this tool won't 
harm your system.

**DISCLAIMER**
THIS MATERIAL IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING, BUT Not LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE, OR NON-INFRINGEMENT. SOME JURISDICTIONS DO NOT ALLOW THE
EXCLUSION OF IMPLIED WARRANTIES, SO THE ABOVE EXCLUSION MAY NOT
APPLY TO YOU. IN NO EVENT WILL I BE LIABLE TO ANY PARTY FOR ANY
DIRECT, INDIRECT, SPECIAL OR OTHER CONSEQUENTIAL DAMAGES FOR ANY
USE OF THIS MATERIAL INCLUDING, WITHOUT LIMITATION, ANY LOST
PROFITS, BUSINESS INTERRUPTION, LOSS OF PROGRAMS OR OTHER DATA ON
YOUR INFORMATION HANDLING SYSTEM OR OTHERWISE, EVEN If WE ARE
EXPRESSLY ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

Paul Tew 2017


