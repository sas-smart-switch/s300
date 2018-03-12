/////////////////////////////////////////////////////////////////////////////////////////
// This file is subject to the terms and conditions of the GNU General Public  Licence.//
// Copyright (C) 2018 Microsemi Corporation                                            //
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef MICROSEMIMACROS
#define MICROSEMIMACROS
                                 
#define MICROSEMI_CALL(method, ...) method( __VA_ARGS__ )
#define MICROSEMI_REMOVED( code ) 
#define MICROSEMI_CODE( code, description ) code
#define MICROSEMI_INCLUDE( file ) file
#define MICROSEMI_BEGIN( )
#define MICROSEMI_END( )
#define MICROSEMI_REMOVED_BEGIN( )
#define MICROSEMI_REMOVED_END( )


#define MICROSEMI_METHOD(method) method

#endif /*MICROSEMIMACROS */
