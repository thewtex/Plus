/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/

#ifndef __PlusCommon_h
#define __PlusCommon_h

#include "itkImageIOBase.h"
#include "vtkOutputWindow.h"
#include "vtkPlusLogger.h"
#include "vtkPlusMacro.h"
#include "vtksys/SystemTools.hxx"
#include <strstream>

enum PlusStatus
{   
  PLUS_FAIL=0,
  PLUS_SUCCESS=1
};

enum PlusImagingMode
{
  Plus_UnknownMode,
  Plus_BMode,
  Plus_RfMode
};

#define UNDEFINED_TIMESTAMP DBL_MAX

/* Define case insensitive string compare for Windows. */
#if defined( _WIN32 ) && !defined(__CYGWIN__)
#  if defined(__BORLANDC__)
#    define STRCASECMP stricmp
#  else
#    define STRCASECMP _stricmp
#  endif
#else
#  define STRCASECMP strcasecmp
#endif

/* Define round function */
#define ROUND(x) (static_cast<int>(floor( x + 0.5 )))

///////////////////////////////////////////////////////////////////
// Logging

#define LOG_ERROR(msg) \
  { \
  std::ostrstream msgStream; \
  msgStream << " " << msg << std::ends; \
  vtkPlusLogger::Instance()->LogMessage(vtkPlusLogger::LOG_LEVEL_ERROR, msgStream.str(), __FILE__, __LINE__); \
  msgStream.rdbuf()->freeze(0); \
  }  

#define LOG_WARNING(msg) \
  { \
  std::ostrstream msgStream; \
  msgStream << " " << msg << std::ends; \
  vtkPlusLogger::Instance()->LogMessage(vtkPlusLogger::LOG_LEVEL_WARNING, msgStream.str(), __FILE__, __LINE__); \
  msgStream.rdbuf()->freeze(0); \
  }
    
#define LOG_INFO(msg) \
  { \
  std::ostrstream msgStream; \
  msgStream << " " << msg << std::ends; \
  vtkPlusLogger::Instance()->LogMessage(vtkPlusLogger::LOG_LEVEL_INFO, msgStream.str(), __FILE__, __LINE__); \
  msgStream.rdbuf()->freeze(0); \
  }
  
#define LOG_DEBUG(msg) \
  { \
  std::ostrstream msgStream; \
  msgStream << " " << msg << std::ends; \
  vtkPlusLogger::Instance()->LogMessage(vtkPlusLogger::LOG_LEVEL_DEBUG, msgStream.str(), __FILE__, __LINE__); \
  msgStream.rdbuf()->freeze(0); \
  }  
  
#define LOG_TRACE(msg) \
  { \
  std::ostrstream msgStream; \
  msgStream << " " << msg << std::ends; \
  vtkPlusLogger::Instance()->LogMessage(vtkPlusLogger::LOG_LEVEL_TRACE, msgStream.str(), __FILE__, __LINE__); \
  msgStream.rdbuf()->freeze(0); \
  }

#define LOG_DYNAMIC(msg, logLevel) \
{ \
  std::ostrstream msgStream; \
  msgStream << " " << msg << std::ends; \
  vtkPlusLogger::Instance()->LogMessage(logLevel, msgStream.str(), __FILE__, __LINE__); \
  msgStream.rdbuf()->freeze(0); \
  }
  
/////////////////////////////////////////////////////////////////// 

/*!
  \class PlusLockGuard
  \brief A class for automatically unlocking objects
  
  This class is used for locking a objects (buffers, mutexes, etc.)
  and releasing the lock automatically when the guard object is deleted
  (typically by getting out of scope).
  
  Example:
  \code
  PlusLockGuard<vtkRecursiveCriticalSection> updateMutexGuardedLock(this->UpdateMutex);
  \endcode

  \ingroup PlusLibCommon
*/
template <typename T> class PlusLockGuard
{
public:
  PlusLockGuard(T* lockableObject)
  {
    m_LockableObject=lockableObject;
    m_LockableObject->Lock();
  }
  ~PlusLockGuard()
  {    
    m_LockableObject->Unlock();
    m_LockableObject=NULL;
  }
private:
  PlusLockGuard(PlusLockGuard&);
  void operator=(PlusLockGuard&);

  T* m_LockableObject;
};

