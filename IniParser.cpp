//
//	IniParser.cc
//	linked list of key-value pairs
//	
//	version 0.1 : 15 Jan. 2010, jhchang@kaeri.re.kr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "IniParser.h"

#ifdef __MINGW32__
// MinGW does not provided strtok_r().

// **TODO : BE SURE TO REMOVE THIS MACRO BLOCK WHEN PUBLISHING THE CODE.
//
// Below is GPLed code.
// This macro block is not required on systems supporting strtok_r().
//

/* Parse S into tokens separated by characters in DELIM.
   If S is NULL, the saved pointer in SAVE_PTR is used as
   the next starting point.  For example:
	char s[] = "-abc-=-def";
	char *sp;
	x = strtok_r(s, "-", &sp);	// x = "abc", sp = "=-def"
	x = strtok_r(NULL, "-=", &sp);	// x = "def", sp = NULL
	x = strtok_r(NULL, "=", &sp);	// x = NULL
		// s = "abc\0-def\0"
*/
char *
strtok_r (char *s, const char *delim, char **save_ptr)
{
  char *token;

  if (s == NULL)
    s = *save_ptr;

  /* Scan leading delimiters.  */
  s += strspn (s, delim);
  if (*s == '\0')
    {
      *save_ptr = s;
      return NULL;
    }

  /* Find the end of the token.  */
  token = s;
  s = strpbrk (token, delim);
  if (s == NULL)
    /* This token finishes the string.  */
    *save_ptr = strchr (token, '\0');
  else
    {
      /* Terminate the token and make *SAVE_PTR point past it.  */
      *s = '\0';
      *save_ptr = s + 1;
    }
  return token;
}

#endif // __MINGW32__

IniParser::IniParser()
{
};

int IniParser::readFile(const char *fname)
{
 FILE *F = fopen(fname,"r");
 if (F == NULL)
 {
  printf("*** IniParser: input data file %s not found.\n",fname);
  return 0;
 }
 pSect = NULL;
 curSect = NULL;
 curSectionName[0] = '\0';
 curKV = NULL;
 curKeyName[0] = '\0';
 curValStr[0] = '\0';
 char backslash = '\\';
 int line=0;
 int ibias = 0;
 int lbuf;
 while (!feof(F))
 {
  fgets(buf+ibias,INIBUFSIZE,F);
  line++;
  // span to next line if line end with backslash
  lbuf = strlen(buf);
  if (lbuf > INIBUFSIZE)
  {
   printf("*** IniParser : a line cannot span more than %d characters.\n",
	INIBUFSIZE);
   printf("    Datafile %s line %d.\n",fname,line);
   exit(0);
  }
  // to handle Windows compatibility
  char chl = buf[lbuf-1];
  if (chl == '\n')
  {
   lbuf--;
   buf[lbuf] = '\0';
   chl = buf[lbuf-1];
  }
  if (chl == backslash) ibias = lbuf-1;
  else
  {
   ibias = 0;
   removeComment(buf);
   int issect  = isSection(buf);
   if (issect)
   {
    newSection(line);
    curKV = NULL;
   }
   int ispair = isKVpair(buf);
   if (ispair)
   {
    newKVpair(line);
   }
  }
 }
 fclose(F);
 curSect = pSect;
 return 1;
};

IniParser::~IniParser()
{
 curSect = pSect;
 Ini_Section *nxtSect;
 Ini_KV *nxtKV,*preKV;
 int levKV,levSect;
 while (curSect != NULL)
 {
  // fetch KVpairs
  curKV = curSect->pKV;
  while (curKV != NULL)
  {
   nxtKV = curKV->nxt;
   curKV = nxtKV;
  }
  // destroy KVpairs
  curKV = curSect->pKV;
  while (curKV != NULL)
  {
   nxtKV = curKV->nxt;
   // delete curKV
   delete [] curKV->key;
   delete [] curKV->val;
   delete curKV;
   curSect->pKV = nxtKV;
   curKV = nxtKV;
  }
  //
  nxtSect = curSect->nxt;
  curSect = nxtSect;
 }
 // delete sections
 while (pSect != NULL)
 {
  nxtSect = pSect->nxt;
  // delete pSect;
  if (pSect->pKV)
  {
   printf("*** Programming error problem in deleteing Section structure.\n");
   printf("    KVpair not empty.\n");
   exit(0);
  }
  delete [] pSect->name;
  delete pSect;
  pSect = nxtSect;
 }
};

