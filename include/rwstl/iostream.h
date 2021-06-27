/*
 * C++ Library file stream.h
 * Copyright (C) ARM Limited, 1993-1998. All rights reserved.
 * Copyright (C) Codemist Limited, 1993.
 */

/*
 * RCS $Revision: 1.12.4.3 $
 * Checkin $Date: 1998/10/26 15:55:37 $
 * Revising $Author: achapman $
 */

#pragma force_top_level
#pragma include_only_once

/* stream.h: Modelled after Stroustrup, "The C++ Programming Language",   */
/* chapter 8, "Streams".                                                  */

#ifndef __stream_h
#define __stream_h

#include <stdio.h>

#ifdef __NAMESPACES
namespace std {
#endif
    typedef int streamsize;
    class ios_base {
      public:
        /* these should be 'static const iostate' */
        enum iostate { goodbit = 0, badbit = 1,
                       eofbit = 2, failbit = 4 };
        enum openmode { trunc = 1, out = 2, in = 4, binary = 8,
                        app = 16, ate = 32 };
        enum fmtflags { boolalpha = 1, dec = 2, fixed = 4, hex = 8,
                        internal = 16, left = 32, oct = 64, right = 128,
                        scientific = 256, showbase = 512, showpoint = 1024,
                        showpos = 2048, skipws = 4096, unitbuf = 8192,
                        uppercase = 16384,
                        adjustfield = left|right|internal,
                        basefield = dec|oct|hex,
                        floatfield = scientific|fixed };
        fmtflags flags() const;
        fmtflags flags(fmtflags);
        fmtflags setf(fmtflags);
        fmtflags setf(fmtflags /*fmtfl*/, fmtflags /*mask*/);
        void unsetf(fmtflags /*mask*/);

        streamsize width() const;
        streamsize width(streamsize);

      protected:
        ios_base() { (void)this; }
        iostate state_;
        streamsize width_;
        fmtflags flags_;

        char* intfmt(char* buf, bool put, bool sign, const char *len = 0);
        char* fltfmt(char* buf, bool put, char len = '\0');
    };

    // ios_base::iostate is a bitmask type 17.2.2.1.1 [lib.bitmask.types]
    inline ios_base::iostate operator & (ios_base::iostate a, ios_base::iostate b)
        { return ios_base::iostate((int)a & (int)b); }
    inline ios_base::iostate operator | (ios_base::iostate a, ios_base::iostate b)
        { return ios_base::iostate((int)a | (int)b); }
    inline ios_base::iostate operator ^ (ios_base::iostate a, ios_base::iostate b)
        { return ios_base::iostate((int)a ^ (int)b); }
    inline ios_base::iostate operator ~ (ios_base::iostate a)
        { return ios_base::iostate(~(int)a); }
    inline ios_base::iostate& operator &= (ios_base::iostate& a, ios_base::iostate b)
        { a = a & b; return a; }
    inline ios_base::iostate& operator |= (ios_base::iostate& a, ios_base::iostate b)
        { a = a | b; return a; }
    inline ios_base::iostate& operator ^= (ios_base::iostate& a, ios_base::iostate b)
        { a = a ^ b; return a; }

    // ios_base::openmode is a bitmask type 17.2.2.1.1 [lib.bitmask.types]
    inline ios_base::openmode operator & (ios_base::openmode a, ios_base::openmode b)
        { return ios_base::openmode((int)a & (int)b); }
    inline ios_base::openmode operator | (ios_base::openmode a, ios_base::openmode b)
        { return ios_base::openmode((int)a | (int)b); }
    inline ios_base::openmode operator ^ (ios_base::openmode a, ios_base::openmode b)
        { return ios_base::openmode((int)a ^ (int)b); }
    inline ios_base::openmode operator ~ (ios_base::openmode a)
        { return ios_base::openmode(~(int)a); }
    inline ios_base::openmode& operator &= (ios_base::openmode& a, ios_base::openmode b)
        { a = a & b; return a; }
    inline ios_base::openmode& operator |= (ios_base::openmode& a, ios_base::openmode b)
        { a = a | b; return a; }
    inline ios_base::openmode& operator ^= (ios_base::openmode& a, ios_base::openmode b)
        { a = a ^ b; return a; }

