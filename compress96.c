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

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#include "stdafx.h"

#define N		 4096	/* size of ring buffer */
#define F		   18	/* upper limit for match_length */
#define THRESHOLD	2   /* encode string into position and length
						   if match_length is greater than this */
#define NIL			N	

unsigned long int
		textsize = 0,	/* text size counter */
		codesize = 0,	/* code size counter */
		printcount = 0;	/* counter for reporting progress every 1K bytes */
unsigned char
		text_buf[N + F - 1];	/* ring buffer of size N,
			with extra F-1 bytes to facilitate string comparison */
int		match_position, match_length,  /* of longest match.  These are
			set by the InsertNode() procedure. */
		lson[N + 1], rson[N + 257], dad[N + 1];  


char strBufGetc(const char *source,int lenSource,int &curPoz){
	if (lenSource <= curPoz ) {
		return EOF;
	} else {
	    curPoz++;
		return source[curPoz-1];

	}
}


void strBufPutc(const char c,char dest[],int &lenDest){
    
		dest[lenDest]=c;
	    lenDest++;
}


void InitTree(void)  /* initialize trees */
{
	int  i;
	for (i = N + 1; i <= N + 256; i++) rson[i] = NIL;
	for (i = 0; i < N; i++) dad[i] = NIL;
}

void InsertNode(int r)
{
	int  i, p, cmp;
	unsigned char  *key;

	cmp = 1;  key = &text_buf[r];  p = N + 1 + key[0];
	rson[r] = lson[r] = NIL;  match_length = 0;
	for ( ; ; ) {
		if (cmp >= 0) {
			if (rson[p] != NIL) p = rson[p];
			else {  rson[p] = r;  dad[r] = p;  return;  }
		} else {
			if (lson[p] != NIL) p = lson[p];
			else {  lson[p] = r;  dad[r] = p;  return;  }
		}
		for (i = 1; i < F; i++)
			if ((cmp = key[i] - text_buf[p + i]) != 0)  break;
		if (i > match_length) {
			match_position = p;
			if ((match_length = i) >= F)  break;
		}
	}
	dad[r] = dad[p];  lson[r] = lson[p];  rson[r] = rson[p];
	dad[lson[p]] = r;  dad[rson[p]] = r;
	if (rson[dad[p]] == p) rson[dad[p]] = r;
	else                   lson[dad[p]] = r;
	dad[p] = NIL;  /* remove p */
}

void DeleteNode(int p)  /* deletes node p from tree */
{
	int  q;
	
	if (dad[p] == NIL) return;  /* not in tree */
	if (rson[p] == NIL) q = lson[p];
	else if (lson[p] == NIL) q = rson[p];
	else {
		q = lson[p];
		if (rson[q] != NIL) {
			do {  q = rson[q];  } while (rson[q] != NIL);
			rson[dad[q]] = lson[q];  dad[lson[q]] = dad[q];
			lson[q] = lson[p];  dad[lson[p]] = q;
		}
		rson[q] = rson[p];  dad[rson[p]] = q;
	}
	dad[q] = dad[p];
	if (rson[dad[p]] == p) rson[dad[p]] = q;  else lson[dad[p]] = q;
	dad[p] = NIL;
}

