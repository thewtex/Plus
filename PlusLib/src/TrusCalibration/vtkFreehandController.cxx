#include "PlusConfigure.h"

#include "vtkFreehandController.h"

#include "vtkObjectFactory.h"
#include "vtkDirectory.h"
#include "vtkFileFinder.h"
#include "vtkXMLUtilities.h"

//-----------------------------------------------------------------------------

vtkFreehandController *vtkFreehandController::Instance = NULL;

//-----------------------------------------------------------------------------

vtkFreehandController* vtkFreehandController::New()
{
	return vtkFreehandController::GetInstance();
}

//-----------------------------------------------------------------------------

vtkFreehandController* vtkFreehandController::GetInstance()
{
	if(!vtkFreehandController::Instance) {
		// Try the factory first
		vtkFreehandController::Instance = (vtkFreehandController*)vtkObjectFactory::CreateInstance("vtkFreehandController");    

		if(!vtkFreehandController::Instance) {
			vtkFreehandController::Instance = new vtkFreehandController();	   
		}
	}
	// return the instance
	return vtkFreehandController::Instance;
}

//-----------------------------------------------------------------------------

vtkFreehandController::vtkFreehandController()
{
	this->DataCollector = NULL;
	this->RecordingFrameRate = 20;
	this->OutputFolder = NULL;
	this->InitializedOff();
	this->TrackingOnlyOn();
	this->Canvas = NULL;
	this->CanvasRenderer = NULL;

	VTK_LOG_TO_CONSOLE_ON
}

//-----------------------------------------------------------------------------

vtkFreehandController::~vtkFreehandController()
{
	if (this->DataCollector != NULL) {
		this->DataCollector->Stop();
	}

	this->SetDataCollector(NULL);
	this->SetCanvasRenderer(NULL);
}

//-----------------------------------------------------------------------------

