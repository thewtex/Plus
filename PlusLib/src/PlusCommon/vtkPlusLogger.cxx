#ifdef _WIN32
#include <Windows.h> // required for setting the text color on the console output
#endif

#include "PlusConfigure.h"
#include "vtkPlusLogger.h"

#include <string>
#include <sstream>

#include "vtkCriticalSection.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtksys/SystemTools.hxx" 

vtkPlusLogger* vtkPlusLogger::m_pInstance = NULL;

vtkStandardNewMacro(vtkPlusLoggerOutputWindow);

void vtkPlusLoggerOutputWindow::ReplaceNewlineBySeparator(std::string &str)
{
  std::replace( str.begin(), str.end(), '\r', '|' );
  std::replace( str.begin(), str.end(), '\n', '|' );
}

//-------------------------------------------------------
vtkPlusLoggerOutputWindow::vtkPlusLoggerOutputWindow()
{
}

//-------------------------------------------------------
vtkPlusLoggerOutputWindow::~vtkPlusLoggerOutputWindow()
{
}

//-------------------------------------------------------
void vtkPlusLoggerOutputWindow::DisplayText(const char* text)
{
  if(text==NULL)
  {
    return;
  }
  std::string textStr=text;
  ReplaceNewlineBySeparator(textStr);
  LOG_INFO("VTK log: " << text);
}

//-------------------------------------------------------
void vtkPlusLoggerOutputWindow::DisplayErrorText(const char* text)
{
  if(text==NULL)
  {
    return;
  }
  std::string textStr=text;
  ReplaceNewlineBySeparator(textStr);
  LOG_ERROR("VTK log: " << textStr);
  this->InvokeEvent(vtkCommand::ErrorEvent, (void*)text);
}

//-------------------------------------------------------
void vtkPlusLoggerOutputWindow::DisplayWarningText(const char* text)
{
  if(text==NULL)
  {
    return;
  }
  std::string textStr=text;
  ReplaceNewlineBySeparator(textStr);
  LOG_WARNING("VTK log: " << textStr);
  this->InvokeEvent(vtkCommand::WarningEvent,(void*) text);
}

//-------------------------------------------------------
void vtkPlusLoggerOutputWindow::DisplayGenericWarningText(const char* text)
{
  if(text==NULL)
  {
    return;
  }
  std::string textStr=text;
  ReplaceNewlineBySeparator(textStr);
  LOG_WARNING("VTK log: " << textStr);
}

//-------------------------------------------------------
void vtkPlusLoggerOutputWindow::DisplayDebugText(const char* text)
{
  if(text==NULL)
  {
    return;
  }
  std::string textStr=text;
  ReplaceNewlineBySeparator(textStr);
  LOG_DEBUG("VTK log: " << textStr);
} 

//-------------------------------------------------------
void vtkPlusLoggerOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VTK logs are redirected to Plus logger" << endl;
} 

//-------------------------------------------------------
vtkPlusLogger::vtkPlusLogger()
{
	m_CriticalSection = vtkSimpleCriticalSection::New();
	m_LogLevel = LOG_LEVEL_INFO;
	m_DisplayLogLevel = LOG_LEVEL_INFO;
	std::ostringstream logfilename;
  logfilename << vtkPlusConfig::GetInstance()->GetOutputDirectory() << "/" << vtkAccurateTimer::GetInstance()->GetDateAndTimeString() << "_PlusLog.txt";
  vtksys::SystemTools::MakeDirectory(vtkPlusConfig::GetInstance()->GetOutputDirectory()); 

	this->m_LogStream.open (logfilename.str().c_str(), ios::out);

  // redirect VTK error logs to the Plus logger
	vtkSmartPointer<vtkPlusLoggerOutputWindow> vtkLogger = vtkSmartPointer<vtkPlusLoggerOutputWindow>::New();
	vtkOutputWindow::SetInstance(vtkLogger);

  std::string strPlusLibVersion = std::string(" PlusLib version: ") + std::string(PLUSLIB_VERSION); 
  this->LogMessage(LOG_LEVEL_INFO, strPlusLibVersion.c_str(), "vtkPlusLogger", 122); 
}

//-------------------------------------------------------
vtkPlusLogger::~vtkPlusLogger()
{
  // Disconnect VTK error logging from the Plus logger (restore default VTK logging)
  vtkOutputWindow::SetInstance(NULL);

	if ( this->m_CriticalSection != NULL ) 
	{
		this->m_CriticalSection->Delete(); 
		this->m_CriticalSection = NULL; 
	} 

	this->m_LogStream.close(); 
}

//-------------------------------------------------------
vtkPlusLogger* vtkPlusLogger::Instance() 
{
	if (m_pInstance==NULL)
	{
		m_pInstance = new vtkPlusLogger;
	}
	return m_pInstance;
}

