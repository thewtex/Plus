
#ifndef __vtkAscension3DGTracker_h
#define __vtkAscension3DGTracker_h

#include "vtkTracker.h"

class vtkTrackerBuffer; 



class
VTK_EXPORT
vtkAscension3DGTracker : public vtkTracker
{
public:

	static vtkAscension3DGTracker *New();
	vtkTypeMacro( vtkAscension3DGTracker,vtkTracker );
	void PrintSelf( ostream& os, vtkIndent indent );

	// Description:
	// Connect to device
	int Connect();

	// Description:
	// Disconnect from device 
	virtual void Disconnect();

	// Description:
	// Probe to see if the tracking system is present on the
	// specified serial port.  If the SerialPort is set to -1, then all serial ports will be checked.
	int Probe();

	// Description:
	// Get an update from the tracking system and push the new transforms
	// to the tools.  This should only be used within vtkTracker.cxx.
	void InternalUpdate();

	// Description:
	// Read/write BrachyStepper configuration to xml data
	void ReadConfiguration( vtkXMLDataElement* config ); 
	void WriteConfiguration( vtkXMLDataElement* config ); 
  
  
protected:
  
	vtkAscension3DGTracker();
	~vtkAscension3DGTracker();

	// Description:
	// Initialize the tracking device
	bool InitAscension3DGTracker();

	// Description:
	// Start the tracking system.  The tracking system is brought from its ground state into full tracking mode.
  // The device will only be reset if communication cannot be established without a reset.
	int InternalStartTracking();

	// Description:
	// Stop the tracking system and bring it back to its ground state:
	// Initialized, not tracking, at 9600 Baud.
	int InternalStopTracking();
  
	vtkTrackerBuffer* LocalTrackerBuffer; 
	
  
private:  // Definitions.
	
	enum {TRANSMITTER_OFF = -1};
	
	// typedef std::map< std::string, std::vector < double > >  TrackerToolTransformContainerType;
	
	
private:  // Functions.
  
  vtkAscension3DGTracker( const vtkAscension3DGTracker& );
	void operator=( const vtkAscension3DGTracker& );  
	
	int CheckReturnStatus( int status );
  
  
private:  // Variables.
  	
	// TrackerToolTransformContainerType  ToolTransformBuffer;
	
	std::vector< bool > SensorSaturated;
	std::vector< bool > SensorAttached;
	std::vector< bool > SensorInMotion;
	
	bool TransmitterAttached;
	
	unsigned int FrameNumber;
  
};

#endif