    // ios_base::fmtflags is a bitmask type 17.2.2.1.1 [lib.bitmask.types]
    inline ios_base::fmtflags operator & (ios_base::fmtflags a, ios_base::fmtflags b)
        { return ios_base::fmtflags((int)a & (int)b); }
    inline ios_base::fmtflags operator | (ios_base::fmtflags a, ios_base::fmtflags b)
        { return ios_base::fmtflags((int)a | (int)b); }
    inline ios_base::fmtflags operator ^ (ios_base::fmtflags a, ios_base::fmtflags b)
        { return ios_base::fmtflags((int)a ^ (int)b); }
    inline ios_base::fmtflags operator ~ (ios_base::fmtflags a)
        { return ios_base::fmtflags(~(int)a); }
    inline ios_base::fmtflags& operator &= (ios_base::fmtflags& a, ios_base::fmtflags b)
        { a = a & b; return a; }
    inline ios_base::fmtflags& operator |= (ios_base::fmtflags& a, ios_base::fmtflags b)
        { a = a | b; return a; }
    inline ios_base::fmtflags& operator ^= (ios_base::fmtflags& a, ios_base::fmtflags b)
        { a = a ^ b; return a; }

    // ios_base manipulators

    ios_base& boolalpha(ios_base&);  // ignored for input
    ios_base& noboolalpha(ios_base&);
    ios_base& showbase(ios_base&);
    ios_base& noshowbase(ios_base&);
    ios_base& showpoint(ios_base&);
    ios_base& noshowpoint(ios_base&);
    ios_base& showpos(ios_base&);
    ios_base& noshowpos(ios_base&);
    ios_base& skipws(ios_base&);
    ios_base& noskipws(ios_base&);
    ios_base& uppercase(ios_base&);
    ios_base& nouppercase(ios_base&);
    ios_base& unitbuf(ios_base&);  // ignored
    ios_base& nounitbuf(ios_base&);  // ignored

    ios_base& internal(ios_base&);  // ignored
    ios_base& left(ios_base&);  // ignored
    ios_base& right(ios_base&);  // ignored

    ios_base& dec(ios_base&);
    ios_base& hex(ios_base&);
    ios_base& oct(ios_base&);

    ios_base& fixed(ios_base&);
    ios_base& scientific(ios_base&);

    /*
    template <class charT, class traits = char_traits<charT> >
        class basic_ios : ios_base { ... };
    typedef basic_ios<char> ios;
    */

    class streambuf;

    /* reverse ios and basic_ios here as a sop to RW rw/stddefs.h */

    class ios : public ios_base {
      public:
        typedef char char_type;
        streambuf* rdbuf() const;
        streambuf* rdbuf(streambuf*);

      protected:
        ios() { (void)this; }
        void init(streambuf*);

      private:
        streambuf* sbuf_;
    };

    typedef ios basic_ios;

    class istream;

    class streambuf {                
      public:
        streambuf(istream*); // not standard
        typedef int int_type;
        int_type sbumpc();

      private:
        istream* mystream;
    };
#ifdef __NAMESPACES
}
#endif

/* A cstream is a C stream (FILE *). We model it after Stroustrup's ill-  */
/* named filebuf class. Beware that we have chosen to work efficiently    */
/* with C rather than attempt compatibility with any extant C++. However, */
/* this gives a sufficient basis for implementing most of iostream.h.     */

class cstream : public basic_ios {
  protected:
    FILE *f;
    iostate state;
    cstream();
    cstream(FILE *ff);
        // Construct a cstream from a FILE *
  public:
    int close();
        // Carefully call fclose...
    int flush();

