#include "PlusConfigure.h"

#include "vtkSmartPointer.h"
#include "vtkCommand.h"
#include "vtksys/CommandLineArguments.hxx" 
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkMath.h"
#include "vtkCalibrationController.h"

#include "FidPatternRecognition.h"

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include <stdlib.h>
#include <iostream>

typedef unsigned char PixelType;
typedef itk::Image< PixelType, 2 > ImageType;
typedef itk::Image< PixelType, 3 > ImageSequenceType;
typedef itk::ImageFileReader< ImageSequenceType > ImageSequenceReaderType;

///////////////////////////////////////////////////////////////////

int main (int argc, char* argv[])
{ 
	std::string inputTestDataPath;
	std::string outputWirePositionFile("./SegmentedWirePositions.txt");
	int inputImageType(1); 

	std::string inputBaselineFileName;
	double inputTranslationErrorThreshold(0); 
	double inputRotationErrorThreshold(0); 

	int verboseLevel=vtkPlusLogger::LOG_LEVEL_INFO;

	vtksys::CommandLineArguments cmdargs;
	cmdargs.Initialize(argc, argv);

	cmdargs.AddArgument("--input-test-data-path", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &inputTestDataPath, "Test data path");
	cmdargs.AddArgument("--input-image-type", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &inputImageType, "Image type (1=SonixVideo, 2=FrameGrabber - Default: SonixVideo");	
	cmdargs.AddArgument("--output-wire-position-file", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &outputWirePositionFile, "Result wire position file name (Default: ./SegmentedWirePositions.txt)");

	cmdargs.AddArgument("--input-baseline-file-name", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &inputBaselineFileName, "Name of file storing baseline calibration results");
	cmdargs.AddArgument("--translation-error-threshold", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &inputTranslationErrorThreshold, "Translation error threshold in mm.");	
	cmdargs.AddArgument("--rotation-error-threshold", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &inputRotationErrorThreshold, "Rotation error threshold in degrees.");	
	cmdargs.AddArgument("--verbose", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &verboseLevel, "Verbose level (1=error only, 2=warning, 3=info, 4=debug)");	

	if ( !cmdargs.Parse() )
	{
		std::cerr << "Problem parsing arguments" << std::endl;
		std::cout << "Help: " << cmdargs.GetHelp() << std::endl;
		exit(EXIT_FAILURE);
	}

	if ( inputTestDataPath.empty() ) 
	{
		std::cerr << "input-test-data-path argument required" << std::endl << std::endl;
		std::cout << "Help: " << cmdargs.GetHelp() << std::endl;
		exit(EXIT_FAILURE);

	}

	vtkPlusLogger::Instance()->SetLogLevel(verboseLevel);

	int SearchRegionXMin(0), SearchRegionXSize(0), SearchRegionYMin(0), SearchRegionYSize(0); 

	switch ( inputImageType )
	{
	case 1: // SonixVideo frames
		SearchRegionXMin = 30; 
		SearchRegionXSize = 565; 
		SearchRegionYMin = 50; 
		SearchRegionYSize = 370;

		break; 
	case 2: // FrameGrabber frames
		SearchRegionXMin = 100; 
		SearchRegionXSize = 360; 
		SearchRegionYMin = 65; 
		SearchRegionYSize = 280;    
		break; 
	}

	LOG_INFO( "Reading sequence meta file");  
  vtkSmartPointer<vtkTrackedFrameList> trackedFrameList = vtkSmartPointer<vtkTrackedFrameList>::New(); 
  if ( trackedFrameList->ReadFromSequenceMetafile(inputTestDataPath.c_str()) != PLUS_SUCCESS )
  {
      LOG_ERROR("Failed to read sequence metafile: " << inputTestDataPath); 
      return EXIT_FAILURE;
  }

	std::ofstream positionInfo;
	positionInfo.open (outputWirePositionFile.c_str(), ios::out );

	LOG_INFO( "Segmenting frames..."); 

	for ( int imgNumber = 0; imgNumber < trackedFrameList->GetNumberOfTrackedFrames(); imgNumber++ )
	{
		vtkPlusLogger::PrintProgressbar( (100.0 * imgNumber) / trackedFrameList->GetNumberOfTrackedFrames() ); 

		ImageType::Pointer frame = ImageType::New();
		ImageType::SizeType size = {trackedFrameList->GetFrameSize()[0], trackedFrameList->GetFrameSize()[1] };
		ImageType::IndexType start = {0,0};
		ImageType::RegionType region;
		region.SetSize(size);
		region.SetIndex(start);
		frame->SetRegions(region);
    try 
    {
        frame->Allocate();
    }
    catch (itk::ExceptionObject & err)
    {
        LOG_ERROR("Failed to allocate memory: " << err ); 
        continue; 
    }

    if (trackedFrameList->GetTrackedFrame(imgNumber)->GetImageData()->GetITKScalarPixelType()!=itk::ImageIOBase::UCHAR)
    {
      LOG_ERROR("vtkSegmentedWiresPositions test only works on unsigned char images");
      continue;
    }
    else
    {
      memcpy(frame->GetBufferPointer() , trackedFrameList->GetTrackedFrame(imgNumber)->GetImageData()->GetBufferPointer(), trackedFrameList->GetFrameSize()[0]*trackedFrameList->GetFrameSize()[1]*sizeof(PixelType));
    }

		vtkSmartPointer<vtkMatrix4x4> transformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
		/*if ( trackedFrameList->GetTrackedFrame(imgNumber)->GetDefaultFrameTransform(imgNumber, transformMatrix) )
		{
			LOG_ERROR("Unable to get default frame transform for frame #" << imgNumber); 
			continue; 
		}
    */
    FidPatternRecognition patternRecognition;
    PatternRecognitionResult segResults;

		try
		{
			// Send the image to the Segmentation component to segment
      if (trackedFrameList->GetTrackedFrame(imgNumber)->GetImageData()->GetITKScalarPixelType()!=itk::ImageIOBase::UCHAR)
      {
        LOG_ERROR("patternRecognition.RecognizePattern only works on unsigned char images");
      }
      else
      {
        patternRecognition.RecognizePattern( trackedFrameList->GetTrackedFrame(imgNumber), segResults );
      }
  	}
		catch(...)
		{
			LOG_ERROR("SegmentImage: The segmentation has failed for due to UNKNOWN exception thrown, the image was ignored!!!"); 
			continue; 
		}

    if ( segResults.GetDotsFound() )
		{
			vtkSmartPointer<vtkTransform> frameTransform = vtkSmartPointer<vtkTransform>::New(); 
			frameTransform->SetMatrix(transformMatrix); 

			double posZ = frameTransform->GetPosition()[2]; 
			double rotZ = frameTransform->GetOrientation()[2]; 

			int dataType = -1; 
			positionInfo << dataType << "\t\t" << posZ << "\t" << rotZ << "\t\t"; 

			for (int i=0; i < segResults.GetFoundDotsCoordinateValue().size(); i++)
			{
				positionInfo << segResults.GetFoundDotsCoordinateValue()[i][0] << "\t" << segResults.GetFoundDotsCoordinateValue()[i][1] << "\t\t"; 
			}

			positionInfo << std::endl; 

		}
	}
	
	positionInfo.close(); 
	vtkPlusLogger::PrintProgressbar(100); 
	std::cout << std::endl; 

	std::cout << "Exit success!!!" << std::endl; 
	return EXIT_SUCCESS; 
}
