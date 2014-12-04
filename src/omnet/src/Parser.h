/* Created by thomas Meyer */

// It parses a file, extracting all fields inside


#ifndef PARSABLE_HPP_
#define PARSABLE_HPP_

#include <string>
#include <cstring>
#include <cstdio>
#include <vector>
#include <assert.h>
#include <climits>

using namespace std;

//--- condition test macros ---------------------------------------------------

#define TEST(expr)                 if ( !testExpression(expr) ) { goto FAILED; }
#define TEST_MSG(expr, var, text)  if ( !testExpression(expr) ) { (var) = (text); goto FAILED; }
#define TEST_DO(expr, stmt)        if ( !testExpression(expr) ) { do { stmt } while ( false ); goto FAILED; }
#define FORCE(expr)                if ( !testExpression(expr) ) { assert( false ); goto FAILED; }
#define FORCE_MSG(expr, var, text) if ( !testExpression(expr) ) { assert( false ); (var) = (text); goto FAILED; }
#define FORCE_DO(expr, stmt)       if ( !testExpression(expr) ) { assert( false ); do { stmt } while ( false ); goto FAILED; }

inline bool testExpression ( bool expr ) { return expr; }
inline bool testExpression ( const void* p ) { return ( 0 != p ); }


//--- types -------------------------------------------------------------------

/**
 * Parse information is the data that remains constant during the parse
 * process.
 */
struct ParseInfo
{
  inline ParseInfo ( void );
  inline ParseInfo ( const string& source, const string& text );

  string source;
  string text;
};

//-----------------------------------------------------------------------------

/**
 * The parse iterator points to the current location in a parsed text.
 * The iterator also contains a number of utility function to iterate on the
 * text, and to parse simple variables.
 */
class ParseIterator
{
public:

  //=== Initialization ========================================================

  inline ParseIterator ( void );
  inline explicit ParseIterator ( const ParseInfo& info );
  inline ParseIterator ( const ParseIterator& other );
  inline virtual ~ParseIterator ( void );

  //=== Assignment ============================================================

  inline ParseIterator& operator= ( const ParseInfo& info );
  inline ParseIterator& operator= ( const ParseIterator& other );

  inline void info ( const ParseInfo& info );
  inline void pos ( unsigned int newPos );
  inline void line ( unsigned int newLine );

  //=== Inspection ============================================================

  inline const ParseInfo& info ( void ) const;
  inline unsigned int pos ( void ) const;
  inline unsigned int line ( void ) const;

  //=== Parse =================================================================

  virtual bool skipWhitespaces ( void );
  virtual char peekCharacter ( void ) const;
  virtual inline bool eatCharacter ( void );
  virtual bool eatCharacters ( unsigned int count );
  virtual bool parseConstCharacter ( char ch );
  virtual bool parseConstCharacters ( const char* pCharacters, char& ch );
  virtual bool parseCharacter ( char& ch );
  virtual bool parseConstString ( const string& str );
  virtual bool parseWord ( string& word );
  virtual bool parseIdentifier ( string& identifier );
  virtual bool parseIdentifierWithIndex ( string& identifier );
  virtual bool parseNumber ( unsigned int& value );
  virtual bool parseNumber ( int& value );
  virtual bool parseNumber ( double& value );
  virtual bool parseQuotation ( char chStart, char chEnd, string& content, bool returnQuotationCharacters = false );
  virtual bool peekEnd ( void ) const;

  //=== Reset =================================================================

  virtual inline void reset ( void );

private:
  const ParseInfo* m_pInfo; // text to be parsed
  unsigned int             m_pos;   // current index within the text
  unsigned int             m_line;  // current line
};

//-----------------------------------------------------------------------------

struct ParseResultType
{
  enum Type
  {
    success,
    error
  };

  static const char* NAME ( Type type );
};

//-----------------------------------------------------------------------------

class ParseResultBuffer
{
public:

  //=== Initialization ========================================================

  inline ParseResultBuffer ( void );
  inline ParseResultBuffer ( const ParseIterator& iter, const string& message );
  inline ParseResultBuffer ( const ParseResultBuffer* pOther );

  //=== Assignment ============================================================

  inline ParseResultBuffer& operator= ( const ParseResultBuffer* pOther );

  //=== Reference Counting ====================================================

  inline void addRef ( void ) const;
  inline void release ( void ) const;
  inline unsigned int refCount ( void ) const;

