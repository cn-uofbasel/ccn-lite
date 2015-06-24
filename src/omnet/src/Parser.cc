// created by Thomas Meyer
// It parses a file, extracting all fields inside


#include <Parser.h>


//--- implementation ( ParseResultType ) --------------------------------------

const char* ParseResultType::NAME ( Type type )
{
  switch ( type )
  {
  case success: return "success";
  case error: return "error";
  default: return 0;
  }
}


//--- implementation ( ParseIterator ) ----------------------------------------

bool ParseIterator::skipWhitespaces ( void )
{
  char c;

  c = m_pInfo->text[ m_pos ];
  for ( ; ; )
  {
    if ( c == '#' )
    {
      // Line comment started; skip until end of line.

      do
      {
        c = m_pInfo->text[ ++m_pos ];
      }
      while ( ( c != '\r' ) && ( c != '\n' ) && ( c != '\0' ) );
    }
    else if ( c == '/' )
    {
      // Line or bock comment? examine next character.

      c = m_pInfo->text[ m_pos + 1U ];
      if ( c == '\0' )
      {
        // End of file.

        break;
      }
      else if ( c == '/' )
      {
        // Line comment started; skip until end of line.

        ++m_pos;
        do
        {
          c = m_pInfo->text[ ++m_pos ];
        }
        while ( ( c != '\r' ) && ( c != '\n' ) && ( c != '\0' ) );
      }
      else if ( m_pInfo->text[ m_pos + 1U ] == '*' )
      {
        // Block comment started; skip until end of block, but consider line
        // breaks.

        c = m_pInfo->text[ m_pos += 2U ];
        do
        {
          if ( c == '*' )
          {
            // End of block comment? examine next character.

            c = m_pInfo->text[ ++m_pos ];
            if ( c == '/' )
            {
              // End of block comment.

              c = m_pInfo->text[ ++m_pos ];
              break;
            }
          }
          else if ( c == '\r' )
          {
            // CR within block comment.

            c = m_pInfo->text[ ++m_pos ];
            if ( c == '\n' )
            {
              c = m_pInfo->text[ ++m_pos ];
            }
            ++m_line;
          }
          else if ( c == '\n' )
          {
            // LF within block comment.

            c = m_pInfo->text[ ++m_pos ];
            if ( c == '\r' )
            {
              c = m_pInfo->text[ ++m_pos ];
            }
            ++m_line;
          }
          else
          {
            c = m_pInfo->text[ ++m_pos ];
          }
        }
        while ( c != '\0' );
      }
      else
      {
        // Neither line nor block comment; this is a data character, and thus
        // stop skipping white spaces.

        break;
      }
    }
    else if ( c == '\r' )
    {
      // CR.

      c = m_pInfo->text[ ++m_pos ];
      if ( c == '\n' )
      {
        c = m_pInfo->text[ ++m_pos ];
      }
      ++m_line;
    }
    else if ( c == '\n' )
    {
      // LF.

      c = m_pInfo->text[ ++m_pos ];
      if ( c == '\r' )
      {
        c = m_pInfo->text[ ++m_pos ];
      }
      ++m_line;
    }
    else if ( isspace( c ) )
    {
      // Other whitespace (' ', '\t', or '\f'; '\r' and '\n' already handled).

      c = m_pInfo->text[ ++m_pos ];
    }
    else
    {
      // This is a data character; stop skipping white spaces.

      break;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------

char ParseIterator::peekCharacter ( void ) const
{
  assert( m_pos <= m_pInfo->text.size() );

  return m_pInfo->text[ m_pos ];
}

//-----------------------------------------------------------------------------

bool ParseIterator::eatCharacters ( unsigned int count )
{
  // Shift the current parsing position for 'count' characters towards the
  // end (if possible).

  TEST( m_pos + count <= m_pInfo->text.size() );
  m_pos += count;

  return true;

FAILED:

  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseConstCharacter ( char ch )
{
  assert( m_pos <= m_pInfo->text.size() );
  assert( ch != '\0' );

  // The character at the current position must match the specified character.

  TEST( m_pInfo->text[ m_pos ] == ch );
  ++m_pos;

  return true;

FAILED:

  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseConstCharacters ( const char* pCharacters, char& ch )
{
  const char* pCh;

  assert( m_pos <= m_pInfo->text.size() );

  // The character at the current position must match one of the specified
  // characters.

  TEST( pCh = strchr( pCharacters, m_pInfo->text[ m_pos ] ) );
  ch = *pCh;
  ++m_pos;

  return true;

FAILED:

  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseCharacter ( char& ch )
{
  assert( m_pos <= m_pInfo->text.size() );
  assert( ch != '\0' );

  // If the current position is not at the end, we return the current character
  // and move forward.

  TEST( m_pos < m_pInfo->text.size() );
  ch = m_pInfo->text[ m_pos ];
  ++m_pos;

  return true;

FAILED:

  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseConstString ( const string& str )
{
  assert( m_pos <= m_pInfo->text.size() );
  assert( str.size() > 0U  );

  // The string starting at the current position must match the specified
  // string. Only compare the strings if the remaining parse buffer is at
  // least as long as the specified string.

  TEST(    ( m_pos + str.size() <= m_pInfo->text.size() )
            && ( strncmp( &m_pInfo->text.c_str()[ m_pos ], str.c_str(), str.size() ) == 0 ) );
  m_pos += str.size();

  return true;

FAILED:

  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseWord ( string& word )
{
  unsigned int startPos = m_pos;
  char c;

  assert( m_pos <= m_pInfo->text.size() );
  c = m_pInfo->text[ m_pos ];

  // There must be a non-whitespace character at the current position...

  TEST(    !isspace( c )
            && ( c != '\0' )
            && ( c != ';' )
            && ( c != ',' )
            && ( c != '=' )
            && ( c != '\0' ) );
  c = m_pInfo->text[ ++m_pos ];

  // ...followed by zero or more non-whitespace characters.

  while (    !isspace( c )
          && ( c != ':' )
          && ( c != ';' )
          && ( c != ',' )
          && ( c != '=' )
          && ( c != '\0' ) )
  {
    c = m_pInfo->text[ ++m_pos ];
  }

  // Extract the parsed word.

  word = m_pInfo->text.substr( startPos, m_pos - startPos );

  return true;

FAILED:

  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseIdentifier ( string& identifier )
{
  unsigned int startPos = m_pos;
  char c;

  assert( m_pos <= m_pInfo->text.size() );
  c = m_pInfo->text[ m_pos ];

  // There must be a character of the set [a-zA-Z_.*?/] at the current position...

  TEST( isalpha( c ) || ( c == '_' ) || ( c == '*' ) || ( c == '.' )|| ( c == '/' ) );
  c = m_pInfo->text[ ++m_pos ];

  // ...followed by zero or more characters of the set [a-zA-Z0-9_-.:]

  while ( isalnum( c ) || ( c == '_' ) || ( c == '-' )|| ( c == '/' ) /*|| ( c == '.' ) */  || ( c == ':' ) )     // disabled the "."
  {
    c = m_pInfo->text[ ++m_pos ];
  }

  // Extract the parsed identifier.

  identifier = m_pInfo->text.substr( startPos, m_pos - startPos );

  return true;

FAILED:

  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseIdentifierWithIndex ( string& identifier )
{
  unsigned int startPos = m_pos;
  char c;

  assert( m_pos <= m_pInfo->text.size() );
  c = m_pInfo->text[ m_pos ];

  // There must a character of the set [a-zA-Z_.*?/] at the current position...

  TEST( isalpha( c ) || ( c == '_' ) || ( c == '*' ) || ( c == '.' )|| ( c == '/' ) );
  c = m_pInfo->text[ ++m_pos ];

  // ...followed by zero or more characters of the set [a-zA-Z0-9_-.[]]

  while ( isalnum( c ) || ( c == '_' ) || ( c == '-' )|| ( c == '/' ) || ( c == '.' ) || ( c == '[' ) || ( c == ']' ) )
  {
    c = m_pInfo->text[ ++m_pos ];
  }

  // Extract the parsed identifier.

  identifier = m_pInfo->text.substr( startPos, m_pos - startPos );

  return true;

FAILED:

  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseNumber ( unsigned int& value )
{
  char c;
  unsigned int v = 0U;

  assert( m_pos <= m_pInfo->text.size() );
  c = m_pInfo->text[ m_pos ];

  if ( c == '0' )
  {
    // Special case: first digit is a zero; no further digits are allowed.

    c = m_pInfo->text[ ++m_pos ];
  }
  else
  {
    // First character must be a digit...

    TEST( isdigit( c ) );

    // ...followed by further digits.

    do
    {
      v = 10U * v + static_cast<unsigned int>( c - '0' );
      c = m_pInfo->text[ ++m_pos ];
    }
    while ( isdigit( c ) );
  }

  // A point after the digits is not allowed, since this would indicate a
  // floating-point number.

  TEST( c != '.' );

  value = v;

  return true;

FAILED:

  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseNumber ( int& value )
{
  unsigned int startPos = m_pos;
  int  v        = 0;
  char c;
  int  sign;

  assert( m_pos <= m_pInfo->text.size() );
  c = m_pInfo->text[ m_pos ];

  if ( c == '0' )
  {
    // Special case: first digit is a zero; no further digits are allowed.

    c = m_pInfo->text[ ++m_pos ];
  }
  else
  {
    // Parse optional sign.

    if ( c == '+' )
    {
      sign = 1;
      c = m_pInfo->text[ ++m_pos ];
    }
    else if ( c == '-' )
    {
      sign = -1;
      c = m_pInfo->text[ ++m_pos ];
    }
    else
    {
      sign = 1;
    }

    // Character after the sign must be a digit...

    TEST( isdigit( c ) );

    // ...followed by further digits.

    do
    {
      v = 10 * v + static_cast<int>( c - '0' );
      c = m_pInfo->text[ ++m_pos ];
    }
    while ( isdigit( c ) );

    v *= sign;
  }

  // A point after the digits is not allowed, since this would indicate a
  // floating-point number.

  TEST( c != '.' );

  value = v;

  return true;

FAILED:

  m_pos = startPos;
  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseNumber ( double& value )
{
  unsigned int   startPos = m_pos;
  double v        = 0.0;
  char   c;
  double sign;

  assert( m_pos <= m_pInfo->text.size() );
  c = m_pInfo->text[ m_pos ];

  // Parse optional sign.

  if ( c == '+' )
  {
    sign = 1.0;
    c = m_pInfo->text[ ++m_pos ];
  }
  else if ( c == '-' )
  {
    sign = -1.0;
    c = m_pInfo->text[ ++m_pos ];
  }
  else
  {
    sign = 1.0;
  }

  // Character after the sign must be a digit...

  TEST( isdigit( c ) );

  // ..followed by further digits.

  do
  {
    v = 10.0 * v + static_cast<double>( c - '0' );
    c = m_pInfo->text[ ++m_pos ];
  }
  while ( isdigit( c ) );

  // Parse optional fraction point.

  if ( c == '.' )
  {
    double frac = 1.0;

    c = m_pInfo->text[ ++m_pos ];

    // Character after the fraction point must be a digit...

    TEST( isdigit( c ) );

    // ...followed by furter digits.

    do
    {
      frac *= 10.0;
      v = 10.0 * v + static_cast<double>( c - '0' );
      c = m_pInfo->text[ ++m_pos ];
    }
    while ( isdigit( c ) );

    v /= frac;
  }

  v *= sign;

  value = v;

  return true;

FAILED:

  m_pos = startPos;
  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::parseQuotation ( char chStart, char chEnd, string& content, bool returnQuotationCharacters )
{
  unsigned int startPos       = m_pos;
  unsigned int quotationCount = 0U;

  assert( m_pos <= m_pInfo->text.size() );

  // Is there an opening quotation mark?

  TEST( m_pInfo->text[ m_pos ] == chStart );
  ++m_pos;
  ++quotationCount;

  // Treat everything as content withing the matching quotation marks.

  while (    ( m_pos < m_pInfo->text.size() )
          && ( quotationCount > 0U ) )
  {
    if ( m_pInfo->text[ m_pos ] == chEnd )
    {
      --quotationCount;
    }
    else if ( m_pInfo->text[ m_pos ] == chStart )
    {
      ++quotationCount;
    }

    ++m_pos;
  }

  TEST( quotationCount == 0U );

  if ( returnQuotationCharacters )
  {
    content = m_pInfo->text.substr( startPos, m_pos - startPos );
  }
  else
  {
    content = m_pInfo->text.substr( startPos + 1U, m_pos - startPos - 2U );
  }

  return true;

FAILED:

  m_pos = startPos;
  return false;
}

//-----------------------------------------------------------------------------

bool ParseIterator::peekEnd ( void ) const
{
  assert( m_pos <= m_pInfo->text.size() );

  // Did we reached the end of the parse buffer?

  return ( m_pos >= m_pInfo->text.size() );
}


//--- implementation ( ParseResultBuffer ) ------------------------------------

void ParseResultBuffer::error ( const ParseIterator& iter, const string& message )
{
  if ( m_type == ParseResultType::success )
  {
    m_type = ParseResultType::error;
    m_iter = iter;
  }

  m_messages.push_back( message );
}

//-----------------------------------------------------------------------------

string ParseResultBuffer::formattedMessage ( void ) const
{
  string s;
  unsigned int   i;

  if ( m_type != ParseResultType::success )
  {
    if ( !m_iter.info().source.empty() )
    {
      s += m_iter.info().source;
      s += ":";
    }

    if ( m_iter.line() < UINT_MAX )
    {
      s += m_iter.line();
      s += ":";
    }

    for ( i = 0U; i < m_messages.size(); ++i )
    {
      if ( i > 0U ) { s += " - "; }
      s += m_messages[ i ];
    }
  }

  return s;
}


//--- implementation ( Parsable ) ---------------------------------------------

Parsable::Parsable ( void )
{
}

//-----------------------------------------------------------------------------

Parsable::~Parsable ( void )
{
}

//-----------------------------------------------------------------------------

void Parsable::print ( FILE* pStream ) const
{
  fprintf( pStream, "%s", text( 0U ).c_str() );
}
