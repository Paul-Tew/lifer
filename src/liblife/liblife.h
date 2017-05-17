/*********************************************************
**                                                      **
**                 liblife.h                            **
**                                                      **
** A library to handle the data from Windows link files **
**                                                      **
**         Copyright Paul Tew 2011 to 2017              **
**                                                      **
** Structures                                           **
** -----------                                          **
** LIF       - Link File data                           **
** LIF_A     - ASCII representation of a LIF            **
**                                                      **
** Major Functions:                                     **
** ----------------                                     **
** test_link(FILE*)                                     **
**       Returns 0 if the file pointed to by fp is a    **
**       Windows Link file -1 if not.                   **
**                                                      **
** get_lif(FILE*, int, LIF*)                            **
**       Populates LIF with the decoded link file data  **
**                                                      **
** get_lif_a(LIF*, LIF_A*)                              **
**       Converts the LIF to a readable version         **
**                                                      **
*********************************************************/

/*
This file is part of Lifer.

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

/*

The numbering system in the comments that follow directly relate to the paragraph
numbering in the Microsoft MS-SHLLINK Open Document (available from:
https://msdn.microsoft.com/en-us/library/dd871305.aspx )

*/

#ifndef _LIBLIFE_H_
#define _LIBLIFE_H_ 1

#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>

/******************************************************************************/
//Minor Definitions & Structures
#define PROPSTORES 5  // The number of LIF_SER_PROPSTORE in a LIF_PROPERTY_STORE_PROPS structure
#define PROPVALUES 5  // The number of LIF_SER_PROPVALUE in each LIF_SER_PROPSTORE structure

// extradata types
enum EDTYPES
{
  EMPTY                         = 0,
  CONSOLE_PROPS                 = 1,
  CONSOLE_FE_PROPS              = 2,
  DARWIN_PROPS                  = 4,
  ENVIRONMENT_PROPS             = 8,
  ICON_ENVIRONMENT_PROPS        = 16,
  KNOWN_FOLDER_PROPS            = 32,
  PROPERTY_STORE_PROPS          = 64,
  SHIM_PROPS                    = 128,
  SPECIAL_FOLDER_PROPS          = 256,
  TRACKER_PROPS                 = 512,
  VISTA_AND_ABOVE_IDLIST_PROPS  = 1024
};

