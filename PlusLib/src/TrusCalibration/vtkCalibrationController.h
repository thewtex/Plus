#ifndef __VTKCALIBRATIONCONTROLLER_H
#define __VTKCALIBRATIONCONTROLLER_H

#include "vtkObject.h"
#include "vtkTrackedFrameList.h"
#include "vtkMatrix4x4.h"
#include "vtkImageData.h"
#include "vtkXMLUtilities.h"
#include "itkImage.h"
#include "vnl/vnl_matrix.h"
#include "vnl/vnl_vector.h"
#include <string>
#include <vector>
#include <deque>
#include "vtkXMLDataElement.h"
#include "FidPatternRecognition.h"
#include "FidPatternRecognitionCommon.h"

enum CalibrationMode
{
	REALTIME, 
	OFFLINE
}; 

//! Description 
// Helper class for storing segmentation results with transformation
class SegmentedFrame
{
public: 
	SegmentedFrame()
	{
		this->TrackedFrameInfo = NULL; 
		DataType = TEMPLATE_TRANSLATION;
	}

	TrackedFrame* TrackedFrameInfo; 
	PatternRecognitionResult SegResults;
	IMAGE_DATA_TYPE DataType; 
};


class vtkCalibrationController : public vtkObject
{
public:
	typedef unsigned char PixelType;
	typedef itk::Image< PixelType, 2 > ImageType;
	typedef std::deque<SegmentedFrame> SegmentedFrameList;

	//! Description 
	// Helper structure for storing image dataset info
	struct ImageDataInfo
	{
		std::string OutputSequenceMetaFileSuffix;
    std::string InputSequenceMetaFileName;
		int NumberOfImagesToAcquire; 
		int NumberOfSegmentedImages; 
	};

	static vtkCalibrationController *New();
	vtkTypeRevisionMacro(vtkCalibrationController, vtkObject);
	virtual void PrintSelf(ostream& os, vtkIndent indent); 

	//! Description 
	// Initialize the calibration controller interface
	virtual PlusStatus Initialize(); 

	//! Description 
	// Read XML based configuration of the calibration controller
	virtual PlusStatus ReadConfiguration( vtkXMLDataElement* configData ); 

	//! Description 
	// Add new tracked data for segmentation and save the segmentation result to the SegmentedFrameContainer
	// The class has to be initialized before the segmentation process. 
	virtual PlusStatus AddTrackedFrameData( TrackedFrame* trackedFrame, IMAGE_DATA_TYPE dataType ); 

	//! Description 
	// VTK/VNL matrix conversion 
	static void ConvertVtkMatrixToVnlMatrixInMeter(vtkMatrix4x4* inVtkMatrix, vnl_matrix<double>& outVnlMatrix ); 

	//! Description 
	// Returns the list of tracked frames of the selected data type
	virtual vtkTrackedFrameList* GetTrackedFrameList( IMAGE_DATA_TYPE dataType ); 
	
	//! Description 
	// Save the selected data type to sequence metafile 
	virtual PlusStatus SaveTrackedFrameListToMetafile( IMAGE_DATA_TYPE dataType, const char* outputFolder, const char* sequenceMetafileName, bool useCompression = false ); 

	//! Description 
	// Flag to show the initialized state
	vtkGetMacro(Initialized, bool);
	vtkSetMacro(Initialized, bool);
	vtkBooleanMacro(Initialized, bool);

	//! Description 
	// Flag to enable the tracked sequence data saving to metafile
	vtkGetMacro(EnableTrackedSequenceDataSaving, bool);
	vtkSetMacro(EnableTrackedSequenceDataSaving, bool);
	vtkBooleanMacro(EnableTrackedSequenceDataSaving, bool);

	//! Flag to enable the erroneously segmented data saving to metafile
	vtkGetMacro(EnableErroneouslySegmentedDataSaving, bool);
	vtkSetMacro(EnableErroneouslySegmentedDataSaving, bool);
	vtkBooleanMacro(EnableErroneouslySegmentedDataSaving, bool);

	//! Flag to enable the visualization component
	vtkGetMacro(EnableVisualization, bool);
	vtkSetMacro(EnableVisualization, bool);
	vtkBooleanMacro(EnableVisualization, bool);

	//! Description 
	// Set/get the calibration mode (see CALIBRATION_MODE)
  void SetCalibrationMode( CalibrationMode mode ) { this->CalibrationMode = mode; }
  CalibrationMode GetCalibrationMode() { return this->CalibrationMode; }