Ini_Section* IniParser::setSection(const char *name)
{
 // find section pointer by searching linked list
 curSect = pSect;
 while ((curSect != NULL) && strcmp(name,curSect->name)) curSect = curSect->nxt;
 return curSect;
};

char* IniParser::getStr(const char *key, char *str)
{
 // select from current section
 curKV = curSect->pKV;
 while ((curKV != NULL) && strcmp(key,curKV->key)) curKV = curKV->nxt;
 if (curKV == NULL) return NULL;	// return "";
 else
 {
  // remove quotes
  if ((curKV->val[0] == '"') || (curKV->val[0] == '\''))
  {
   strcpy(str,curKV->val+1);
   str[strlen(str)-1] = '\0';
  }
  else strcpy(str,curKV->val);
 }
 return curKV->val;
};

int IniParser::getInt(const char *key, int intdefault)
{
 char str[100];
 // select from current section
 curKV = curSect->pKV;
 while ((curKV != NULL) && strcmp(key,curKV->key)) curKV = curKV->nxt;
 if (curKV == NULL) return intdefault;
 else
 {
  // remove quotes
  if ((curKV->val[0] == '"') || (curKV->val[0] == '\''))
  {
   strcpy(str,curKV->val+1);
  }
  else strcpy(str,curKV->val);
 }
 return atoi(str);
};

double IniParser::getDouble(const char *key, double dbldefault)
{
 char str[100];
 // select from current section
 curKV = curSect->pKV;
 while ((curKV != NULL) && strcmp(key,curKV->key)) curKV = curKV->nxt;
 if (curKV == NULL) return dbldefault;
 else
 {
  // remove quotes
  if ((curKV->val[0] == '"') || (curKV->val[0] == '\''))
  {
   strcpy(str,curKV->val+1);
  }
  else strcpy(str,curKV->val);
 }
 return atof(str);
};

int IniParser::getStrArray(const char *key, char **strarr, int maxidx)
{
 // create new strings
 int idx = 0;
 char word[100];
 // select from current section
 curKV = curSect->pKV;
 while ((curKV != NULL) && strcmp(key,curKV->key)) curKV = curKV->nxt;
 if (curKV == NULL) return 0;
 strcpy(buf,curKV->val);
 int lbuf = strlen(buf);
 int ib = 0;
 int i,ie;
 while (ib < lbuf)
 {
  // find first non blank
  for (i=ib; (buf[i] == ' ') && (i < lbuf); i++);
  ib = i;
  char ch = buf[ib];
  if (ch == '"')
  {
   // find next quote
   for (i=ib+1; (buf[i] != '"') && (i < lbuf); i++);
   strncpy(word,buf+ib+1,i-ib-1);
   word[i-ib-1] = '\0';
   if (strarr[idx] == NULL) strarr[idx] = new char[strlen(word)+1];
   strcpy(strarr[idx],word);
   ib = i+1;
   idx++;
   if (idx > maxidx)
   {
    printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
    exit(0);
   }
  }
  else if (ch == ',')
  {
   // find next non-blank
   for (i=ib+1; (buf[i] == ' ') && (i < lbuf); i++);
   ib = i;
   if (buf[i] == ',')
   {
    idx++;
    if (idx > maxidx)
    {
     printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
     exit(0);
    }
   }
  }
  else
  {
   // find next comma
   for (i=ib+1; (buf[i] != ',') && (i < lbuf); i++);
   strncpy(word,buf+ib,i-ib);
   word[i-ib] = '\0';
   if (strarr[idx] == NULL) strarr[idx] = new char[strlen(word)+1];
   strcpy(strarr[idx],word);
   ib = i+1;
   idx++;
   if (idx > maxidx)
   {
    printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
    exit(0);
   }
  }
 }
 return idx;
};