enum PROPERTY_TYPE // From MS-OLEPS https://msdn.microsoft.com/en-us/library/dd942532.aspx
{
  VT_EMPTY              = 0x0000, // 0 bytes
  VT_NULL               = 0x0001, // 0 bytes
  VT_I2                 = 0x0002, // 2 bytes + 2 padding bytes | VECTOR | ARRAY
  VT_I4                 = 0x0003, // 4 bytes | VECTOR | ARRAY
  VT_R4                 = 0x0004, // 4 bytes | VECTOR | ARRAY
  VT_R8                 = 0x0005, // 8 bytes | VECTOR | ARRAY
  VT_CY                 = 0x0006, // 8 bytes | VECTOR | ARRAY
  VT_DATE               = 0x0007, // 8 bytes | VECTOR | ARRAY
  VT_BSTR               = 0x0008, // Variable ANSI string - size is first 4 bytes | VECTOR | ARRAY
  VT_ERROR              = 0x000A, // 4 bytes (Unsiged Int)  | VECTOR | ARRAY
  VT_BOOL               = 0x000B, // 2 bytes + 2 padding bytes, 0xFFFF = VARIANT_TRUE, 0x0000 = VARIANT_FALSE | VECTOR | ARRAY
  VT_VARIANT            = 0x000C, // Not available on its own  | VECTOR | ARRAY
  VT_DECIMAL            = 0x000E, // 16 bytes | ARRAY
  VT_I1                 = 0x0010, // 1 byte + 3 padding | VECTOR | ARRAY
  VT_UI1                = 0x0011, // 1 byte + 3 padding | VECTOR | ARRAY
  VT_UI2                = 0x0012, // 2 bytes + 2 padding bytes | VECTOR | ARRAY
  VT_UI4                = 0x0013, // 4 bytes | VECTOR | ARRAY
  VT_I8                 = 0x0014, // 8 bytes | VECTOR
  VT_UI8                = 0x0015, // 8 bytes | VECTOR
  VT_INT                = 0x0016, // 4 bytes | VECTOR | ARRAY
  VT_UINT               = 0x0017, // 4 bytes | VECTOR | ARRAY
  VT_LPSTR              = 0x001E, // Variable ANSI string - size is first 4 bytes | VECTOR
  VT_LPWSTR             = 0x001F, // Variable UNI string - length is first 4 bytes | VECTOR
  VT_FILETIME           = 0x0040, // 8 bytes | VECTOR
  VT_BLOB               = 0x0041, // Variable - size is first 4 bytes
  VT_STREAM             = 0x0042, // Variable ANSI string - size is first 4 bytes
  VT_STORAGE            = 0x0043, // Variable ANSI string - size is first 4 bytes
  VT_STREAMED_OBJECT    = 0x0044, // Variable ANSI string - size is first 4 bytes
  VT_STORED_OBJECT      = 0x0045, // Variable ANSI string - size is first 4 bytes
  VT_BLOB_OBJECT        = 0x0046, // Variable byte array - size is first 4 bytes
  VT_CF_OBJECT          = 0x0047, // Variable byte array - size is first 4 bytes (data includes a 'Format' field) | VECTOR
  VT_CLSID              = 0x0048, // 16 bytes | VECTOR
  VT_VERSIONED_STREAM   = 0x0049, // 16 bytes (CLSID) followed by: Variable ANSI string - size is first 4 bytes
  /* The following are not strictly defined as this because there is a strict 
  subset of the above that can be logically OR'ed with them but this can be 
  dealt with later in the logic */
  VT_VECTOR             = 0x1000, // Variable scalar of objects - length is the first 4 bytes
  VT_ARRAY              = 0x2000  // Variable Dimension of objects - Type (4 bytes) * NumDimensions (next 4 bytes) * sizeof Type = (size - 8 bytes)
};

struct LIF_CLSID
{
  uint32_t           Data1;       //32bit Data1         - Represented LE
  uint16_t           Data2;       //16bit Data2         - Represented LE
  uint16_t           Data3;       //16bit Data3         - Represented LE
  unsigned char      Data4hi[2];  //High part of data 4 - Represented BE
  unsigned char      Data4lo[6];  //Low part of data 4  - Represented BE
};

struct LIF_CLSID_A
{
  unsigned char     UUID[40];    //UUID Representation
  unsigned char     Version[40];
  unsigned char     Variant[40]; // 'NCS', 'ITU', 'Microsoft' or 'Future ITU'
  unsigned char     Time[30];
  unsigned char     Time_long[40];
  unsigned char     ClockSeq[10];
  unsigned char     Node[20];
};

// Following definition taken from MS-PROPSTORE and MS-OLEPS
struct LIF_SER_PROPVALUE
{
  // MS-PROPSTORE S2.3.1 & S2.3.2
  uint32_t            ValueSize;
  uint32_t            NameSizeOrID;
  uint8_t             Reserved;
  unsigned char       Name[300];  // Will be "[N/A]" if an Integer Type
  // MS-OLEPS S2.15
  uint16_t            PropertyType; // From the PROPERTY_TYPE enumeration
  uint16_t            Padding;
  unsigned char       Value[400]; // Raw property content
};
struct LIF_SER_PROPVALUE_A
{
  unsigned char       ValueSize[12];
  unsigned char       NameSizeOrID[12];
  unsigned char       Reserved[5];
  unsigned char       Name[300];  // Will be "[N/A]" if an Integer Type
  unsigned char       PropertyType[40]; // Something like: "VT_VECTOR | VT_FILETIME"
  unsigned char       Padding[7]; // Should be "0x0000"
  unsigned char       Value[400]; // Interpreted property content
};