    void open(const char *name, openmode);
    int setvbuf(char *buf, int mode, size_t size);
    int seek(long offset, int flag);
    long tell();
    int error();
    operator void* () const;
    bool operator ! () const { return fail(); }
    bool good() const;
    bool eof() const;
    bool fail() const;
    bool bad() const;
    void clear(iostate state = goodbit);
    void setstate(iostate state);
    iostate rdstate() const;        

    bool ipfx(int=0) const {(void) this; return true;}   // obsolete
    void isfx() const {(void) this; }                    // obsolete
};

/* ostream: Modelled after Stroustrup, "The C++ Programming Language",    */
/* section 8.2, "ostream" class.                                          */

class ostream : public cstream {
  public:
    ostream();
    ostream(const char* name, openmode = out|trunc);
    ostream(FILE *f);
    ~ostream();

    void open(const char* name, openmode mode = out|trunc) { cstream::open(name, mode); }

    size_t write(void *ptr, size_t size);

    ostream& operator << (char);
    ostream& operator << (unsigned char);
    ostream& operator << (short);
    ostream& operator << (unsigned short);
    ostream& operator << (const char *);
    ostream& operator << (int);
    ostream& operator << (unsigned int);
    ostream& operator << (long);
    ostream& operator << (unsigned long);
    ostream& operator << (float);
    ostream& operator << (double);
    ostream& operator << (long double);
    ostream& operator << (const void *);
    ostream& operator << (bool);

#ifndef __DIALECT_FUSSY
    ostream& operator << (long long);
    ostream& operator << (unsigned long long);
#endif

    ostream& operator << (ostream& (*manip)(ostream&))
        { return manip(*this); }
    ostream& operator << (basic_ios& (*manip)(basic_ios&))
        { (void)manip(*this); return *this; }
    ostream& operator << (ios_base& (*manip)(ios_base&))
        { (void)manip(*this); return *this; }

    ostream& put(char);
};

extern ostream cout, cerr;

typedef ostream ofstream;

/* ostream manipulators */
inline ostream& endl(ostream& os) { os.put('\n'); os.flush(); return os; }
inline ostream& ends(ostream& os) { os.put('\0'); return os; }
inline ostream& flush(ostream& os) { os.flush(); return os; }

/* istream: Modelled after Stroustrup, "The C++ Programming Language",    */
/* section 8.4, "Input" (the istream class).                              */

class istream : public cstream {
  public:
    istream();
    istream(const char* name, openmode = in);
    istream(FILE *f);
    ~istream();

    class sentry {
      public:
        sentry(istream&, bool /*noskipws*/ = false);
        operator bool();

      private:
        bool ok_;
    };

    void open(const char* name, openmode mode = in) { cstream::open(name, mode); }

    size_t read(void *ptr, size_t size);
    int get();

    istream& operator >> (short&);
    istream& operator >> (int&);
    istream& operator >> (long&);
    istream& operator >> (unsigned short&);
    istream& operator >> (unsigned int&);
    istream& operator >> (unsigned long&);
    istream& operator >> (float&);
    istream& operator >> (double&);
    istream& operator >> (long double&);
    istream& operator >> (void*&);
    istream& operator >> (bool&);

#ifndef __DIALECT_FUSSY
    istream& operator >> (long long&);
    istream& operator >> (unsigned long long&);
#endif

    istream& operator >> (istream& (*manip)(istream&))
        { return manip(*this); }
    istream& operator >> (basic_ios& (*manip)(basic_ios&))
        { (void)manip(*this); return *this; }
    istream& operator >> (ios_base& (*manip)(ios_base&))
        { (void)manip(*this); return *this; }

    istream& get(char_type& c);
    istream& get(void *p, int n, char_type stop='\n');

    istream& putback(char_type ch);

  private:
    streambuf sbuf;
};

istream& operator>>(istream&, char*);
istream& operator>>(istream&, unsigned char*);
istream& operator>>(istream&, signed char*);

istream& operator>>(istream&, char&);
istream& operator>>(istream&, unsigned char&);
istream& operator>>(istream&, signed char&);

istream& ws(istream&);


extern istream cin;

typedef istream ifstream;

#endif

/* End of stream.h */