/*! 
  \def DELETE_IF_NOT_NULL(Object)
  \brief A macro to safely delete a VTK object (usable if the VTK object pointer is already NULL).
  \ingroup PlusLibCommon
*/
#define DELETE_IF_NOT_NULL( Object ) {\
  if ( Object != NULL ) {\
    Object->Delete();\
    Object = NULL;\
  }\
}

class vtkXMLDataElement;

namespace PlusCommon
{
  typedef itk::ImageIOBase::IOComponentType ITKScalarPixelType;
  typedef int VTKScalarPixelType;
  typedef int IGTLScalarPixelType; 

  //----------------------------------------------------------------------------
  /*! Quick and robust string to int conversion */
  template<class T>
  VTK_EXPORT PlusStatus StringToInt(const char* strPtr, T &result)
  {
    if (strPtr==NULL || strlen(strPtr) == 0 )
    {
      return PLUS_FAIL;
    }
    char * pEnd=NULL;
    result = static_cast<int>(strtol(strPtr, &pEnd, 10)); 
    if (pEnd != strPtr+strlen(strPtr) ) 
    {
      return PLUS_FAIL;
    }
    return PLUS_SUCCESS;
  }

  //----------------------------------------------------------------------------
  /*! Quick and robust string to double conversion */
  template<class T>
  VTK_EXPORT PlusStatus StringToDouble(const char* strPtr, T &result)
  {
    if (strPtr==NULL || strlen(strPtr) == 0 )
    {
      return PLUS_FAIL;
    }
    char * pEnd=NULL;
    result = strtod(strPtr, &pEnd);
    if (pEnd != strPtr+strlen(strPtr)) 
    {
      return PLUS_FAIL;
    }
    return PLUS_SUCCESS;
  }

  //----------------------------------------------------------------------------
  /*! Quick and robust string to int conversion */
  template<class T>
  VTK_EXPORT PlusStatus StringToLong(const char* strPtr, T &result)
  {
    if (strPtr==NULL || strlen(strPtr) == 0 )
    {
      return PLUS_FAIL;
    }
    char * pEnd=NULL;
    result = strtol(strPtr, &pEnd, 10);
    if (pEnd != strPtr+strlen(strPtr) ) 
    {
      return PLUS_FAIL;
    }
    return PLUS_SUCCESS;
  }

  VTK_EXPORT void SplitStringIntoTokens(const std::string &s, char delim, std::vector<std::string> &elems);

  VTK_EXPORT PlusStatus CreateTemporaryFilename( std::string& aString, const std::string& anOutputDirectory );

  /*! Trim whitespace characters from the left and right */
  VTK_EXPORT void Trim(std::string &str);
  
  VTK_EXPORT std::string Trim(const char* c);

  /*!
    Writes an XML element to file. The output is nicer that with the built-in vtkXMLDataElement::PrintXML, as
    there are no extra lines, if there are many attributes then each of them is printed on separate line, and
    matrix elements (those that contain Matrix or Transform in the attribute name and 16 numerical elements in the attribute value)
    are printed in 4 lines.
  */
  VTK_EXPORT PlusStatus PrintXML(const char* fname, vtkXMLDataElement* elem);
  /*!
    Writes an XML element to a stream. The output is nicer that with the built-in vtkXMLDataElement::PrintXML, as
    there are no extra lines, if there are many attributes then each of them is printed on separate line, and
    matrix elements (those that contain Matrix or Transform in the attribute name and 16 numerical elements in the attribute value)
    are printed in 4 lines.
  */
  VTK_EXPORT PlusStatus PrintXML(ostream& os, vtkIndent indent, vtkXMLDataElement* elem);

#if (VTK_MAJOR_VERSION < 6)
  /*!
    Workaround for vtkXMLDataElement::RemoveAttribute bug.
    See details in https://www.assembla.com/spaces/plus/tickets/859
  */
  VTK_EXPORT void RemoveAttribute(vtkXMLDataElement* elem, const char *name);
#endif