struct LIF_SER_PROPSTORE
{
  // MS-PROPSTORE S2.1 & S2.2
  uint32_t                  StorageSize;
  uint32_t                  Version;  // According to the spec, should equal 0x53505331
  struct LIF_CLSID          FormatID; // A GUID
  unsigned char             NameType; // Not in spec: '> 0' = "Integer" name type & '== 0' = "String" name type
  uint16_t                  NumValues; // Not in spec but used to report the number of values in this store.
  struct LIF_SER_PROPVALUE  PropValues[PROPVALUES]; 
};

struct LIF_SER_PROPSTORE_A
{
  unsigned char               StorageSize[12];
  unsigned char               Version[12];
  struct LIF_CLSID_A          FormatID;
  unsigned char               NameType[8]; // Not in spec but will specify "Integer" or "String"
  unsigned char               NumValues[7];
  struct LIF_SER_PROPVALUE_A  PropValues[PROPVALUES];
};

struct LIF_CONSOLE_PROPS
{
  uint16_t       Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t       Size;
  uint32_t       sig;
  uint16_t       FillAttributes;
  uint16_t       PopupFillAttributes;
  uint16_t       ScreenBufferSizeX;
  uint16_t       ScreenBufferSizeY;
  uint16_t			 WindowSizeX;
  uint16_t			 WindowSizeY;
  uint16_t			 WindowOriginX;
  uint16_t			 WindowOriginY;
  uint32_t			 Unused1;
  uint32_t			 Unused2;
  uint32_t			 FontSize;
  uint32_t			 FontFamily;
  uint32_t			 FontWeight;
  wchar_t				 FaceName[32];
  uint32_t			 CursorSize;
  uint32_t			 FullScreen;
  uint32_t			 QuickEdit;
  uint32_t			 InsertMode;
  uint32_t			 AutoPosition;
  uint32_t			 HistoryBufferSize;
  uint32_t			 NumberOfHistoryBuffers;
  uint32_t			 HistoryNoDup;
  uint32_t			 ColorTable[16];
};

struct LIF_CONSOLE_PROPS_A
{
  unsigned char      Posn[8];
  unsigned char      Size[10];
  unsigned char      sig[12];
  unsigned char      FillAttributes[8];
  unsigned char      PopupFillAttributes[8];
  unsigned char      ScreenBufferSizeX[8];
  unsigned char      ScreenBufferSizeY[8];
  unsigned char			 WindowSizeX[8];
  unsigned char			 WindowSizeY[8];
  unsigned char			 WindowOriginX[8];
  unsigned char			 WindowOriginY[8];
  unsigned char			 Unused1[12];
  unsigned char			 Unused2[12];
  unsigned char			 FontSize[12];
  unsigned char			 FontFamily[12];
  unsigned char			 FontWeight[12];
  unsigned char			 FaceName[64];
  unsigned char			 CursorSize[12];
  unsigned char			 FullScreen[12];
  unsigned char			 QuickEdit[12];
  unsigned char			 InsertMode[12];
  unsigned char			 AutoPosition[12];
  unsigned char			 HistoryBufferSize[12];
  unsigned char			 NumberOfHistoryBuffers[12];
  unsigned char			 HistoryNoDup[12];
  unsigned char			 ColorTable[16][12];
};

struct LIF_CONSOLE_FE_PROPS
{
  uint16_t           Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t           Size;
  uint32_t           sig;
  uint32_t           CodePage;
};

struct LIF_CONSOLE_FE_PROPS_A
{
  unsigned char      Posn[8];
  unsigned char      Size[10];
  unsigned char      sig[12];
  unsigned char      CodePage[12];
};

