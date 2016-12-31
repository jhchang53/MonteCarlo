//
//	IniParser.h
//	linked list of key-value pairs
//
//	version 0.1 : 15 Jan. 2010
struct Ini_KV
{
    char *key;
    char *val;
    Ini_KV *nxt;
    int line;
};

struct Ini_Section
{
    char *name;
    Ini_KV *pKV;
    Ini_Section *nxt;
    int line;
};

#define INIBUFSIZE 4096
#define MAXVAL  4096

class IniParser
{
public:
    IniParser();
    ~IniParser();
    int readFile(const char *fname);
    Ini_Section *setSection(const char *name);
    char *getStr(const char *key, char *str);
    int getInt(const char *key, int intdefault);
    double getDouble(const char *key, double dbldefault);
    int getStrArray(const char *key, char **strarr, int maxidx);
    int getIntArray(const char *key, int *iarr, int maxidx);
    int getDoubleArray(const char *key, double *darr, int maxidx);
private:
    void removeComment(char *buf);
    int isSection(char *buf);
    int isKVpair(char *buf);
    int newSection(int line);
    int newKVpair(int line);
    char buf[INIBUFSIZE];
    char curSectionName[100];
    char curKeyName[100],curValStr[MAXVAL];
    Ini_Section *pSect, *curSect;
    Ini_KV *curKV;
};