//-------------------------------------------------------
int vtkPlusLogger::GetLogLevel() 
{ 
	return m_LogLevel; 
}

//-------------------------------------------------------
void vtkPlusLogger::SetLogLevel(int logLevel) 
{ 
	m_CriticalSection->Lock(); 
	m_LogLevel=logLevel; 
	m_CriticalSection->Unlock(); 
}

//-------------------------------------------------------
int vtkPlusLogger::GetDisplayLogLevel() 
{ 
	return m_DisplayLogLevel; 
}

//-------------------------------------------------------
void vtkPlusLogger::SetDisplayLogLevel(int logLevel) 
{ 
	m_CriticalSection->Lock(); 
	m_DisplayLogLevel=logLevel; 
	m_CriticalSection->Unlock(); 
}

//-------------------------------------------------------
void vtkPlusLogger::LogMessage(LogLevelType level, const char *msg, const char* fileName, int lineNumber)
{
	if (m_LogLevel<level && m_DisplayLogLevel<level)
	{
		// no need to log
		return;
	}

	std::string timestamp=vtkAccurateTimer::GetInstance()->GetDateAndTimeMSecString();

	std::ostringstream log; 
	switch (level)
	{
	case LOG_LEVEL_ERROR:
		log << "[ERROR]  ";
		break;
	case LOG_LEVEL_WARNING:
		log << "[WARNING]"; 
		break;
	case LOG_LEVEL_INFO:
		log << "[INFO]   "; 
		break;
	case LOG_LEVEL_DEBUG:
		log << "[DEBUG]  "; 
		break;
	case LOG_LEVEL_TRACE:
		log << "[TRACE]  "; 
		break;
	default:
		log << "[UNKNOWN]"; 
		break;
	}

  // Add timestamp to the log message
  double currentTime = vtkAccurateTimer::GetSystemTime(); 
  log << " [" << std::fixed << std::setw(10) << std::right << std::setfill('0') << currentTime << "] "; 

	log << msg;
  if ( m_LogLevel > LOG_LEVEL_INFO || level != LOG_LEVEL_INFO )
	{
		log << " [in " << fileName << "(" << lineNumber << ")]"; // add filename and line number
	}

  m_CriticalSection->Lock(); 

	if (m_DisplayLogLevel>=level)
	{ 

#ifdef _WIN32

		// Set the text color to highlight error and warning messages (supported only on windows)
		switch (level)
		{
		case LOG_LEVEL_ERROR:  
			{
				HANDLE hStdout = GetStdHandle(STD_ERROR_HANDLE); 
				SetConsoleTextAttribute(hStdout, FOREGROUND_RED|FOREGROUND_INTENSITY);
			}
			break;
		case LOG_LEVEL_WARNING:
			{
				HANDLE hStdout = GetStdHandle(STD_ERROR_HANDLE); 
				SetConsoleTextAttribute(hStdout, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
			}
			break;
		default:
			{
				HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
				SetConsoleTextAttribute(hStdout, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
			}
			break;
		}    
#endif

		if (level>LOG_LEVEL_WARNING)
		{
			std::cout << log.str() << std::endl; 
		}
		else
		{
			std::cerr << log.str() << std::endl; 
		}

#ifdef _WIN32
		// Revert the text color (supported only on windows)
		if (level==LOG_LEVEL_ERROR || level==LOG_LEVEL_WARNING)
		{
			HANDLE hStdout = GetStdHandle(STD_ERROR_HANDLE); 
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
		}
#endif

    // Call display message callbacks if higher priority than trace
    if (level < LOG_LEVEL_TRACE)
    {
    	std::ostringstream callDataStream;
      callDataStream << level << "|" << log.str();

      InvokeEvent(vtkCommand::UserEvent, (void*)(callDataStream.str().c_str()));
    }
	}

	if (m_LogLevel>=level)
	{
    this->m_LogStream << std::setw(17) << std::left << timestamp << " " << log.str() << std::endl; 
		this->m_LogStream.flush(); 
	}

	m_CriticalSection->Unlock(); 
}


//-------------------------------------------------------
void vtkPlusLogger::PrintProgressbar( int percent )
{
	if ( vtkPlusLogger::Instance()->GetLogLevel() != vtkPlusLogger::LOG_LEVEL_INFO )
	{
		return; 
	}

	std::string bar;
	for(int i = 0; i < 50; i++)
	{
		if( i < (percent/2))
		{
			bar.replace(i,1,"=");
		}
		else if( i == (percent/2))
		{
			bar.replace(i,1,">");
		}
		else
		{
			bar.replace(i,1," ");
		}
	}

	std::cout << "\r" "[" << bar << "] ";
	std::cout.width( 3 );
	std::cout << percent << "%     " << std::flush;

	if ( percent == 100)
	{
		std::cout << std::endl << std::endl;
	}
}