int IniParser::getIntArray(const char *key, int *iarr, int maxidx)
{
 char vstr[100],mstr[20];
 int mult;
 int ival;
 int idx = -1;
 // select from current section
 curKV = curSect->pKV;
 while ((curKV != NULL) && strcmp(key,curKV->key)) curKV = curKV->nxt;
 if (curKV == NULL) return 0;
 else
 {
  // find comma
  int ib = 0;
  int ie = 0;
  idx = 0;
  while (curKV->val[ie] != '\0')
  {
   for (ie=ib; (curKV->val[ie] != ',') && (curKV->val[ie] != '\0'); ie++);
   strncpy(buf,curKV->val+ib,ie-ib);
   buf[ie-ib] = '\0';
   // remove leading blanks
   int ist = 0;
   for (ist=0; (buf[ist] == ' ') && (buf[ist] != '\0'); ist++);
   for (int jj=0; jj < ie-ib+1; jj++) buf[jj] = buf[jj+ist];
   if (buf[0] == '\0')
   {
    idx++;
    if (idx > maxidx)
    {
     printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
     exit(0);
    }
   }
   else
   {
    // blank separation
    char *ptrptr;
    char *v = strtok_r(buf," ",&ptrptr);
    strcpy(vstr,v);
    // check if * exist
    int istar;
    for (istar=0;(vstr[istar] != '*') && (istar < strlen(vstr)); istar++);
    if (vstr[istar] != '*')
    {
     ival = atoi(vstr);
     iarr[idx] = ival;
     idx++;
     if (idx > maxidx)
     {
      printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
      exit(0);
     }
    }
    else
    {
     strncpy(mstr,vstr,istar);
     mstr[istar] = '\0';
     mult = atoi(mstr);
     ival = atoi(vstr+istar+1);
     for (int m=0; m < mult; m++)
     {
      iarr[idx] = ival;
      idx++;
      if (idx > maxidx)
      {
       printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
       exit(0);
      }
     }
    }
    //
    while (v != NULL)
    {
     v = strtok_r(NULL," ",&ptrptr);
     if (v != NULL)
     {
      // check if * exist
      for (istar=0;(vstr[istar] != '*') && (istar < strlen(vstr)); istar++);
      if (vstr[istar] != '*')
      {
       ival = atoi(vstr);
       iarr[idx] = ival;
       idx++;
       if (idx > maxidx)
       {
        printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
        exit(0);
       }
      }
      else
      {
       strncpy(mstr,vstr,istar);
       mstr[istar] = '\0';
       mult = atoi(mstr);
       ival = atoi(vstr+istar+1);
       for (int m=0; m < mult; m++)
       {
        iarr[idx] = ival;
        idx++;
        if (idx > maxidx)
        {
         printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
         exit(0);
        }
       }
      }
     }
    }
   }
   ib = ie+1;
  }
 }
 return idx;
};

int IniParser::getDoubleArray(const char *key, double *darr, int maxidx)
{
 char vstr[100],mstr[20];
 int mult;
 double dval;
 int idx = -1;
 // select from current section
 curKV = curSect->pKV;
 while ((curKV != NULL) && strcmp(key,curKV->key)) curKV = curKV->nxt;
 if (curKV == NULL) return 0;
 else
 {
  // find comma
  int ib = 0;
  int ie = 0;
  idx = 0;
  while (curKV->val[ie] != '\0')
  {
   for (ie=ib; (curKV->val[ie] != ',') && (curKV->val[ie] != '\0'); ie++);
   strncpy(buf,curKV->val+ib,ie-ib);
   buf[ie-ib] = '\0';
   // remove leading blanks
   int ist = 0;
   for (ist=0; (buf[ist] == ' ') && (buf[ist] != '\0'); ist++);
   for (int jj=0; jj < ie-ib+1; jj++) buf[jj] = buf[jj+ist];
   if (buf[0] == '\0')
   {
    idx++;
    if (idx > maxidx)
    {
     printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
     exit(0);
    }
   }
   else
   {
    // blank separation
    char *ptrptr;
    char *v = strtok_r(buf," ",&ptrptr);
    strcpy(vstr,v);
    // check if * exist
    int istar;
    for (istar=0;(vstr[istar] != '*') && (istar < strlen(vstr)); istar++);
    if (vstr[istar] != '*')
    {
     dval = atof(vstr);
     darr[idx] = dval;
     idx++;
     if (idx > maxidx)
     {
      printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
      exit(0);
     }
    }
    else
    {
     strncpy(mstr,vstr,istar);
     mstr[istar] = '\0';
     mult = atoi(mstr);
     dval = atof(vstr+istar+1);
     for (int m=0; m < mult; m++)
     {
      darr[idx] = dval;
      idx++;
      if (idx > maxidx)
      {
       printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
       exit(0);
      }
     }
    }
    //
    while (v != NULL)
    {
     v = strtok_r(NULL," ",&ptrptr);
     if (v != NULL)
     {
      // check if * exist
      for (istar=0;(vstr[istar] != '*') && (istar < strlen(vstr)); istar++);
      if (vstr[istar] != '*')
      {
       dval = atof(vstr);
       darr[idx] = dval;
       idx++;
       if (idx > maxidx)
       {
        printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
        exit(0);
       }
      }
      else
      {
       strncpy(mstr,vstr,istar);
       mstr[istar] = '\0';
       mult = atoi(mstr);
       dval = atof(vstr+istar+1);
       for (int m=0; m < mult; m++)
       {
        darr[idx] = dval;
        idx++;
        if (idx > maxidx)
        {
         printf("*** Too many data for key %s, need only %d.\n",key,maxidx);
         exit(0);
        }
       }
      }
     }
    }
   }
   ib = ie+1;
  }
 }
 return idx;
};