struct LIF_DARWIN_PROPS
{
  uint16_t           Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t           Size;
  uint32_t           sig;
  unsigned char      DarwinDataAnsi[260];
  wchar_t            DarwinDataUnicode[260];
};

struct LIF_DARWIN_PROPS_A
{
  unsigned char      Posn[8];
  unsigned char      Size[10];
  unsigned char      sig[12];
  unsigned char      DarwinDataAnsi[260];
  unsigned char      DarwinDataUnicode[520];
};

struct LIF_ENVIRONMENT_PROPS
{
  uint16_t           Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t           Size;
  uint32_t           sig;
  unsigned char      TargetAnsi[260];
  wchar_t            TargetUnicode[260];
};

struct LIF_ENVIRONMENT_PROPS_A
{
  unsigned char      Posn[8];
  unsigned char      Size[10];
  unsigned char      sig[12];
  unsigned char      TargetAnsi[260];
  unsigned char      TargetUnicode[520];
};

struct LIF_ICON_ENVIRONMENT_PROPS
{
  uint16_t           Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t           Size;
  uint32_t           sig;
  unsigned char      TargetAnsi[260];
  wchar_t            TargetUnicode[260];
};

struct LIF_ICON_ENVIRONMENT_PROPS_A
{
  unsigned char      Posn[8];
  unsigned char      Size[10];
  unsigned char      sig[12];
  unsigned char      TargetAnsi[260];
  unsigned char      TargetUnicode[520];
};

struct LIF_KNOWN_FOLDER_PROPS
{
  uint16_t           Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t           Size;
  uint32_t           sig;
  struct LIF_CLSID   KFGUID;
  uint32_t           KFOffset;
};

struct LIF_KNOWN_FOLDER_PROPS_A
{
  unsigned char       Posn[8];
  unsigned char       Size[10];
  unsigned char       sig[12];
  struct LIF_CLSID_A  KFGUID;
  unsigned char       KFOffset[10];
};

struct LIF_PROPERTY_STORE_PROPS
{
  uint16_t                  Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t                  Size;
  uint32_t                  sig;
  int32_t                   NumStores;
  struct LIF_SER_PROPSTORE  Stores[PROPSTORES];
};

struct LIF_PROPERTY_STORE_PROPS_A
{
  unsigned char               Posn[8];
  unsigned char               Size[10];
  unsigned char               sig[12];
  unsigned char               NumStores[10];
  struct LIF_SER_PROPSTORE_A  Stores[PROPSTORES]; 
};

struct LIF_SHIM_PROPS
{
  uint16_t           Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t           Size;
  uint32_t           sig;
  wchar_t            LayerName[600];
};

struct LIF_SHIM_PROPS_A
{
  unsigned char      Posn[8];
  unsigned char      Size[10];
  unsigned char      sig[12];
  unsigned char      LayerName[600];
};

struct LIF_SPECIAL_FOLDER_PROPS
{
  uint16_t           Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t           Size;
  uint32_t           sig;
  uint32_t           SpecialFolderID;
  uint32_t           Offset;
};

struct LIF_SPECIAL_FOLDER_PROPS_A
{
  unsigned char      Posn[8];
  unsigned char      Size[10];
  unsigned char      sig[12];
  unsigned char      SpecialFolderID[10];
  unsigned char      Offset[10];
};

struct LIF_TRACKER_PROPS
{
  uint16_t           Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t           Size;
  uint32_t           sig;
  uint32_t           Length;
  uint32_t           Version;
  unsigned char      MachineID[16]; //NetBios names have a Max of 16 chars
  struct LIF_CLSID   Droid1;
  struct LIF_CLSID   Droid2;
  struct LIF_CLSID   DroidBirth1;
  struct LIF_CLSID   DroidBirth2;
};

