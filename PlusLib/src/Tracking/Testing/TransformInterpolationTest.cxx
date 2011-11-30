/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/

#include "PlusConfigure.h"
#include "PlusMath.h"
#include "vtksys/CommandLineArguments.hxx"
#include "vtkTrackerBuffer.h"
#include "vtkHTMLGenerator.h"
#include "vtkGnuplotExecuter.h"
#include "vtkTrackedFrameList.h"
#include "vtksys/SystemTools.hxx"
#include "vtkMath.h"

int main(int argc, char **argv)
{
  int numberOfErrors(0); 

	bool printHelp(false);
  std::string inputMetafile;
  std::string inputBaselineReportFilePath(""); 
  double inputMaxTranslationDifference(0.5); 
  double inputMaxRotationDifference(1.0); 

	int verboseLevel = vtkPlusLogger::LOG_LEVEL_DEFAULT;

	vtksys::CommandLineArguments args;
	args.Initialize(argc, argv);

	args.AddArgument("--help", vtksys::CommandLineArguments::NO_ARGUMENT, &printHelp, "Print this help.");	
  args.AddArgument("--input-metafile", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &inputMetafile, "Input sequence metafile.");
  args.AddArgument("--input-max-rotation-difference", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &inputMaxRotationDifference, "Maximum rotation difference in degrees (Default: 1 deg).");
  args.AddArgument("--input-max-translation-difference", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &inputMaxTranslationDifference, "Maximum translation difference (Default: 0.5 mm).");
	args.AddArgument("--verbose", vtksys::CommandLineArguments::EQUAL_ARGUMENT, &verboseLevel, "Verbose level (1=error only, 2=warning, 3=info, 4=debug, 5=trace)");	
	
	if ( !args.Parse() )
	{
		std::cerr << "Problem parsing arguments" << std::endl;
		std::cout << "Help: " << args.GetHelp() << std::endl;
		exit(EXIT_FAILURE);
	}

	if ( printHelp ) 
	{
		std::cout << "Help: " << args.GetHelp() << std::endl;
		exit(EXIT_SUCCESS); 

	}

  if ( inputMetafile.empty() )
  {
    std::cerr << "input-metafile argument required!" << std::endl; 
    std::cout << "Help: " << args.GetHelp() << std::endl;
		exit(EXIT_FAILURE); 
  }

	vtkPlusLogger::Instance()->SetLogLevel(verboseLevel);

  // Read buffer 
  LOG_INFO("Reading tracker meta file..."); 
  vtkSmartPointer<vtkTrackedFrameList> trackerFrameList = vtkSmartPointer<vtkTrackedFrameList>::New(); 
  if ( trackerFrameList->ReadFromSequenceMetafile(inputMetafile.c_str()) != PLUS_SUCCESS )
  {
    LOG_ERROR("Failed to read sequence metafile from file: " << inputMetafile ); 
  }

  LOG_INFO("Copy buffer to tracker buffer..."); 
  vtkSmartPointer<vtkTrackerBuffer> trackerBuffer = vtkSmartPointer<vtkTrackerBuffer>::New(); 
  // force recomputation of the filtered timestamps to test timestamp filtering as well
  if (trackerBuffer->CopyDefaultTransformFromTrackedFrameList(trackerFrameList, vtkTrackerBuffer::READ_UNFILTERED_COMPUTE_FILTERED_TIMESTAMPS)!=PLUS_SUCCESS)
  {
    LOG_ERROR("CopyDefaultTrackerDataToBuffer failed");
    numberOfErrors++;
  }

  vtkPlusLogger::PrintProgressbar( 100 ); 
  std::cout << std::endl; 

  // Check interpolation results 
  //****************************

  double endTime(0); 
  if ( trackerBuffer->GetLatestTimeStamp(endTime) != ITEM_OK )
  {
    LOG_ERROR("Failed to get latest timestamp from tracker buffer!"); 
    exit(EXIT_FAILURE);  
  }

  double startTime(0); 
  if ( trackerBuffer->GetOldestTimeStamp(startTime) != ITEM_OK )
  {
    LOG_ERROR("Failed to get oldest timestamp from tracker buffer!"); 
    exit(EXIT_FAILURE);  
  }

  const double frameRate = trackerBuffer->GetFrameRate(); 

  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New(); 
  vtkSmartPointer<vtkMatrix4x4> prevmatrix = vtkSmartPointer<vtkMatrix4x4>::New(); 

  for ( double newTime = startTime; newTime < endTime; newTime += 1.0 / (frameRate * 5.0) )
  {
    TrackerBufferItem bufferItem;
    if ( trackerBuffer->GetTrackerBufferItemFromTime(newTime, &bufferItem, vtkTrackerBuffer::INTERPOLATED) != ITEM_OK )
    {
      LOG_DEBUG("Failed to get tracker buffer item from time: " << std::fixed << newTime ); 
      continue; 
    }

    if ( bufferItem.GetStatus()!=TR_OK )
    {
      LOG_DEBUG("Tracker item is missing or invalid (index: " << bufferItem.GetIndex() << ")" ); 
      continue; 
    }
    
    long bufferIndex = bufferItem.GetIndex(); 

    if ( bufferItem.GetMatrix(matrix) != PLUS_SUCCESS )
    {
      LOG_ERROR("Failed to get matrix from buffer!"); 
      numberOfErrors++; 
      continue; 
    }

    if ( fabs(newTime - startTime) < 0.0001 )
    {
      // this is the first matrix, we cannot compare it with the previous one. 
      prevmatrix->DeepCopy(matrix); 
      newTime += 1.0 / (frameRate * 5.0); 
      continue; 
    }


    double rotDiff = PlusMath::GetOrientationDifference(matrix, prevmatrix); 
    if ( rotDiff > inputMaxRotationDifference )
    {
      LOG_ERROR("Rotation difference is larger than the max rotation difference (difference=" << std::fixed << rotDiff << ", threshold=" << inputMaxRotationDifference << ", itemIndex=" << bufferIndex << ", timestamp=" << newTime << ")!"); 
      numberOfErrors++; 
    }

    double transDiff = PlusMath::GetPositionDifference(matrix, prevmatrix); 
    if ( transDiff > inputMaxTranslationDifference)
    {
      LOG_ERROR("Translation difference is larger than the max translation difference (difference=" << std::fixed << transDiff << ", threshold=" << inputMaxTranslationDifference << ", itemIndex=" << bufferIndex << ", timestamp=" << newTime << ")!"); 
      numberOfErrors++; 
    }

    LOG_DEBUG("bufferIndex = " << bufferIndex << " Rotation diff = " << rotDiff << ",   translation diff = " << transDiff);

    prevmatrix->DeepCopy(matrix);      
  }

  if ( numberOfErrors != 0 )
  {
    LOG_INFO("Test failed!");
    return EXIT_FAILURE; 
  }

  LOG_INFO("Test completed successfully!");
	return EXIT_SUCCESS; 
 } 