  //=== Generate ==============================================================

  void error ( const ParseIterator& iter, const string& message );

  //=== Convert ===============================================================

  inline ParseResultType::Type type ( void ) const;
  string formattedMessage ( void ) const;

protected:

  //=== Initialization ========================================================

  inline ~ParseResultBuffer ( void );

private:
  ParseResultBuffer ( const ParseResultBuffer& other );
  ParseResultBuffer& operator= ( const ParseResultBuffer& other );

  mutable unsigned int           m_refCount;
  ParseResultType::Type  m_type;
  ParseIterator          m_iter;
  vector<string>         m_messages;
};

//-----------------------------------------------------------------------------

class ParseResult
{
public:

  //=== Initialization ========================================================

  inline ParseResult ( void );
  inline ParseResult ( const ParseIterator& iter, const string& message );
  inline ParseResult ( const ParseResult& other );
  inline ~ParseResult ( void );

  //=== Assignment ============================================================

  inline ParseResult& operator= ( const ParseResult& other );

  //=== Generate ==============================================================

  inline void error ( const ParseIterator& iter, const string& message );

  //=== Convert ===============================================================

  inline ParseResultType::Type type ( void ) const;
  inline operator bool ( void ) const;
  inline string formattedMessage ( void ) const;

  //=== Reset =================================================================

  inline void reset ( void );

private:
  inline void prepareWrite ( void );

  ParseResultBuffer* m_pBuffer;
};

//-----------------------------------------------------------------------------

class Parsable
{
public:

  //=== Initialization ========================================================

  virtual ~Parsable ( void );

  //=== Parsable Interface ====================================================

  virtual ParseResult parse ( ParseIterator& iter ) = 0;
  virtual string text ( unsigned int indent ) const = 0;
  virtual void print ( FILE* pStream ) const;

protected:

  //=== Initialization ========================================================

  Parsable ( void );

private:
  Parsable ( const Parsable& other );
  Parsable& operator= ( const Parsable& other );
};


//--- implementation ( ParseInfo ) --------------------------------------------

inline ParseInfo::ParseInfo ( void )
: source()
, text()
{
}

//-----------------------------------------------------------------------------

inline ParseInfo::ParseInfo ( const string& source, const string& text )
: source( source )
, text( text )
{
}


//--- implementation ( ParseIterator ) ----------------------------------------

inline ParseIterator::ParseIterator ( void )
: m_pInfo( 0 )
, m_pos( UINT_MAX )
, m_line( UINT_MAX )
{
}

//-----------------------------------------------------------------------------

inline ParseIterator::ParseIterator ( const ParseInfo& info )
: m_pInfo( &info )
, m_pos( 0U )
, m_line( 1U )
{
}

//-----------------------------------------------------------------------------

inline ParseIterator::ParseIterator ( const ParseIterator& other )
: m_pInfo( other.m_pInfo )
, m_pos( other.m_pos )
, m_line( other.m_line )
{
}

//-----------------------------------------------------------------------------

inline ParseIterator::~ParseIterator ( void )
{
}

//-----------------------------------------------------------------------------

inline ParseIterator& ParseIterator::operator= ( const ParseInfo& info )
{
  m_pInfo = &info;
  m_pos   = 0U;
  m_line  = 1U;

  return *this;
}

//-----------------------------------------------------------------------------

inline ParseIterator& ParseIterator::operator= ( const ParseIterator& other )
{
  if ( this != &other )
  {
    m_pInfo = other.m_pInfo;
    m_pos   = other.m_pos;
    m_line  = other.m_line;
  }

  return *this;
}

//-----------------------------------------------------------------------------

inline void ParseIterator::info ( const ParseInfo& info )
{
  m_pInfo = &info;
}

//-----------------------------------------------------------------------------

inline void ParseIterator::pos ( unsigned int newPos )
{
  m_pos = newPos;

  assert( m_pos < m_pInfo->text.length() );
}

//-----------------------------------------------------------------------------

inline void ParseIterator::line ( unsigned int newLine )
{
  m_line = newLine;
}

//-----------------------------------------------------------------------------

inline const ParseInfo& ParseIterator::info ( void ) const
{
  assert( 0 != m_pInfo );

  return *m_pInfo;
}

//-----------------------------------------------------------------------------

inline unsigned int ParseIterator::pos ( void ) const
{
  return m_pos;
}

//-----------------------------------------------------------------------------

