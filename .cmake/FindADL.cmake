# Module for locating AMD Display Library (ADL)
#
# Customizable variables:
#   ADL_ROOT_DIR
#     Specifies ADL's root directory. The find module uses this variable to
#     locate ADL header files. The variable will be filled automatically unless
#     explicitly set using CMake's -D command-line option. Instead of setting a
#     CMake variable, an environment variable called ADLROOT can be used.
#
# Read-only variables:
#   ADL_FOUND
#     Indicates whether ADL has been found.
#
#   ADL_INCLUDE_DIR
#     Specifies the ADL include directories.
#
#   ADL_LIBRARY
#     Specifies the ADL libraries that should be passed to
#     target_link_libararies.

INCLUDE (FindPackageHandleStandardArgs)

FIND_PATH (ADL_INCLUDE_DIR
  NAMES adl_sdk.h
        ADL/adl_sdk.h
        adl/adl_sdk.h
  PATHS $ENV{ADLROOT}
        ${ADL_ROOT_DIR}
  DOC "AMD Display Library (ADL) include directory")

FIND_PATH (ADL_LIBRARY_DIR
  NAMES libatiadlxx.so
	/usr/lib/fglrx
  PATHS /usr/lib
  DOC "AMD Display Library (ADL) directory")

FIND_LIBRARY(ADL_LIBRARY
  NAMES atiadlxx
        atiadlxy
  HINTS ${ADL_LIBRARY_DIR}
  NO_DEFAULT_PATH
  DOC "AMD Display Library (ADL) location")

MESSAGE("** AMD Display Library (ADL) root directory .....: " ${ADL_ROOT_DIR})
IF (ADL_INCLUDE_DIR)
  MESSAGE("** AMD Display Library (ADL) include directory...: " ${ADL_INCLUDE_DIR})
ENDIF (ADL_INCLUDE_DIR)
IF (ADL_LIBRARY)
  MESSAGE("** AMD Display Library (ADL) library directory...: " ${ADL_LIBRARY_DIR})
  MESSAGE("** AMD Display Library (ADL) library.............: " ${ADL_LIBRARY})
ELSE (ADL_LIBRARY)
  MESSAGE("** AMD Display Library (ADL) library.............: NOT FOUND")
  MESSAGE("")
  MESSAGE("If your have an AMD GPU, please install AMD Device Drivers!")
  MESSAGE("Otherwise, you should configure BOSP to DISABLE AMD Power Management")
  MESSAGE("")
ENDIF (ADL_LIBRARY)

# MARK_AS_ADVANCED (ADL_INCLUDE_DIR ADL_LIBRARY)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(ADL DEFAULT_MSG ADL_INCLUDE_DIR)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ADL DEFAULT_MSG ADL_LIBRARY)