struct LIF_TRACKER_PROPS_A
{
  unsigned char       Posn[8];
  unsigned char       Size[10];
  unsigned char       sig[12];
  unsigned char       Length[10];
  unsigned char       Version[10];
  unsigned char       MachineID[17];
  struct LIF_CLSID_A  Droid1;
  struct LIF_CLSID_A  Droid2;
  struct LIF_CLSID_A  DroidBirth1;
  struct LIF_CLSID_A  DroidBirth2;
};

struct LIF_VISTA_IDLIST_PROPS
{
  uint16_t           Posn;	// Not in the spec but included to assist in forensic analysis and authentication of results
  uint32_t           Size;
  uint32_t           sig;
  //TODO Fix this!
  //  struct ITEMID**    Items;      //A variable number of variable length structures
  uint16_t           NumItemIDs; //This isn't in the specification but it seemed like a good idea to include it.
};

struct LIF_VISTA_IDLIST_PROPS_A
{
  unsigned char      Posn[8];
  unsigned char      Size[10];
  unsigned char      sig[12];
  //  struct ITEMID_A**  Items;          //A variable number of variable length structures
  unsigned char      NumItemIDs[10];
};

struct LIF_EXTRA_DATA
{
  uint32_t                             Size;     // The overall size of the ED Block is not in the spec but it seems sensible to include it.
  enum EDTYPES                         edtypes;  // This isn't in the spec either but it is nice to show what ED structures are present.
                                                 // The following ED structures could concievably appear in any order so it seems sensible to
                                                 // include a file offset (position) value to assist examiners to locate it, notwithstanding
                                                 // that a positional value does not appear in the spec.
  struct LIF_CONSOLE_PROPS             lcp;      // ConsoleDataBlock
  struct LIF_CONSOLE_FE_PROPS          lcfep;    // ConsoleFEDataBlock
  struct LIF_DARWIN_PROPS              ldp;      // DarwinDataBlock
  struct LIF_ENVIRONMENT_PROPS         lep;      // EnvironmentVariableDataBlock
  struct LIF_ICON_ENVIRONMENT_PROPS    liep;     // IconEnvironmentDataBlock
  struct LIF_KNOWN_FOLDER_PROPS        lkfp;     // KnownFolderDataBlock
  struct LIF_PROPERTY_STORE_PROPS      lpsp;     // PropertyStoreDataBlock
  struct LIF_SHIM_PROPS                lsp;      // ShimDataBlock 
  struct LIF_SPECIAL_FOLDER_PROPS      lsfp;     // SpecialFolderDataBlock
  struct LIF_TRACKER_PROPS             ltp;      // TrackerDataBlock
  struct LIF_VISTA_IDLIST_PROPS        lvidlp;   // VistaAndAboveIDListDataBlock
  uint32_t                             terminal; // The End!
};

struct LIF_EXTRA_DATA_A
{
  unsigned char                          Size[10];
  unsigned char                          edtypes[300];
  struct LIF_CONSOLE_PROPS_A             lcpa;
  struct LIF_CONSOLE_FE_PROPS_A          lcfepa;
  struct LIF_DARWIN_PROPS_A              ldpa;
  struct LIF_ENVIRONMENT_PROPS_A         lepa;
  struct LIF_ICON_ENVIRONMENT_PROPS_A    liepa;
  struct LIF_KNOWN_FOLDER_PROPS_A        lkfpa;
  struct LIF_PROPERTY_STORE_PROPS_A      lpspa;
  struct LIF_SHIM_PROPS_A                lspa;
  struct LIF_SPECIAL_FOLDER_PROPS_A      lsfpa;
  struct LIF_TRACKER_PROPS_A             ltpa;
  struct LIF_VISTA_IDLIST_PROPS_A        lvidlpa;
  char                                   terminal[15];
};

struct LIF_STRINGDATA
{
  uint32_t           Size;          //This isn't in the specification but I've included it to help calculate the position
  uint16_t           CountChars[5];
  unsigned char      Data[5][300]; //StringData can be any length but I've restricted it to returning just 300 chars
};

