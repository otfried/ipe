// -*- C++ -*-
// --------------------------------------------------------------------
// Base header file --- must be included by all Ipe components.
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2023 Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef IPEBASE_H
#define IPEBASE_H

#include <cstdio>
#include <vector>
#include <algorithm>
#include <memory>

// --------------------------------------------------------------------

#undef assert
#define assert(e) ((e) ? (void)0 : ipeAssertionFailed(__FILE__, __LINE__, #e))
extern void ipeAssertionFailed(const char *, int, const char *);

extern void ipeDebug(const char *msg, ...) noexcept;

// --------------------------------------------------------------------

namespace ipe {

  template <typename T>
  inline int size(const std::vector<T> &v) {
    return static_cast<int>(v.size()); }

  constexpr double IPE_PI = 3.14159265358979323846;

#ifdef WIN32
  constexpr char IPESEP = '\\';
#else
  constexpr char IPESEP = '/';
#endif

  //! Ipelib version.
  /*! \ingroup base */
  const int IPELIB_VERSION = 70227;

  //! Oldest readable file format version.
  /*! \ingroup base */
  const int OLDEST_FILE_FORMAT = 70000;
  //! Current file format version.
  /*! \ingroup base */
  const int FILE_FORMAT = 70218;

  enum class LatexType { Default, Pdftex, Xetex, Luatex };

  // --------------------------------------------------------------------

  class Stream;

  class String {
  public:
    String() noexcept;
    String(const char *str) noexcept;
    String(const char *str, int len) noexcept;
    String(const String &rhs) noexcept;
    String(const String &rhs, int index, int len) noexcept;
    String &operator=(const String &rhs) noexcept;
#ifdef WIN32
    String(const wchar_t *data);
    std::wstring w() const noexcept;
#endif
    ~String() noexcept;
    static String withData(char *data, int len = 0) noexcept;
    //! Return character at index i.
    inline char operator[](int i) const noexcept { return iImp->iData[i]; }
    //! Is the string empty?
    inline bool empty() const noexcept { return (size() == 0); }
    //! Return read-only pointer to the data.
    const char *data() const noexcept { return iImp->iData; }
    //! Return number of bytes in the string.
    inline int size() const noexcept { return iImp->iSize; }
    //! Operator syntax for append.
    inline void operator+=(const String &rhs) noexcept { append(rhs); }
    //! Operator syntax for append.
    inline void operator+=(const char *rhs) noexcept { append(rhs); }
    //! Operator syntax for append.
    void operator+=(char ch) noexcept { append(ch); }
    //! Create substring.
    inline String substr(int i, int len = -1) const noexcept {
      return String(*this, i, len); }
    //! Create substring at the left.
    inline String left(int i) const noexcept {
      return String(*this, 0, i); }
    String right(int i) const noexcept;
    //! Operator !=.
    inline bool operator!=(const String &rhs) const noexcept {
      return !(*this == rhs); }
    //! Operator !=.
    inline bool operator!=(const char *rhs) const noexcept {
      return !(*this == rhs); }

    int find(char ch) const noexcept;
    int rfind(char ch) const noexcept;
    int find(const char *rhs) const noexcept;
    void erase() noexcept;
    void append(const String &rhs) noexcept;
    void append(const char *rhs) noexcept;
    void append(char ch) noexcept;
    void appendUtf8(uint16_t ch) noexcept;
    bool hasPrefix(const char *rhs) const noexcept;
    bool operator==(const String &rhs) const noexcept;
    bool operator==(const char *rhs) const noexcept;
    bool operator<(const String &rhs) const noexcept;
    String operator+(const String &rhs) const noexcept;
    int unicode(int &index) const noexcept;
    String getLine(int &index) const noexcept;
    const char *z() const noexcept;
  private:
    void detach(int n) noexcept;
  private:
    struct Imp {
      int iRefCount;
      int iSize;
      int iCapacity;
      char *iData;
    };
    static Imp *theEmptyString;
    static Imp *emptyString() noexcept;
    Imp *iImp;
  };

  // --------------------------------------------------------------------

  class Fixed {
  public:
    explicit Fixed(int val) : iValue(val * 1000) { /* nothing */ }
    explicit Fixed() { /* nothing */ }
    inline static Fixed fromInternal(int32_t val);
    static Fixed fromDouble(double val);
    inline int toInt() const { return iValue / 1000; }
    inline double toDouble() const { return iValue / 1000.0; }
    inline int internal() const { return iValue; }
    Fixed mult(int a, int b) const;
    inline bool operator==(const Fixed &rhs) const;
    inline bool operator!=(const Fixed &rhs) const;
    inline bool operator<(const Fixed &rhs) const;
    inline bool isInteger() const;
  private:
    int32_t iValue;

    friend Stream &operator<<(Stream &stream, const Fixed &f);
  };

  // --------------------------------------------------------------------

  class Lex {
  public:
    explicit Lex(String str);

    String token();
    String nextToken();
    int getInt();
    int getHexByte();
    Fixed getFixed();
    unsigned long int getHexNumber();
    double getDouble();
    //! Extract next character (not skipping anything).
    inline char getChar() {
      return iString[iPos++]; }
    void skipWhitespace();

    //! Operator syntax for getInt().
    inline Lex &operator>>(int &i) {
      i = getInt(); return *this; }
    //! Operator syntax for getDouble().
    inline Lex &operator>>(double &d) {
      d = getDouble(); return *this; }

    //! Operator syntax for getFixed().
    inline Lex &operator>>(Fixed &d) {
      d = getFixed(); return *this; }

