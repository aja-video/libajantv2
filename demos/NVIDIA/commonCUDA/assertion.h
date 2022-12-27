/* SPDX-License-Identifier: MIT */
#ifndef _UTIL_ASSERTION_H
#define _UTIL_ASSERTION_H

#include <exception>
#include <string>

namespace util
{
  class assertion_failure : public std::exception
  {
  public:
    explicit assertion_failure( const std::string& assertion );

    virtual ~assertion_failure() throw();

    virtual const char* what() const throw();

  private:
    std::string what_;
  };

  inline assertion_failure::assertion_failure( const std::string& assertion )
    : what_( "assertion failure: " )
  {
    what_ += assertion;
  }

  inline assertion_failure::~assertion_failure() throw()
  {
  }

  inline const char* assertion_failure::what() const throw()
  {
    return what_.c_str();
  }

  inline void assertion_check( const bool& check, const char* assertion )
  {
    if ( !check ) throw assertion_failure( assertion );
  }

  inline void assertion_check( const bool& check, const std::string& assertion )
  {
    if ( !check ) throw assertion_failure( assertion );
  }
}

#define IMPLIES(a, b) \
  (!(a) || (b))

#endif
