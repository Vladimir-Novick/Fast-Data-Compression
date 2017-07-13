//
// Author: Vladimir Novick , <vlad.novick@gmail.com>
//
//               https://www.linkedin.com/in/vladimirnovick
//
//                  https://github.com/Vladimir-Novick/
//                      
// (c) 1996 - 2017,  Vladimir Novick
//
// Purpose: Fast Data Compression 
//
//*****************************************************************************/

#ifndef _COMPRESS_H
#define _COMPRESS_H
  void Encode96(const char *source,int lenSource, char *dest,int &lenDest);
  void Decode96(const char *source,int lenSource, char *dest,int &lenDest);
#endif