void Encode96(const char *source,int lenSource, char *dest,int &lenDest)
{
	int curPoz=0;
	lenDest = 0;
	int  i, c, len, r, s, last_match_length, code_buf_ptr;
	unsigned char  code_buf[17], mask;
	
	InitTree();  /* initialize trees */
	code_buf[0] = 0;  /* code_buf[1..16] saves eight units of code, and
		code_buf[0] works as eight flags, "1" representing that the unit
		is an unencoded letter (1 byte), "0" a position-and-length pair
		(2 bytes).  Thus, eight units require at most 16 bytes of code. */
	code_buf_ptr = mask = 1;
	s = 0;  r = N - F;
	for (i = s; i < r; i++) text_buf[i] = ' ';  
	for (len = 0; len < F && (c = strBufGetc(source,lenSource,curPoz)) != EOF; len++)
		text_buf[r + len] = c;  /* Read F bytes into the last F bytes of
			the buffer */
	if ((textsize = len) == 0) return;  /* text of size zero */
	for (i = 1; i <= F; i++) InsertNode(r - i);  /* Insert the F strings,
		each of which begins with one or more 'space' characters.  Note
		the order in which these strings are inserted.  This way,
		degenerate trees will be less likely to occur. */
	InsertNode(r);  /* Finally, insert the whole string just read.  The
		global variables match_length and match_position are set. */
	do {
		if (match_length > len) match_length = len;  /* match_length
			may be spuriously long near the end of text. */
		if (match_length <= THRESHOLD) {
			match_length = 1;  /* Not long enough match.  Send one byte. */
			code_buf[0] |= mask;  /* 'send one byte' flag */
			code_buf[code_buf_ptr++] = text_buf[r];  /* Send uncoded. */
		} else {
			code_buf[code_buf_ptr++] = (unsigned char) match_position;
			code_buf[code_buf_ptr++] = (unsigned char)
				(((match_position >> 4) & 0xf0)
			  | (match_length - (THRESHOLD + 1)));  /* Send position and
					length pair. Note match_length > THRESHOLD. */
		}
		if ((mask <<= 1) == 0) {  /* Shift mask left one bit. */
			for (i = 0; i < code_buf_ptr; i++)  /* Send at most 8 units of */
				strBufPutc(code_buf[i], dest,lenDest);     /* code together */
			codesize += code_buf_ptr;
			code_buf[0] = 0;  code_buf_ptr = mask = 1;
		}
		last_match_length = match_length;
		for (i = 0; i < last_match_length &&
				(c = strBufGetc(source,lenSource,curPoz)) != EOF; i++) {
			DeleteNode(s);		/* Delete old strings and */
			text_buf[s] = c;	/* read new bytes */
			if (s < F - 1) text_buf[s + N] = c;  /* If the position is
				near the end of buffer, extend the buffer to make
				string comparison easier. */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
				/* Since this is a ring buffer, increment the position
				   modulo N. */
			InsertNode(r);	/* Register the string in text_buf[r..r+F-1] */
		}
		if ((textsize += i) > printcount) {
			/*printf("%12ld\r", textsize);*/  printcount += 1024;
				/* Reports progress each time the textsize exceeds
				   multiples of 1024. */
		}
		while (i++ < last_match_length) {	/* After the end of text, */
			DeleteNode(s);					/* no need to read, but */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			if (--len) InsertNode(r);		/* buffer may not be empty. */
		}
	} while (len > 0);	/* until length of string to be processed is zero */
	if (code_buf_ptr > 1) {		/* Send remaining code. */
		for (i = 0; i < code_buf_ptr; i++) strBufPutc(code_buf[i], dest,lenDest);
		codesize += code_buf_ptr;
	}

}

void Decode96(const char *source,int lenSource, char *dest,int &lenDest)	/* Just the reverse of Encode(). */
{
	int curPoz=0;
	lenDest = 0;
	int  i, j, k, r, c;
	unsigned int  flags;
	
	for (i = 0; i < N - F; i++) text_buf[i] = ' ';
	r = N - F;  flags = 0;
	for ( ; ; ) {
		if (((flags >>= 1) & 256) == 0) {
			if ((c = strBufGetc(source,lenSource,curPoz)) == EOF) break;
			flags = c | 0xff00;		/* uses higher byte cleverly */
		}							/* to count eight */
		if (flags & 1) {
			if ((c = strBufGetc(source,lenSource,curPoz)) == EOF) break;
			strBufPutc(c, dest,lenDest);  text_buf[r++] = c;  r &= (N - 1);
		} else {
			if ((i = strBufGetc(source,lenSource,curPoz)) == EOF) break;
			if ((j = strBufGetc(source,lenSource,curPoz)) == EOF) break;
			i |= ((j & 0xf0) << 4);  j = (j & 0x0f) + THRESHOLD;
			for (k = 0; k <= j; k++) {
				c = text_buf[(i + k) & (N - 1)];
				strBufPutc(c, dest,lenDest);  text_buf[r++] = c;  r &= (N - 1);
			}
		}
	}
}

