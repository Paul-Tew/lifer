/*********************************************************
**                                                      **
**                      lifer                           **
**                                                      **
**            A Windows link file examiner              **
**                                                      **
**         Copyright Paul Tew 2011 to 2017              **
**                                                      **
** Usage:                                               **
** lifer [-vh]                                          **
** lifer [-s] [-o csv|tsv|txt] dir|file(s)              **
**                                                      **
*********************************************************/

/*
This file is part of lifer.

    Lifer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    lifer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with lifer.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <wchar.h>
// local headers
#include "./liblife/liblife.h"
#include "./version.h"

//Conditional includes and definitions dependant on OS
#ifdef _WIN32
// Windows 
#include <io.h>
#include "./win/dirent.h"
#include "./win/getopt.h"
#include <direct.h>
#define PATH_MAX _MAX_PATH // Why is this different between Win & *nix? (I have no idea BTW)
#else
// *nix 
#include <unistd.h>
#include <dirent.h>
#define _getcwd getcwd  // _getcwd() is Windows, getcwd() is *nix
#define _chdir chdir    // same issue here
#endif

//Global stuff
enum otype { csv, tsv, txt };
enum otype output_type;
int filecount;

//Function help_message() prints a help message to stdout
void help_message()
{
  printf("********************************************************************************\n");
  printf("\nlifer - A Windows link file analyser\n");
  printf("lifer - A Windows link file analyser\n");
  printf("Version: %u.%u.%u\n", _MAJOR, _MINOR, _BUILD);
  printf("Usage: lifer  [-vhs] [-o csv|tsv|txt] file(s)|directory\n\n");
  printf("Options:\n  -v    print version number\n  -h    print this help\n");
  printf("  -s    shortened output (default is to output all fields)\n");
  printf("  -i    print idlist information (only compatible with output type: full 'txt')\n");
  printf("  -o    output type (choose from csv, tsv or txt). \n");
  printf("        The default is txt.\n\n");
  printf("Output is to standard output, to send to a file, use the redirection\n");
  printf("operator '>'.\n\n");
  printf("Example:\n  lifer -o csv {DIRECTORY} > Links.csv\n\n");
  printf("This will create a comma seperated file named Links.csv in the current\n");
  printf("directory. The file can be viewed in a spreadsheet and will contain details\n");
  printf("of all the link files found in the named directory.\n\n");
  printf("********************************************************************************\n");
}


//
//Function: replace_comma(unsigned char * str, int len)
//          Takes a zero terminated string 'str' of length 'len' max
//          and substitutes semi-colons for commas.
//          Return value is the number of substitutions.
//          This function is useful for CSV output so that strings don't mess 
//          up the formatting
int replace_comma(unsigned char * str, uint16_t len)
{
  int result = 0, i;
  for (i = 0; (i < len); i++)
  {
    if (str[i] == ',')
    {
      str[i] = ';';
      result++;
    }
  }
  return result;
}

//
//Function: sv_out(FILE * fp) processes the link file and outputs the csv or tsv
//          version of the decoded data.
void sv_out(FILE* fp, char* fname, int less, char sep)
{
  struct LIF   lif;
  struct LIF_A lif_a;
  struct stat statbuf;
  char buf[40];
  int  i, j;


  // Get the stat info for the file itself
  stat(fname, &statbuf);

  if (get_lif(fp, statbuf.st_size, &lif) < 0)
  {
    fprintf(stderr, "Error processing file \'%s\' - sorry\n", fname);
    return;
  }
  if (get_lif_a(&lif, &lif_a))
  {
    fprintf(stderr, "Could not make ASCII version of \'%s\' - sorry\n", fname);
    return;
  }
  //Now print the header if needed
  if (filecount == 0)
  {
    printf("File Name%c", sep);
    if (less == 0)
    {
      printf("Link File Size%c", sep);
    }
    printf("Link File Last Accessed%cLink File Last Modified%c", sep, sep);
    printf("Link File Last Changed%c", sep);
    if (less == 0)
    {
      printf("Hdr Size%cHdr CLSID%cHdr Flags%c", sep, sep, sep);
    }
    printf("Hdr Attributes%c", sep);
    printf("Hdr FileCreate%cHdr FileAccess%c", sep, sep);
    printf("Hdr FileWrite%c", sep);
    printf("Hdr TargetSize%c", sep);
    if (less == 0)
    {
      printf("Hdr IconIndex%cHdr WindowState%cHdr HotKeys%c", sep, sep, sep);
      printf("Hdr Reserved1%cHdr Reserved2%cHdr Reserved3%c", sep, sep, sep);
      printf("IDList Size%c", sep);
      printf("IDList No Items%c", sep);
      printf("LinkInfo Size%c", sep);
      printf("LinkInfo Hdr Size%c", sep);
      printf("LinkInfo Flags%c", sep);
      printf("LinkInfo VolID Offset%c", sep);
      printf("LinkInfo Base Path Offset%c", sep);
      printf("LinkInfo CNR Offset%c", sep);
      printf("LinkInfo CPS Offset%c", sep);
      printf("LinkInfo LBP Offset Unicode%c", sep);
      printf("LinkInfo CPS Offset Unicode%c", sep);
      printf("LinkInfo VolID Size%c", sep);
    }
    printf("LinkInfo VolID Drive Type%c", sep);
    printf("LinkInfo VolID Drive Ser No%c", sep);
    if (less == 0)
    {
      printf("LinkInfo VolID VLOffset%c", sep);
      printf("LinkInfo VolID VLOffsetU%c", sep);
    }
    printf("LinkInfo VolID Vol Label%c", sep);
    printf("LinkInfo VolID Vol LabelU%c", sep);
    printf("LinkInfo Local Base Path%c", sep);
    if (less == 0)
    {
      printf("LinkInfo CNR Size%c", sep);
      printf("LinkInfo CNR Flags%c", sep);
      printf("LinkInfo CNR NetNameOffset%c", sep);
      printf("LinkInfo CNR DeviceNameOffset%c", sep);
    }
    printf("LinkInfo CNR NetwkProviderType%c", sep);
    if (less == 0)
    {
      printf("LinkInfo CNR NetNameOffsetU%c", sep);
      printf("LinkInfo CNR DeviceNameOffsetU%c", sep);
    }
    printf("LinkInfo CNR NetName%c", sep);
    printf("LinkInfo CNR DeviceName%c", sep);
    printf("LinkInfo CNR NetNameU%c", sep);
    printf("LinkInfo CNR DeviceNameU%c", sep);

    printf("LinkInfo Common Path Suffix%c", sep);
    printf("LinkInfo Local Base Path Unicode%c", sep);
    printf("LinkInfo Common Path Suffix Unicode%c", sep);

    if (less == 0)
    {
      printf("StrData Total Size (bytes)%c", sep);
      printf("StrData Name Num Chars%c", sep);
    }
    printf("StrData - Name%c", sep);
    if (less == 0)
    {
      printf("StrData Rel Path Num Chars%c", sep);
    }
    printf("StrData Relative Path%c", sep);
    if (less == 0)
    {
      printf("StrData Working Dir Num Chars%c", sep);
    }
    printf("StrData Working Dir%c", sep);
    if (less == 0)
    {
      printf("StrData Cmd Line Args Num Chars%c", sep);
    }
    printf("StrData Cmd Line Args%c", sep);
    if (less == 0)
    {
      printf("StrData Icon Loc Num Chars%c", sep);
    }
    printf("StrData Icon Location%c", sep);
    // S2.5 ExtraData structures
    if (less == 0)
    {
      printf("ExtraData Total Size (bytes)%c", sep);
    }
    printf("ExtraData Structures%c", sep);

    // S2.5.1 ConsoleDataBlock
    if (less == 0)
    {
      printf("ED CDB File Offset (bytes)%c", sep);
      printf("ED CDB Size (bytes)%c", sep);
      printf("ED CDB Signature%c", sep);
      printf("ED CDB FillAttributes%c", sep);
      printf("ED CDB PopupFillAttr%c", sep);
      printf("ED CDB ScrBufSizeX%c", sep);
      printf("ED CDB ScrBufSizeY%c", sep);
      printf("ED CDB WindowSizeX%c", sep);
      printf("ED CDB WindowSizeY%c", sep);
      printf("ED CDB WindowOriginX%c", sep);
      printf("ED CDB WindowOriginY%c", sep);
      printf("ED CDB Unused1%c", sep);
      printf("ED CDB Unused2%c", sep);
      printf("ED CDB FontSize%c", sep);
      printf("ED CDB FontFamily%c", sep);
      printf("ED CDB FontWeight%c", sep);
      printf("ED CDB FaceName%c", sep);
      printf("ED CDB CursorSize%c", sep);
      printf("ED CDB FullScreen%c", sep);
      printf("ED CDB QuickEdit%c", sep);
      printf("ED CDB InsertMode%c", sep);
      printf("ED CDB AutoPosition%c", sep);
      printf("ED CDB HistoryBufSize%c", sep);
      printf("ED CDB NumHistoryBuf%c", sep);
      printf("ED CDB HistoryNoDup%c", sep);
      printf("ED CDB ColorTable%c", sep);
    }
    // S2.5.2 ConsoleFEDataBlock
    if (less == 0)
    {
      printf("ED CFEDB File Offset (bytes)%c", sep);
      printf("ED CFEDB Size (bytes)%c", sep);
      printf("ED CFEDB Signature%c", sep);
      printf("ED CFEDB CodePage%c", sep);
    }
    // S2.5.3 DarwinDataBlock
    if (less == 0)
    {
      printf("ED DDB File Offset (bytes)%c", sep);
      printf("ED DDB Size (bytes)%c", sep);
      printf("ED DDB Signature%c", sep);
      printf("ED DDB DarwinDataAnsi%c", sep);
      printf("ED DDB DarwinDataUnicode%c", sep);
    }
    // S2.5.4 EnvironmentVariableDataBlock
    if (less == 0)
    {
      printf("ED EVDB File Offset (bytes)%c", sep);
      printf("ED EVDB Size (bytes)%c", sep);
      printf("ED EVDB Signature%c", sep);
      printf("ED EVDB TargetAnsi%c", sep);
      printf("ED EVDB TargetUnicode%c", sep);
    }
    // S2.5.5 IconEnvironmentDataBlock
    if (less == 0)
    {
      printf("ED IEDB File Offset (bytes)%c", sep);
      printf("ED IEDB Size (bytes)%c", sep);
      printf("ED IEDB Signature%c", sep);
      printf("ED IEDB TargetAnsi%c", sep);
      printf("ED IEDB TargetUnicode%c", sep);
    }
    // S2.5.6 KnownFolderDataBlock
    if (less == 0)
    {
      printf("ED KFDB File Offset (bytes)%c", sep);
      printf("ED KFDB Size (bytes)%c", sep);
      printf("ED KFDB Signature%c", sep);
      printf("ED KFDB KnownFolderID%c", sep);
      printf("ED KFDB Offset%c", sep);
    }
    // S2.5.7 PropertyStoreDataBlock
    if (less == 0)
    {
      printf("ED PS File Offset (bytes)%c", sep);
      printf("ED PS Size (bytes)%c", sep);
      printf("ED PS Signature%c", sep);
      printf("ED PS Number of Stores %c", sep);
    }
    // S2.5.9 SpecialFolderDataBlock
    if (less == 0)
    {
      printf("ED SFolderData File Offset (bytes)%c", sep);
      printf("ED SFolderData Size (bytes)%c", sep);
      printf("ED SFolderData Signature%c", sep);
      printf("ED SFolderData ID%c", sep);
      printf("ED SFolderData Offset%c", sep);
    }
    // S 2.5.10 TrackerDataBlock
    if (less == 0)
    {
      printf("ED TrackerData File Offset (bytes)%c", sep);
      printf("ED TrackerData Size (bytes)%c", sep);
      printf("ED TrackerData Signature%c", sep);
      printf("ED TrackerData Length%c", sep);
      printf("ED TrackerData Version%c", sep);
    }
    printf("ED TrackerData MachineID%c", sep);
    printf("ED TrackerData Droid1%c", sep);
    if (less == 0)
    {
      printf("ED TD Droid1 Version%c", sep);
      printf("ED TD Droid1 Variant%c", sep);
    }
    printf("ED TD Droid1 Time%c", sep);
    printf("ED TD Droid1 Clock Seq%c", sep);
    printf("ED TD Droid1 Node%c", sep);
    printf("ED TrackerData Droid2%c", sep);
    if (less == 0)
    {
      printf("ED TD Droid2 Version%c", sep);
      printf("ED TD Droid2 Variant%c", sep);
    }
    printf("ED TD Droid2 Time%c", sep);
    printf("ED TD Droid2 Clock Seq%c", sep);
    printf("ED TD Droid2 Node%c", sep);
    printf("ED TrackerData DroidBirth1%c", sep);
    if (less == 0)
    {
      printf("ED TD DroidBirth1 Version%c", sep);
      printf("ED TD DroidBirth1 Variant%c", sep);
    }
    printf("ED TD DroidBirth1 Time%c", sep);
    printf("ED TD DroidBirth1 Clock Seq%c", sep);
    printf("ED TD DroidBirth1 Node%c", sep);
    printf("ED TrackerData DroidBirth2%c", sep);
    if (less == 0)
    {
      printf("ED TD DroidBirth2 Version%c", sep);
      printf("ED TD DroidBirth2 Variant%c", sep);
    }
    printf("ED TD DroidBirth2 Time%c", sep);
    printf("ED TD DroidBirth2 Clock Seq%c", sep);
    printf("ED TD DroidBirth2 Node%c", sep);
    //ED Vista & above IDList
    if (less == 0)
    {
      printf("ED >= Vista IDList File Offset (bytes)%c", sep);
      printf("ED >= Vista IDList Size%c", sep);
      printf("ED >= Vista IDList Signature%c", sep);
      printf("ED >= Vista IDList Num Items%c", sep);
    }
    printf("\n");
  }
  //Print a record
  printf("%s%c", fname, sep);
  if (less == 0)
  {
    printf("%u%c", (unsigned int)statbuf.st_size, sep);
  }
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_atime));
  printf("%s%c", buf, sep);
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_mtime));
  printf("%s%c", buf, sep);
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_ctime));
  printf("%s%c", buf, sep);
  if (less == 0)
  {
    printf("%s%c%s%c", lif_a.lha.H_size, sep, lif_a.lha.CLSID, sep);
    printf("%s%c", lif_a.lha.Flags, sep);
  }
  printf("%s%c", lif_a.lha.Attr, sep);
  if (less == 0)
  {
    printf("%s%c", lif_a.lha.CrDate_long, sep);
    printf("%s%c%s%c", lif_a.lha.AcDate_long, sep, lif_a.lha.WtDate_long, sep);
  }
  else
  {
    printf("%s%c", lif_a.lha.CrDate, sep);
    printf("%s%c%s%c", lif_a.lha.AcDate, sep, lif_a.lha.WtDate, sep);
  }
  printf("%s%c", lif_a.lha.Size, sep);
  if (less == 0)
  {
    printf("%s%c%s%c", lif_a.lha.IconIndex, sep, lif_a.lha.ShowState, sep);
    printf("%s%c%s%c", lif_a.lha.Hotkey, sep, lif_a.lha.Reserved1, sep);
    printf("%s%c%s%c", lif_a.lha.Reserved2, sep, lif_a.lha.Reserved3, sep);
    printf("%s%c", lif_a.lidla.IDListSize, sep);
    printf("%s%c", lif_a.lidla.NumItemIDs, sep);
    printf("%s%c", lif_a.lia.Size, sep);
    printf("%s%c", lif_a.lia.HeaderSize, sep);
    printf("%s%c", lif_a.lia.Flags, sep);
    printf("%s%c", lif_a.lia.IDOffset, sep);
    printf("%s%c", lif_a.lia.LBPOffset, sep);
    printf("%s%c", lif_a.lia.CNRLOffset, sep);
    printf("%s%c", lif_a.lia.CPSOffset, sep);
    printf("%s%c", lif_a.lia.LBPOffsetU, sep);
    printf("%s%c", lif_a.lia.CPSOffsetU, sep);
    printf("%s%c", lif_a.lia.VolID.Size, sep);
  }
  printf("%s%c", lif_a.lia.VolID.DriveType, sep);
  printf("%s%c", lif_a.lia.VolID.DriveSN, sep);
  if (less == 0)
  {
    printf("%s%c", lif_a.lia.VolID.VLOffset, sep);
    printf("%s%c", lif_a.lia.VolID.VLOffsetU, sep);
  }
  printf("%s%c", lif_a.lia.VolID.VolumeLabel, sep);
  printf("%s%c", lif_a.lia.VolID.VolumeLabelU, sep);

  printf("%s%c", lif_a.lia.LBP, sep);
  if (less == 0)
  {
    printf("%s%c", lif_a.lia.CNR.Size, sep);
    printf("%s%c", lif_a.lia.CNR.Flags, sep);
    printf("%s%c", lif_a.lia.CNR.NetNameOffset, sep);
    printf("%s%c", lif_a.lia.CNR.DeviceNameOffset, sep);
  }
  printf("%s%c", lif_a.lia.CNR.NetworkProviderType, sep);
  if (less == 0)
  {
    printf("%s%c", lif_a.lia.CNR.NetNameOffsetU, sep);
    printf("%s%c", lif_a.lia.CNR.DeviceNameOffsetU, sep);
  }
  printf("%s%c", lif_a.lia.CNR.NetName, sep);
  printf("%s%c", lif_a.lia.CNR.DeviceName, sep);
  printf("%s%c", lif_a.lia.CNR.NetNameU, sep);
  printf("%s%c", lif_a.lia.CNR.DeviceNameU, sep);
  printf("%s%c", lif_a.lia.CPS, sep);
  printf("%s%c", lif_a.lia.LBPU, sep);
  printf("%s%c", lif_a.lia.CPSU, sep);

  if (less == 0)
  {
    printf("%s%c", lif_a.lsda.Size, sep);
  }
  for (i = 0; i < 5; i++)
  {
    if (less == 0)
    {
      printf("%s%c", lif_a.lsda.CountChars[i], sep);
    }
    //If csv output then replace a comma in the string with a semi-colon
    if (output_type == csv)
    {
      replace_comma(lif_a.lsda.Data[i], lif.lsd.CountChars[i]);
    }
    printf("%s%c", lif_a.lsda.Data[i], sep);
  }
  // S2.5 ExtraData
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.Size, sep);
  }
  printf("%s%c", lif_a.leda.edtypes, sep);

  // S2.5.1 ConsoleDataBlock
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.lcpa.Posn, sep);
    printf("%s%c", lif_a.leda.lcpa.Size, sep);
    printf("%s%c", lif_a.leda.lcpa.sig, sep);
    printf("%s%c", lif_a.leda.lcpa.FillAttributes, sep);
    printf("%s%c", lif_a.leda.lcpa.PopupFillAttributes, sep);
    printf("%s%c", lif_a.leda.lcpa.ScreenBufferSizeX, sep);
    printf("%s%c", lif_a.leda.lcpa.ScreenBufferSizeY, sep);
    printf("%s%c", lif_a.leda.lcpa.WindowSizeX, sep);
    printf("%s%c", lif_a.leda.lcpa.WindowSizeY, sep);
    printf("%s%c", lif_a.leda.lcpa.WindowOriginX, sep);
    printf("%s%c", lif_a.leda.lcpa.WindowOriginY, sep);
    printf("%s%c", lif_a.leda.lcpa.Unused1, sep);
    printf("%s%c", lif_a.leda.lcpa.Unused2, sep);
    printf("%s%c", lif_a.leda.lcpa.FontSize, sep);
    printf("%s%c", lif_a.leda.lcpa.FontFamily, sep);
    printf("%s%c", lif_a.leda.lcpa.FontWeight, sep);
    printf("%s%c", lif_a.leda.lcpa.FaceName, sep);
    printf("%s%c", lif_a.leda.lcpa.CursorSize, sep);
    printf("%s%c", lif_a.leda.lcpa.FullScreen, sep);
    printf("%s%c", lif_a.leda.lcpa.QuickEdit, sep);
    printf("%s%c", lif_a.leda.lcpa.InsertMode, sep);
    printf("%s%c", lif_a.leda.lcpa.AutoPosition, sep);
    printf("%s%c", lif_a.leda.lcpa.HistoryBufferSize, sep);
    printf("%s%c", lif_a.leda.lcpa.NumberOfHistoryBuffers, sep);
    printf("%s%c", lif_a.leda.lcpa.HistoryNoDup, sep);
    for (j = 0; j < 15; j++)
    {
      //15 consecutive ColorTable Entries
      printf("%s%c", lif_a.leda.lcpa.ColorTable[j], ';');
    }
    // And the last one terminated with the field separator
    printf("%s%c", lif_a.leda.lcpa.ColorTable[j], sep);
  }
  // S2.5.2 ConsoleFEDataBlock
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.lcfepa.Posn, sep);
    printf("%s%c", lif_a.leda.lcfepa.Size, sep);
    printf("%s%c", lif_a.leda.lcfepa.sig, sep);
    printf("%s%c", lif_a.leda.lcfepa.CodePage, sep);
  }
  // S2.5.3 DarwinDataBlock
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.ldpa.Posn, sep);
    printf("%s%c", lif_a.leda.ldpa.Size, sep);
    printf("%s%c", lif_a.leda.ldpa.sig, sep);
    if (output_type == csv)
    {
      replace_comma(lif_a.leda.ldpa.DarwinDataAnsi, 260);
      replace_comma(lif_a.leda.ldpa.DarwinDataUnicode, 520);
    }

    printf("%s%c", lif_a.leda.ldpa.DarwinDataAnsi, sep);
    printf("%s%c", lif_a.leda.ldpa.DarwinDataUnicode, sep);
  }
  // S2.5.4 EnvironmentVariableDataBlock
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.lepa.Posn, sep);
    printf("%s%c", lif_a.leda.lepa.Size, sep);
    printf("%s%c", lif_a.leda.lepa.sig, sep);
    if (output_type == csv)
    {
      replace_comma(lif_a.leda.lepa.TargetAnsi, 260);
      replace_comma(lif_a.leda.lepa.TargetUnicode, 520);
    }

    printf("%s%c", lif_a.leda.lepa.TargetAnsi, sep);
    printf("%s%c", lif_a.leda.lepa.TargetUnicode, sep);
  }
  // S2.5.5 IconEnvironmentDataBlock
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.liepa.Posn, sep);
    printf("%s%c", lif_a.leda.liepa.Size, sep);
    printf("%s%c", lif_a.leda.liepa.sig, sep);
    if (output_type == csv)
    {
      replace_comma(lif_a.leda.liepa.TargetAnsi, 260);
      replace_comma(lif_a.leda.liepa.TargetUnicode, 520);
    }
    printf("%s%c", lif_a.leda.liepa.TargetAnsi, sep);
    printf("%s%c", lif_a.leda.liepa.TargetUnicode, sep);
  }
  // S2.5.7 PropertyStoreDataBlock
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.lkfpa.Posn, sep);
    printf("%s%c", lif_a.leda.lkfpa.Size, sep);
    printf("%s%c", lif_a.leda.lkfpa.sig, sep);
    printf("%s%c", lif_a.leda.lkfpa.KFGUID.UUID, sep);
    printf("%s%c", lif_a.leda.lkfpa.KFOffset, sep);
  }
  // S2.5.7 PropertyStoreDataBlock
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.lpspa.Posn, sep);
    printf("%s%c", lif_a.leda.lpspa.Size, sep);
    printf("%s%c", lif_a.leda.lpspa.sig, sep);
    printf("%s%c", lif_a.leda.lpspa.NumStores, sep);
  }
  // S2.5.9 SpecialFolderDataBlock
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.lsfpa.Posn, sep);
    printf("%s%c", lif_a.leda.lsfpa.Size, sep);
    printf("%s%c", lif_a.leda.lsfpa.sig, sep);
    printf("%s%c", lif_a.leda.lsfpa.SpecialFolderID, sep);
    printf("%s%c", lif_a.leda.lsfpa.Offset, sep);
  }
  // S2.5.10 TrackerDataBlock
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.ltpa.Posn, sep);
    printf("%s%c", lif_a.leda.ltpa.Size, sep);
    printf("%s%c", lif_a.leda.ltpa.sig, sep);
    printf("%s%c", lif_a.leda.ltpa.Length, sep);
    printf("%s%c", lif_a.leda.ltpa.Version, sep);
  }
  printf("%s%c", lif_a.leda.ltpa.MachineID, sep);
  printf("%s%c", lif_a.leda.ltpa.Droid1.UUID, sep);
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.ltpa.Droid1.Version, sep);
    printf("%s%c", lif_a.leda.ltpa.Droid1.Variant, sep);
    printf("%s%c", lif_a.leda.ltpa.Droid1.Time_long, sep);
  }
  else
  {
    printf("%s%c", lif_a.leda.ltpa.Droid1.Time, sep);
  }
  printf("%s%c", lif_a.leda.ltpa.Droid1.ClockSeq, sep);
  printf("%s%c", lif_a.leda.ltpa.Droid1.Node, sep);
  printf("%s%c", lif_a.leda.ltpa.Droid2.UUID, sep);
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.ltpa.Droid2.Version, sep);
    printf("%s%c", lif_a.leda.ltpa.Droid2.Variant, sep);
    printf("%s%c", lif_a.leda.ltpa.Droid2.Time_long, sep);
  }
  else
  {
    printf("%s%c", lif_a.leda.ltpa.Droid2.Time, sep);
  }
  printf("%s%c", lif_a.leda.ltpa.Droid2.ClockSeq, sep);
  printf("%s%c", lif_a.leda.ltpa.Droid2.Node, sep);
  printf("%s%c", lif_a.leda.ltpa.DroidBirth1.UUID, sep);
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.ltpa.DroidBirth1.Version, sep);
    printf("%s%c", lif_a.leda.ltpa.DroidBirth1.Variant, sep);
    printf("%s%c", lif_a.leda.ltpa.DroidBirth1.Time_long, sep);
  }
  else
  {
    printf("%s%c", lif_a.leda.ltpa.DroidBirth1.Time, sep);
  }
  printf("%s%c", lif_a.leda.ltpa.DroidBirth1.ClockSeq, sep);
  printf("%s%c", lif_a.leda.ltpa.DroidBirth1.Node, sep);
  printf("%s%c", lif_a.leda.ltpa.DroidBirth2.UUID, sep);
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.ltpa.DroidBirth2.Version, sep);
    printf("%s%c", lif_a.leda.ltpa.DroidBirth2.Variant, sep);
    printf("%s%c", lif_a.leda.ltpa.DroidBirth2.Time_long, sep);
  }
  else
  {
    printf("%s%c", lif_a.leda.ltpa.DroidBirth2.Time, sep);
  }
  printf("%s%c", lif_a.leda.ltpa.DroidBirth2.ClockSeq, sep);
  printf("%s%c", lif_a.leda.ltpa.DroidBirth2.Node, sep);
  // S2.5.11 VistaAndAboveIDListDataBlock
  if (less == 0)
  {
    printf("%s%c", lif_a.leda.lvidlpa.Posn, sep);
    printf("%s%c", lif_a.leda.lvidlpa.Size, sep);
    printf("%s%c", lif_a.leda.lvidlpa.sig, sep);
    printf("%s%c", lif_a.leda.lvidlpa.NumItemIDs, sep);
  }
  printf("\n");
}

//
//Function: text_out(FILE * fp) processes the link file and outputs the text
//          version of the decoded data.
void text_out(FILE* fp, char* fname, int less, int itemid)
{
  struct LIF     lif;
  struct LIF_A   lif_a;
  struct stat    statbuf;
  char           buf[200];
  int            i, j, k, idpos = 0;
  struct LIF_PROPERTY_STORE_PROPS  psp;
  struct LIF_SER_PROPSTORE_A  psa;

  // Get the stat info for the file itself
  stat(fname, &statbuf);

  if (get_lif(fp, statbuf.st_size, &lif) < 0)
  {
    fprintf(stderr, "Error processing file \'%s\' - sorry\n", fname);
    return;
  }
  if (get_lif_a(&lif, &lif_a))
  {
    fprintf(stderr, "Could not make ASCII version of \'%s\' - sorry\n", fname);
    return;
  }
  //Print out the results
  printf("\nLINK FILE -------------- %s\n", fname);
  printf("{**OPERATING SYSTEM (stat) DATA**}\n");
  //Print a record
  if (less == 0) //omit this stuff if short info required
  {
    printf("  File Size:           %u bytes\n", (unsigned int)statbuf.st_size);
  }
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_atime));
  printf("  Last Accessed:       %s\n", buf);
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_mtime));
  printf("  Last Modified:       %s\n", buf);
  strftime(buf, 29, "%Y-%m-%d %H:%M:%S (UTC)", gmtime(&statbuf.st_ctime));
  printf("  Last Changed:        %s\n\n", buf);

  printf("{**LINK FILE EMBEDDED DATA**}\n");
  printf("  {S_2.1 - ShellLinkHeader}\n");
  if (less == 0)
  {
    printf("    Header Size:         %s bytes\n", lif_a.lha.H_size);
    printf("    Link File Class ID:  %s\n", lif_a.lha.CLSID);
    printf("    Flags:               %s\n", lif_a.lha.Flags);
  }
  printf("    Attributes:          %s\n", lif_a.lha.Attr);
  if (less == 0)
  {
    printf("    Creation Time:       %s\n", lif_a.lha.CrDate_long);
    printf("    Access Time:         %s\n", lif_a.lha.AcDate_long);
    printf("    Write Time:          %s\n", lif_a.lha.WtDate_long);
  }
  else
  {
    printf("    Creation Time:       %s\n", lif_a.lha.CrDate);
    printf("    Access Time:         %s\n", lif_a.lha.AcDate);
    printf("    Write Time:          %s\n", lif_a.lha.WtDate);
  }
  printf("    Target Size:         %s bytes\n", lif_a.lha.Size);
  if (less == 0) //omit this stuff if short info required
  {
    printf("    Icon Index:          %s\n", lif_a.lha.IconIndex);
    printf("    Window State:        %s\n", lif_a.lha.ShowState);
    printf("    Hot Keys:            %s\n", lif_a.lha.Hotkey);
    printf("    Reserved1:           %s\n", lif_a.lha.Reserved1);
    printf("    Reserved2:           %s\n", lif_a.lha.Reserved2);
    printf("    Reserved3:           %s\n", lif_a.lha.Reserved3);
  }
  if (lif.lh.Flags & 0x00000001) //If there is an ItemIDList
  {
    if (less == 0)
    {
      printf("  {S_2.2 - LinkTargetIDList}\n");
      printf("    Size:                %u bytes\n",
        lif.lidl.IDListSize + 2);
      if (itemid > 0) // If the '-i' option is switched on
      {
        idpos = lif.lh.H_size;
        printf("    IDList Size:         %s bytes\n",
          lif_a.lidla.IDListSize);
        printf("    Number of ItemIDs    %s\n", lif_a.lidla.NumItemIDs);
        for (i = 0; i < lif.lidl.NumItemIDs; i++)
        {
          printf("    {ItemID %i}\n", i + 1);
          printf("      ItemID  Size:      %s bytes\n", lif_a.lidla.Items[i].ItemIDSize);
          if (find_propstores((unsigned char*)&lif.lidl.Items[i].Data, lif.lidl.Items[i].ItemIDSize, idpos, &psp) == 0)
          {
            // If PropStoreProps exist:
            printf("      [Property Stores found within this ItemID]\n");
            printf("      Propstores Size:   %u bytes\n", psp.Size);
            printf("      File Offset:       %u bytes\n", psp.Posn);
            printf("      No of Prop Stores: %u\n", psp.NumStores);
            for (j = 0; j < psp.NumStores; j++)
            {
              if (get_propstore_a(&psp.Stores[j], &psa) == 0)
              {
                printf("      {ItemID %u Property Store %u}\n", i + 1, j + 1);
                printf("        Store Size:      %s bytes\n", psa.StorageSize);
                printf("        Version:         %s\n", psa.Version);
                printf("        Format ID:       %s\n", psa.FormatID.UUID);
                printf("        Name Type:       %s\n", psa.NameType);
                printf("        No of Values:    %s\n", psa.NumValues);
                for (k = 0; k < psp.Stores[j].NumValues; k++)
                {
                  printf("        {Item ID %u Property Store %u Property Value %u}\n", i + 1, j + 1, k + 1);
                  printf("          Value Size:    %s bytes\n", psa.PropValues[k].ValueSize);
                  if (psp.Stores[j].PropValues[k].ValueSize > 0)
                  {
                    if (psp.Stores[j].NameType == 0)
                    {
                      printf("          Name Size:     %s bytes\n", psa.PropValues[k].NameSizeOrID);
                      printf("          Name:          %s\n", psa.PropValues[k].Name);
                    }
                    else
                    {
                      printf("          ID:            %s\n", psa.PropValues[k].NameSizeOrID);
                    }
                    printf("          Property Type: %s\n", psa.PropValues[k].PropertyType);
                    printf("          Value:         %s\n", psa.PropValues[k].Value);

                  }
                }
              }
              else
              {
                printf("        [Unable to interpret Property Store %u]\n", j);
              }
            }
            idpos += lif.lidl.Items[i].ItemIDSize; // point idpos at the start of the next ItemID
          }
          else
          {
            printf("      [No Property Stores found in this ITemID]\n");
          }
        }
        printf("    IDList Terminator    2 bytes\n");
      }
    }
  }
  if (lif.lh.Flags & 0x00000002) //If there is a LinkInfo
  {
    printf("  {S_2.3 - LinkInfo}\n");
    if (less == 0)
    {
      printf("    Total Size:          %s bytes\n", lif_a.lia.Size);
      printf("    Header Size:         %s bytes\n", lif_a.lia.HeaderSize);
      printf("    Flags:               %s\n", lif_a.lia.Flags);
      printf("    Volume ID Offset:    %s\n", lif_a.lia.IDOffset);
      printf("    Base Path Offset:    %s\n", lif_a.lia.LBPOffset);
      printf("    CNR Link Offset:     %s\n", lif_a.lia.CNRLOffset);
      printf("    CPS Offset:          %s\n", lif_a.lia.CPSOffset);
      printf("    LBP Offset Unicode:  %s\n", lif_a.lia.LBPOffsetU);
      printf("    CPS Offset Unicode:  %s\n", lif_a.lia.CPSOffsetU);
    }
    //There is a Volume ID structure (& LBP)
    if (lif.li.Flags & 0x00000001)
    {
      printf("    {S_2.3.1 - LinkInfo - VolumeID}\n");
      if (less == 0)
      {
        printf("      Vol ID Size:       %s bytes\n", lif_a.lia.VolID.Size);
      }
      printf("      Drive Type:        %s\n", lif_a.lia.VolID.DriveType);
      printf("      Drive Serial No:   %s\n", lif_a.lia.VolID.DriveSN);
      if (less == 0)
      {
        if (!(lif.li.HeaderSize >= 0x00000024))//Which to use?
          //ANSI or Unicode versions
        {
          printf("      Vol Label Offset:  %s\n", lif_a.lia.VolID.VLOffset);
        }
        else
        {
          printf("      Vol Label OffsetU: %s\n", lif_a.lia.VolID.VLOffsetU);
        }
      }
      if (!(lif.li.HeaderSize >= 0x00000024))
      {
        printf("      Volume Label:      %s\n", lif_a.lia.VolID.VolumeLabel);
      }
      else
      {
        printf("      Volume LabelU:     %s\n", lif_a.lia.VolID.VolumeLabelU);
      }
      printf("      Local Base Path:   %s\n", lif_a.lia.LBP);
    }//End of VolumeID
  //CommonNetworkRelativeLink
    if (lif.li.Flags & 0x00000002)
    {
      printf("    {S_2.3.2 - LinkInfo - CommonNetworkRelativeLink}\n");
      if (less == 0)
      {
        printf("      CNR Size:          %s\n", lif_a.lia.CNR.Size);
        printf("      Flags:             %s\n", lif_a.lia.CNR.Flags);
        printf("      Net Name Offset:   %s\n", lif_a.lia.CNR.NetNameOffset);
        printf("      Device Name Off:   %s\n", lif_a.lia.CNR.DeviceNameOffset);
      }
      printf("      Net Provider Type: %s\n", lif_a.lia.CNR.NetworkProviderType);
      if ((less == 0) && (lif.li.CNR.NetNameOffset > 0x00000014))
      {
        printf("      Net Name Offset U: %s\n", lif_a.lia.CNR.NetNameOffsetU);
        printf("      Device Name Off U: %s\n", lif_a.lia.CNR.DeviceNameOffsetU);
      }
      printf("      Net Name:          %s\n", lif_a.lia.CNR.NetName);
      printf("      Device Name:       %s\n", lif_a.lia.CNR.DeviceName);
      if (lif.li.CNR.NetNameOffset > 0x00000014)
      {
        printf("      Net Name Unicode:  %s\n", lif_a.lia.CNR.NetNameU);
        printf("      Device Name Uni:   %s\n", lif_a.lia.CNR.DeviceNameU);
      }
      printf("    Common Path Suffix:  %s\n", lif_a.lia.CPS);
    }//End of CNR
    if (lif.li.LBPOffsetU > 0)
    {
      printf("    Local Base Path Uni: %s\n", lif_a.lia.LBPU);
    }
    if (lif.li.CPSOffsetU > 0)
    {
      printf("    Local Base Path Uni: %s\n", lif_a.lia.CPSU);
    }
  }//End of Link Info
//STRINGDATA
  if (lif.lh.Flags & 0x0000007C)
  {
    printf("  {S_2.4 - StringData}\n");
    if (less == 0)
    {
      printf("    StringData Size:     %s bytes\n", lif_a.lsda.Size);
    }
    if (lif.lh.Flags & 0x00000004)
    {
      printf("    {S_2.4 - StringData - NAME_STRING}\n");
      if (less == 0)
      {
        printf("      CountCharacters:   %s characters\n",
          lif_a.lsda.CountChars[0]);
      }
      printf("      Name String:       %s\n", lif_a.lsda.Data[0]);
    }
    if (lif.lh.Flags & 0x00000008)
    {
      printf("    {S_2.4 - StringData - RELATIVE_PATH}\n");
      if (less == 0)
      {
        printf("      CountCharacters:   %s characters\n",
          lif_a.lsda.CountChars[1]);
      }
      printf("      Relative Path:     %s\n", lif_a.lsda.Data[1]);
    }
    if (lif.lh.Flags & 0x00000010)
    {
      printf("    {S_2.4 - StringData - WORKING_DIR}\n");
      if (less == 0)
      {
        printf("      CountCharacters:   %s characters\n",
          lif_a.lsda.CountChars[2]);
      }
      printf("      Working Dir:       %s\n", lif_a.lsda.Data[2]);
    }
    if (lif.lh.Flags & 0x00000020)
    {
      printf("    {S_2.4 - StringData - COMMAND_LINE_ARGUMENTS}\n");
      if (less == 0)
      {
        printf("      CountCharacters:   %s characters\n",
          lif_a.lsda.CountChars[3]);
      }
      printf("      Cmd Line Args:     %s\n", lif_a.lsda.Data[3]);
    }
    if (lif.lh.Flags & 0x00000040)
    {
      printf("    {S_2.4 - StringData - ICON_LOCATION}\n");
      if (less == 0)
      {
        printf("      CountCharacters:   %s characters\n",
          lif_a.lsda.CountChars[4]);
      }
      printf("      Icon Location:     %s\n", lif_a.lsda.Data[4]);
    }

  }// End of STRINGDATA

//EXTRADATA
  printf("  {S_2.5 - ExtraData}\n");
  if (less == 0)
  {
    printf("    Extra Data Size:     %s bytes\n", lif_a.leda.Size);
    printf("    ED Structures:       %s\n", lif_a.leda.edtypes);
  }
  if (lif.led.edtypes & CONSOLE_PROPS)
  {
    // Even if we are printing the shortened version we show that there is a 
    // ConsoleDataBlock structure present.
    printf("    {S_2.5.1 - ExtraData - ConsoleDataBlock}\n");
    if (less == 0)
    {
      printf("      File Offset:       %s bytes\n", lif_a.leda.lcpa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.lcpa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.lcpa.sig);
      buf[0] = (char)0;
      if (lif.led.lcp.FillAttributes & 0x0001) strncat(buf, "FOREGROUND_BLUE | ", 18);
      if (lif.led.lcp.FillAttributes & 0x0002) strncat(buf, "FOREGROUND_GREEN | ", 19);
      if (lif.led.lcp.FillAttributes & 0x0004) strncat(buf, "FOREGROUND_RED | ", 17);
      if (lif.led.lcp.FillAttributes & 0x0008) strncat(buf, "FOREGROUND_INTENSITY | ", 23);
      if (lif.led.lcp.FillAttributes & 0x0010) strncat(buf, "BACKGROUND_BLUE | ", 18);
      if (lif.led.lcp.FillAttributes & 0x0020) strncat(buf, "BACKGROUND_GREEN | ", 19);
      if (lif.led.lcp.FillAttributes & 0x0040) strncat(buf, "BACKGROUND_RED | ", 17);
      if (lif.led.lcp.FillAttributes & 0x0080) strncat(buf, "BACKGROUND_INTENSITY | ", 23);
      i = strlen(buf);
      if (i > 2)
      {
        buf[i - 3] = (unsigned char)0; // Remove the last pipe character
      }
      else
      {
        snprintf(buf, 300, "No FillAttributes");
      }
      printf("      FillAttributes:    %s   %s\n", lif_a.leda.lcpa.FillAttributes, buf);
      buf[0] = (char)0;
      if (lif.led.lcp.PopupFillAttributes & 0x0001) strncat(buf, "FOREGROUND_BLUE | ", 18);
      if (lif.led.lcp.PopupFillAttributes & 0x0002) strncat(buf, "FOREGROUND_GREEN | ", 19);
      if (lif.led.lcp.PopupFillAttributes & 0x0004) strncat(buf, "FOREGROUND_RED | ", 17);
      if (lif.led.lcp.PopupFillAttributes & 0x0008) strncat(buf, "FOREGROUND_INTENSITY | ", 23);
      if (lif.led.lcp.PopupFillAttributes & 0x0010) strncat(buf, "BACKGROUND_BLUE | ", 18);
      if (lif.led.lcp.PopupFillAttributes & 0x0020) strncat(buf, "BACKGROUND_GREEN | ", 19);
      if (lif.led.lcp.PopupFillAttributes & 0x0040) strncat(buf, "BACKGROUND_RED | ", 17);
      if (lif.led.lcp.PopupFillAttributes & 0x0080) strncat(buf, "BACKGROUND_INTENSITY | ", 23);
      i = strlen(buf);
      if (i > 2)
      {
        buf[i - 3] = (unsigned char)0; // Remove the last pipe character
      }
      else
      {
        snprintf(buf, 300, "No PopupFillAttributes");
      }
      printf("      PopupFillAttr:     %s   %s\n", lif_a.leda.lcpa.PopupFillAttributes, buf);
      printf("      ScreenBufSizeX:    %s\n", lif_a.leda.lcpa.ScreenBufferSizeX);
      printf("      ScreenBufSizeY:    %s\n", lif_a.leda.lcpa.ScreenBufferSizeY);
      printf("      WindowSizeX:       %s\n", lif_a.leda.lcpa.WindowSizeX);
      printf("      WindowSizeY:       %s\n", lif_a.leda.lcpa.WindowSizeY);
      printf("      WindowOriginX:     %s\n", lif_a.leda.lcpa.WindowOriginX);
      printf("      WindowOriginY:     %s\n", lif_a.leda.lcpa.WindowOriginY);
      printf("      Unused1:           %s\n", lif_a.leda.lcpa.Unused1);
      printf("      Unused2:           %s\n", lif_a.leda.lcpa.Unused2);
      printf("      FontSize:          %s\n", lif_a.leda.lcpa.FontSize);
      buf[0] = (char)0;
      switch (lif.led.lcp.FontFamily)
      {
      case 0x0000:
        strncat(buf, "FF_DONTCARE", 11);
        break;
      case 0x0010:
        strncat(buf, "FF_ROMAN", 8);
        break;
      case 0x0020:
        strncat(buf, "FF_SWISS", 8);
        break;
      case 0x0030:
        strncat(buf, "FF_MODERN", 9);
        break;
      case 0x0040:
        strncat(buf, "FF_SCRIPT", 9);
        break;
      case 0x0050:
        strncat(buf, "FF_DECORATIVE", 13);
        break;
      default:
        strncat(buf, "UNKNOWN (Not allowed in specification)", 39);
      }
      printf("      FontFamily:        %s   %s\n", lif_a.leda.lcpa.FontFamily, buf);
      buf[0] = (char)0;
      if (lif.led.lcp.FontWeight < 700)
      {
        strncat(buf, "A regular-weight font", 21);
      }
      else
      {
        strncat(buf, "A bold font", 11);
      }
      printf("      FontWeight:        %s   %s\n", lif_a.leda.lcpa.FontWeight, buf);
      printf("      FaceName:          %s\n", lif_a.leda.lcpa.FaceName);
      buf[0] = (char)0;
      if (lif.led.lcp.CursorSize <= 25)
      {
        strncat(buf, "A small cursor", 14);
      }
      else if ((lif.led.lcp.CursorSize > 25) & (lif.led.lcp.CursorSize <= 50))
      {
        strncat(buf, "A medium cursor", 15);
      }
      else if ((lif.led.lcp.CursorSize > 50) & (lif.led.lcp.CursorSize <= 100))
      {
        strncat(buf, "A large cursor", 14);
      }
      else
      {
        strncat(buf, "An undefined cursor size", 25);
      }
      printf("      CursorSize:        %s   %s\n", lif_a.leda.lcpa.CursorSize, buf);
      buf[0] = (char)0;
      if (lif.led.lcp.FullScreen == 0)
      {
        strncat(buf, "Off", 3);
      }
      else
      {
        strncat(buf, "On", 2);
      }
      printf("      FullScreen:        %s   %s\n", lif_a.leda.lcpa.FullScreen, buf);
      buf[0] = (char)0;
      if (lif.led.lcp.QuickEdit == 0)
      {
        strncat(buf, "Off", 3);
      }
      else
      {
        strncat(buf, "On", 2);
      }
      printf("      QuickEdit:         %s   %s\n", lif_a.leda.lcpa.QuickEdit, buf);
      buf[0] = (char)0;
      if (lif.led.lcp.InsertMode == 0)
      {
        strncat(buf, "Disabled", 8);
      }
      else
      {
        strncat(buf, "Enabled", 7);
      }
      printf("      InsertMode:        %s   %s\n", lif_a.leda.lcpa.InsertMode, buf);
      buf[0] = (char)0;
      if (lif.led.lcp.AutoPosition == 0)
      {
        strncat(buf, "Off", 20);
      }
      else
      {
        strncat(buf, "On", 19);
      }
      printf("      AutoPosition:      %s   %s\n", lif_a.leda.lcpa.AutoPosition, buf);
      printf("      HistoryBufSize:    %s\n", lif_a.leda.lcpa.HistoryBufferSize);
      printf("      NumHistBuffers:    %s\n", lif_a.leda.lcpa.NumberOfHistoryBuffers);
      buf[0] = (char)0;
      if (lif.led.lcp.HistoryNoDup == 0)
      {
        strncat(buf, "Duplicates not allowed", 22);
      }
      else
      {
        strncat(buf, "Duplicates allowed", 18);
      }
      printf("      HistoryNoDup:      %s   %s\n", lif_a.leda.lcpa.HistoryNoDup, buf);
      printf("      ColorTable:        ");
      printf("%s %s %s %s\n", lif_a.leda.lcpa.ColorTable[0],
        lif_a.leda.lcpa.ColorTable[1],
        lif_a.leda.lcpa.ColorTable[2],
        lif_a.leda.lcpa.ColorTable[3]);
      printf("                         %s %s %s %s\n", lif_a.leda.lcpa.ColorTable[4],
        lif_a.leda.lcpa.ColorTable[5],
        lif_a.leda.lcpa.ColorTable[6],
        lif_a.leda.lcpa.ColorTable[7]);
      printf("                         %s %s %s %s\n", lif_a.leda.lcpa.ColorTable[8],
        lif_a.leda.lcpa.ColorTable[9],
        lif_a.leda.lcpa.ColorTable[10],
        lif_a.leda.lcpa.ColorTable[11]);
      printf("                         %s %s %s %s\n", lif_a.leda.lcpa.ColorTable[12],
        lif_a.leda.lcpa.ColorTable[13],
        lif_a.leda.lcpa.ColorTable[14],
        lif_a.leda.lcpa.ColorTable[15]);
    }
  }
  if (lif.led.edtypes & CONSOLE_FE_PROPS)
  {
    printf("    {S_2.5.2 - ExtraData - ConsoleFEDataBlock}\n");
    if (less == 0)
    {
      printf("      File Offset:       %s bytes\n", lif_a.leda.lcfepa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.lcfepa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.lcfepa.sig);
      printf("      Code Page:         %s\n", lif_a.leda.lcfepa.CodePage);
    }
  }
  if (lif.led.edtypes & DARWIN_PROPS)
  {
    printf("    {S_2.5.3 - ExtraData - DarwinDataBlock}\n");
    if (less == 0)
    {
      printf("      File Offset:       %s bytes\n", lif_a.leda.ldpa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.ldpa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.ldpa.sig);
      printf("      DarwinDataAnsi:    %s\n", lif_a.leda.ldpa.DarwinDataAnsi);
      printf("      DarwinDataUnicode: %s\n", lif_a.leda.ldpa.DarwinDataUnicode);
    }
  }
  if (lif.led.edtypes & ENVIRONMENT_PROPS)
  {
    printf("    {S_2.5.4 - ExtraData - EnvironmentVariableDataBlock}\n");
    if (less == 0)
    {
      printf("      File Offset:       %s bytes\n", lif_a.leda.lepa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.lepa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.lepa.sig);
      printf("      TargetAnsi:        %s\n", lif_a.leda.lepa.TargetAnsi);
      printf("      TargetUnicode:     %s\n", lif_a.leda.lepa.TargetUnicode);
    }
  }
  if (lif.led.edtypes & ICON_ENVIRONMENT_PROPS)
  {
    printf("    {S_2.5.5 - ExtraData - IconEnvironmentDataBlock}\n");
    if (less == 0)
    {
      printf("      File Offset:       %s bytes\n", lif_a.leda.liepa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.liepa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.liepa.sig);
      printf("      TargetAnsi:        %s\n", lif_a.leda.liepa.TargetAnsi);
      printf("      TargetUnicode:     %s\n", lif_a.leda.liepa.TargetUnicode);
    }
  }
  if (lif.led.edtypes & KNOWN_FOLDER_PROPS)
  {
    printf("    {S_2.5.6 - ExtraData - KnownFolderDataBlock}\n");
    if (less == 0)
    {
      printf("      File Offset:       %s bytes\n", lif_a.leda.lkfpa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.lkfpa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.lkfpa.sig);
      printf("      KnownFolderID:     %s\n", lif_a.leda.lkfpa.KFGUID.UUID);
      printf("      Offset:            %s\n", lif_a.leda.lkfpa.KFOffset);
    }
  }
  if (lif.led.edtypes & PROPERTY_STORE_PROPS)
  {
    printf("    {S_2.5.7 - ExtraData - PropertyStoreDataBlock}\n");
    if (less == 0)
    {
      printf("      File Offset:       %s bytes\n", lif_a.leda.lpspa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.lpspa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.lpspa.sig);
      printf("      Number of Stores:  %s\n", lif_a.leda.lpspa.NumStores);
      for (i = 0; i < lif.led.lpsp.NumStores; i++)
      {
        printf("      {Property Store %i}\n", i+1);
        printf("        Store Size:       %s bytes\n", lif_a.leda.lpspa.Stores[i].StorageSize);
        printf("        Version:          %s\n", lif_a.leda.lpspa.Stores[i].Version);
        printf("        Format ID:        %s\n", lif_a.leda.lpspa.Stores[i].FormatID.UUID);
        printf("        Name Type:        %s\n", lif_a.leda.lpspa.Stores[i].NameType);
        printf("        Number of Values: %s\n", lif_a.leda.lpspa.Stores[i].NumValues);
        for (j = 0; j < lif.led.lpsp.Stores[i].NumValues; j++)
        {
          printf("        {Property Store %i Property Value %i}\n", i + 1, j + 1);
          printf("          Value Size:      %s bytes\n", lif_a.leda.lpspa.Stores[i].PropValues[j].ValueSize);
          if (lif.led.lpsp.Stores[i].PropValues[j].ValueSize > 0)
          {
            if (lif.led.lpsp.Stores[i].NameType == 0)
            {
              printf("          Name Size:       %s bytes\n", lif_a.leda.lpspa.Stores[i].PropValues[j].NameSizeOrID);
              printf("          Name:            %s\n", lif_a.leda.lpspa.Stores[i].PropValues[j].Name);
            }
            else
            {
              printf("          ID:              %s\n", lif_a.leda.lpspa.Stores[i].PropValues[j].NameSizeOrID);
            }
            printf("          Property Type:   %s\n", lif_a.leda.lpspa.Stores[i].PropValues[j].PropertyType);
            printf("          Value:           %s\n", lif_a.leda.lpspa.Stores[i].PropValues[j].Value);

          }
        }
      }
    }
  }
  if (lif.led.edtypes & SHIM_PROPS)
  {
    printf("    {S_2.5.7 - ExtraData - ShimDataBlock}\n");
    if (less == 0)
    {
      printf("      File Offset:       %s bytes\n", lif_a.leda.lspa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.lspa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.lspa.sig);
      printf("      Number of Stores:  %s\n", lif_a.leda.lspa.LayerName);
    }
  }
  if (lif.led.edtypes & SPECIAL_FOLDER_PROPS)
  {
    printf("    {S_2.5.9 - ExtraData - SpecialFolderDataBlock}\n");
    if (less == 0)
    {
      printf("      File Offset:       %s bytes\n", lif_a.leda.lsfpa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.lsfpa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.lsfpa.sig);
      printf("      Folder ID:         %s\n", lif_a.leda.lsfpa.SpecialFolderID);
      printf("      Offset:            %s\n", lif_a.leda.lsfpa.Offset);
    }
  }
  if (lif.led.edtypes & TRACKER_PROPS)
  {
    printf("    {S_2.5.10 - ExtraData - TrackerDataBlock}\n");
    if (less == 0)
    {
      printf("      File Offset:       %s bytes\n", lif_a.leda.ltpa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.ltpa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.ltpa.sig);
      printf("      Length:            %s bytes\n", lif_a.leda.ltpa.Length);
      printf("      Version:           %s\n", lif_a.leda.ltpa.Version);
    }
    printf("      MachineID:         %s\n", lif_a.leda.ltpa.MachineID);
    printf("      Droid1:            %s\n", lif_a.leda.ltpa.Droid1.UUID);
    if (less == 0)
    {
      printf("        UUID Version:      %s\n", lif_a.leda.ltpa.Droid1.Version);
      printf("        UUID Variant:      %s\n", lif_a.leda.ltpa.Droid1.Variant);
    }
    if ((lif_a.leda.ltpa.Droid1.Version[0] == '1')
      & (lif_a.leda.ltpa.Droid1.Version[1] == ' '))
    {
      printf("        UUID Sequence:     %s\n",
        lif_a.leda.ltpa.Droid1.ClockSeq);
      if (less == 0)
      {
        printf("        UUID Time:         %s\n",
          lif_a.leda.ltpa.Droid1.Time_long);
      }
      else
      {
        printf("        UUID Time:         %s\n", lif_a.leda.ltpa.Droid1.Time);
      }
      printf("        UUID Node (MAC):   %s\n",
        lif_a.leda.ltpa.Droid1.Node);
    }
    printf("      Droid2:            %s\n", lif_a.leda.ltpa.Droid2.UUID);
    if (less == 0)
    {
      printf("        UUID Version:      %s\n", lif_a.leda.ltpa.Droid2.Version);
      printf("        UUID Variant:      %s\n", lif_a.leda.ltpa.Droid2.Variant);
    }
    if ((lif_a.leda.ltpa.Droid2.Version[0] == '1')
      & (lif_a.leda.ltpa.Droid2.Version[1] == ' '))
    {
      printf("        UUID Sequence:     %s\n",
        lif_a.leda.ltpa.Droid2.ClockSeq);
      if (less == 0)
      {
        printf("        UUID Time:         %s\n",
          lif_a.leda.ltpa.Droid2.Time_long);
      }
      else
      {
        printf("        UUID Time:         %s\n", lif_a.leda.ltpa.Droid2.Time);
      }
      printf("        UUID Node (MAC):   %s\n",
        lif_a.leda.ltpa.Droid2.Node);
    }
    //Rather a simplistic test to see if the two sets of Droids are the same
    if (!((lif.led.ltp.Droid1.Data1 == lif.led.ltp.DroidBirth1.Data1)
      & (lif.led.ltp.Droid2.Data1 == lif.led.ltp.DroidBirth2.Data1)
      & (less != 0)))
    {
      printf("      DroidBirth1:       %s\n",
        lif_a.leda.ltpa.DroidBirth1.UUID);
      if (less == 0)
      {
        printf("        UUID Version:      %s\n",
          lif_a.leda.ltpa.DroidBirth1.Version);
        printf("        UUID Variant:      %s\n",
          lif_a.leda.ltpa.DroidBirth1.Variant);
      }
      if ((lif_a.leda.ltpa.DroidBirth1.Version[0] == '1')
        & (lif_a.leda.ltpa.DroidBirth1.Version[1] == ' '))
      {
        printf("        UUID Sequence:     %s\n",
          lif_a.leda.ltpa.DroidBirth1.ClockSeq);
        if (less == 0)
        {
          printf("        UUID Time:         %s\n",
            lif_a.leda.ltpa.DroidBirth1.Time_long);
        }
        else
        {
          printf("        UUID Time:         %s\n",
            lif_a.leda.ltpa.DroidBirth1.Time);
        }
        printf("        UUID Node (MAC):   %s\n",
          lif_a.leda.ltpa.DroidBirth1.Node);
      }
      printf("      DroidBirth2:       %s\n",
        lif_a.leda.ltpa.DroidBirth2.UUID);
      if (less == 0)
      {
        printf("        UUID Version:      %s\n",
          lif_a.leda.ltpa.DroidBirth2.Version);
        printf("        UUID Variant:      %s\n",
          lif_a.leda.ltpa.DroidBirth2.Variant);
      }
      if ((lif_a.leda.ltpa.DroidBirth2.Version[0] == '1')
        & (lif_a.leda.ltpa.DroidBirth2.Version[1] == ' '))
      {
        printf("        UUID Sequence:     %s\n",
          lif_a.leda.ltpa.DroidBirth2.ClockSeq);
        if (less == 0)
        {
          printf("        UUID Time:         %s\n",
            lif_a.leda.ltpa.DroidBirth2.Time_long);
        }
        else
        {
          printf("        UUID Time:         %s\n",
            lif_a.leda.ltpa.DroidBirth2.Time);
        }
        printf("        UUID Node (MAC):   %s\n",
          lif_a.leda.ltpa.DroidBirth2.Node);
      }
    }
  }
  if (lif.led.edtypes & VISTA_AND_ABOVE_IDLIST_PROPS)
  {
    if (less == 0)
    {
      printf("    {S_2.5.11 - ExtraData - VistaAndAboveIDListDataBlock}\n");
      printf("      File Offset:       %s bytes\n", lif_a.leda.lvidlpa.Posn);
      printf("      BlockSize:         %s bytes\n", lif_a.leda.lvidlpa.Size);
      printf("      BlockSignature:    %s\n", lif_a.leda.lvidlpa.sig);
      printf("      Number of Items:     %s\n", lif_a.leda.lvidlpa.NumItemIDs);
    }
  }
  printf("\n");
}

//
//Function: proc_file() processes regular files
void proc_file(char* fname, int less, int idlist)
{
  FILE *fp;
  struct stat statbuf;

  //Try to open a file pointer
  if ((fp = fopen(fname, "rb")) == NULL)
  {
    //unsuccessful
    perror("Error");
    fprintf(stderr, "whilst processing file: \'%s\'\n", fname);
  }
  else
  {
    stat(fname, &statbuf);
    if (statbuf.st_size >= 76) //Don't bother with files that aren't big enough
    {
      //successful
      if (test_link(fp) == 0) // Test to see if the file has the right magic
      {
        switch (output_type)
        {
        case csv:
          sv_out(fp, fname, less, ','); // Output to a separated file with the separator being a comma
          break;
        case tsv:
          sv_out(fp, fname, less, '\t'); // Output to a separated file with the separator being a tab
          break;
        case txt:
        default:       //Anything other than these 3 options should have been
          //trapped already - this is just belt & braces!
          text_out(fp, fname, less, idlist); // Output to plain text
        }
        filecount++;
      }
      else
      {
        fprintf(stderr, "Not a Link File:\t%s\n", fname);
      }

      if (fclose(fp) != 0)
      {
        //Can't close the file for some reason
        perror("Error in function proc_file()");
        fprintf(stderr, "whilst closing file: \'%s\'\n", fname);
        exit(EXIT_FAILURE);
      }
    }
    else
    {
      fprintf(stderr, "Not a Link File:\t%s\n", fname);
    }
  }
}

//
//Function: read_dir() iterates through the files in a directory and processes
//them
void read_dir(char* dirname, int less, int idlist)
{
  DIR *dp;
  struct dirent *entry;
  struct stat statbuf;

  if ((dp = opendir(dirname)) == NULL)
  {
    perror("Error");
    fprintf(stderr, "whilst processing directory: \'%s\'\n", dirname);
    return;
  }
  _chdir(dirname);
  //Iterate through the directory entries
  while ((entry = readdir(dp)) != NULL)
  {
    stat(entry->d_name, &statbuf);
    //Don't want anything but regular files
    if ((statbuf.st_mode & S_IFMT) == S_IFREG)
    {
      proc_file(entry->d_name, less, idlist);
    }
  }//End of iterating through directory entry
}

//
//Main function
int main(int argc, char *argv[])
{
  int opt, process = 1, less = 0, idlist = 0; // less is the flag for short info 
                // (can't use short, it's a keyword)
  int proc_dir = 0;           // A flag to deal with processing just one directory
  struct stat statbuffer;     // File details buffer

  output_type = txt;      //default output type
  filecount = 0;

  //if someone calls lifer with no options whatsoever then print help
  if (argc == 1)
  {
    help_message();
    process = 0;
  }

  //Parse the options
  while ((opt = getopt(argc, argv, "vhsio:")) != -1)
  {
    // Parse supplied command line options
    switch (opt)
    {
    case 'v':
      printf("lifer - A Windows link file analyser\n");
      printf("Version: %u.%u.%u\n", _MAJOR, _MINOR, _BUILD);
      process = 0; // Turn off any further processing
      break;
    case 'h':
      help_message();
      process = 0;
      break;
    case '?':
      printf("Usage: lifer [-vhsi] [-o csv|tsv|txt] file(s)|directory\n");
      process = 0;
      break;
    case 's':
      less++;
      break;
    case 'i':
      idlist++;
      break;
    case 'o':
      if (strcmp(optarg, "csv") == 0)
      {
        output_type = csv;
      }
      else if (strcmp(optarg, "tsv") == 0)
      {
        output_type = tsv;
      }
      else if (strcmp(optarg, "txt") == 0)
      {
        output_type = txt;
      }
      else
      {
        printf("Invalid argument to option \'-o\'\n");
        printf("Valid arguments are: \'csv\', \'tsv\', or \'txt\'[default]\n");
        process = 0;
      }
      break;
    default:
      help_message();
    }
  }

  if (process)
  {
    //Deal with the situation where valid options have been supplied
    //but no files or directory argument.
    if (optind >= argc)
    {
      fprintf(stderr, "No file(s) or directory supplied.\n");
      help_message();
      exit(EXIT_FAILURE);
    }
    for (; optind < argc; optind++)
    {
      if (stat(argv[optind], &statbuffer) != 0)
      {
        //Getting file stats failed so report the error
        perror("Error in function main()");
        fprintf(stderr, "whilst processing argument: \'%s\'\n", argv[optind]);
      }
      //Process directory
      if (((statbuffer.st_mode & S_IFMT) == S_IFDIR) & (proc_dir == 0))
      {
        //Deal with the erroneous situation where the user provides a directory
        //argument followed by other arguments
        if (argc > (optind + 1))
        {
          fprintf(stderr, "Sorry, only one directory argument allowed\n");
          help_message();
          exit(EXIT_FAILURE);
        }
        else
        {
          read_dir(argv[optind], less, idlist);
        }
      }
      //Process regular files
      else if (((statbuffer.st_mode & S_IFMT) == S_IFREG))
      {
        proc_file(argv[optind], less, idlist);
      }
      proc_dir = 1; //Prevent processing of directories after first argument
      //(The default behaviour is to process 1 directory OR
      //several files)
    }
  }
  exit(EXIT_SUCCESS);
}