inline unsigned int ParseIterator::line ( void ) const
{
  return m_line;
}

//-----------------------------------------------------------------------------

inline bool ParseIterator::eatCharacter ( void )
{
  return eatCharacters( 1U );
}

//-----------------------------------------------------------------------------

inline void ParseIterator::reset ( void )
{
  m_pos  = 0U;
  m_line = 1U;
}


//--- implementation ( ParseResultBuffer ) ------------------------------------

inline ParseResultBuffer::ParseResultBuffer ( void )
: m_refCount( 0U )
, m_type( ParseResultType::success )
, m_iter()
, m_messages()
{
}

//-----------------------------------------------------------------------------

inline ParseResultBuffer::ParseResultBuffer ( const ParseIterator& iter,
                                              const string&    message )
: m_refCount( 0U )
, m_type( ParseResultType::success )
, m_iter( iter )
, m_messages()
{
  m_messages.push_back( message );
}

//-----------------------------------------------------------------------------

inline ParseResultBuffer::ParseResultBuffer ( const ParseResultBuffer* pOther )
: m_refCount( 0U )
, m_type( pOther->m_type )
, m_iter( pOther->m_iter )
, m_messages( pOther->m_messages )
{
  assert( 0 != pOther );
}

//-----------------------------------------------------------------------------

inline ParseResultBuffer::~ParseResultBuffer ( void )
{
  assert( m_refCount == 0U );
}

//-----------------------------------------------------------------------------

inline ParseResultBuffer& ParseResultBuffer::operator= ( const ParseResultBuffer* pOther )
{
  assert( 0 != pOther );

  if ( this != pOther )
  {
    m_type     = pOther->m_type;
    m_iter     = pOther->m_iter;
    m_messages = pOther->m_messages;
  }

  return *this;
}

//-----------------------------------------------------------------------------

inline void ParseResultBuffer::addRef ( void ) const
{
  ++m_refCount;
}

//-----------------------------------------------------------------------------

inline void ParseResultBuffer::release ( void ) const
{
  assert( m_refCount > 0U );

  if ( --m_refCount == 0U )
  {
    delete this;
  }
}

//-----------------------------------------------------------------------------

inline unsigned int ParseResultBuffer::refCount ( void ) const
{
  return m_refCount;
}

//-----------------------------------------------------------------------------

inline ParseResultType::Type ParseResultBuffer::type ( void ) const
{
  return m_type;
}


//--- implementation ( ParseResult ) ------------------------------------------

inline void ParseResult::prepareWrite ( void )
{
  if ( m_pBuffer == 0 )
  {
    m_pBuffer = new ParseResultBuffer();
  }
}

//-----------------------------------------------------------------------------

inline ParseResult::ParseResult ( void )
: m_pBuffer()
{
}

//-----------------------------------------------------------------------------

inline ParseResult::ParseResult ( const ParseIterator& iter, const string& message )
: m_pBuffer( new ParseResultBuffer( iter, message ) )
{
}

//-----------------------------------------------------------------------------

inline ParseResult::ParseResult ( const ParseResult& other )
: m_pBuffer( other.m_pBuffer )
{
}

//-----------------------------------------------------------------------------

inline ParseResult::~ParseResult ( void )
{
}

//-----------------------------------------------------------------------------

inline ParseResult& ParseResult::operator= ( const ParseResult& other )
{
  if ( this != &other )
  {
    m_pBuffer = other.m_pBuffer;
  }

  return *this;
}

//-----------------------------------------------------------------------------

inline void ParseResult::error ( const ParseIterator& iter, const string& message )
{
  prepareWrite();

  assert( m_pBuffer != 0 );
  m_pBuffer->error( iter, message );
}

//-----------------------------------------------------------------------------

inline ParseResultType::Type ParseResult::type ( void ) const
{
  if ( m_pBuffer != 0 )
  {
    return m_pBuffer->type();
  }

  return ParseResultType::success;
}

//-----------------------------------------------------------------------------

inline ParseResult::operator bool ( void ) const
{
  return ( type() == ParseResultType::success );
}

//-----------------------------------------------------------------------------

inline string ParseResult::formattedMessage ( void ) const
{
  if ( m_pBuffer != 0 )
  {
    return m_pBuffer->formattedMessage();
  }

  return string();
}

//-----------------------------------------------------------------------------

inline void ParseResult::reset ( void )
{
  m_pBuffer = 0;
}

#endif // PARSABLE_HPP_