  VTK_EXPORT std::string GetPlusLibVersionString();
};

/*!
  \class PlusTransformName 
  \brief Stores the from and to coordinate frame names for transforms 

  The PlusTransformName stores and generates the from and to coordinate frame names for transforms.
  To enable robust serialization to/from a simple string (...To...Transform), the coordinate frame names must
  start with an uppercase character and it shall not contain "To" followed by an uppercase character. E.g., valid
  coordinate frame names are Tracker, TrackerBase, Tool; invalid names are tracker, trackerBase, ToImage.

  Example usage:
  Setting a transform name:
  \code
  PlusTransformName tnImageToProbe("Image", "Probe"); 
  \endcode
  or
  \code
  PlusTransformName tnImageToProbe; 
  if ( tnImageToProbe->SetTransformName("ImageToProbe") != PLUS_SUCCESS )
  {
    LOG_ERROR("Failed to set transform name!");
    return PLUS_FAIL;
  }
  \endcode
  Getting coordinate frame or transform names:
  \code
  std::string fromFrame = tnImageToProbe->From(); 
  std::string toFrame = tnImageToProbe->To(); 
  \endcode
  or
  \code
  std::string strImageToProbe; 
  if ( tnImageToProbe->GetTransformName(strImageToProbe) != PLUS_SUCCESS )
  {
    LOG_ERROR("Failed to get transform name!");
    return PLUS_FAIL;
  }
  \endcode

  \ingroup PlusLibCommon
*/
class VTK_EXPORT PlusTransformName
{
public:
  PlusTransformName(); 
  ~PlusTransformName(); 
  PlusTransformName(std::string aFrom, std::string aTo ); 
  PlusTransformName(const std::string& transformName ); 

  /*! 
    Set 'From' and 'To' coordinate frame names from a combined transform name with the following format [FrameFrom]To[FrameTo]. 
    The combined transform name might contain only one 'To' phrase followed by a capital letter (e.g. ImageToToProbe is not allowed) 
    and the coordiante frame names should be in camel case format starting with capitalized letters. 
  */
  PlusStatus SetTransformName(const char* aTransformName); 

  /*! Return combined transform name between 'From' and 'To' coordinate frames: [From]To[To] */
  PlusStatus GetTransformName(std::string& aTransformName) const; 
  std::string GetTransformName() const; 

  /*! Return 'From' coordinate frame name, give a warning if it's not capitalized and capitalize it*/ 
  std::string From() const; 

  /*! Return 'To' coordinate frame name, give a warning if it's not capitalized and capitalize it */ 
  std::string To() const; 

  /*! Clear the 'From' and 'To' fields */
  void Clear();

  /*! Check if the current transform name is valid */ 
  bool IsValid() const; 

  bool operator== (const PlusTransformName& in) const
  {
    return (in.m_From == m_From && in.m_To == m_To ); 
  }

private: 

  /*! Check if the input string is capitalized, if not capitalize it */ 
  void Capitalize(std::string& aString ); 
  std::string m_From; /*! From coordinate frame name */
  std::string m_To; /*! To coordinate frame name */
}; 

#define RETRY_UNTIL_TRUE(command_, numberOfRetryAttempts_, delayBetweenRetryAttemptsSec_) \
  { \
    bool success = false; \
    int numOfTries = 0; \
    while ( !success && numOfTries < numberOfRetryAttempts_ ) \
    { \
      success = (command_);   \
      if (success)  \
      { \
        /* command successfully completed, continue without waiting */ \
        break; \
      } \
      /* command failed, wait for some time and retry */ \
      numOfTries++;   \
      vtkAccurateTimer::Delay(delayBetweenRetryAttemptsSec_); \
    } \
  }


#endif //__PlusCommon_h