//
// private functions

void IniParser::removeComment(char *buf)
{
 // remove string from special character ; except quoted
 int quoted = 0;
 int lbuf = strlen(buf);
 for (int i=0; i < lbuf; i++)
 {
  char ch = buf[i];
  // remove end of line first
  if (ch == '\n')
  {
   ch = '\0';
   buf[i] = '\0';
  }
  // change tab to blank
  if (ch == '\t')
  {
   ch = ' ';
   buf[i] = ' ';
  }
  if (quoted)
  {
   if (ch == quoted) quoted = 0;
  }
  else
  {
   switch (ch)
   {
   case ';':
   case '#':
    buf[i] = '\0';
    lbuf = i;
    break;
   case '"':
   case '\'':
    quoted = ch;
    break;
   }
  }
 }
};

int IniParser::isSection(char *buf)
{
 // section name should start with a [, should be less than 100 characters
 int issect = 0;
 if (buf[0] == '[')
 {
  for (int i=1; i < strlen(buf); i++)
  {
   if (buf[i] == ']')
   {
    // we have section name
    strncpy(curSectionName,buf+1,i-1);
    curSectionName[i-1] = '\0';
    issect = 1;
   }
  }
 }
 return issect;
};

int IniParser::isKVpair(char *buf)
{
 // find string parts separated by =
 int ispair = 0;
 int ieq = -1;
 int lbuf = strlen(buf);
 for (int i=0;(ieq < 0) && (i < lbuf); i++)
 {
  char ch = buf[i];
  if (ch == '=') ieq = i;
 }
 if (ieq < 0) return 0;
 // trim key
 int ib,ie;
 for (ib = 0; (buf[ib] == ' ') && (ib < ieq); ib++);
 for (ie = ieq-1; (buf[ie] == ' ') && (ie > 0); ie--);
 strncpy(curKeyName,buf+ib,ie-ib+1);
 curKeyName[ie-ib+1] = '\0';
 // trim value
 for (ib = ieq+1; (buf[ib] == ' ') && (ib < lbuf); ib++);
 for (ie = lbuf-1; (buf[ie] == ' ') && (ie > 0); ie--);
 strncpy(curValStr,buf+ib,ie-ib+1);
 curValStr[ie-ib+1] = '\0';
 return ieq;
};

int IniParser::newSection(int line)
{
 // create a new Section struct
 Ini_Section *nSect = new Ini_Section;
 nSect->line = line;
 nSect->name = new char[strlen(curSectionName)+1];
 strcpy(nSect->name,curSectionName);
 nSect->name[strlen(curSectionName)] = '\0';
 nSect->pKV = NULL;
 nSect->nxt = NULL;
 if (curSect == NULL) pSect = nSect;
 else curSect->nxt = nSect;
 curSect = nSect;
};

int IniParser::newKVpair(int line)
{
 // create a new KV struct
 Ini_KV *nKV = new Ini_KV;
 nKV->line = line;
 nKV->key = new char[strlen(curKeyName)+1];
 strcpy(nKV->key,curKeyName);
 nKV->val = new char[strlen(curValStr)+1];
 strcpy(nKV->val,curValStr);
 nKV->nxt = NULL;
 if (curSect == NULL)
 {
  printf("*** .INI file error ***\n");
  printf("    key = value   started without section definition.\n");
  exit(0);
 }
 if (curKV == NULL) curSect->pKV = nKV;
 else curKV->nxt = nKV;
 curKV = nKV;
};