  //! Description 
	// Get offline image data
	vtkGetObjectMacro(OfflineImageData, vtkImageData);
  
  //! Description 
	// Set/get the calibration date in string format
	vtkSetStringMacro(CalibrationDate); 
	vtkGetStringMacro(CalibrationDate);

  //! Flag to identify the calibration state 
	vtkGetMacro(CalibrationDone, bool);
	vtkSetMacro(CalibrationDone, bool);
	vtkBooleanMacro(CalibrationDone, bool);
  
	//! Flag to enable the Segmentation Analysis
	vtkGetMacro(EnableSegmentationAnalysis, bool);
	vtkSetMacro(EnableSegmentationAnalysis, bool);
	vtkBooleanMacro(EnableSegmentationAnalysis, bool);

	//! Description 
	// Get the segmentation result container 
	// Stores the segmentation results with transformation for each frame
	SegmentedFrameList GetSegmentedFrameContainer() { return this->SegmentedFrameContainer; };

  //! Description
  //Get the fiducial pattern recognition master object
  FidPatternRecognition * GetPatternRecognition() { return & this->PatternRecognition; };
  void SetPatternRecognition( FidPatternRecognition value) { PatternRecognition = value; };

  //! Description
  //Get the fiducial pattern recognition master object
  PatternRecognitionResult * GetPatRecognitionResult() { return & this->PatRecognitionResult; };
  void SetPatRecognitionResult( PatternRecognitionResult value) { PatRecognitionResult = value; };

  //! Description
  // Clear all datatype segmented frames from container 
  void ClearSegmentedFrameContainer(IMAGE_DATA_TYPE dataType); 

	//! Description 
	// Get/set the saved image data info
	ImageDataInfo GetImageDataInfo( IMAGE_DATA_TYPE dataType ) { return this->ImageDataInfoContainer[dataType]; }
	virtual void SetImageDataInfo( IMAGE_DATA_TYPE dataType, ImageDataInfo imageDataInfo ) { this->ImageDataInfoContainer[dataType] = imageDataInfo; }
	
	//! Description 
	// Callback function that is executed each time a segmentation is finished
	typedef void (*SegmentationProgressPtr)(int percent);
  void SetSegmentationProgressCallbackFunction(SegmentationProgressPtr cb) { SegmentationProgressCallbackFunction = cb; } 

protected:

	vtkCalibrationController();
	virtual ~vtkCalibrationController();

	//! Operation: Add frame to renderer in offline mode
	virtual PlusStatus SetOfflineImageData(vtkImageData* frame); 
	virtual PlusStatus SetOfflineImageData(const ImageType::Pointer& frame); 

	//! Description 
	// Read CalibrationController data element
	virtual PlusStatus ReadCalibrationControllerConfiguration(vtkXMLDataElement* rootElement); 

protected:
	//! Flag to enable the tracked sequence data saving to metafile
	bool EnableTrackedSequenceDataSaving;

	//! Flag to enable the erroneously segmented data saving to metafile
	bool EnableErroneouslySegmentedDataSaving; 

	//! Flag to enable the visualization component
	bool EnableVisualization; 

	//! Flag to enable the Segmentation Analysis file
	bool EnableSegmentationAnalysis; 

	//! Flag to show the initialized state
	bool Initialized; 

  //! Flag to identify the calibration state 
	bool CalibrationDone; 

	//! calibration mode (see CALIBRATION_MODE)
	CalibrationMode CalibrationMode; 

  //! calibration date in string format 
  char* CalibrationDate; 

	//! Pointer to the callback function that is executed each time a segmentation is finished
  SegmentationProgressPtr SegmentationProgressCallbackFunction;

	//! Stores the tracked frames for each data type 
	std::vector<vtkTrackedFrameList*> TrackedFrameListContainer;

	//! Stores dataset information
	std::vector<ImageDataInfo> ImageDataInfoContainer; 
	
	//! Stores the segmentation results with transformation for each frame
	SegmentedFrameList SegmentedFrameContainer; 

  //! Stores the fiducial pattern recognition master object
  FidPatternRecognition PatternRecognition;

  //! Stores the segmentation results of a single frame
  PatternRecognitionResult PatRecognitionResult;

  //! Stores the image data in offline mode
  vtkImageData* OfflineImageData; 

private:
	vtkCalibrationController(const vtkCalibrationController&);
	void operator=(const vtkCalibrationController&);
};

#endif //  __VTKCALIBRATIONCONTROLLER_H