struct LIF_STRINGDATA_A
{
  unsigned char               Size[10];
  unsigned char               CountChars[5][10];
  unsigned char               Data[5][300];
};

struct LIF_CNR //Common Network Relative Link structure
{
  uint32_t           Size;
  uint32_t           Flags;
  uint32_t           NetNameOffset;
  uint32_t           DeviceNameOffset;
  uint32_t           NetworkProviderType;
  uint32_t           NetNameOffsetU;
  uint32_t           DeviceNameOffsetU;
  unsigned char      NetName[300]; //This is probably way in excess of what is
  //needed but I can't find any documentation on
  //maxima
  unsigned char      DeviceName[300]; //Ditto
  wchar_t            NetNameU[300];
  wchar_t            DeviceNameU[300];
};

struct LIF_CNR_A
{
  unsigned char               Size[10];
  unsigned char               Flags[30];
  unsigned char               NetNameOffset[10];
  unsigned char               DeviceNameOffset[10];
  unsigned char               NetworkProviderType[35];
  unsigned char               NetNameOffsetU[10];
  unsigned char               DeviceNameOffsetU[10];
  unsigned char               NetName[300];
  unsigned char               DeviceName[300];
  unsigned char               NetNameU[300];
  unsigned char               DeviceNameU[300];
};

struct LIF_VOLID
{
  uint32_t           Size;
  uint32_t           DriveType;
  uint32_t           DriveSN; // Drive Serial Number
  uint32_t           VLOffset; // Volume Label Offset
  uint32_t           VLOffsetU; // Optional Unicode Volume Label Offset
  unsigned char      VolumeLabel[33];
  wchar_t            VolumeLabelU[33];
};

struct LIF_VOLID_A
{
  unsigned char               Size[10];
  unsigned char               DriveType[20];
  unsigned char               DriveSN[20];
  unsigned char               VLOffset[20];
  unsigned char               VLOffsetU[20];
  unsigned char               VolumeLabel[33];
  unsigned char               VolumeLabelU[33];
};

struct VKEY
{
  unsigned char      LowKey;        //Main key pressed
  unsigned char      HighKey;       //Shift, Ctrl or Alt
};

struct LIF_INFO  // Link Info Structure
{
  uint32_t           Size;
  uint32_t           HeaderSize;
  uint32_t           Flags;
  uint32_t           IDOffset; //ID Offset
  uint32_t           LBPOffset; //Local Base Path Offset
  uint32_t           CNRLOffset; //Common Network Relative Link Offset
  uint32_t           CPSOffset; //Common Path Suffix Offset
  uint32_t           LBPOffsetU; //Local Base path Offset (Unicode)
  uint32_t           CPSOffsetU; //Common Path Suffix Offset Unicode
  struct LIF_VOLID   VolID;     //Volume ID structure
  unsigned char      LBP[300]; //Local Base Path
  struct LIF_CNR     CNR;      //Common Network Relative Link structure
  unsigned char      CPS[100]; //Common Path Suffix
  wchar_t            LBPU[300]; //Local Base Path, Unicode version
  wchar_t            CPSU[100]; //Common Path Suffix, Unicode
};

struct LIF_INFO_A
{
  unsigned char               Size[10];
  unsigned char               HeaderSize[10];
  unsigned char               Flags[100];
  unsigned char               IDOffset[10];
  unsigned char               LBPOffset[10];
  unsigned char               CNRLOffset[10];
  unsigned char               CPSOffset[10];
  unsigned char               LBPOffsetU[10];
  unsigned char               CPSOffsetU[10];
  struct LIF_VOLID_A		  VolID;
  unsigned char               LBP[300]; //Local Base Path
  struct LIF_CNR_A			  CNR;
  unsigned char               CPS[100]; //Common Path Suffix
  unsigned char               LBPU[300]; //Local Base Path, Unicode version
  unsigned char               CPSU[100]; //Common Path Suffix, Unicode
};