PlusStatus vtkFreehandController::Initialize()
{
	LOG_TRACE("vtkFreehandController::Initialize"); 

	if (this->Initialized) {
		return PLUS_SUCCESS;
	}

	// Set up canvas renderer
	vtkSmartPointer<vtkRenderer> canvasRenderer = vtkSmartPointer<vtkRenderer>::New(); 
	canvasRenderer->SetBackground(0.1, 0.1, 0.1);
  canvasRenderer->SetBackground2(0.4, 0.4, 0.4);
  canvasRenderer->SetGradientBackground(true);
	this->SetCanvasRenderer(canvasRenderer);

	// Create directory for the output
	vtkSmartPointer<vtkDirectory> dir = vtkSmartPointer<vtkDirectory>::New(); 
	if ((this->OutputFolder != NULL) && (dir->Open(this->OutputFolder) == 0)) {	
		dir->MakeDirectory(this->OutputFolder);
	}

	this->SetInitialized(true);

	return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

void vtkFreehandController::SetTrackingOnly(bool aOn)
{
	LOG_TRACE("vtkFreehandController::SetTrackingOnly(" << (aOn ? "true" : "false") << ")");

	this->TrackingOnly = aOn;

	if (this->DataCollector != NULL) {
		this->DataCollector->SetTrackingOnly(aOn);
	}
}

//-----------------------------------------------------------------------------

vtkXMLDataElement* vtkFreehandController::GetConfigurationData()
{
	LOG_TRACE("vtkFreehandController::GetConfigurationData"); 

	if (this->DataCollector != NULL) {
		return this->DataCollector->GetConfigurationData();
  } else {
    return NULL;
  }
}

//-----------------------------------------------------------------------------

PlusStatus vtkFreehandController::StartDataCollection()
{
	LOG_TRACE("vtkFreehandController::StartDataCollection"); 

	// Stop data collection if already started
	if (this->GetDataCollector() != NULL) {
		this->GetDataCollector()->Stop();
	}

	// Initialize data collector and read configuration
	vtkSmartPointer<vtkDataCollector> dataCollector = vtkSmartPointer<vtkDataCollector>::New(); 
	this->SetDataCollector(dataCollector);

  if (this->DataCollector->ReadConfigurationFromFile(vtkFileFinder::GetInstance()->GetConfigurationFileName()) != PLUS_SUCCESS) {
		return PLUS_FAIL;
	}

	if (this->DataCollector->Initialize() != PLUS_SUCCESS) {
		return PLUS_FAIL;
	}

	if (this->DataCollector->Start() != PLUS_SUCCESS) {
		return PLUS_FAIL;
	}

	if ((this->DataCollector->GetTracker() == NULL) || (this->DataCollector->GetTracker()->GetNumberOfTools() < 1)) {
		LOG_WARNING("Unable to initialize Tracker!"); 
	}

	if (! this->DataCollector->GetInitialized()) {
		LOG_ERROR("Unable to initialize DataCollector!"); 
		return PLUS_FAIL;
	}

	return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

vtkXMLDataElement* vtkFreehandController::ParseXMLOrFillWithInternalData(const char* aFile)
{
  LOG_TRACE("vtkFreehandController::ParseXMLOrFillWithInternalData(" << aFile << ")");

	vtkXMLDataElement* rootElement = NULL;

  if ((aFile != NULL) && (vtksys::SystemTools::FileExists(aFile, true))) {
		rootElement = vtkXMLUtilities::ReadElementFromFile(aFile);

		if (rootElement == NULL) {	
			LOG_ERROR("Unable to get the configuration data from file " << aFile << " !"); 
			return NULL;
		}
	} else {
		LOG_DEBUG("Configuration file " << aFile << " does not exist, using configuration data in vtkFreehandController"); 

		rootElement = vtkFreehandController::GetInstance()->GetConfigurationData();

		if (rootElement == NULL) {	
			LOG_ERROR("Unable to get the configuration data from neither the file " << aFile << " nor from vtkFreehandController"); 
			return NULL;
		}
	}

	return rootElement;
}

//-----------------------------------------------------------------------------

PlusStatus vtkFreehandController::SaveConfigurationToFile(const char* aFile)
{
  LOG_TRACE("vtkFreehandController::SaveConfigurationToFile(" << aFile << ")");

  if ( this->DataCollector == NULL ) {
		LOG_ERROR("Data collector is NULL!");
		return PLUS_FAIL;
	}

  vtkFileFinder::GetInstance()->SetConfigurationFileName(aFile);
  
 return this->DataCollector->SaveConfigurationToFile(aFile); 
}

//-----------------------------------------------------------------------------

PlusStatus vtkFreehandController::DumpBuffersToDirectory(const char* aDirectory)
{
  LOG_TRACE("vtkFreehandController::DumpBuffersToDirectory(" << aDirectory << ")");

  if ((this->DataCollector == NULL) || (! this->DataCollector->GetInitialized())) {
		LOG_ERROR("Data collector is not initialized!");
		return PLUS_FAIL;
	}

  // Assemble file names
  std::string dateAndTime = vtksys::SystemTools::GetCurrentDateTime("%Y%m%d_%H%M%S");
  std::string outputVideoBufferSequenceFileName = "BufferDump_Video_";
  outputVideoBufferSequenceFileName.append(dateAndTime);
  std::string outputTrackerBufferSequenceFileName = "BufferDump_Tracker_";
  outputTrackerBufferSequenceFileName.append(dateAndTime);

  // Dump buffers to file 
  if ( this->DataCollector->GetVideoSource() != NULL )  {
    LOG_INFO("Write video buffer to " << outputVideoBufferSequenceFileName);
    this->DataCollector->WriteVideoBufferToMetafile( this->DataCollector->GetVideoSource()->GetBuffer(), aDirectory, outputVideoBufferSequenceFileName.c_str(), false); 
  }

  if ( this->DataCollector->GetTracker() != NULL ) {
    LOG_INFO("Write tracker buffer to " << outputTrackerBufferSequenceFileName);
    this->DataCollector->WriteTrackerToMetafile( this->DataCollector->GetTracker(), aDirectory, outputTrackerBufferSequenceFileName.c_str(), false); 
  }

  return PLUS_SUCCESS;
}
