/*********************************************************
**                                                      **
**                 liblife.c                            **
**                                                      **
** A library to handle the data from Windows link files **
**                                                      **
**         Copyright Paul Tew 2011 to 2017              **
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

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "./liblife.h"

//Declaration of functions used privately
int get_lhdr(FILE *, struct LIF *);
int get_lhdr_a(struct LIF_HDR *, struct LIF_HDR_A *);
int get_idlist(FILE *, int, int, struct LIF *);
int get_idlist_a(struct LIF_IDLIST *, struct LIF_IDLIST_A *);
int get_linkinfo(FILE *, int, int, struct LIF *);
int get_linkinfo_a(struct LIF_INFO *, struct LIF_INFO_A *);
int get_stringdata(FILE *, int, struct LIF *);
int get_stringdata_a(struct LIF_STRINGDATA *, struct LIF_STRINGDATA_A *);
int get_extradata(FILE *, int, struct LIF *);
int get_extradata_a(struct LIF_EXTRA_DATA *, struct LIF_EXTRA_DATA_A *);
void get_flag_a(unsigned char *, struct LIF_HDR *);
void get_attr_a(unsigned char *, struct LIF_HDR *);
uint64_t get_le_uint64(unsigned char[], int);
int64_t get_le_int64(unsigned char[], int);
uint32_t  get_le_uint32(unsigned char[], int);
uint16_t get_le_uint16(unsigned char[], int);
int32_t get_le_int32(unsigned char[], int);
int16_t get_le_int16(unsigned char[], int);
void get_chars(unsigned char[], int, int, unsigned char[]);
int get_le_unistr(unsigned char[], int, int, wchar_t[]);
void get_filetime_a_short(int64_t, unsigned char[]);
void get_filetime_a_long(int64_t, unsigned char[]);
void get_ltp(struct LIF_TRACKER_PROPS *, unsigned char*);
void get_droid_a(struct LIF_CLSID *, struct LIF_CLSID_A *);
void led_setnull(struct LIF_EXTRA_DATA *);


//Function get_lif(FILE* fp, int size, struct LIF lif) takes an open file
//pointer and populates the LIF with relevant data.
extern int get_lif(FILE* fp, int size, struct LIF* lif)
{
  int pos = 0;

  assert(size >= 0x4C);   //Min size for a LIF (must contain a header at least)
  if (get_lhdr(fp, lif) < 0)
  {
    return -1;
  }
  pos += 0x4C;

  if (get_idlist(fp, size, pos, lif) < 0)
  {
    return -2;
  }
  if (lif->lidl.IDListSize > 0)
  {
    pos += (lif->lidl.IDListSize + 2);
  }

  if (get_linkinfo(fp, size, pos, lif) < 0)
  {
    return -3;
  }
  pos += (lif->li.Size);

  if (get_stringdata(fp, pos, lif) < 0)
  {
    return -4;
  }
  pos += (lif->lsd.Size);

  if (pos < size) //Only get the extra data if it exists
  {
    if (get_extradata(fp, pos, lif) < 0)
    {
      return -5;
    }
    pos += (lif->led.Size);
  }
  else //If it does not exist then set it to null
  {
    led_setnull(&lif->led);
  }

  return 0;
}
//
//Function get_lif_a(struct LIF* lif, struct LIF_A* lif_a) populates the LIF_A
//structure with the ASCII representation of a LIF
extern int get_lif_a(struct LIF* lif, struct LIF_A* lif_a)
{
  if (get_lhdr_a(&lif->lh, &lif_a->lha) < 0)
  {
    return -1;
  }
  if (get_idlist_a(&lif->lidl, &lif_a->lidla) < 0)
  {
    return -2;
  }
  if (get_linkinfo_a(&lif->li, &lif_a->lia) < 0)
  {
    return -3;
  }
  if (get_stringdata_a(&lif->lsd, &lif_a->lsda) < 0)
  {
    return -4;
  }
  if (get_extradata_a(&lif->led, &lif_a->leda) < 0)
  {
    return -5;
  }
  return 0;
}
//
//Function test_link(FILE *fp) takes an open file pointer as an argument
//and returns 0 if the file IS a Windows link file or -1 if not.
extern int test_link(FILE* fp)
{
  struct LIF lif;
  int i;

  assert(fp >= 0); //Ensure we have a live file pointer - this kills execution on failure
  if (fp < 0)
  {
    return -1; //Same as the previous but won't kill execution if NDEBUG is defined
  }

  get_lhdr(fp, &lif);
  //Check the value of HeaderSize
  if (lif.lh.H_size != 0x0000004C)
    return -1;
  //Check the CLSID
  if (lif.lh.CLSID.Data1 != 0x00021401)
    return -2;
  if (lif.lh.CLSID.Data2 != 0x0000)
    return -3;
  if (lif.lh.CLSID.Data3 != 0x0000)
    return -4;
  if (!((lif.lh.CLSID.Data4hi[0] == 0xC0) && (lif.lh.CLSID.Data4hi[1] == 0)))
    return -5;
  for (i = 0; i < 5; i++)
  {
    if (lif.lh.CLSID.Data4lo[i] != 0)
    {
      return -6;
    }
  }
  if (lif.lh.CLSID.Data4lo[5] != 0x46)
    return -7;
  //Now check that the reserved data areas are 0 (as specified in MS-SHLLINK)
  if (lif.lh.Reserved1 != 0x0000)
    return -8;
  if (lif.lh.Reserved2 != 0x00000000)
    return -9;
  if (lif.lh.Reserved3 != 0x00000000)
    return -10;
  return 0;
}

//TODO EXPERIMENTAL!!
//Function: find_propstore(unsigned char * data_buf, int size, struct LIF_PROPERTY_STORE_PROPS * psp)
//          Takes a data buffer 'data_buf' no bigger than 'size' and
//          searches for the first LIF_SER_PROPSTORE it can find (by looking
//          for the header version signature which should equal 0x53505331 
//          but in LE format). It is assumed that this is the first in a 
//          possible series of LIF_SER_PROPSTORE objects with a terminator of
//          0x00000000
//          'posn' is the location of the first byte of data_buf relative to the start of
//          the link file.
//
//          Return value is 0 on success (object found) and !0 on object not found or
//          error. If the return value is 0 then the position of the first
//          LIF_SER_PROPSTORE is in psp->Posn.
extern int find_propstores(unsigned char * data_buf, int size, int position, struct LIF_PROPERTY_STORE_PROPS * psp)
{
  int i, j, k, p, vp, posn;

  for (k = 4; k < (size - 23); k++) // No point looking for Version sig prior to posn 4 or 
                                    // in last 23 bytes (length of which is determined from: 0 size Property Value [4 bytes]
                                    // + FormatID GUID [16 bytes] + the 3 bytes remaining in the Version signature).
  {
    if (data_buf[k] == 0x31)
    {
      if (data_buf[k + 1] == 0x53)
      {
        if (data_buf[k + 2] == 0x50)
        {
          if (data_buf[k + 3] == 0x53)
          {
            // Version signature found
            posn = k - 4; // The location of the first LIF_PROPERTY_STORE_PROPS
                         // This is constructed data for the most part as LIF_PROPERTY_STORE_PROPS 
                         // does not exist in an ITemID but it is used here because it is useful to
                         // draw together a series of LIF_SER_PROPSTORE objects.
            psp->Posn = posn + position;
            psp->Size = 4; // The size of the last (uncounted) property store
            psp->sig = 0; // This is not needed here
            psp->NumStores = 0;
            //**************************************
            for (i = 0; i < PROPSTORES; i++) // Cycle through all the valid property stores
            {
              psp->Stores[i].NumValues = 0;
              psp->Stores[i].StorageSize = get_le_uint32(data_buf, posn);
              psp->Size += psp->Stores[i].StorageSize; // Keep a running total
              if (psp->Stores[i].StorageSize == 0) // An empty property store
              {
                break;
              }
              p = posn + 4;
              psp->Stores[i].Version = get_le_uint32(data_buf, p);
              p += 4;
              psp->Stores[i].FormatID.Data1 = get_le_uint32(data_buf, p);
              p += 4;
              psp->Stores[i].FormatID.Data2 = get_le_uint16(data_buf, p);
              p += 2;
              psp->Stores[i].FormatID.Data3 = get_le_uint16(data_buf, p);
              p += 2;
              get_chars(data_buf, p, 2, psp->Stores[i].FormatID.Data4hi);
              p += 2;
              get_chars(data_buf, p, 6, psp->Stores[i].FormatID.Data4lo);
              p += 6;
              if ((psp->Stores[i].FormatID.Data1 == 0xD5CDD505) &&
                (psp->Stores[i].FormatID.Data2 == 0x2E9C) &&
                (psp->Stores[i].FormatID.Data3 == 0x101B) &&
                (psp->Stores[i].FormatID.Data4hi[0] == 0x93) &&
                (psp->Stores[i].FormatID.Data4hi[1] == 0x97) &&
                (psp->Stores[i].FormatID.Data4hi[0] == 0x08) &&
                (psp->Stores[i].FormatID.Data4hi[1] == 0x00) &&
                (psp->Stores[i].FormatID.Data4hi[2] == 0x2B) &&
                (psp->Stores[i].FormatID.Data4hi[3] == 0x2C) &&
                (psp->Stores[i].FormatID.Data4hi[4] == 0xF9) &&
                (psp->Stores[i].FormatID.Data4hi[5] == 0xAE)
                )
              {
                psp->Stores[i].NameType = 0x00;
              }
              else
              {
                psp->Stores[i].NameType = 0xFF;
              }
              for (j = 0; j < PROPVALUES; j++) // Cycle through all the valid property values
              {
                vp = p; // Save the position of the start of this value
                psp->Stores[i].PropValues[j].ValueSize = get_le_uint32(data_buf, vp);
                p += (int)psp->Stores[i].PropValues[j].ValueSize;// Move p to the next value store
                if (psp->Stores[i].PropValues[j].ValueSize == 0)
                {
                  psp->Stores[i].NumValues++; // Unlike a Property Store, an empty Value Store is counted
                  break;
                }
                psp->Stores[i].PropValues[j].NameSizeOrID = get_le_uint32(data_buf, vp + 4);
                psp->Stores[i].PropValues[j].Reserved = (uint8_t)data_buf[vp + 8];
                if (psp->Stores[i].NameType == 0)
                {
                  get_chars(data_buf, vp + 9, psp->Stores[i].PropValues[j].NameSizeOrID, psp->Stores[i].PropValues[j].Name);
                  vp += psp->Stores[i].PropValues[j].NameSizeOrID; // In the Case of a name type, offset the value pointer
                }
                psp->Stores[i].PropValues[j].PropertyType = get_le_uint16(data_buf, vp + 9);
                psp->Stores[i].PropValues[j].Padding = get_le_uint16(data_buf, vp + 11);
                get_chars(data_buf, vp + 13, (psp->Stores[i].PropValues[j].ValueSize), psp->Stores[i].PropValues[j].Value);
                psp->Stores[i].NumValues++;
              }
              posn += psp->Stores[i].StorageSize; // Move to the next propertystore
              psp->NumStores++;
            } //Cycle through the Propstores

            // TODO Fill LIF_SER_PROPSTORE - ongoing
            //**************************************
            return 0;
          }
        }
      }
    }
  }
  return -1;
}

// TODO - THIS FUNCTION IS EXPERIMENTAL!!!
//Function get_propstores_a(struct LIF_PROPERTY_STORE_PROPS * psp, struct LIF_PROPERTY_STORE_PROPS_A * pspa)
//Property Stores (MS-PROPSTORE S2) turn up in Link files in a number of places:
//  • Extradata.PropertyStoreDataBlock
//  • LinkTargetIDList.ItemID   and
//  • VistaAndAboveIDListDataBlock.IDList.ItemID
//Because there are so many different locations for this structure to appear, this function is 
//used to provide a common process to extract the property store data and place the interpreted
//ANSI string into the pspa structure (essentially a psp.ToString() function).
// Entry Conditions:
//   psp should be a filled 'struct LIF_PROPERTY_STORE_PROPS'
//   pspa should be an EMPTY 'struct LIF_PROPERTY_STORE_PROPS_A'
// Exit Conditions:
//   int == 0 = success
//   int != 0 = failure
extern int get_propstore_a(struct LIF_SER_PROPSTORE * ps, struct LIF_SER_PROPSTORE_A * psa)
{
  int       j, len;
  uint8_t   uinteger8, decimalscale, decimalsign;
  int8_t    integer8;
  uint16_t  uinteger16, boolean;
  int16_t   integer16;
  uint32_t  uinteger32, hresult, decimalHi32;
  int32_t   integer32;
  uint64_t  uinteger64, filetime, decimalLo64;
  int64_t   integer64;
  double    currency;
  char      decsign[9], lp_buf[300];
  wchar_t   lpw_buf[300];
  struct LIF_CLSID    guid;
  struct LIF_CLSID_A  guida;

  snprintf((char *)psa->StorageSize, 12, "%"PRIu32, ps->StorageSize);
  snprintf((char *)psa->Version, 12, "0x%.8"PRIX32, ps->Version);
  get_droid_a(&ps->FormatID, &psa->FormatID);
  if (ps->NameType == 0x00)
  {
    snprintf((char *)psa->NameType, 13, "String Name");
  }
  else
  {
    snprintf((char *)psa->NameType, 13, "Integer Name");
  }
  snprintf((char *)psa->NumValues, 7, "%"PRIu16, ps->NumValues);
  for (j = 0; j < ps->NumValues; j++) // Cycle through the property values
  {
    snprintf((char *)psa->PropValues[j].ValueSize, 12, "%"PRIu32, ps->PropValues[j].ValueSize);
    if (ps->PropValues[j].ValueSize > 0)
    {
      // Print No of bytes (Name type) or ID (Integer Type)
      if (ps->NameType == 0) // Name
      {
        snprintf((char *)psa->PropValues[j].NameSizeOrID, 12, "%"PRIu32, ps->PropValues[j].NameSizeOrID);
        snprintf((char *)psa->PropValues[j].Name, ps->PropValues[j].NameSizeOrID, "%s", ps->PropValues[j].Name);
      }
      else // Integer
      {
        snprintf((char *)psa->PropValues[j].NameSizeOrID, 12, "0x%.8"PRIX32, ps->PropValues[j].NameSizeOrID);
        snprintf((char *)psa->PropValues[j].Name, 6, "[N/A]");
      }
      snprintf((char *)psa->PropValues[j].Reserved, 6, "0x%.2"PRIX8, ps->PropValues[j].Reserved);
      snprintf((char *)psa->PropValues[j].PropertyType, 12, "0x%.4"PRIX16, ps->PropValues[j].PropertyType);
      snprintf((char *)psa->PropValues[j].Padding, 12, "0x%.4"PRIX16, ps->PropValues[j].Padding);
      switch (ps->PropValues[j].PropertyType)
      {
      case VT_EMPTY:  //  Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_EMPTY");
        snprintf((char *)psa->PropValues[j].Value, 6, "[N/A]");
        break;
      case VT_NULL:  //  Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_NULL");
        snprintf((char *)psa->PropValues[j].Value, 6, "[N/A]");
        break;
      case VT_I2:
        strcat((char *)psa->PropValues[j].PropertyType, " VT_I2");
        integer16 = get_le_int16(ps->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 50, "0x%.4"PRIX16" (%"PRIi16")", integer16, integer16);
        break;
      case VT_I4:
        strcat((char *)psa->PropValues[j].PropertyType, " VT_I4");
        integer32 = get_le_int32(ps->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 50, "0x%.8"PRIX32" (%"PRIi32")", integer32, integer32);
        break;
      case VT_R4:  //  Not tested
                   // TODO Fully Implement this
        strcat((char *)psa->PropValues[j].PropertyType, " VT_R4");
        uinteger32 = get_le_uint32(ps->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 100, "0x%.8"PRIX32" [Conversion from binary to IEEE 32 bit floating point not implemented]", uinteger32);
        break;
      case VT_R8:  //  Not tested
                   // TODO Fully Implement this
        strcat((char *)psa->PropValues[j].PropertyType, " VT_R8");
        uinteger64 = get_le_uint64(ps->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 100, "0x%.16"PRIX64" [Conversion from binary to IEEE 64 bit floating point not implemented]", uinteger64);
        break;
      case VT_CY:  //  Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_CY");
        integer64 = get_le_int64(ps->PropValues[j].Value, 0);
        currency = (double)integer64 / 10000;
        snprintf((char *)psa->PropValues[j].Value, 22, "%f.5 (Currency Units)", currency);
        break;
      case VT_DATE:  // Not tested
                     // TODO Fully Implement this
        strcat((char *)psa->PropValues[j].PropertyType, " VT_DATE");
        integer64 = get_le_int64(ps->PropValues[j].Value, 0);
        // An idea here is to convert the day using the integer part of the number (straight forward) 
        // then calculate how many seconds are in the fractional part using 1 second = 1/86400
        // the hours minutes and full seconds can be calculated from this. Whatever is left is 
        // the fraction of a second.
        snprintf((char *)psa->PropValues[j].Value, 100, "0x%.16"PRIX64" [Conversion from binary to DATE format not implemented]", uinteger64);
        break;
      case VT_BSTR:
        strcat((char *)psa->PropValues[j].PropertyType, " VT_BSTR");
        len = get_le_uint32(ps->PropValues[j].Value, 0);
        get_chars(ps->PropValues[j].Value, 4, len, (unsigned char *)lp_buf);
        if ((lp_buf[len - 1] == 0) && (lp_buf[len - 2] == 0)) // Is it unicode (2 byte string terminator)?
        {
          get_le_unistr(ps->PropValues[j].Value, 4, len / 2, lpw_buf);
          snprintf((char *)psa->PropValues[j].Value, len, "%ls", lpw_buf);
        }
        else // Must be ANSI
        {
          snprintf((char *)psa->PropValues[j].Value, len, "%s", lp_buf);
        }
        break;
      case VT_ERROR:  // Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_ERROR");
        hresult = get_le_uint32(ps->PropValues[j].Value, 0);
        // TODO Fully Implement this
        snprintf((char *)psa->PropValues[j].Value, 100, "0x%.16"PRIX32" [Conversion from HRESULT not fully implemented]", hresult);
        break;
      case VT_BOOL:
        strcat((char *)psa->PropValues[j].PropertyType, " VT_BOOL");
        boolean = get_le_uint16(ps->PropValues[j].Value, 0);
        if (boolean == 0x0000)
        {
          snprintf((char *)psa->PropValues[j].Value, 20, "0x0000 (FALSE)");
        }
        else
        {
          snprintf((char *)psa->PropValues[j].Value, 20, "0x%.4"PRIX16" (TRUE)", boolean);
        }
        break;
      case VT_DECIMAL:  // Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_DECIMAL");
        decimalscale = (uint8_t)psa->PropValues[j].Value[2]; // First two bytes are reserved
        decimalsign = (uint8_t)psa->PropValues[j].Value[3];
        decimalHi32 = get_le_uint32(ps->PropValues[j].Value, 4);
        decimalLo64 = get_le_uint64(ps->PropValues[j].Value, 8);
        if (decimalsign == 0)
        {
          strcat((char *)decsign, "POSITIVE");
        }
        else if (decimalsign == 0x80)
        {
          strcat((char *)decsign, "NEGATIVE");
        }
        else
        {
          strcat((char *)decsign, "ERROR");
        }
        snprintf((char *)psa->PropValues[j].Value, 200, "DECIMAL - scale: %"PRIu8", sign: %s, Hi32: %"PRIu32", Lo64: %"PRIu64, decimalscale, decsign, decimalHi32, decimalLo64);
        break;
      case VT_I1:  // Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_I1");
        integer8 = (int8_t)psa->PropValues[j].Value[0];
        snprintf((char *)psa->PropValues[j].Value, 50, "0x%.2"PRIX8" (%"PRIi8")", integer8, integer8);
        break;
      case VT_UI1:  // Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_UI1");
        uinteger8 = (uint8_t)psa->PropValues[j].Value[0];
        snprintf((char *)psa->PropValues[j].Value, 50, "0x%.2"PRIX8" (%"PRIu8")", uinteger8, uinteger8);
        break;
      case VT_UI2:  // Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_UI2");
        uinteger16 = get_le_uint16(psa->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 50, "0x%.4"PRIX16" (%"PRIu16")", uinteger16, uinteger16);
        break;
      case VT_UI4:
        strcat((char *)psa->PropValues[j].PropertyType, " VT_UI4");
        uinteger32 = get_le_uint32(psa->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 50, "0x%.8"PRIX32" (%"PRIu32")", uinteger32, uinteger32);
        break;
      case VT_I8:  // Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_I8");
        integer64 = get_le_uint64(ps->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 80, "0x%.16"PRIX64" (%"PRIi64")", integer64, integer64);
        break;
      case VT_UI8:
        strcat((char *)psa->PropValues[j].PropertyType, " VT_UI8");
        uinteger64 = get_le_uint64(ps->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 80, "0x%.16"PRIX64" (%"PRIu64")", uinteger64, uinteger64);
        break;
      case VT_INT:  // Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_INT");
        integer32 = get_le_int32(ps->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 50, "0x%.8"PRIX32" (%"PRIi32")", integer32, integer32);
        break;
      case VT_UINT:  // Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_UINT");
        uinteger32 = get_le_uint32(psa->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 50, "0x%.8"PRIX32" (%"PRIu32")", uinteger32, uinteger32);
        break;
      case VT_LPSTR:  // Because the definition is CodePageString this could be Unicode
        strcat((char *)psa->PropValues[j].PropertyType, " VT_LPSTR");
        len = get_le_uint32(ps->PropValues[j].Value, 0);
        get_chars(ps->PropValues[j].Value, 4, len, (unsigned char *)lp_buf);
        if ((lp_buf[len - 1] == 0) && (lp_buf[len - 2] == 0)) // Is it unicode (2 byte string terminator)?
        {
          get_le_unistr(ps->PropValues[j].Value, 4, len / 2, lpw_buf);
          snprintf((char *)psa->PropValues[j].Value, len, "%ls", lpw_buf);
        }
        else // Must be ANSI
        {
          snprintf((char *)psa->PropValues[j].Value, len, "%s", lp_buf);
        }
        break;
      case VT_LPWSTR: // Always Unicode
        strcat((char *)psa->PropValues[j].PropertyType, " VT_LPWSTR");
        len = get_le_uint32(ps->PropValues[j].Value, 0);
        get_le_unistr(ps->PropValues[j].Value, 4, len, lpw_buf);
        snprintf((char *)psa->PropValues[j].Value, len, "%ls", lpw_buf);
        break;
      case VT_FILETIME:
        strcat((char *)psa->PropValues[j].PropertyType, " VT_FILETIME");
        filetime = get_le_uint64(ps->PropValues[j].Value, 0);
        get_filetime_a_long(filetime, psa->PropValues[j].Value);
        break;
      case VT_BLOB: //Not Tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_BLOB");
        uinteger32 = get_le_uint32(ps->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 50, "Size: %"PRIi32" bytes, [BLOB not shown]", uinteger32);
        break;
      case VT_STREAM: //Not Tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_STREAM");
        len = get_le_uint32(ps->PropValues[j].Value, 0);
        get_chars(ps->PropValues[j].Value, 4, len, (unsigned char *)lp_buf);
        if ((lp_buf[len - 1] == 0) && (lp_buf[len - 2] == 0)) // Is it unicode (2 byte string terminator)?
        {
          get_le_unistr(ps->PropValues[j].Value, 4, len / 2, lpw_buf);
          snprintf((char *)psa->PropValues[j].Value, len, "%ls", lpw_buf);
        }
        else // Must be ANSI
        {
          snprintf((char *)psa->PropValues[j].Value, len, "%s", lp_buf);
        }
        break;
      case VT_STORAGE: //Not Tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_STORAGE");
        len = get_le_uint32(ps->PropValues[j].Value, 0);
        get_chars(ps->PropValues[j].Value, 4, len, (unsigned char *)lp_buf);
        if ((lp_buf[len - 1] == 0) && (lp_buf[len - 2] == 0)) // Is it unicode (2 byte string terminator)?
        {
          get_le_unistr(ps->PropValues[j].Value, 4, len / 2, lpw_buf);
          snprintf((char *)psa->PropValues[j].Value, len, "%ls", lpw_buf);
        }
        else // Must be ANSI
        {
          snprintf((char *)psa->PropValues[j].Value, len, "%s", lp_buf);
        }
        break;
      case VT_STREAMED_OBJECT: //Not Tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_STREAMED_OBJECT");
        len = get_le_uint32(ps->PropValues[j].Value, 0);
        get_chars(ps->PropValues[j].Value, 4, len, (unsigned char *)lp_buf);
        if ((lp_buf[len - 1] == 0) && (lp_buf[len - 2] == 0)) // Is it unicode (2 byte string terminator)?
        {
          get_le_unistr(ps->PropValues[j].Value, 4, len / 2, lpw_buf);
          snprintf((char *)psa->PropValues[j].Value, len, "%ls", lpw_buf);
        }
        else // Must be ANSI
        {
          snprintf((char *)psa->PropValues[j].Value, len, "%s", lp_buf);
        }
        break;
      case VT_STORED_OBJECT: //Not Tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_STORED_OBJECT");
        len = get_le_uint32(ps->PropValues[j].Value, 0);
        get_chars(ps->PropValues[j].Value, 4, len, (unsigned char *)lp_buf);
        if ((lp_buf[len - 1] == 0) && (lp_buf[len - 2] == 0)) // Is it unicode (2 byte string terminator)?
        {
          get_le_unistr(ps->PropValues[j].Value, 4, len / 2, lpw_buf);
          snprintf((char *)psa->PropValues[j].Value, len, "%ls", lpw_buf);
        }
        else // Must be ANSI
        {
          snprintf((char *)psa->PropValues[j].Value, len, "%s", lp_buf);
        }
        break;
      case VT_BLOB_OBJECT: //Not Tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_BLOB_OBJECT");
        uinteger32 = get_le_uint32(ps->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 50, "Size: %"PRIi32" bytes, [BLOB not shown]", uinteger32);
        break;
      case VT_CF:  // Not tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_CF");
        uinteger32 = get_le_uint32(ps->PropValues[j].Value, 0);
        snprintf((char *)psa->PropValues[j].Value, 50, "Size: %"PRIi32" bytes, [Clipboard Data not shown]", uinteger32);
        break;
      case VT_CLSID:
        strcat((char *)psa->PropValues[j].PropertyType, " VT_CLSID");
        guid.Data1 = get_le_uint32(ps->PropValues[j].Value, 0);
        guid.Data2 = get_le_uint16(ps->PropValues[j].Value, 4);
        guid.Data3 = get_le_uint16(ps->PropValues[j].Value, 6);
        get_chars(ps->PropValues[j].Value, 8, 2, guid.Data4hi);
        get_chars(ps->PropValues[j].Value, 10, 6, guid.Data4lo);
        get_droid_a(&guid, &guida);
        // For now just print out the GUID, and (if appropriate) the time and MAC address
        snprintf((char *)psa->PropValues[j].Value, 150, "UUID: %s, Time: %s, Node (MAC addr): %s", guida.UUID, guida.Time_long, guida.Node);
        break;
      case VT_VERSIONED_STREAM: //Not Tested
        strcat((char *)psa->PropValues[j].PropertyType, " VT_VERSIONED_STREAM");
        len = get_le_uint32(ps->PropValues[j].Value, 0);
        get_chars(ps->PropValues[j].Value, 4, len, (unsigned char *)lp_buf);
        if ((lp_buf[len - 1] == 0) && (lp_buf[len - 2] == 0)) // Is it unicode (2 byte string terminator)?
        {
          get_le_unistr(ps->PropValues[j].Value, 4, len / 2, lpw_buf);
          snprintf((char *)psa->PropValues[j].Value, len, "%ls", lpw_buf);
        }
        else // Must be ANSI
        {
          snprintf((char *)psa->PropValues[j].Value, len, "%s", lp_buf);
        }
        break;
      default:
        if (ps->PropValues[j].PropertyType || 0x1000)
        {
          strcat((char *)psa->PropValues[j].PropertyType, " VT_VECTOR | ?");
        }
        else if (ps->PropValues[j].PropertyType || 0x2000)
        {
          strcat((char *)psa->PropValues[j].PropertyType, " VT_ARRAY | ?");
        }
        snprintf((char *)psa->PropValues[j].Value, 43, "[Sorry, interpretation is not implemented]");
      }
    }
    else
    {
      snprintf((char *)psa->PropValues[j].NameSizeOrID, 6, "[N/A]");
      snprintf((char *)psa->PropValues[j].Reserved, 6, "[N/A]");
      snprintf((char *)psa->PropValues[j].Name, 6, "[N/A]");
      snprintf((char *)psa->PropValues[j].PropertyType, 6, "[N/A]");
      snprintf((char *)psa->PropValues[j].Padding, 6, "[N/A]");
      snprintf((char *)psa->PropValues[j].Value, 6, "[N/A]");
    }
  }
  return 0;
}
//
//Function get_lhdr(FILE *fp, struct LIF_HDR *lh) takes an open file pointer
//and a pointer to a LIF_HDR structure.
//On exit the LIF_HDR will be populated.
int get_lhdr(FILE *fp, struct LIF *lif)
{
  unsigned char header[0x4C];
  int chr;
  int i;


  assert(fp >= 0); //Ensure we have a live file pointer - this kills execution
  if (fp < 0)
  {
    return -1; //Same as the previous but won't kill execution if NDEBUG defined
  }

  rewind(fp);
  //I'd love to use 'read()' here but I'm trying to avoid using unistd.h
  //because I want the library to compile under Windoze
  for (i = 0; i < 0x4C; i++)
  {
    chr = getc(fp);
    if (chr != EOF)
    {
      header[i] = (unsigned char)chr;
    }
    else
    {
      perror("Error in function get_lhdr()");
    }
  }
  lif->lh.H_size = get_le_uint32(header, 0);
  lif->lh.CLSID.Data1 = get_le_uint32(header, 4);
  lif->lh.CLSID.Data2 = get_le_uint16(header, 8);
  lif->lh.CLSID.Data3 = get_le_uint16(header, 10);
  get_chars(header, 12, 2, lif->lh.CLSID.Data4hi);
  get_chars(header, 14, 6, lif->lh.CLSID.Data4lo);
  lif->lh.Flags = get_le_uint32(header, 20);
  lif->lh.Attr = get_le_uint32(header, 24);
  lif->lh.CrDate = get_le_uint64(header, 28);
  lif->lh.AcDate = get_le_uint64(header, 36);
  lif->lh.WtDate = get_le_uint64(header, 44);
  lif->lh.Size = get_le_uint32(header, 52);
  lif->lh.IconIndex = get_le_int32(header, 56);
  lif->lh.ShowState = get_le_uint32(header, 60);
  lif->lh.Hotkey.LowKey = header[64];
  lif->lh.Hotkey.HighKey = header[65];
  lif->lh.Reserved1 = get_le_uint16(header, 66);
  lif->lh.Reserved2 = get_le_uint32(header, 68);
  lif->lh.Reserved3 = get_le_uint32(header, 72);
  return 0;
}
//
//Function get_lhdr_a(LIF_HDR*, LIF_HDR_A*) converts the data in a LIF_HDR
//into a readable form and populates the strings in LIF_HDR_A
int get_lhdr_a(struct LIF_HDR* lh, struct LIF_HDR_A* lha)
{
  unsigned char lk[20], hk1[10], hk2[10], hk3[10], attr_str[400], flag_str[600];

  snprintf((char *)lha->H_size, 10, "%"PRIu32, lh->H_size);
  snprintf((char *)lha->CLSID, 40, "{00021401-0000-0000-C000-000000000046}");
  get_flag_a(flag_str, lh);
  snprintf((char *)lha->Flags, 550, "0x%.8"PRIX32"  %s", lh->Flags, flag_str);
  get_attr_a(attr_str, lh);
  snprintf((char *)lha->Attr, 250, "0x%.8"PRIX32"  %s", lh->Attr, attr_str);
  get_filetime_a_short(lh->CrDate, lha->CrDate);
  get_filetime_a_long(lh->CrDate, lha->CrDate_long);
  get_filetime_a_short(lh->AcDate, lha->AcDate);
  get_filetime_a_long(lh->AcDate, lha->AcDate_long);
  get_filetime_a_short(lh->WtDate, lha->WtDate);
  get_filetime_a_long(lh->WtDate, lha->WtDate_long);
  snprintf((char *)lha->Size, 25, "%"PRIu32, lh->Size);
  snprintf((char *)lha->IconIndex, 25, "%"PRId32, lh->IconIndex);
  switch (lh->ShowState)
  {
  case 0x3:
    snprintf((char *)lha->ShowState, 40, "SW_SHOWMAXIMIZED");
    break;
  case 0x7:
    snprintf((char *)lha->ShowState, 40, "SW_SHOWMINNOACTIVE");
    break;
  default:
    snprintf((char *)lha->ShowState, 40, "SW_SHOWNORMAL");
  }
  // 
  // High key first
  if (lh->Hotkey.HighKey & 0x01)
  {
    sprintf((char *)hk1, "SHIFT + ");
  }
  else
  {
    //Just create a null length string if the relevant key isn't there
    hk1[0] = 0;
  }
  if (lh->Hotkey.HighKey & 0x02)
  {
    sprintf((char *)hk2, "CTRL + ");
  }
  else
  {
    hk2[0] = 0;
  }
  if (lh->Hotkey.HighKey & 0x04)
  {
    sprintf((char *)hk3, "ALT + ");
  }
  else
  {
    hk3[0] = 0;
  }
  //
  // Then the Low key
  if (((lh->Hotkey.LowKey > 0x2F) && (lh->Hotkey.LowKey < 0x5B)))
  {
    // Regular keys
    sprintf((char *)lk, "%u", (unsigned int)lh->Hotkey.LowKey);
  }
  else if (((lh->Hotkey.LowKey > 0x6F) && (lh->Hotkey.LowKey < 0x88)))
  {
    // Function keys
    sprintf((char *)lk, "F%u", (unsigned int)lh->Hotkey.LowKey - 111);
  }
  // Special keys
  else if (lh->Hotkey.LowKey == 0x90)
  {
    sprintf((char *)lk, "NUM LOCK");
  }
  else if (lh->Hotkey.LowKey == 0x91)
  {
    sprintf((char *)lk, "SCROLL LOCK");
  }
  // Probably an exotic keyboard if we land in this bit of code.
  else
  {
    sprintf((char *)lk, "[NOT DEFINED]");
  }

  //Now write the keys to the LIF_HDR_A
  snprintf((char *)lha->Hotkey, 40, "%s%s%s%s", hk1, hk2, hk3, lk);
  snprintf((char *)lha->Reserved1, 10, "0x0000");
  snprintf((char *)lha->Reserved2, 20, "0x00000000");
  snprintf((char *)lha->Reserved3, 20, "0x00000000");

  return 0; //There aren't any fail states built into this function just yet
  //but feel free to return -1 if you need to
}
//
// Function 'get_idlist()' fills a LIF_IDLIST structure with data from the
// opened file fp
int get_idlist(FILE * fp, int size, int loc, struct LIF * lif)
{
  unsigned char   size_buf[2];   //A small buffer to hold the size element
  int             numItems = 0, posn = loc + 2, i, datasize;

  if (lif->lh.Flags & 0x00000001)
  {
    fseek(fp, loc, SEEK_SET);
    size_buf[0] = getc(fp);
    size_buf[1] = getc(fp);
    lif->lidl.IDListSize = get_le_uint16(size_buf, 0);
    if (lif->lidl.IDListSize > 0)
    {
      //posn points to the first ItemID relative to the start of TargetIDList
      while (posn < (loc + 2 + lif->lidl.IDListSize))
      {
        fseek(fp, posn, SEEK_SET);
        size_buf[0] = getc(fp);
        size_buf[1] = getc(fp);
        lif->lidl.Items[numItems].ItemIDSize = get_le_uint16(size_buf, 0);
        datasize = lif->lidl.Items[numItems].ItemIDSize - 2;
        if (lif->lidl.Items[numItems].ItemIDSize == 0)
        {
          break;
        }
        else
        {
          for (i = 0; (i < datasize) && (i <= MAXITEMIDSIZE); i++)
          {
            lif->lidl.Items[numItems].Data[i] = getc(fp);
          }
        }
        posn = posn + lif->lidl.Items[numItems].ItemIDSize;
        numItems++;
      }
      lif->lidl.NumItemIDs = numItems;
    }
  }
  else //No ID List
  {
    lif->lidl.IDListSize = 0;
    lif->lidl.NumItemIDs = 0;
  }

  return 0;
}
//
// Converts the data in a LIF_IDLIST into its ASCII representation
int get_idlist_a(struct LIF_IDLIST * lidl, struct LIF_IDLIST_A * lidla)
{
  int  i;

  if (!(lidl->IDListSize == 0))
  {
    snprintf((char *)lidla->IDListSize, 10, "%"PRIu16, lidl->IDListSize);
    snprintf((char *)lidla->NumItemIDs, 10, "%"PRIu16, lidl->NumItemIDs);
  }
  else
  {
    snprintf((char *)lidla->IDListSize, 10, "[N/A]");
    snprintf((char *)lidla->NumItemIDs, 10, "[N/A]");
  }
  for (i = 0; i < lidl->NumItemIDs; i++)
  {
    snprintf((char *)lidla->Items[i].ItemIDSize, 10, "%"PRIu16, lidl->Items[i].ItemIDSize);
  }

  return 0;
}
//
// Fills a LIF_INFO structure with data
// This includes filling the VolID and CNR structures (a lot of data, hence the
// big function)
int get_linkinfo(FILE * fp, int size, int pos, struct LIF * lif)
{
  unsigned char      size_buf[4];   //A small buffer to hold the size element
  unsigned char *    data_buf;
  int       i;
  //uint16_t  u16;

  if (lif->lh.Flags & 0x00000002) //There is a LinkInfo structure
  {
    if (size < (pos + 4)) // There is something wrong because the size of the
      // file is less than the current position
      // (just a sanity check)
    {
      return -1;
    }
    fseek(fp, pos, SEEK_SET);
    for (i = 0; i < 4; i++) // Get the initial size
    {
      size_buf[i] = getc(fp);
    }
    lif->li.Size = get_le_uint32(size_buf, 0);
    // The general idea here is to fill a temporary buffer with the characters
    // (rather than read the data directly) I then have control over reading the
    // data from the buffer as little/big endian or ANSI vs Unicode too.
    data_buf = (unsigned char*)malloc((size_t)(lif->li.Size - 4));
    assert(data_buf != NULL);
    for (i = 0; i < (lif->li.Size - 4); i++)
    {
      data_buf[i] = getc(fp);
    }
    lif->li.HeaderSize = get_le_uint32(data_buf, 0);
    lif->li.Flags = get_le_uint32(data_buf, 4);
    lif->li.IDOffset = get_le_uint32(data_buf, 8);
    lif->li.LBPOffset = get_le_uint32(data_buf, 12);
    lif->li.CNRLOffset = get_le_uint32(data_buf, 16);
    lif->li.CPSOffset = get_le_uint32(data_buf, 20);
    if (lif->li.HeaderSize >= 0x00000024)
    {
      lif->li.LBPOffsetU = get_le_uint32(data_buf, 24);
      lif->li.CPSOffsetU = get_le_uint32(data_buf, 28);
    }
    else
    {
      lif->li.LBPOffsetU = 0;
      lif->li.CPSOffsetU = 0;
    }
    if (lif->li.Flags & 0x00000001) //There is a Volume ID structure
    {
      //IDOffset is from start of LinkInfo but our buffer starts at pos 4
      lif->li.VolID.Size = get_le_uint32(data_buf, ((lif->li.IDOffset) - 4));

      lif->li.VolID.DriveType = get_le_uint32(data_buf, ((lif->li.IDOffset) - 4) + 4);

      lif->li.VolID.DriveSN = get_le_uint32(data_buf, ((lif->li.IDOffset) - 4) + 8);

      lif->li.VolID.VLOffset = get_le_uint32(data_buf, ((lif->li.IDOffset) - 4) + 12);

      //Is the volume label ANSI or Unicode?
      //There are two ways to work this out...
      //1) lif->li.HeaderSize < 0x00000024 = ANSI (MSSHLLINK Sec 2.3)   or
      //2) lif->li.VolID.VLOffset != 0x00000014 = ANSI  (MSSHLLINK Sec 2.3.1)
      if (lif->li.HeaderSize < 0x00000024) //ANSI
      {
        snprintf((char *)lif->li.VolID.VolumeLabel, 33, "%s", &data_buf[(lif->li.VolID.VLOffset) + ((lif->li.IDOffset) - 4)]);

        if (strlen((char *)lif->li.VolID.VolumeLabel) == 0)
        {
          snprintf((char *)lif->li.VolID.VolumeLabel, 33, "[EMPTY]");
        }
        //The Unicode Volume Label is not used if the ANSI one is
        lif->li.VolID.VLOffsetU = 0;
        lif->li.VolID.VolumeLabelU[0] = (wchar_t)0;
      }
      else //Unicode
      {
        //Fetch the unicode string
        get_le_unistr(data_buf, ((lif->li.VolID.VLOffsetU) + ((lif->li.IDOffset) - 4)), 33, lif->li.VolID.VolumeLabelU);

        snprintf((char *)lif->li.VolID.VolumeLabel, 33, "[NOT USED]");
      }

      //Get the Local Base Path string
      //We get this now because it is dependant on the
      //VolumeIDAndLocalBasePath flag being set just as the VolumeID is
      snprintf((char *)lif->li.LBP, 300, "%s", &data_buf[(lif->li.LBPOffset - 4)]);
    }
    else //There isn't a VolumeID structure so fill that part of the LIF with
      //empty values
    {
      lif->li.VolID.Size = 0;
      lif->li.VolID.DriveType = 0;
      lif->li.VolID.DriveSN = 0;
      lif->li.VolID.VLOffset = 0;
      lif->li.VolID.VLOffsetU = 0;
      snprintf((char *)lif->li.VolID.VolumeLabel, 33, "[NOT SET]");
      lif->li.VolID.VolumeLabelU[0] = (wchar_t)0;

      snprintf((char *)lif->li.LBP, 300, "[NOT SET]");
    }

    //There is a CNR structure
    if (lif->li.Flags & 0x00000002)
    {
      lif->li.CNR.Size = get_le_uint32(data_buf, (lif->li.CNRLOffset) - 4);
      lif->li.CNR.Flags = get_le_uint32(data_buf, (lif->li.CNRLOffset + 4) - 4);
      lif->li.CNR.NetNameOffset = get_le_uint32(data_buf, (lif->li.CNRLOffset + 8) - 4);
      lif->li.CNR.DeviceNameOffset = get_le_uint32(data_buf, (lif->li.CNRLOffset + 12) - 4);
      lif->li.CNR.NetworkProviderType = get_le_uint32(data_buf, (lif->li.CNRLOffset + 16) - 4);
      if (lif->li.CNR.NetNameOffset > 0x00000014)
      {
        lif->li.CNR.NetNameOffsetU = get_le_uint32(data_buf, (lif->li.CNRLOffset + 20) - 4);
        lif->li.CNR.DeviceNameOffsetU = get_le_uint32(data_buf, (lif->li.CNRLOffset + 24) - 4);
      }
      else
      {
        lif->li.CNR.NetNameOffsetU = 0;
        lif->li.CNR.DeviceNameOffsetU = 0;
      }
      //Get the NetName
      if (lif->li.CNR.NetNameOffset > 0)
      {
        snprintf((char *)lif->li.CNR.NetName, 300, "%s", &data_buf[(lif->li.CNR.NetNameOffset) + ((lif->li.CNRLOffset) - 4)]);
      }
      else
      {
        snprintf((char *)lif->li.CNR.NetName, 300, "[NOT USED]");
      }
      //Get the DeviceName
      if (lif->li.CNR.DeviceNameOffset > 0)
      {
        snprintf((char *)lif->li.CNR.DeviceName, 300, "%s", &data_buf[(lif->li.CNR.DeviceNameOffset) + ((lif->li.CNRLOffset) - 4)]);
      }
      else
      {
        snprintf((char *)lif->li.CNR.DeviceName, 300, "[NOT USED]");
      }
      //Get the NetNameUnicode and DeviceNameUnicode
      if (lif->li.CNR.NetNameOffset > 0x00000014)
      {
        get_le_unistr(data_buf, ((lif->li.CNR.NetNameOffsetU) + ((lif->li.IDOffset) - 4)), 300, lif->li.CNR.NetNameU);
        get_le_unistr(data_buf, ((lif->li.CNR.DeviceNameOffsetU) + ((lif->li.IDOffset) - 4)), 300, lif->li.CNR.DeviceNameU);
      }
      else
      {
        lif->li.CNR.NetNameU[0] = (wchar_t)0;
        lif->li.CNR.DeviceNameU[0] = (wchar_t)0;
      }
    }
    else // No CNR
    {
      lif->li.CNR.Size = 0;
      lif->li.CNR.Flags = 0;
      lif->li.CNR.NetNameOffset = 0;
      lif->li.CNR.DeviceNameOffset = 0;
      lif->li.CNR.NetworkProviderType = 0;
      lif->li.CNR.NetNameOffsetU = 0;
      lif->li.CNR.DeviceNameOffsetU = 0;
      snprintf((char *)lif->li.CNR.NetName, 300, "[NOT SET]");
      snprintf((char *)lif->li.CNR.DeviceName, 300, "[NOT SET]");
      lif->li.CNR.NetNameU[0] = (wchar_t)0;
      lif->li.CNR.DeviceNameU[0] = (wchar_t)0;
    }

    //There is a common path suffix
    if (lif->li.CPSOffset > 0)
    {
      snprintf((char *)lif->li.CPS, 100, "%s", &data_buf[(lif->li.CPSOffset - 4)]);
    }
    //There is a LocalBasePathUnicode
    if (lif->li.LBPOffsetU > 0)
    {
      //Fetch the unicode string
      get_le_unistr(data_buf, ((lif->li.LBPOffsetU) - 4), 300, lif->li.LBPU);
    }
    else
    {
      lif->li.LBPU[0] = (wchar_t)0;
    }
    //There is a CommonPathPathSuffixUnicode
    if (lif->li.CPSOffsetU > 0)
    {
      //Fetch the unicode string
      get_le_unistr(data_buf, ((lif->li.LBPOffsetU) - 4), 100, lif->li.CPSU);
    }
    else
    {
      lif->li.CPSU[0] = (wchar_t)0;
    }

    free(data_buf);
  }
  else // What to fill the LinkInfo structure with, in case it does not exist
  {
    lif->li.Size = 0;
    lif->li.HeaderSize = 0;
    lif->li.Flags = 0;
    lif->li.IDOffset = 0;
    lif->li.LBPOffset = 0;
    lif->li.CNRLOffset = 0;
    lif->li.CPSOffset = 0;
    lif->li.LBPOffsetU = 0;
    lif->li.CPSOffsetU = 0;

    lif->li.VolID.Size = 0;
    lif->li.VolID.DriveType = 0;
    lif->li.VolID.DriveSN = 0;
    lif->li.VolID.VLOffset = 0;
    lif->li.VolID.VLOffsetU = 0;
    snprintf((char *)lif->li.VolID.VolumeLabel, 33, "[NOT SET]");
    lif->li.VolID.VolumeLabelU[0] = (wchar_t)0;
    snprintf((char *)lif->li.LBP, 300, "[NOT SET]");
    lif->li.CNR.Size = 0;
    lif->li.CNR.Flags = 0;
    lif->li.CNR.NetNameOffset = 0;
    lif->li.CNR.DeviceNameOffset = 0;
    lif->li.CNR.NetworkProviderType = 0;
    lif->li.CNR.NetNameOffsetU = 0;
    lif->li.CNR.DeviceNameOffsetU = 0;
    snprintf((char *)lif->li.CNR.NetName, 300, "[NOT SET]");
    snprintf((char *)lif->li.CNR.DeviceName, 300, "[NOT SET]");
    lif->li.CNR.NetNameU[0] = (wchar_t)0;
    lif->li.CNR.DeviceNameU[0] = (wchar_t)0;
    snprintf((char *)lif->li.CPS, 100, "[NOT SET]");
    lif->li.LBPU[0] = (wchar_t)0;
    lif->li.CPSU[0] = (wchar_t)0;
  }

  return 0;
}
//
// Converts a LIF_INFO structure into its ASCII representation
int get_linkinfo_a(struct LIF_INFO *li, struct LIF_INFO_A *lia)
{
  if (li->Size > 0)
  {
    snprintf((char *)lia->Size, 10, "%"PRIu32, li->Size);
    snprintf((char *)lia->HeaderSize, 10, "%"PRIu32, li->HeaderSize);
    snprintf((char *)lia->Flags, 100, "0x%.8"PRIX32"  ", li->Flags);
    if (li->Flags & 0x00000001)
      strcat((char *)lia->Flags, "VolumeIDAndLocalBasePath | ");
    if (li->Flags & 0x00000002)
      strcat((char *)lia->Flags, "CommonNetworkRelativeLinkAndPathSuffix | ");
    if (strlen((char *)lia->Flags) > 11)
      lia->Flags[strlen((char *)lia->Flags) - 3] = (char)0;
    snprintf((char *)lia->IDOffset, 10, "%"PRIu32, li->IDOffset);
    snprintf((char *)lia->LBPOffset, 10, "%"PRIu32, li->LBPOffset);
    snprintf((char *)lia->CNRLOffset, 10, "%"PRIu32, li->CNRLOffset);
    snprintf((char *)lia->CPSOffset, 10, "%"PRIu32, li->CPSOffset);
    if (li->HeaderSize >= 0x00000024)
    {
      snprintf((char *)lia->LBPOffsetU, 10, "%"PRIu32, li->LBPOffsetU);
      snprintf((char *)lia->CPSOffsetU, 10, "%"PRIu32, li->CPSOffsetU);
    }
    else
    {
      snprintf((char *)lia->LBPOffsetU, 10, "[NOT SET]");
      snprintf((char *)lia->CPSOffsetU, 10, "[NOT SET]");
    }
    //There is a Volume ID structure (and a LBP)
    if (li->Flags & 0x00000001)
    {
      snprintf((char *)lia->VolID.Size, 10, "%"PRIu32, li->VolID.Size);
      switch (li->VolID.DriveType)
      {
      case 0x00000000:
        snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_UNKNOWN");
        break;
      case 0x00000001:
        snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_NO_ROOT_DIR");
        break;
      case 0x00000002:
        snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_REMOVABLE");
        break;
      case 0x00000003:
        snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_FIXED");
        break;
      case 0x00000004:
        snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_REMOTE");
        break;
      case 0x00000005:
        snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_CDROM");
        break;
      case 0x00000006:
        snprintf((char *)lia->VolID.DriveType, 20, "DRIVE_RAMDISK");
        break;
      default:
        snprintf((char *)lia->VolID.DriveType, 20, "ERROR");
      }
      snprintf((char *)lia->VolID.DriveSN, 20, "%"PRIX32, li->VolID.DriveSN);
      snprintf((char *)lia->VolID.VLOffset, 20, "%"PRIu32, li->VolID.VLOffset);
      snprintf((char *)lia->VolID.VLOffsetU, 20, "%"PRIu32, li->VolID.VLOffsetU);
      snprintf((char *)lia->VolID.VolumeLabel, 33, "%s", li->VolID.VolumeLabel);
      snprintf((char *)lia->VolID.VolumeLabelU, 33, "%ls", li->VolID.VolumeLabelU);
      switch (li->VolID.VolumeLabelU[0])
      {
      case 0:
        snprintf((char *)lia->VolID.VolumeLabelU, 33, "[NOT SET]");
        break;
      case 1:
        snprintf((char *)lia->VolID.VolumeLabelU, 33, "[EMPTY]");
        break;
      default:
        snprintf((char *)lia->VolID.VolumeLabelU, 33, "%ls", li->VolID.VolumeLabelU);
      }

      snprintf((char *)lia->LBP, 300, "%s", li->LBP);

    }
    else //If there is No VolID and LBP
    {
      sprintf((char *)lia->VolID.Size, "[N/A]");
      sprintf((char *)lia->VolID.DriveType, "[N/A]");
      sprintf((char *)lia->VolID.DriveSN, "[N/A]");
      sprintf((char *)lia->VolID.VLOffset, "[N/A]");
      sprintf((char *)lia->VolID.VLOffsetU, "[N/A]");
      snprintf((char *)lia->VolID.VolumeLabel, 33, "%s", li->VolID.VolumeLabel);
      snprintf((char *)lia->VolID.VolumeLabelU, 33, "%ls", li->VolID.VolumeLabelU);
      sprintf((char *)lia->VolID.VolumeLabelU, "[NOT SET]");
      snprintf((char *)lia->LBP, 300, "%s", li->LBP);
    }
    if (strlen((char *)li->CPS) > 0)
    {
      snprintf((char *)lia->CPS, 100, "%s", li->CPS);
    }
    else
    {
      snprintf((char *)lia->CPS, 100, "[NOT SET]");
    }
    //Is there a CNR?
    if (li->Flags & 0x00000002)
    {
      snprintf((char *)lia->CNR.Size, 10, "%"PRIu32, li->CNR.Size);
      switch (li->CNR.Flags)
      {
      case 0:
        snprintf((char *)lia->CNR.Flags, 30, "[NO FLAGS SET]");
        break;
      case 1:
        snprintf((char *)lia->CNR.Flags, 30, "ValidDevice");
        break;
      case 2:
        snprintf((char *)lia->CNR.Flags, 30, "ValidNetType");
        break;
      case 3:
        snprintf((char *)lia->CNR.Flags, 30, "ValidDevice | ValidNetType");
        break;
      default:
        snprintf((char *)lia->CNR.Flags, 30, "[INVALID VALUE]");
      }
      snprintf((char *)lia->CNR.NetNameOffset, 10, "%"PRIu32, li->CNR.NetNameOffset);
      snprintf((char *)lia->CNR.DeviceNameOffset, 10, "%"PRIu32, li->CNR.DeviceNameOffset);
      if (li->CNR.Flags & 0x00000002)
      {
        switch (li->CNR.NetworkProviderType)
        {
        case 0x001A0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_AVID");
          break;
        case 0x001B0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DOCUSPACE");
          break;
        case 0x001C0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_MANGOSOFT");
          break;
        case 0x001D0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_SERNET");
          break;
        case 0x001E0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_RIVERFRONT1");
          break;
        case 0x001F0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_RIVERFRONT2");
          break;
        case 0x00200000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DECORB");
          break;
        case 0x00210000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_PROTSTOR");
          break;
        case 0x00220000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_FJ_REDIR");
          break;
        case 0x00230000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DISTINCT");
          break;
        case 0x00240000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_TWINS");
          break;
        case 0x00250000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_RDR2SAMPLE");
          break;
        case 0x00260000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_CSC");
          break;
        case 0x00270000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_3IN1");
          break;
        case 0x00290000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_EXTENDNET");
          break;
        case 0x002A0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_STAC");
          break;
        case 0x002B0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_FOXBAT");
          break;
        case 0x002C0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_YAHOO");
          break;
        case 0x002D0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_EXIFS");
          break;
        case 0x002E0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DAV");
          break;
        case 0x002F0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_KNOWARE");
          break;
        case 0x00300000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_OBJECT_DIRE");
          break;
        case 0x00310000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_MASFAX");
          break;
        case 0x00320000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_HOB_NFS");
          break;
        case 0x00330000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_SHIVA");
          break;
        case 0x00340000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_IBMAL");
          break;
        case 0x00350000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_LOCK");
          break;
        case 0x00360000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_TERMSRV");
          break;
        case 0x00370000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_SRT");
          break;
        case 0x00380000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_QUINCY");
          break;
        case 0x00390000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_OPENAFS");
          break;
        case 0x003A0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_AVID1");
          break;
        case 0x003B0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DFS");
          break;
        case 0x003C0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_KWNP");
          break;
        case 0x003D0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_ZENWORKS");
          break;
        case 0x003E0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_DRIVEONWEB");
          break;
        case 0x003F0000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_VMWARE");
          break;
        case 0x00400000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_RSFX");
          break;
        case 0x00410000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_MFILES");
          break;
        case 0x00420000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_MS_NFS");
          break;
        case 0x00430000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "WNNC_NET_GOOGLE");
          break;
        case 0x00020000:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "[UNKNOWN (Possibly Local Server)]");
          break;
        default:
          snprintf((char *)lia->CNR.NetworkProviderType, 35, "0x%.8"PRIX32" [UNKNOWN TYPE]", li->CNR.NetworkProviderType);
        }
      }
      else //Not a valid Net Type
      {
        snprintf((char *)lia->CNR.NetworkProviderType, 35, "[N/A]");
      }
      if (li->CNR.NetNameOffset > 0x00000014)
      {
        snprintf((char *)lia->CNR.NetNameOffsetU, 10, "%"PRIu32, li->CNR.NetNameOffsetU);
        snprintf((char *)lia->CNR.DeviceNameOffsetU, 10, "%"PRIu32, li->CNR.DeviceNameOffsetU);
      }
      else //Unicode strings not used
      {
        sprintf((char *)lia->CNR.NetNameOffsetU, "[N/A]");
        sprintf((char *)lia->CNR.DeviceNameOffsetU, "[N/A]");
      }
      snprintf((char *)lia->CNR.NetName, 300, "%s", li->CNR.NetName);
      snprintf((char *)lia->CNR.DeviceName, 300, "%s", li->CNR.DeviceName);
      if (li->CNR.NetNameOffset > 0x00000014)
      {
        snprintf((char *)lia->CNR.NetNameU, 300, "%ls", li->CNR.NetNameU);
        snprintf((char *)lia->CNR.DeviceNameU, 300, "%ls", li->CNR.DeviceNameU);
      }
      else //Unicode strings not used
      {
        sprintf((char *)lia->CNR.NetNameU, "[NOT SET]");
        sprintf((char *)lia->CNR.DeviceNameU, "[NOT SET]");
      }
    }
    //No CNR
    else
    {
      sprintf((char *)lia->CNR.Size, "[N/A]");
      sprintf((char *)lia->CNR.Flags, "[N/A]");
      sprintf((char *)lia->CNR.NetNameOffset, "[N/A]");
      sprintf((char *)lia->CNR.DeviceNameOffset, "[N/A]");
      sprintf((char *)lia->CNR.NetworkProviderType, "[N/A]");
      sprintf((char *)lia->CNR.NetNameOffsetU, "[N/A]");
      sprintf((char *)lia->CNR.DeviceNameOffsetU, "[N/A]");
      sprintf((char *)lia->CNR.DeviceName, "[NOT SET]");
      sprintf((char *)lia->CNR.NetName, "[NOT SET]");
      sprintf((char *)lia->CNR.NetNameU, "[NOT SET]");
      sprintf((char *)lia->CNR.DeviceNameU, "[NOT SET]");
    }
    if (li->LBPOffsetU > 0) //There is a unicode local base path
    {
      snprintf((char *)lia->LBPU, 300, "%ls", li->LBPU);
    }
    else // There is no Unicode local base path
    {
      snprintf((char *)lia->LBPU, 300, "[NOT SET]");
    }
    if (li->CPSOffsetU > 0) //There is a unicode common path suffix
    {
      snprintf((char *)lia->CPSU, 100, "%ls", li->CPSU);
    }
    else // There is no Unicode common path suffix
    {
      snprintf((char *)lia->CPSU, 100, "[NOT SET]");
    }
  }
  else //If there is no LinkInfo
  {
    sprintf((char *)lia->Size, "[N/A]");
    sprintf((char *)lia->HeaderSize, "[N/A]");
    sprintf((char *)lia->Flags, "[N/A]");
    sprintf((char *)lia->IDOffset, "[N/A]");
    sprintf((char *)lia->LBPOffset, "[N/A]");
    sprintf((char *)lia->CNRLOffset, "[N/A]");
    sprintf((char *)lia->CPSOffset, "[N/A]");
    sprintf((char *)lia->LBPOffsetU, "[N/A]");
    sprintf((char *)lia->CPSOffsetU, "[N/A]");
    sprintf((char *)lia->VolID.Size, "[N/A]");
    sprintf((char *)lia->VolID.DriveType, "[N/A]");
    sprintf((char *)lia->VolID.DriveSN, "[N/A]");
    sprintf((char *)lia->VolID.VLOffset, "[N/A]");
    sprintf((char *)lia->VolID.VLOffsetU, "[N/A]");
    snprintf((char *)lia->VolID.VolumeLabel, 33, "[NOT SET]");
    snprintf((char *)lia->VolID.VolumeLabelU, 33, "[NOT SET]");
    snprintf((char *)lia->LBP, 300, "%s", li->LBP);
    sprintf((char *)lia->CNR.Size, "[N/A]");
    sprintf((char *)lia->CNR.Flags, "[N/A]");
    sprintf((char *)lia->CNR.NetNameOffset, "[N/A]");
    sprintf((char *)lia->CNR.DeviceNameOffset, "[N/A]");
    sprintf((char *)lia->CNR.NetworkProviderType, "[N/A]");
    sprintf((char *)lia->CNR.NetNameOffsetU, "[N/A]");
    sprintf((char *)lia->CNR.DeviceNameOffsetU, "[N/A]");
    sprintf((char *)lia->CNR.DeviceName, "[NOT SET]");
    sprintf((char *)lia->CNR.NetName, "[NOT SET]");
    sprintf((char *)lia->CNR.NetNameU, "[NOT SET]");
    sprintf((char *)lia->CNR.DeviceNameU, "[NOT SET]");
    snprintf((char *)lia->CPS, 100, "[NOT SET]");
    snprintf((char *)lia->LBPU, 300, "[NOT SET]");
    snprintf((char *)lia->CPSU, 100, "[NOT SET]");
  }

  return 0;
}
//
//Fills the LIF_STRINGDATA structure with the necessary data (converting
//Unicode strings to ASCII if necessary)
int get_stringdata(FILE * fp, int pos, struct LIF * lif)
{
  unsigned char      size_buf[2];   //A small buffer to hold the size element
  uint32_t           tsize = 0, str_size = 0;
  int                i, j;
  unsigned char      data_buf[600];
  wchar_t            uni_buf[300];

  // Initialise the lif->lsd values to 0
  for (i = 0; i < 5; i++)
  {
    lif->lsd.CountChars[i] = 0;
    lif->lsd.Data[i][0] = 0;
  }
  if (lif->lh.Flags & 0x00000080)  //Unicode
  {
    for (i = 0; i < 5; i++)
    {
      if (lif->lh.Flags & (0x00000004 << i))
      {
        fseek(fp, (pos + (tsize)), SEEK_SET);
        size_buf[0] = getc(fp);
        size_buf[1] = getc(fp);
        str_size = get_le_uint16(size_buf, 0);
        lif->lsd.CountChars[i] = str_size;
        if (str_size > 299)
        {
          str_size = 299;
        }
        for (j = 0; j < (str_size * 2); j++)
        {
          data_buf[j] = getc(fp);
        }
        data_buf[str_size * 2] = 0;
        data_buf[(str_size * 2) + 1] = 0;
        get_le_unistr(data_buf, 0, str_size + 1, uni_buf);
        snprintf((char *)lif->lsd.Data[i], 300, "%ls", uni_buf);

        tsize += ((lif->lsd.CountChars[i] * 2) + 2);
      }
    }
  }
  else //ASCII
  {
    for (i = 0; i < 5; i++)
    {
      if (lif->lh.Flags & (0x00000004 << i))
      {
        fseek(fp, (pos + (tsize)), SEEK_SET);
        size_buf[0] = getc(fp);
        size_buf[1] = getc(fp);
        str_size = get_le_uint16(size_buf, 0);
        lif->lsd.CountChars[i] = str_size;
        if (str_size > 299)
        {
          str_size = 299;
        }
        for (j = 0; j < lif->lsd.CountChars[i]; j++)
        {
          data_buf[j] = getc(fp);
        }
        get_chars(data_buf, 0, str_size + 1, lif->lsd.Data[i]);
        if (str_size == 299)
        {
          lif->lsd.Data[i][str_size] = 0;
        }
        tsize += (lif->lsd.CountChars[i] + 2);
      }
    }
  }

  lif->lsd.Size = tsize;
  return 0;
}
//
//Function get_stringdata_a(struct LIF_STRINGDATA *, struct LIF_STRINGDATA_A)
//copies the strings and creates an ASCII representation of the CountChars
//and Size values.
int get_stringdata_a(struct LIF_STRINGDATA * lsd, struct LIF_STRINGDATA_A * lsda)
{
  int i;

  snprintf((char *)lsda->Size, 10, "%"PRIu32, lsd->Size);
  for (i = 0; i < 5; i++)
  {
    snprintf((char *)lsda->CountChars[i], 10, "%"PRIu32, lsd->CountChars[i]);
    if (lsd->CountChars[i] > 0)
    {
      snprintf((char *)lsda->Data[i], 300, "%s", lsd->Data[i]);
    }
    else
    {
      snprintf((char *)lsda->Data[i], 300, "[EMPTY]");
    }
  }
  return 0;
}
//
//Fills the LIF_EXTRA_DATA structure with the necessary data (converting
//Unicode strings to ASCII if necessary)
int get_extradata(FILE * fp, int pos, struct LIF * lif)
{
  int                i = 0, j = 0, p = 0, vp = 0, posn = 0, offset = pos;
  uint32_t           blocksize, blocksig, datasize;
  unsigned char      size_buf[4];   //A small buffer to hold the size element
  unsigned char      sig_buf[4];
  unsigned char      data_buf[4096];

  led_setnull(&lif->led); //set all the extradata BlockSize and BlockSignature sections to 0 initially
  lif->led.edtypes = EMPTY;

  fseek(fp, pos, SEEK_SET);
  size_buf[0] = getc(fp);
  size_buf[1] = getc(fp);
  size_buf[2] = getc(fp);
  size_buf[3] = getc(fp);
  blocksize = get_le_uint32(size_buf, 0);
  while (blocksize > 3) //The spec is that anything less than 4 signifies a terminal block
  {
    if (blocksize >= 4096)    //Don't want to exceed the limits of the buffer 4KiB seems a reasonable limit (for now)
    {
      fprintf(stderr, "ExtraData block is too large\n");
      fprintf(stderr, "Processing of ExtraData block terminated.\n");
      return -1;
    }
    datasize = blocksize - 8;
    sig_buf[0] = getc(fp);
    sig_buf[1] = getc(fp);
    sig_buf[2] = getc(fp);
    sig_buf[3] = getc(fp);
    blocksig = get_le_uint32(sig_buf, 0);
    for (i = 0; i < datasize; i++)
    {
      // data_buf holds just the data for this ExtraData Block
      data_buf[i] = getc(fp);
    }
    switch (blocksig)
    {
    case 0xA0000001: // Signature for a EnvironmentVariableDataBlock S2.5.4
      lif->led.lep.Posn = (uint16_t)offset;
      assert(blocksize == 0x00000314); // Spec states this MUST be the value
      lif->led.lep.Size = blocksize;
      lif->led.lep.sig = blocksig;
      lif->led.edtypes += ENVIRONMENT_PROPS;
      get_chars(data_buf, 0, 260, lif->led.lep.TargetAnsi);
      if (get_le_unistr(data_buf, 260, 260, lif->led.lep.TargetUnicode) < 0)
      {
        lif->led.lep.TargetUnicode[0] = (wchar_t)0;
      }
      break;
    case 0xA0000002: // Signature for a ConsoleDataBlock S2.5.1
      lif->led.lcp.Posn = (uint16_t)offset;
      assert(blocksize == 0x000000CC); // Spec states this MUST be the value
      lif->led.lcp.Size = blocksize;
      lif->led.lcp.sig = blocksig;
      lif->led.edtypes += CONSOLE_PROPS;
      lif->led.lcp.FillAttributes = get_le_uint16(data_buf, 0);
      lif->led.lcp.PopupFillAttributes = get_le_uint16(data_buf, 2);
      lif->led.lcp.ScreenBufferSizeX = get_le_uint16(data_buf, 4);
      lif->led.lcp.ScreenBufferSizeY = get_le_uint16(data_buf, 6);
      lif->led.lcp.WindowSizeX = get_le_uint16(data_buf, 8);
      lif->led.lcp.WindowSizeY = get_le_uint16(data_buf, 10);
      lif->led.lcp.WindowOriginX = get_le_uint16(data_buf, 12);
      lif->led.lcp.WindowOriginY = get_le_uint16(data_buf, 14);
      lif->led.lcp.Unused1 = get_le_uint32(data_buf, 16);
      lif->led.lcp.Unused2 = get_le_uint32(data_buf, 20);
      lif->led.lcp.FontSize = get_le_uint32(data_buf, 24);
      lif->led.lcp.FontFamily = get_le_uint32(data_buf, 28);
      lif->led.lcp.FontWeight = get_le_uint32(data_buf, 32);
      if (get_le_unistr(data_buf, 36, 32, lif->led.lcp.FaceName) == 0)
      {
        lif->led.lcp.FaceName[0] = (wchar_t)0; //Null string if no characters read
      }
      lif->led.lcp.CursorSize = get_le_uint32(data_buf, 100);
      lif->led.lcp.FullScreen = get_le_uint32(data_buf, 104);
      lif->led.lcp.QuickEdit = get_le_uint32(data_buf, 108);
      lif->led.lcp.InsertMode = get_le_uint32(data_buf, 112);
      lif->led.lcp.AutoPosition = get_le_uint32(data_buf, 116);
      lif->led.lcp.HistoryBufferSize = get_le_uint32(data_buf, 120);
      lif->led.lcp.NumberOfHistoryBuffers = get_le_uint32(data_buf, 124);
      lif->led.lcp.HistoryNoDup = get_le_uint32(data_buf, 128);
      for (j = 0; j < 16; j++)
      {
        lif->led.lcp.ColorTable[j] = get_le_uint32(data_buf, (j * 4) + 132);
      }
      break;
    case 0xA0000003: //Signature for a TrackerDataBlock S2.5.10
      lif->led.ltp.Posn = (uint16_t)offset;
      lif->led.ltp.Size = blocksize;
      lif->led.ltp.sig = blocksig;
      lif->led.edtypes += TRACKER_PROPS;
      get_ltp(&lif->led.ltp, data_buf);
      break;
    case 0xA0000004: // Signature for a ConsoleFEDataBlock S2.5.2
      lif->led.lcfep.Posn = (uint16_t)offset;
      assert(blocksize == 0x0000000C); // Spec states this MUST be the value
      lif->led.lcfep.Size = blocksize;
      lif->led.lcfep.sig = blocksig;
      lif->led.edtypes += CONSOLE_FE_PROPS;
      lif->led.lcfep.CodePage = get_le_int32(data_buf, 0);
      break;
    case 0xA0000005: // Signature for a SpecialFolderDataBlock S2.5.9
      lif->led.lsfp.Posn = (uint16_t)offset;
      lif->led.lsfp.Size = blocksize;
      lif->led.lsfp.sig = blocksig;
      lif->led.edtypes += SPECIAL_FOLDER_PROPS;
      lif->led.lsfp.SpecialFolderID = get_le_uint32(data_buf, 0);
      lif->led.lsfp.Offset = get_le_uint32(data_buf, 4);
      break;
    case 0xA0000006: // Signature for a DarwinDataBlock S2.5.3
      lif->led.ldp.Posn = (uint16_t)offset;
      assert(blocksize == 0x00000314); // Spec states this MUST be the value
      lif->led.ldp.Size = blocksize;
      lif->led.ldp.sig = blocksig;
      lif->led.edtypes += DARWIN_PROPS;
      get_chars(data_buf, 0, 260, lif->led.ldp.DarwinDataAnsi);
      if(get_le_unistr(data_buf, 260, 260, lif->led.ldp.DarwinDataUnicode) < 0)
      {
        lif->led.ldp.DarwinDataUnicode[0] = (wchar_t)0;
      }
      break;
    case 0xA0000007: // Signature for a IconEnvironmentDataBlock S2.5.5
      lif->led.liep.Posn = (uint16_t)offset;
      assert(blocksize == 0x00000314); // Spec states this MUST be the value
      lif->led.liep.Size = blocksize;
      lif->led.liep.sig = blocksig;
      lif->led.edtypes += ICON_ENVIRONMENT_PROPS;
      get_chars(data_buf, 0, 260, lif->led.liep.TargetAnsi);
      if (get_le_unistr(data_buf, 260, 260, lif->led.liep.TargetUnicode) < 0)
      {
        lif->led.liep.TargetUnicode[0] = (wchar_t)0;
      }
      break;
    case 0xA0000008: // Signature for a ShimDataBlock S2.5.8
      lif->led.lsp.Posn = (uint16_t)offset;
      lif->led.lsp.Size = blocksize;
      lif->led.lsp.sig = blocksig;
      lif->led.edtypes += SHIM_PROPS;
      if (get_le_unistr(data_buf, 0, 600, lif->led.lsp.LayerName) < 0)
      {
        lif->led.lsp.LayerName[0] = (wchar_t)0;
      }
      break;
    case 0xA0000009: // Signature for a PropertyStoreDataBlock S2.5.7
      lif->led.lpsp.Posn = (uint16_t)offset;
      lif->led.lpsp.Size = blocksize;
      lif->led.lpsp.sig = blocksig;
      lif->led.edtypes += PROPERTY_STORE_PROPS;
      for (i = 0; i < PROPSTORES; i++) // Cycle through all the valid property stores
      {
        lif->led.lpsp.Stores[i].NumValues = 0;
        lif->led.lpsp.Stores[i].StorageSize = get_le_uint32(data_buf, posn);
        if (lif->led.lpsp.Stores[i].StorageSize == 0) // An empty property store
        {
          break;
        }
        p = posn + 4;
        lif->led.lpsp.Stores[i].Version = get_le_uint32(data_buf, p);
        p += 4;
        lif->led.lpsp.Stores[i].FormatID.Data1 = get_le_uint32(data_buf, p);
        p += 4;
        lif->led.lpsp.Stores[i].FormatID.Data2 = get_le_uint16(data_buf, p);
        p += 2;
        lif->led.lpsp.Stores[i].FormatID.Data3 = get_le_uint16(data_buf, p);
        p += 2;
        get_chars(data_buf, p, 2, lif->led.lpsp.Stores[i].FormatID.Data4hi);
        p += 2;
        get_chars(data_buf, p, 6, lif->led.lpsp.Stores[i].FormatID.Data4lo);
        p += 6;
        if((lif->led.lpsp.Stores[i].FormatID.Data1 == 0xD5CDD505) &&
          (lif->led.lpsp.Stores[i].FormatID.Data2 == 0x2E9C) &&
          (lif->led.lpsp.Stores[i].FormatID.Data3 == 0x101B) &&
          (lif->led.lpsp.Stores[i].FormatID.Data4hi[0] == 0x93) &&
          (lif->led.lpsp.Stores[i].FormatID.Data4hi[1] == 0x97) &&
          (lif->led.lpsp.Stores[i].FormatID.Data4hi[0] == 0x08) &&
          (lif->led.lpsp.Stores[i].FormatID.Data4hi[1] == 0x00) &&
          (lif->led.lpsp.Stores[i].FormatID.Data4hi[2] == 0x2B) &&
          (lif->led.lpsp.Stores[i].FormatID.Data4hi[3] == 0x2C) &&
          (lif->led.lpsp.Stores[i].FormatID.Data4hi[4] == 0xF9) &&
          (lif->led.lpsp.Stores[i].FormatID.Data4hi[5] == 0xAE)
          )
        {
          lif->led.lpsp.Stores[i].NameType = 0x00;
        }
        else
        {
          lif->led.lpsp.Stores[i].NameType = 0xFF;
        }
        for (j = 0; j < PROPVALUES; j++) // Cycle through all the valid property values
        {
          vp = p; // Save the position of the start of this value
          lif->led.lpsp.Stores[i].PropValues[j].ValueSize = get_le_uint32(data_buf, vp);
          p += (int)lif->led.lpsp.Stores[i].PropValues[j].ValueSize;// Move p to the next value store
          if (lif->led.lpsp.Stores[i].PropValues[j].ValueSize == 0)
          {
            lif->led.lpsp.Stores[i].NumValues++; // Unlike a Property Store, an empty Value Store is counted
            break;
          }
          lif->led.lpsp.Stores[i].PropValues[j].NameSizeOrID = get_le_uint32(data_buf, vp + 4);
          lif->led.lpsp.Stores[i].PropValues[j].Reserved = (uint8_t)data_buf[vp + 8];
          if (lif->led.lpsp.Stores[i].NameType == 0)
          {
            get_chars(data_buf, vp + 9, lif->led.lpsp.Stores[i].PropValues[j].NameSizeOrID, lif->led.lpsp.Stores[i].PropValues[j].Name);
            vp += lif->led.lpsp.Stores[i].PropValues[j].NameSizeOrID; // In the Case of a name type, offset the value pointer
          }
          lif->led.lpsp.Stores[i].PropValues[j].PropertyType = get_le_uint16(data_buf, vp + 9);
          lif->led.lpsp.Stores[i].PropValues[j].Padding = get_le_uint16(data_buf, vp + 11);
          get_chars(data_buf, vp + 13, (lif->led.lpsp.Stores[i].PropValues[j].ValueSize), lif->led.lpsp.Stores[i].PropValues[j].Value);
          lif->led.lpsp.Stores[i].NumValues++;
        }
        posn += lif->led.lpsp.Stores[i].StorageSize; // Move to the next propertystore
        lif->led.lpsp.NumStores++;
      } //Cycle through the Propstores
      break;
    case 0xA000000A: // Signature for a VistaAndAboveIDListDataBlock S2.5.11
      lif->led.lvidlp.Posn = (uint16_t)offset;
      lif->led.lvidlp.Size = blocksize;
      lif->led.lvidlp.sig = blocksig;
      lif->led.lvidlp.NumItemIDs = 0;
      lif->led.edtypes += VISTA_AND_ABOVE_IDLIST_PROPS;
      while (posn < (lif->led.lvidlp.Size - 8))
      {
        i = get_le_uint16(data_buf, posn);
        posn += i;
        lif->led.lvidlp.NumItemIDs++;
      }
      break;
    case 0xA000000B: // Signature for a KnownFolderDataBlock S2.5.6
      lif->led.lkfp.Posn = (uint16_t)offset;
      lif->led.lkfp.Size = blocksize;
      lif->led.lkfp.sig = blocksig;
      lif->led.edtypes += KNOWN_FOLDER_PROPS;
      lif->led.lkfp.KFGUID.Data1 = get_le_uint32(data_buf, 0);
      lif->led.lkfp.KFGUID.Data2 = get_le_uint16(data_buf, 4);
      lif->led.lkfp.KFGUID.Data3 = get_le_uint16(data_buf, 6);
      get_chars(data_buf, 8, 2, lif->led.lkfp.KFGUID.Data4hi);
      get_chars(data_buf, 10, 6, lif->led.lkfp.KFGUID.Data4lo);
      lif->led.lkfp.KFOffset = get_le_uint32(data_buf, 16);
      break;
    }
    offset += blocksize;

    size_buf[0] = getc(fp); //Get the next block size (or the terminal block)
    size_buf[1] = getc(fp);
    size_buf[2] = getc(fp);
    size_buf[3] = getc(fp);
    blocksize = get_le_uint32(size_buf, 0);
  }//End of the while loop that parses each ExtraData block

  lif->led.terminal = blocksize;

  //Compute the size of the ExtraData section
  lif->led.Size = lif->led.lcp.Size +
    lif->led.lcfep.Size +
    lif->led.ldp.Size +
    lif->led.lep.Size +
    lif->led.liep.Size +
    lif->led.lkfp.Size +
    lif->led.lpsp.Size +
    lif->led.lsp.Size +
    lif->led.lsfp.Size +
    lif->led.ltp.Size +
    lif->led.lvidlp.Size +
    4;
  return lif->led.Size;
}
//
//Function get_extradata_a(struct LIF_EXTRA_DATA*, struct LIF_EXTRA_DATA_A)
//copies the strings and creates an ASCII representation of the data.
int get_extradata_a(struct LIF_EXTRA_DATA * led, struct LIF_EXTRA_DATA_A * leda)
{
  int       i;

  snprintf((char *)leda->Size, 10, "%"PRIu32, led->Size);
  leda->edtypes[0] = (char)0;
  //Get Console Data block
  if (led->edtypes & CONSOLE_PROPS)
  {
    strcat((char *)leda->edtypes, "CONSOLE_PROPS | ");
    snprintf((char *)leda->lcpa.Posn, 8, "%"PRIu16, led->lcp.Posn);
    snprintf((char *)leda->lcpa.Size, 10, "%"PRIu32, led->lcp.Size);
    snprintf((char *)leda->lcpa.sig, 12, "0x%.8"PRIX32, led->lcp.sig);
    snprintf((char *)leda->lcpa.FillAttributes, 8, "0x%.4"PRIX16, led->lcp.FillAttributes);
    snprintf((char *)leda->lcpa.PopupFillAttributes, 8, "0x%.4"PRIX16, led->lcp.PopupFillAttributes);
    snprintf((char *)leda->lcpa.ScreenBufferSizeX, 8, "%"PRIu16, led->lcp.ScreenBufferSizeX);
    snprintf((char *)leda->lcpa.ScreenBufferSizeY, 8, "%"PRIu16, led->lcp.ScreenBufferSizeY);
    snprintf((char *)leda->lcpa.WindowSizeX, 8, "%"PRIu16, led->lcp.WindowSizeX);
    snprintf((char *)leda->lcpa.WindowSizeY, 8, "%"PRIu16, led->lcp.WindowSizeY);
    snprintf((char *)leda->lcpa.WindowOriginX, 8, "%"PRIu16, led->lcp.WindowOriginX);
    snprintf((char *)leda->lcpa.WindowOriginY, 8, "%"PRIu16, led->lcp.WindowOriginY);
    snprintf((char *)leda->lcpa.Unused1, 12, "0x%.8"PRIX32, led->lcp.Unused1);
    snprintf((char *)leda->lcpa.Unused2, 12, "0x%.8"PRIX32, led->lcp.Unused2);
    snprintf((char *)leda->lcpa.FontSize, 12, "%"PRIu32, led->lcp.FontSize);
    snprintf((char *)leda->lcpa.FontFamily, 12, "0x%.4"PRIX32, led->lcp.FontFamily);
    snprintf((char *)leda->lcpa.FontWeight, 12, "%"PRIu32, led->lcp.FontWeight);
    snprintf((char *)leda->lcpa.FaceName, 64, "%ls", led->lcp.FaceName);
    snprintf((char *)leda->lcpa.CursorSize, 12, "%"PRIu32, led->lcp.CursorSize);
    snprintf((char *)leda->lcpa.FullScreen, 12, "0x%.8"PRIX32, led->lcp.FullScreen);
    snprintf((char *)leda->lcpa.QuickEdit, 12, "0x%.8"PRIX32, led->lcp.QuickEdit);
    snprintf((char *)leda->lcpa.InsertMode, 12, "0x%.8"PRIX32, led->lcp.InsertMode);
    snprintf((char *)leda->lcpa.AutoPosition, 12, "0x%.8"PRIX32, led->lcp.AutoPosition);
    snprintf((char *)leda->lcpa.HistoryBufferSize, 12, "%"PRIu32, led->lcp.HistoryBufferSize);
    snprintf((char *)leda->lcpa.NumberOfHistoryBuffers, 12, "%"PRIu32, led->lcp.NumberOfHistoryBuffers);
    snprintf((char *)leda->lcpa.HistoryNoDup, 12, "0x%.8"PRIX32, led->lcp.HistoryNoDup);
    for (i = 0; i < 16; i++)
    {
      snprintf((char *)leda->lcpa.ColorTable[i], 12, "0x%.8"PRIX32, led->lcp.ColorTable[i]);
    }
  }
  else
  {
    snprintf((char *)leda->lcpa.Posn, 8, "[N/A]");
    snprintf((char *)leda->lcpa.Size, 10, "[N/A]");
    snprintf((char *)leda->lcpa.sig, 12, "[N/A]");
    snprintf((char *)leda->lcpa.FillAttributes, 8, "[N/A]");
    snprintf((char *)leda->lcpa.PopupFillAttributes, 8, "[N/A]");
    snprintf((char *)leda->lcpa.ScreenBufferSizeX, 8, "[N/A]");
    snprintf((char *)leda->lcpa.ScreenBufferSizeY, 8, "[N/A]");
    snprintf((char *)leda->lcpa.WindowSizeX, 8, "[N/A]");
    snprintf((char *)leda->lcpa.WindowSizeY, 8, "[N/A]");
    snprintf((char *)leda->lcpa.WindowOriginX, 8, "[N/A]");
    snprintf((char *)leda->lcpa.WindowOriginY, 8, "[N/A]");
    snprintf((char *)leda->lcpa.Unused1, 12, "[N/A]");
    snprintf((char *)leda->lcpa.Unused2, 12, "[N/A]");
    snprintf((char *)leda->lcpa.FontSize, 12, "[N/A]");
    snprintf((char *)leda->lcpa.FontFamily, 12, "[N/A]");
    snprintf((char *)leda->lcpa.FontWeight, 12, "[N/A]");
    snprintf((char *)leda->lcpa.FaceName, 64, "[N/A]");
    snprintf((char *)leda->lcpa.CursorSize, 12, "[N/A]");
    snprintf((char *)leda->lcpa.FullScreen, 12, "[N/A]");
    snprintf((char *)leda->lcpa.QuickEdit, 12, "[N/A]");
    snprintf((char *)leda->lcpa.InsertMode, 12, "[N/A]");
    snprintf((char *)leda->lcpa.AutoPosition, 12, "[N/A]");
    snprintf((char *)leda->lcpa.HistoryBufferSize, 12, "[N/A]");
    snprintf((char *)leda->lcpa.NumberOfHistoryBuffers, 12, "[N/A]");
    snprintf((char *)leda->lcpa.HistoryNoDup, 12, "[N/A]");
    for (i = 0; i < 16; i++)
    {
      snprintf((char *)leda->lcpa.ColorTable[i], 12, "[N/A]");
    }
  }
  //Get Console FE Data block
  if (led->edtypes & CONSOLE_FE_PROPS)
  {
    strcat((char *)leda->edtypes, "CONSOLE_FE_PROPS | ");
    snprintf((char *)leda->lcfepa.Posn, 8, "%"PRIu16, led->lcfep.Posn);
    snprintf((char *)leda->lcfepa.Size, 10, "%"PRIu32, led->lcfep.Size);
    snprintf((char *)leda->lcfepa.sig, 12, "0x%.8"PRIX32, led->lcfep.sig);
    snprintf((char *)leda->lcfepa.CodePage, 12, "0x%.8"PRIX32, led->lcfep.CodePage);
  }
  else
  {
    snprintf((char *)leda->lcfepa.Posn, 8, "[N/A]");
    snprintf((char *)leda->lcfepa.Size, 10, "[N/A]");
    snprintf((char *)leda->lcfepa.sig, 12, "[N/A]");
    snprintf((char *)leda->lcfepa.CodePage, 12, "[N/A]");
  }
  //Get Darwin Data block
  if (led->edtypes & DARWIN_PROPS)
  {
    strcat((char *)leda->edtypes, "DARWIN_PROPS | ");
    snprintf((char *)leda->ldpa.Posn, 8, "%"PRIu16, led->ldp.Posn);
    snprintf((char *)leda->ldpa.Size, 10, "%"PRIu32, led->ldp.Size);
    snprintf((char *)leda->ldpa.sig, 12, "0x%.8"PRIX32, led->ldp.sig);
    snprintf((char *)leda->ldpa.DarwinDataAnsi, 260, "%s", led->ldp.DarwinDataAnsi);
    snprintf((char *)leda->ldpa.DarwinDataUnicode, 520, "%ls", led->ldp.DarwinDataUnicode);
  }
  else
  {
    snprintf((char *)leda->ldpa.Posn, 8, "[N/A]");
    snprintf((char *)leda->ldpa.Size, 10, "[N/A]");
    snprintf((char *)leda->ldpa.sig, 12, "[N/A]");
    snprintf((char *)leda->ldpa.DarwinDataAnsi, 260, "[N/A]");
    snprintf((char *)leda->ldpa.DarwinDataUnicode, 520, "[N/A]");
  }
  //Get Environment Variable Data block
  if (led->edtypes & ENVIRONMENT_PROPS)
  {
    strcat((char *)leda->edtypes, "ENVIRONMENT_PROPS | ");
    snprintf((char *)leda->lepa.Posn, 8, "%"PRIu16, led->lep.Posn);
    snprintf((char *)leda->lepa.Size, 10, "%"PRIu32, led->lep.Size);
    snprintf((char *)leda->lepa.sig, 12, "0x%.8"PRIX32, led->lep.sig);
    snprintf((char *)leda->lepa.TargetAnsi, 260, "%s", led->lep.TargetAnsi);
    snprintf((char *)leda->lepa.TargetUnicode, 520, "%ls", led->lep.TargetUnicode);
  }
  else
  {
    snprintf((char *)leda->lepa.Posn, 8, "[N/A]");
    snprintf((char *)leda->lepa.Size, 10, "[N/A]");
    snprintf((char *)leda->lepa.sig, 12, "[N/A]");
    snprintf((char *)leda->lepa.TargetAnsi, 260, "[N/A]");
    snprintf((char *)leda->lepa.TargetUnicode, 520, "[N/A]");
  }
  //Get Icon Environment Data block
  if (led->edtypes & ICON_ENVIRONMENT_PROPS)
  {
    strcat((char *)leda->edtypes, "ICON_ENVIRONMENT_PROPS | ");
    snprintf((char *)leda->liepa.Posn, 8, "%"PRIu16, led->liep.Posn);
    snprintf((char *)leda->liepa.Size, 10, "%"PRIu32, led->liep.Size);
    snprintf((char *)leda->liepa.sig, 12, "0x%.8"PRIX32, led->liep.sig);
    snprintf((char *)leda->liepa.TargetAnsi, 260, "%s", led->liep.TargetAnsi);
    snprintf((char *)leda->liepa.TargetUnicode, 520, "%ls", led->liep.TargetUnicode);
  }
  else
  {
    snprintf((char *)leda->liepa.Posn, 8, "[N/A]");
    snprintf((char *)leda->liepa.Size, 10, "[N/A]");
    snprintf((char *)leda->liepa.sig, 12, "[N/A]");
    snprintf((char *)leda->liepa.TargetAnsi, 260, "[N/A]");
    snprintf((char *)leda->liepa.TargetUnicode, 520, "[N/A]");
  }
  //Get Known Folder data block
  if (led->edtypes & KNOWN_FOLDER_PROPS)
  {
    strcat((char *)leda->edtypes, "KNOWN_FOLDER_PROPS | ");
    snprintf((char *)leda->lkfpa.Posn, 8, "%"PRIu16, led->lkfp.Posn);
    snprintf((char *)leda->lkfpa.Size, 10, "%"PRIu32, led->lkfp.Size);
    snprintf((char *)leda->lkfpa.sig, 12, "0x%.8"PRIX32, led->lkfp.sig);
    get_droid_a(&led->lkfp.KFGUID, &leda->lkfpa.KFGUID);
    snprintf((char *)leda->lkfpa.KFOffset, 10, "%"PRIu32, led->lkfp.KFOffset);
  }
  else
  {
    snprintf((char *)leda->lkfpa.Posn, 8, "[N/A]");
    snprintf((char *)leda->lkfpa.Size, 10, "[N/A]");
    snprintf((char *)leda->lkfpa.sig, 10, "[N/A]");
    snprintf((char *)leda->lkfpa.KFGUID.UUID, 40, "[N/A]");
    snprintf((char *)leda->lkfpa.KFGUID.Version, 40, "[N/A]");
    snprintf((char *)leda->lkfpa.KFGUID.Variant, 40, "[N/A]");
    snprintf((char *)leda->lkfpa.KFGUID.Time, 30, "[N/A]");
    snprintf((char *)leda->lkfpa.KFGUID.Time_long, 40, "[N/A]");
    snprintf((char *)leda->lkfpa.KFGUID.ClockSeq, 10, "[N/A]");
    snprintf((char *)leda->lkfpa.KFOffset, 10, "[N/A]");
  }
  //Get Property Store data block
  if (led->edtypes & PROPERTY_STORE_PROPS)
  {
    strcat((char *)leda->edtypes, "PROPERTY_STORE_PROPS | ");
    snprintf((char *)leda->lpspa.Posn, 8, "%"PRIu16, led->lpsp.Posn);
    snprintf((char *)leda->lpspa.Size, 10, "%"PRIu32, led->lpsp.Size);
    snprintf((char *)leda->lpspa.sig, 12, "0x%.8"PRIX32, led->lpsp.sig);
    snprintf((char *)leda->lpspa.NumStores, 10, "%"PRIi32, led->lpsp.NumStores);
    for (i = 0; i < led->lpsp.NumStores; i++)
    {
      get_propstore_a(&led->lpsp.Stores[i], &leda->lpspa.Stores[i]);
    }
  }
  else
  {
    snprintf((char *)leda->lpspa.Posn, 8, "[N/A]");
    snprintf((char *)leda->lpspa.Size, 10, "[N/A]");
    snprintf((char *)leda->lpspa.sig, 10, "[N/A]");
    snprintf((char *)leda->lpspa.NumStores, 10, "[N/A]");
  }
  //Get Shim Data block
  if (led->edtypes & SHIM_PROPS)
  {
    strcat((char *)leda->edtypes, "SHIM_PROPS | ");
    snprintf((char *)leda->lspa.Posn, 8, "%"PRIu16, led->lsp.Posn);
    snprintf((char *)leda->lspa.Size, 10, "%"PRIu32, led->lsp.Size);
    snprintf((char *)leda->lspa.sig, 12, "0x%.8"PRIX32, led->lsp.sig);
    snprintf((char *)leda->lspa.LayerName, 600, "%ls", led->lsp.LayerName);
  }
  else
  {
    snprintf((char *)leda->lspa.Posn, 8, "[N/A]");
    snprintf((char *)leda->lspa.Size, 10, "[N/A]");
    snprintf((char *)leda->lspa.sig, 10, "[N/A]");
    snprintf((char *)leda->lspa.LayerName, 600, "[N/A]");
  }
  //Get Special Folder Data block
  if (led->edtypes & SPECIAL_FOLDER_PROPS)
  {
    strcat((char *)leda->edtypes, "SPECIAL_FOLDER_PROPS | ");
    snprintf((char *)leda->lsfpa.Posn, 8, "%"PRIu16, led->lsfp.Posn);
    snprintf((char *)leda->lsfpa.Size, 10, "%"PRIu32, led->lsfp.Size);
    snprintf((char *)leda->lsfpa.sig, 12, "0x%.8"PRIX32, led->lsfp.sig);
    snprintf((char *)leda->lsfpa.SpecialFolderID, 10, "%"PRIu32, led->lsfp.SpecialFolderID);
    snprintf((char *)leda->lsfpa.Offset, 10, "%"PRIu32, led->lsfp.Offset);
  }
  else
  {
    snprintf((char *)leda->lsfpa.Posn, 8, "[N/A]");
    snprintf((char *)leda->lsfpa.Size, 10, "[N/A]");
    snprintf((char *)leda->lsfpa.sig, 10, "[N/A]");
    snprintf((char *)leda->lsfpa.SpecialFolderID, 10, "[N/A]");
    snprintf((char *)leda->lsfpa.Offset, 10, "[N/A]");
  }
  //Get the Link File Tracker Properties
  if (led->edtypes & TRACKER_PROPS)
  {
    strcat((char *)leda->edtypes, "TRACKER_PROPS | ");
    snprintf((char *)leda->ltpa.Posn, 8, "%"PRIu16, led->ltp.Posn);
    snprintf((char *)leda->ltpa.Size, 10, "%"PRIu32, led->ltp.Size);
    snprintf((char *)leda->ltpa.sig, 12, "0x%.8"PRIX32, led->ltp.sig);
    snprintf((char *)leda->ltpa.Length, 10, "%"PRIu32, led->ltp.Length);
    snprintf((char *)leda->ltpa.Version, 10, "%"PRIu32, led->ltp.Version);
    snprintf((char *)leda->ltpa.MachineID, 17, "%s", led->ltp.MachineID);
    get_droid_a(&led->ltp.Droid1, &leda->ltpa.Droid1);
    get_droid_a(&led->ltp.Droid2, &leda->ltpa.Droid2);
    get_droid_a(&led->ltp.DroidBirth1, &leda->ltpa.DroidBirth1);
    get_droid_a(&led->ltp.DroidBirth2, &leda->ltpa.DroidBirth2);
  }
  else
  {
    snprintf((char *)leda->ltpa.Posn, 8, "[N/A]");
    snprintf((char *)leda->ltpa.Size, 10, "[N/A]");
    snprintf((char *)leda->ltpa.sig, 10, "[N/A]");
    snprintf((char *)leda->ltpa.Length, 10, "[N/A]");
    snprintf((char *)leda->ltpa.Version, 10, "[N/A]");
    snprintf((char *)leda->ltpa.MachineID, 17, "[N/A]");
    snprintf((char *)leda->ltpa.Droid1.UUID, 40, "[N/A]");
    snprintf((char *)leda->ltpa.Droid1.Version, 40, "[N/A]");
    snprintf((char *)leda->ltpa.Droid1.Variant, 40, "[N/A]");
    snprintf((char *)leda->ltpa.Droid1.Time, 30, "[N/A]");
    snprintf((char *)leda->ltpa.Droid1.Time_long, 40, "[N/A]");
    snprintf((char *)leda->ltpa.Droid1.ClockSeq, 10, "[N/A]");
    snprintf((char *)leda->ltpa.Droid1.Node, 20, "[N/A]");
    snprintf((char *)leda->ltpa.Droid2.UUID, 40, "[N/A]");
    snprintf((char *)leda->ltpa.Droid2.Version, 40, "[N/A]");
    snprintf((char *)leda->ltpa.Droid2.Variant, 40, "[N/A]");
    snprintf((char *)leda->ltpa.Droid2.Time, 30, "[N/A]");
    snprintf((char *)leda->ltpa.Droid2.Time_long, 40, "[N/A]");
    snprintf((char *)leda->ltpa.Droid2.ClockSeq, 10, "[N/A]");
    snprintf((char *)leda->ltpa.Droid2.Node, 20, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth1.UUID, 40, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth1.Version, 40, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth1.Variant, 40, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth1.Time, 30, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth1.Time_long, 40, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth1.ClockSeq, 10, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth1.Node, 20, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth2.UUID, 40, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth2.Version, 40, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth2.Variant, 40, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth2.Time, 30, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth2.Time_long, 40, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth2.ClockSeq, 10, "[N/A]");
    snprintf((char *)leda->ltpa.DroidBirth2.Node, 20, "[N/A]");
  }
  //Get Vista and above ID List
  if (led->edtypes & VISTA_AND_ABOVE_IDLIST_PROPS)
  {
    strcat((char *)leda->edtypes, "VISTA_AND_ABOVE_IDLIST_PROPS | ");
    snprintf((char *)leda->lvidlpa.Posn, 8, "%"PRIu16, led->lvidlp.Posn);
    snprintf((char *)leda->lvidlpa.Size, 10, "%"PRIu32, led->lvidlp.Size);
    snprintf((char *)leda->lvidlpa.sig, 12, "0x%.8"PRIX32, led->lvidlp.sig);
    snprintf((char *)leda->lvidlpa.NumItemIDs, 10, "%"PRIu32, led->lvidlp.NumItemIDs);
  }
  else
  {
    snprintf((char *)leda->lvidlpa.Posn, 8, "[N/A]");
    snprintf((char *)leda->lvidlpa.Size, 10, "[N/A]");
    snprintf((char *)leda->lvidlpa.sig, 12, "[N/A]");
    snprintf((char *)leda->lvidlpa.NumItemIDs, 10, "[N/A]");
  }
  //Trim the edtypes string (if necessary)
  i = strlen((char *)leda->edtypes);
  if (i > 2)
  {
    leda->edtypes[i - 3] = (unsigned char)0;
  }
  else
  {
    snprintf((char *)leda->edtypes, 300, "No EXTRADATA structures");
  }

  //Finaly the terminal block
  snprintf((char *)leda->terminal, 15, "0x.8%"PRIX32, led->terminal);

  return 0;
}
//
//Function get_attr_a(char *attr_str, struct LIF_HDR *lh) converts the
//attributes in the LIF header to a readable string
void get_flag_a(unsigned char *flag_str, struct LIF_HDR *lh)
{
  sprintf((char *)flag_str, " ");

  //check for the states that are constant
  if (lh->Attr == 0) //No attributes set
  {
    sprintf((char *)flag_str, "NONE");
    return;
  }
  if (lh->Flags & 0x1)
    strcat((char *)flag_str, "HasLinkTargetIDList | ");
  if (lh->Flags & 0x2)
    strcat((char *)flag_str, "HasLinkInfo | ");
  if (lh->Flags & 0x4)
    strcat((char *)flag_str, "HasName | ");
  if (lh->Flags & 0x8)
    strcat((char *)flag_str, "HasRelativePath | ");
  if (lh->Flags & 0x10)
    strcat((char *)flag_str, "HasWorkingDir | ");
  if (lh->Flags & 0x20)
    strcat((char *)flag_str, "HasArguments | ");
  if (lh->Flags & 0x40)
    strcat((char *)flag_str, "HasIconLocation | ");
  if (lh->Flags & 0x80)
    strcat((char *)flag_str, "IsUnicode | ");
  if (lh->Flags & 0x100)
    strcat((char *)flag_str, "ForceNoLinkInfo | ");
  if (lh->Flags & 0x200)
    strcat((char *)flag_str, "HasExpString | ");
  if (lh->Flags & 0x400)
    strcat((char *)flag_str, "RunInSeparateProcess | ");
  if (lh->Flags & 0x800)
    strcat((char *)flag_str, "Unused1 | ");
  if (lh->Flags & 0x1000)
    strcat((char *)flag_str, "HasDarwinID | ");
  if (lh->Flags & 0x2000)
    strcat((char *)flag_str, "RunAsUser | ");
  if (lh->Flags & 0x4000)
    strcat((char *)flag_str, "HasExpIcon | ");
  if (lh->Flags & 0x8000)
    strcat((char *)flag_str, "NoPidlAlias | ");
  if (lh->Flags & 0x10000)
    strcat((char *)flag_str, "Unused2 | ");
  if (lh->Flags & 0x20000)
    strcat((char *)flag_str, "RunWithShimLayer | ");
  if (lh->Flags & 0x40000)
    strcat((char *)flag_str, "ForceNoLinkTrack | ");
  if (lh->Flags & 0x80000)
    strcat((char *)flag_str, "EnableTargetMetadata | ");
  if (lh->Flags & 0x100000)
    strcat((char *)flag_str, "DisableLinkPathTracking | ");
  if (lh->Flags & 0x200000)
    strcat((char *)flag_str, "DisableKnownFolderTracking | ");
  if (lh->Flags & 0x400000)
    strcat((char *)flag_str, "DisableKnownFolderAlias | ");
  if (lh->Flags & 0x800000)
    strcat((char *)flag_str, "AllowLinkToLink | ");
  if (lh->Flags & 0x1000000)
    strcat((char *)flag_str, "UnaliasOnSave | ");
  if (lh->Flags & 0x2000000)
    strcat((char *)flag_str, "PreferEnvironmentPath | ");
  if (lh->Flags & 0x4000000)
    strcat((char *)flag_str, "KeepLocalIDListForUNCTarget | ");

  if (strlen((char *)flag_str) > 0)
    flag_str[strlen((char *)flag_str) - 3] = (unsigned char)0;
  return;
}
//
//Function get_attr_a(char *attr_str, struct LIF_HDR *lh) converts the
//attributes in the LIF header to a readable string
void get_attr_a(unsigned char *attr_str, struct LIF_HDR *lh)
{
  sprintf((char *)attr_str, " ");

  //check for the states that are constant
  if (lh->Attr == 0) //No attributes set
  {
    sprintf((char *)attr_str, "NONE");
    return;
  }
  if (lh->Attr == 0x80) //'NORMAL attribute set - no others allowed
  {
    sprintf((char *)attr_str, "FILE_ATTRIBUTE_NORMAL");
    return;
  }
  if (lh->Attr & 0x1)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_READONLY | ");
  if (lh->Attr & 0x2)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_HIDDEN | ");
  if (lh->Attr & 0x4)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_SYSTEM | ");
  if (lh->Attr & 0x10)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_DIRECTORY | ");
  if (lh->Attr & 0x20)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_ARCHIVE | ");
  if (lh->Attr & 0x40)
    // There is something wrong with the link file if we are here
    // According to MS-SHLLINK S.2.1.2 G
    strcat((char *)attr_str, "Reserved2 | ");
  // FILE_ATTRIBUTE_NORMAL (0x80) is dealt with above
  if (lh->Attr & 0x100)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_TEMPORARY | ");
  if (lh->Attr & 0x200)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_SPARSE_FILE | ");
  if (lh->Attr & 0x400)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_REPARSE_POINT | ");
  if (lh->Attr & 0x800)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_COMPRESSED | ");
  if (lh->Attr & 0x1000)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_OFFLINE | ");
  if (lh->Attr & 0x2000)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | ");
  if (lh->Attr & 0x4000)
    strcat((char *)attr_str, "FILE_ATTRIBUTE_ENCRYPTED | ");
  if (strlen((char *)attr_str) > 3)
  {
    // Remove the last space,pipe,space combination
    attr_str[strlen((char *)attr_str) - 3] = (unsigned char)0;
  }
  else
  {
    //The only way to get here is to have an unrecognised file attribute
    sprintf((char *)attr_str, "[UNKNOWN FILE ATTRIBUTE]");
  }
  return;
}
//
//Function get_le_ulong_int(unsigned char *, int pos) reads 4 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the unsigned long integer
//The definition of unsigned long is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
uint32_t get_le_uint32(unsigned char buf[], int pos)
{
  int i;
  uint32_t result = 0;

  for (i = 0; i < 4; i++)
  {
    result += (buf[(i + pos)] << (8 * i));
  }
  return result;
}
//
//Function get_le_ulonglong_int(unsigned char *, int pos) reads 8 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the unsigned long integer
//The definition of unsigned long is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
uint64_t get_le_uint64(unsigned char buf[], int pos)
{
  uint64_t result = 0;
  uint32_t lo = 0, hi = 0; //Have to split the 64 bits in two
  //because '<< 8*i' breaks on my 64 bit machine
  lo = get_le_uint32(buf, pos);
  hi = get_le_uint32(buf, (pos + 4));
  result = ((uint64_t)hi << 32) + lo;
  return result;
}
//
//Function get_le_ulonglong_int(unsigned char *, int pos) reads 8 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the unsigned long integer
//The definition of unsigned long is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
int64_t get_le_int64(unsigned char buf[], int pos)
{
  int64_t result = 0;
  uint64_t interim = 0;
  uint32_t lo = 0, hi = 0; //Have to split the 64 bits in two
  //because '<< 8*i' breaks on my 64 bit machine
  lo = get_le_uint32(buf, pos);
  hi = get_le_uint32(buf, (pos + 4));
  interim = ((uint64_t)hi << 32) + lo;
  if (interim > 0x7FFFFFFFFFFFFFFFLL)
  {
    result = (int64_t)(interim - 0x8000000000000000LL);
  }
  else
  {
    result = (int64_t)interim;
  }

  return result;
}
//
//Function get_le_slong_int(unsigned char *, int pos) reads 4 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the signed long integer
//The definition of signed long is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
int32_t get_le_int32(unsigned char buf[], int pos)
{
  int i;
  int32_t stor = 0;

  for (i = 0; i < 4; i++)
  {
    stor += (buf[(i + pos)] << (8 * i));
  }
  return stor;
}
//
//Function get_le_u_int(unsigned char *, int pos) reads 2 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the unsigned integer
//The definition of unsigned int is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
uint16_t get_le_uint16(unsigned char buf[], int pos)
{
  int i;
  uint16_t result = 0;

  for (i = 0; i < 2; i++)
  {
    result += (buf[(i + pos)] << (8 * i));
  }
  return result;
}
//
//Function get_le_int16(unsigned char *, int pos) reads 2 unsigned
//characters starting at pos. It will interpret these as little endian and
//return the signed short integer
//The definition of signed short is the one from the Microsoft (TM) open
//document 'MS_SHLLINK'
int16_t get_le_int16(unsigned char buf[], int pos)
{
  int i;
  int32_t stor = 0;

  for (i = 0; i < 2; i++)
  {
    stor += (buf[(i + pos)] << (8 * i));
  }
  return stor;
//
//Function get_filetime_ashort(struct FILETIME ft) returns the character string
//representation of the Filetime passed in ft. The output is as per the
//ISO 8601 specification (i.e. 'yyyy-mm-dd hh:mm:ss')
}
//
//Function get_filetime_a_short(struct FILETIME ft) returns the character string
//representation of the Filetime passed in ft. The output is as per the
//ISO 8601 specification (i.e. 'yyyy-mm-dd hh:mm:ss')
void get_filetime_a_short(int64_t ft, unsigned char result[])
{
  struct tm* tms;
  time_t time;
  int64_t epoch_diff = 11644473600LL, cns2sec = 10000000L;

  ft = ft / cns2sec; //Reduce to seconds
  ft = ft - epoch_diff; //Number of seconds between epoch dates

  if ((sizeof(time_t) == sizeof(int64_t)) && ((ft > 0) && (ft < 0x7FFFFFFFL)))
  {
    time = (time_t)ft;
    tms = gmtime(&time);
    strftime((char *)result, 29, "%Y-%m-%d %H:%M:%S (UTC)", tms);
  }
  //Can't cope with large time_t values
  else if (ft == -11644473600LL)
  {
    snprintf((char *)result, 30, "Date not set (i.e. 0 value)");
  }
  else
  {
    snprintf((char *)result, 30, "Could not convert");
  }
}
//
//Function get_filetime_a_long(struct FILETIME ft) returns the character string
//representation of the Filetime passed in ft. The output is as per the
//ISO 8601 specification (i.e. 'yyyy-mm-dd hh:mm:ss.sssssss')
void get_filetime_a_long(int64_t ft, unsigned char result[])
{
  struct tm* tms;
  time_t time;
  uint64_t cns; //100 nanosecond component
  int64_t epoch_diff = 11644473600LL, cns2sec = 10000000;
  unsigned char interim[30];

  cns = (uint64_t)ft%cns2sec; //Extract the 100 nanosecond component
  ft = ft / cns2sec; //Reduce to seconds
  ft = ft - epoch_diff; //Number of seconds between epoch dates
  if ((sizeof(time_t) == sizeof(int64_t)) && ((ft > 0) && (ft < 0x7FFFFFFFL)))
  {
    time = (time_t)ft;
    tms = gmtime(&time);
    strftime((char *)interim, 29, "%Y-%m-%d %H:%M:%S", tms);
    snprintf((char *)result, 40, "%s.%"PRIu64" (UTC)", interim, cns);
  }
  //Can't cope with large time_t values
  else if (ft == -11644473600LL)
  {
    snprintf((char *)result, 40, "Date not set (i.e. 0 value)");
  }
  else
  {
    snprintf((char *)result, 40, "Could not convert");
  }
}
//
//Function get_chars(unsigned char buf[], int pos ,int num, unsigned char targ[])
// reads num unsigned characters starting at pos in buf. It will interpret these
// as big endian (straight copy) and place them in targ
void get_chars(unsigned char buf[], int pos, int num, unsigned char targ[])
{
  int i;

  for (i = 0; i < num; i++)
  {
    targ[i] = buf[(i + pos)];
  }
}
//
//Function get_le_unistr(unsigned char buf[], int pos, int max, wchar_t targ[])
//Fetches a unicode string from buf starting at position pos. It quits when a
//(wchar_t) 0 is encountered or max (in whchar_t terms) characters are copied.
//The encoding is considered to be the Windows default (little endian)
//The result is placed in targ. The function returns the number of wchar_t
//characters that have been copied or -1 on failure.
int get_le_unistr(unsigned char buf[], int pos, int max, wchar_t targ[])
{
  int i, n = 0;
  uint16_t widechar;
  unsigned char temp_buf[2];

  for (i = 0; i < (max - 1); i++)
  {
    temp_buf[0] = buf[((i * 2) + pos)];
    temp_buf[1] = buf[((i * 2) + 1 + pos)];
    widechar = get_le_uint16(temp_buf, 0);
    targ[i] = (wchar_t)widechar;
    n++;
    if (widechar == 0x0000)
    {
      n--;
      break;
    }
  }
  targ[max - 1] = (wchar_t)0;
  return n;
}
//
//Function void get_ltp(struct LIF_TRACKER_PROPS *, unsigned char[]);
//fills the LIF_TRACKER_PROPS properties with the relevant data from the
//character buffer
void get_ltp(struct LIF_TRACKER_PROPS * ltp, unsigned char * data_buf)
{
  int pos = 0;

  //Size and sig should be set
  ltp->Length = get_le_uint32(data_buf, pos);
  pos += 4;
  ltp->Version = get_le_uint32(data_buf, pos);
  pos += 4;
  get_chars(data_buf, pos, 16, ltp->MachineID);
  pos += 16;
  ltp->MachineID[15] = 0; //Make sure that we can read it as a string
  //Get Droid1
  ltp->Droid1.Data1 = get_le_uint32(data_buf, pos);
  pos += 4;
  ltp->Droid1.Data2 = get_le_uint16(data_buf, pos);
  pos += 2;
  ltp->Droid1.Data3 = get_le_uint16(data_buf, pos);
  pos += 2;
  get_chars(data_buf, pos, 2, ltp->Droid1.Data4hi);
  pos += 2;
  get_chars(data_buf, pos, 6, ltp->Droid1.Data4lo);
  pos += 6;
  //Get Droid2
  ltp->Droid2.Data1 = get_le_uint32(data_buf, pos);
  pos += 4;
  ltp->Droid2.Data2 = get_le_uint16(data_buf, pos);
  pos += 2;
  ltp->Droid2.Data3 = get_le_uint16(data_buf, pos);
  pos += 2;
  get_chars(data_buf, pos, 2, ltp->Droid2.Data4hi);
  pos += 2;
  get_chars(data_buf, pos, 6, ltp->Droid2.Data4lo);
  pos += 6;
  //Get DroidBirth1
  ltp->DroidBirth1.Data1 = get_le_uint32(data_buf, pos);
  pos += 4;
  ltp->DroidBirth1.Data2 = get_le_uint16(data_buf, pos);
  pos += 2;
  ltp->DroidBirth1.Data3 = get_le_uint16(data_buf, pos);
  pos += 2;
  get_chars(data_buf, pos, 2, ltp->DroidBirth1.Data4hi);
  pos += 2;
  get_chars(data_buf, pos, 6, ltp->DroidBirth1.Data4lo);
  pos += 6;
  //Get DroidBirth2
  ltp->DroidBirth2.Data1 = get_le_uint32(data_buf, pos);
  pos += 4;
  ltp->DroidBirth2.Data2 = get_le_uint16(data_buf, pos);
  pos += 2;
  ltp->DroidBirth2.Data3 = get_le_uint16(data_buf, pos);
  pos += 2;
  get_chars(data_buf, pos, 2, ltp->DroidBirth2.Data4hi);
  pos += 2;
  get_chars(data_buf, pos, 6, ltp->DroidBirth2.Data4lo);
  pos += 6;
}
//
//Function get_Droid_a(struct LIF_CLSID *, char *)
//Converts the Droid data in the Tracker properties to ASCII versions
void get_droid_a(struct LIF_CLSID * droid, struct LIF_CLSID_A * droid_a)
{
  uint8_t  Version, Variant;
  int16_t Timehi, ClockSeq;
  int64_t Time;
  // Build the UUID string
  snprintf((char *)droid_a->UUID, 40, "{%.8"PRIX32"-%.4"PRIX16"-%.4"PRIX16"-%.2"PRIX8"%.2"PRIX8"-%.2"PRIX8"%.2"PRIX8"%.2"PRIX8"%.2"PRIX8"%.2"PRIX8"%.2"PRIX8"}",
    droid->Data1,
    droid->Data2,
    droid->Data3,
    droid->Data4hi[0],
    droid->Data4hi[1],
    droid->Data4lo[0],
    droid->Data4lo[1],
    droid->Data4lo[2],
    droid->Data4lo[3],
    droid->Data4lo[4],
    droid->Data4lo[5]);

  // Work out the Version Number
  Version = (uint8_t)((droid->Data3 & 0xF000) >> 12);
  switch (Version)
  {
  case 1:
    snprintf((char *)droid_a->Version, 40, "1 - ITU time based");
    break;
  case 2:
    snprintf((char *)droid_a->Version, 40, "2 - DCE security version");
    break;
  case 3:
    snprintf((char *)droid_a->Version, 40, "3 - ITU name based MD5");
    break;
  case 4:
    snprintf((char *)droid_a->Version, 40, "4 - ITU random number");
    break;
  case 5:
    snprintf((char *)droid_a->Version, 40, "5 - ITU name based SHA1");
    break;
  default:
    snprintf((char *)droid_a->Version, 40, "%"PRIu8" - Unknown version", Version);
  }

  // Work out the Variant
  Variant = (uint8_t)((droid->Data4hi[0] & 0xC0) >> 6);
  switch (Variant)
  {
  case 0:
  case 1:
    snprintf((char *)droid_a->Variant, 40, "NCS backward compatible");
    break;
  case 2:
    snprintf((char *)droid_a->Variant, 40, "ITU variant");
    break;
  case 3:
    snprintf((char *)droid_a->Variant, 40, "Microsoft variant");
    break;
  default:
    snprintf((char *)droid_a->Variant, 40, "Unknown variant"); // Shouldn't happen
  }


  //If it's a time based version
  if (Version == 1)
  {
    //Work out the Clock Sequence
    ClockSeq = ((uint16_t)((droid->Data4hi[0] & 0x3F << 8)))
      | (droid->Data4hi[1]);
    snprintf((char *)droid_a->ClockSeq, 10, "%"PRIu16, ClockSeq);

    //Work out the time
    //****
    // Build up the time in simple steps
    Time = (int64_t)droid->Data1;
    Time += ((int64_t)droid->Data2) << 32;
    Timehi = ((int16_t)droid->Data3 & 0x0FFF);
    Time += ((int64_t)Timehi) << 48;

    //Now convert to filetime
    Time -= (((int64_t)(1000 * 1000 * 10))*((int64_t)(60 * 60 * 24))*
      ((int64_t)(17 + 30 + 31 + (365 * 18) + 5)));
    //Now get sensible answers
    get_filetime_a_long(Time, droid_a->Time_long);
    get_filetime_a_short(Time, droid_a->Time);

    // The MAC address (node)
    snprintf((char *)droid_a->Node, 20,
      "%.2"PRIX8":%.2"PRIX8":%.2"PRIX8":%.2"PRIX8":%.2"PRIX8":%.2"PRIX8,
      droid->Data4lo[0],
      droid->Data4lo[1],
      droid->Data4lo[2],
      droid->Data4lo[3],
      droid->Data4lo[4],
      droid->Data4lo[5]);
  }
  else
  {
    snprintf((char *)droid_a->Time, 30, "[N/A]");
    snprintf((char *)droid_a->Time_long, 40, "[N/A]");
    snprintf((char *)droid_a->ClockSeq, 10, "[N/A]");
    snprintf((char *)droid_a->Node, 20, "[N/A]");
  }
}
//
//Function led_setnull(struct LIF_EXTRA_DATA * led) just sets all the Extra Data
//structures to 0
void led_setnull(struct LIF_EXTRA_DATA * led)
{
  int i,j;

  led->Size = 0;
  led->lcp.Size = 0;
  led->lcp.sig = 0;
  led->lcp.Posn = 0;

  led->lcfep.Size = 0;
  led->lcfep.sig = 0;
  led->lcfep.Posn = 0;

  led->ldp.Size = 0;
  led->ldp.sig = 0;
  led->ldp.Posn = 0;

  led->lep.Size = 0;
  led->lep.sig = 0;
  led->lep.Posn = 0;

  led->liep.Size = 0;
  led->liep.sig = 0;
  led->liep.Posn = 0;

  led->lkfp.Size = 0;
  led->lkfp.sig = 0;
  led->lkfp.Posn = 0;

  led->lpsp.Size = 0;
  led->lpsp.sig = 0;
  led->lpsp.Posn = 0;
  led->lpsp.NumStores = 0;
  for (i = 0; i < PROPSTORES; i++)
  {
    led->lpsp.Stores[i].StorageSize = 0;
    for (j = 0; j < PROPVALUES; j++)
    {
      led->lpsp.Stores[i].PropValues[j].ValueSize = 0;
    }
  }

  led->lsp.Size = 0;
  led->lsp.sig = 0;
  led->lsp.Posn = 0;

  led->lsfp.Size = 0;
  led->lsfp.sig = 0;
  led->lsfp.Posn = 0;

  led->ltp.Size = 0;
  led->ltp.sig = 0;
  led->ltp.Posn = 0;

  led->lvidlp.Size = 0;
  led->lvidlp.sig = 0;
  led->lvidlp.Posn = 0;

  led->terminal = 0;
}