struct LIF_IDLIST //This is a considerable simplification of the data for IDLIST
  // and ITEMID which are both contained in this structure.
{
  uint16_t           IDL_size;
  //  struct ITEMID**    Items;      //A variable number of variable length
    //structures
  uint16_t           NumItemIDs; //This isn't in the specification but it seemed
  // like a good idea to include it.
};

struct LIF_IDLIST_A
{
  unsigned char               IDL_size[10];
  //  struct ITEMID_A**  Items;          //A variable number of variable length
  //structures
  unsigned char               NumItemIDs[10];
};

struct LIF_HDR
{
  uint32_t           H_size;       //header size     - must be 0x0000004C
  struct LIF_CLSID   CLSID;        //GUID identifier - must be standard
  uint32_t           Flags;        //What can I say? flags! (Sec 2.1.1)
  uint32_t           Attr;         //File attributes (Sec 2.1.2)
  int64_t            CrDate;       //Embedded Creation Time
  int64_t            AcDate;       //Embedded Access Time
  int64_t            WtDate;       //Embedded Write Time
  uint32_t           Size;         //File Size
  int32_t            IconIndex;    //Icon Location in a given resource.
  uint32_t           ShowState;    //Starting state for the application.
  struct VKEY        Hotkey;       //Virtual Key to activate this link (S 2.1.3)
  uint16_t           Reserved1;    //Must be 0
  uint32_t           Reserved2;    //Must be 0
  uint32_t           Reserved3;    //Must be 0
};

struct LIF_HDR_A
{
  unsigned char               H_size[10];
  unsigned char               CLSID[40];
  unsigned char               Flags[550];       //Sec 2.1.1
  unsigned char               Attr[250];        //Sec 2.1.2
  unsigned char               CrDate[30];
  unsigned char               AcDate[30];
  unsigned char               WtDate[30];
  unsigned char               CrDate_long[40];
  unsigned char               AcDate_long[40];
  unsigned char               WtDate_long[40];
  unsigned char               Size[25];
  unsigned char               IconIndex[25];
  unsigned char               ShowState[40];
  unsigned char               Hotkey[40];       //Sec 2.1.3
  unsigned char               Reserved1[10];
  unsigned char               Reserved2[20];
  unsigned char               Reserved3[20];
};

/******************************************************************************/
//Major Structure Definitions

struct LIF //LInk File structure
{
  struct LIF_HDR           lh;   //Section 2.1 of MS-SHLLINK
  struct LIF_IDLIST        lidl; //Section 2.2
  struct LIF_INFO          li;   //Section 2.3
  struct LIF_STRINGDATA    lsd;  //Section 2.4
  struct LIF_EXTRA_DATA    led;  //Section 2.5
};

struct LIF_A //ASCII version of the LIF structure
{
  struct LIF_HDR_A         lha;   //Section 2.1 of MS-SHLLINK
  struct LIF_IDLIST_A      lidla; //Section 2.2
  struct LIF_INFO_A        lia;   //Section 2.3
  struct LIF_STRINGDATA_A  lsda;  //Section 2.4
  struct LIF_EXTRA_DATA_A  leda;  //Section 2.5
};
/******************************************************************************/
//Public Function Declarations

//Tests to see if a file is a link file (0 if it is, < -1 if not)
extern int test_link(FILE *);
//FILE* is an opened FILE pointer

//fills the LIF structure with data (0 if successful < -1 if not)
extern int get_lif(FILE *, int, struct LIF *);
//FILE* is an opened FILE pointer
//int is the size of the opened file
//LIF is a pointer to a struct LIF which will hold the data

//fills LIF_A with the ASCII representation of the LIF
//(0 if successful < -1 if not)
extern int get_lif_a(struct LIF *, struct LIF_A *);
//LIF must be a filled LIF structure
//LIF_A is an empty LIF_A structure

#endif