    //! Mark the current position.
    inline void mark() {
      iMark = iPos; }
    //! Reset reader to the marked position.
    inline void fromMark() {
      iPos = iMark; }

    //! Return true if at end of string (not even whitespace left).
    inline bool eos() const {
      return (iPos == iString.size()); }

  private:
    String iString;
    int iPos;
    int iMark;
  };

  // --------------------------------------------------------------------

  class Buffer {
  public:
    explicit Buffer() = default;
    explicit Buffer(int size);
    explicit Buffer(const char *data, int size);
    //! Character access.
    inline char &operator[](int index) noexcept { return (*iData)[index]; }
    //! Character access (const version).
    inline const char &operator[](int index) const noexcept { return (*iData)[index]; }
    //! Return size of buffer;
    inline int size() const noexcept { return iData ? static_cast<int>((*iData).size()) : 0; }
    //! Return pointer to buffer data.
    inline char *data() noexcept { return iData ? iData->data() : nullptr; }
    //! Return pointer to buffer data (const version).
    inline const char *data() const noexcept { return iData ? iData->data() : nullptr; }

  private:
    std::shared_ptr<std::vector<char>> iData;
  };

  extern void ipeDebugBuffer(Buffer data, int maxsize);

  // --------------------------------------------------------------------

  class Stream {
  public:
    //! Virtual destructor.
    virtual ~Stream();
    //! Output character.
    virtual void putChar(char ch) = 0;
    //! Close the stream.  No more writing allowed!
    virtual void close();
    //! Output string.
    virtual void putString(String s);
    //! Output C string.
    virtual void putCString(const char *s);
  //! Output raw character data.
    virtual void putRaw(const char *data, int size);
    //! Output character.
    inline Stream &operator<<(char ch) { putChar(ch); return *this; }
    //! Output string.
    inline Stream &operator<<(const String &s) { putString(s); return *this; }
    //! Output C string.
    inline Stream &operator<<(const char *s) { putCString(s); return *this; }
    Stream &operator<<(int i);
    Stream &operator<<(double d);
    void putHexByte(char b);
    void putXmlString(String s);
  };

  /*! \class ipe::TellStream
    \ingroup base
    \brief Adds position feedback to IpeStream.
  */

  class TellStream : public Stream {
  public:
    virtual long tell() const = 0;
  };

  class StringStream : public TellStream {
  public:
    StringStream(String &string);
    virtual void putChar(char ch);
    virtual void putString(String s);
    virtual void putCString(const char *s);
    virtual void putRaw(const char *data, int size);
    virtual long tell() const;
  private:
    String &iString;
  };

  class FileStream : public TellStream {
  public:
    FileStream(std::FILE *file);
    virtual void putChar(char ch);
    virtual void putString(String s);
    virtual void putCString(const char *s);
    virtual void putRaw(const char *data, int size);
    virtual long tell() const;
  private:
    std::FILE *iFile;
  };

  // --------------------------------------------------------------------

  class DataSource {
  public:
    virtual ~DataSource() = 0;
    //! Get one more character, or EOF.
    virtual int getChar() = 0;
    virtual int length() const;
    virtual void setPosition(int pos);
    virtual int position() const;
  };

  class FileSource : public DataSource {
  public:
    FileSource(std::FILE *file);
    virtual int getChar() override;
    virtual int length() const override;
    virtual void setPosition(int pos) override;
    virtual int position() const override;
  private:
    std::FILE *iFile;
  };

  class BufferSource : public DataSource {
  public:
    BufferSource(const Buffer &buffer);
    virtual int getChar() override;
    virtual int length() const override;
    virtual void setPosition(int pos) override;
    virtual int position() const override;
  private:
    const Buffer &iBuffer;
    int iPos;
  };

  // --------------------------------------------------------------------

  class Platform {
  public:
    using DebugHandler = void (*)(const char *);

#ifdef IPEBUNDLE
    static String ipeDir(const char *suffix, const char *fname = nullptr);
#endif
#ifdef WIN32
    static FILE *fopen(const char *fname, const char *mode);
    static int mkdir(const char *dname);
#else
    inline static FILE *fopen(const char *fname, const char *mode) {
      return ::fopen(fname, mode);
    }
#endif
    static String ipeDrive();
    static int libVersion();
    static void initLib(int version);
    static void setDebug(bool debug);
    static String currentDirectory();
    static String latexDirectory();
    static String latexPath();
    static bool fileExists(String fname);
    static bool listDirectory(String path, std::vector<String> &files);
    static String realPath(String fname);
    static String readFile(String fname);
    static int runLatex(String dir, LatexType engine, String docname) noexcept;
    static double toDouble(String s);
    static int toNumber(String s, int &iValue, double &dValue);
    static String spiroVersion();
    static String gslVersion();
  };

  // --------------------------------------------------------------------

  inline bool Fixed::operator==(const Fixed &rhs) const
  {
    return iValue == rhs.iValue;
  }

  inline bool Fixed::operator!=(const Fixed &rhs) const
  {
    return iValue != rhs.iValue;
  }

  inline bool Fixed::operator<(const Fixed &rhs) const
  {
    return iValue < rhs.iValue;
  }

  inline bool Fixed::isInteger() const
  {
    return (iValue % 1000) == 0;
  }

  inline Fixed Fixed::fromInternal(int val)
  {
    Fixed f;
    f.iValue = val;
    return f;
  }

} // namespace

// --------------------------------------------------------------------
#endif
