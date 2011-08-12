/*=========================================================================

Program:   AtamaiTracking for VTK
Module:    $RCSfile: vtkTrackerTool.cxx,v $
Creator:   David Gobbi <dgobbi@atamai.com>
Language:  C++
Author:    $Author: glehmann $
Date:      $Date: 2006/06/26 12:08:43 $
Version:   $Revision: 1.4 $

==========================================================================

Copyright (c) 2000-2005 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
form, must retain the above copyright notice, this license,
the following disclaimer, and any notices that refer to this
license and/or the following disclaimer.  

2) Redistribution in binary form must include the above copyright
notice, a copy of this license and the following disclaimer
in the documentation or with other materials provided with the
distribution.

3) Modified copies of the source code must be clearly marked as such,
and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
ODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/

#include "PlusConfigure.h"

#include "vtkTrackerTool.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkDoubleArray.h"
#include "vtkAmoebaMinimizer.h"
#include "vtkTrackerBuffer.h"
#include "vtkObjectFactory.h"
#include "vtkXMLUtilities.h"
#include "vtkFileFinder.h"

//----------------------------------------------------------------------------
vtkTrackerTool* vtkTrackerTool::New()
{
	// First try to create the object from the vtkObjectFactory
	vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTrackerTool");
	if(ret)
	{
		return (vtkTrackerTool*)ret;
	}
	// If the factory was unable to create the object, then create it here.
	return new vtkTrackerTool;
}

//----------------------------------------------------------------------------
vtkTrackerTool::vtkTrackerTool()
{  
	this->Tracker = 0;
	this->CalibrationMatrix = vtkMatrix4x4::New();

	this->Minimizer = vtkAmoebaMinimizer::New();
	this->CalibrationArray = vtkDoubleArray::New();
	this->CalibrationArray->SetNumberOfComponents(16);

	this->LED1 = 0;
	this->LED2 = 0;
	this->LED3 = 0;

	this->ToolType = TRACKER_TOOL_NONE;
	this->ToolRevision = 0;
	this->ToolSerialNumber = 0;
	this->ToolPartNumber = 0;
	this->ToolManufacturer = 0;
	this->ToolName = 0;
	this->ToolModel = 0;
	this->Tool3DModelFileName = 0;
	this->ToolPort = 0;

	this->CalibrationMatrixName = 0; 
	this->CalibrationDate = 0; 
	this->CalibrationError = 0.0; 

	this->SendToLink = 0; 

	// Set tool enabled off by default 
	this->EnabledOff(); 

	this->ModelToToolTransform = vtkTransform::New();
	this->ModelToToolTransform->Identity();
	//this->ToolToToolReferenceTransform = vtkTransform::New();
	//this->ToolToToolReferenceTransform->Identity();
	this->SetToolName("");
	this->SetTool3DModelFileName("");
	this->SetToolModel("");
	this->SetCalibrationDate("");

	this->Buffer = vtkTrackerBuffer::New();
	this->Buffer->SetToolCalibrationMatrix(this->CalibrationMatrix);
	
	this->FrameNumber = 0;
}

//----------------------------------------------------------------------------
vtkTrackerTool::~vtkTrackerTool()
{
	this->CalibrationMatrix->Delete();
	this->CalibrationArray->Delete();
	this->Minimizer->Delete();

	this->SetTool3DModelFileName(NULL); 
	this->SetToolRevision(NULL); 
	this->SetToolSerialNumber(NULL); 
	this->SetToolManufacturer(NULL); 

	this->SetModelToToolTransform(NULL); 
	this->SetToolModel(NULL); 
	this->SetTool3DModelFileName(NULL); 
	
	if ( this->Buffer )
	{
		this->Buffer->Delete(); 
	}
}

//----------------------------------------------------------------------------
void vtkTrackerTool::PrintSelf(ostream& os, vtkIndent indent)
{
	vtkObject::PrintSelf(os,indent);

	os << indent << "Tracker: " << this->Tracker << "\n";
	os << indent << "ToolPort: " << this->ToolPort << "\n";
	os << indent << "LED1: " << this->GetLED1() << "\n"; 
	os << indent << "LED2: " << this->GetLED2() << "\n"; 
	os << indent << "LED3: " << this->GetLED3() << "\n";
	os << indent << "SendTo: " << this->GetSendToLink() << "\n";
	os << indent << "ToolType: " << this->GetToolType() << "\n";
	os << indent << "ToolModel: " << this->GetToolModel() << "\n";
	os << indent << "ModelToToolTransform: " << this->GetModelToToolTransform() << "\n";
	//os << indent << "ToolToToolReferenceTransform: " << this->GetToolToToolReferenceTransform() << "\n";
	os << indent << "ToolRevision: " << this->GetToolRevision() << "\n";
	os << indent << "ToolManufacturer: " << this->GetToolManufacturer() << "\n";
	os << indent << "ToolPartNumber: " << this->GetToolPartNumber() << "\n";
	os << indent << "ToolSerialNumber: " << this->GetToolSerialNumber() << "\n";
	os << indent << "CalibrationMatrix: " << this->CalibrationMatrix << "\n";
	this->CalibrationMatrix->PrintSelf(os,indent.GetNextIndent());
	os << indent << "Buffer: " << this->Buffer << "\n";
	this->Buffer->PrintSelf(os,indent.GetNextIndent());
}

////----------------------------------------------------------------------------
//// the update copies the latest matrix from the buffer
//void vtkTrackerTool::Update()
//{
//  TrackerBufferItem bufferItem; 
//  if ( this->Buffer->GetLatestTrackerBufferItem(&bufferItem, false) != PLUS_SUCCESS )
//  {
//    LOG_ERROR("Failed to get latest tracker buffer item!"); 
//    return PLUS_FAIL; 
//  }
//
//  this->Status = bufferItem.GetStatus(); 
//  this->TimeStamp = bufferItem.GetTimestamp( this-
//	this->Buffer->Lock();
//
//	this->Flags = this->Buffer->GetFlags(0);
//
//	if ((this->Flags & (TR_MISSING | TR_OUT_OF_VIEW | TR_REQ_TIMEOUT))  == 0) 
//	{
//		this->Buffer->GetMatrix(this->TempMatrix, 0);
//	} 
//
//	this->TimeStamp = this->Buffer->GetTimeStamp(0);
//
//	this->Buffer->Unlock();
//
//	this->Modified();
//}

//----------------------------------------------------------------------------
void vtkTrackerTool::InitializeToolTipCalibration()
{
	this->CalibrationArray->SetNumberOfTuples(0);
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackerTool::InsertNextCalibrationPoint()
{
  TrackerBufferItem bufferItem; 
	if ( this->Buffer->GetLatestTrackerBufferItem(&bufferItem, false) != ITEM_OK )
  {
    LOG_ERROR("Failed to get latest tracker item from buffer!"); 
    return PLUS_FAIL; 
  }

  vtkSmartPointer<vtkMatrix4x4> tempMatrix=vtkSmartPointer<vtkMatrix4x4>::New();
  if (bufferItem.GetMatrix(tempMatrix)!=PLUS_SUCCESS)
  {
    LOG_ERROR("Failed to get tempMatrix"); 
    return PLUS_FAIL;
  }

	this->CalibrationArray->InsertNextTuple(*tempMatrix->Element);

  return PLUS_SUCCESS; 
}

//----------------------------------------------------------------------------
void vtkTrackerToolCalibrationFunction(void *userData)
{
	vtkTrackerTool *self = (vtkTrackerTool *)userData;

	int i;
	int n = self->CalibrationArray->GetNumberOfTuples();

	double x = self->Minimizer->GetParameterValue("x");
	double y = self->Minimizer->GetParameterValue("y");
	double z = self->Minimizer->GetParameterValue("z");
	double nx,ny,nz,sx,sy,sz,sxx,syy,szz;

	double matrix[4][4];

	sx = sy = sz = 0.0;
	sxx = syy = szz = 0.0;

	for (i = 0; i < n; i++)
	{
		self->CalibrationArray->GetTuple(i,*matrix);

		nx = matrix[0][0]*x + matrix[0][1]*y + matrix[0][2]*z + matrix[0][3];
		ny = matrix[1][0]*x + matrix[1][1]*y + matrix[1][2]*z + matrix[1][3];
		nz = matrix[2][0]*x + matrix[2][1]*y + matrix[2][2]*z + matrix[2][3];

		sx += nx;
		sy += ny;
		sz += nz;

		sxx += nx*nx;
		syy += ny*ny;
		szz += nz*nz;
	}

	double r;

	if (n > 1)
	{
		r = sqrt((sxx - sx*sx/n)/(n-1) +
			(syy - sy*sy/n)/(n-1) +
			(szz - sz*sz/n)/(n-1));
	}
	else
	{
		r = 0;
	}

	self->Minimizer->SetFunctionValue(r);
	/*
	cerr << self->Minimizer->GetIterations() << " (" << x << ", " << y << ", " << z << ")" << " " << r << " " << (sxx - sx*sx/n)/(n-1) << (syy - sy*sy/n)/(n-1) << (szz - sz*sz/n)/(n-1) << "\n";
	*/
}

//----------------------------------------------------------------------------
double vtkTrackerTool::DoToolTipCalibration()
{
	this->Minimizer->SetFunction(vtkTrackerToolCalibrationFunction,this);
	this->Minimizer->SetFunctionArgDelete(NULL);
	this->Minimizer->SetParameterValue("x",0);
	this->Minimizer->SetParameterScale("x",1000);
	this->Minimizer->SetParameterValue("y",0);
	this->Minimizer->SetParameterScale("y",1000);
	this->Minimizer->SetParameterValue("z",0);
	this->Minimizer->SetParameterScale("z",1000);

	this->Minimizer->Minimize();
	double minimum = this->Minimizer->GetFunctionValue();

	double x = this->Minimizer->GetParameterValue("x");
	double y = this->Minimizer->GetParameterValue("y");
	double z = this->Minimizer->GetParameterValue("z");

	this->CalibrationMatrix->SetElement(0,3,x);
	this->CalibrationMatrix->SetElement(1,3,y);
	this->CalibrationMatrix->SetElement(2,3,z);
	this->Modified();

	return minimum;
}

//----------------------------------------------------------------------------
void vtkTrackerTool::SetLED1(int state)
{
	this->Tracker->SetToolLED(this->ToolPort,1,state);
}

//----------------------------------------------------------------------------
void vtkTrackerTool::SetLED2(int state)
{
	this->Tracker->SetToolLED(this->ToolPort,2,state);
}

//----------------------------------------------------------------------------
void vtkTrackerTool::SetLED3(int state)
{
	this->Tracker->SetToolLED(this->ToolPort,3,state);
}

//----------------------------------------------------------------------------
void vtkTrackerTool::SetTracker(vtkTracker *tracker)
{
	// The Tracker is not reference counted, since that would cause a
	// reference loop

	if (tracker == this->Tracker)
	{
		return;
	}

	if (this->Tracker)
	{
		this->Buffer->SetWorldCalibrationMatrix(NULL);
		this->Tracker = NULL;
	}

	if (tracker)
	{
		this->Tracker = tracker;
		this->Buffer->SetWorldCalibrationMatrix(tracker->GetWorldCalibrationMatrix());
	}
	else
	{
		this->Tracker = NULL;
	}

	this->Modified();
}

//----------------------------------------------------------------------------
void vtkTrackerTool::DeepCopy(vtkTrackerTool *tool)
{
	LOG_TRACE("vtkTrackerTool::DeepCopy"); 

	this->CalibrationMatrix->DeepCopy( tool->GetCalibrationMatrix() ); 

	this->SetLED1( tool->GetLED1() );
	this->SetLED2( tool->GetLED2() );
	this->SetLED3( tool->GetLED3() );

	this->SetSendToLink( tool->GetSendToLink() );

	this->SetToolType( tool->GetToolType() );
	this->SetToolModel( tool->GetToolModel() );
	this->SetTool3DModelFileName( tool->GetTool3DModelFileName() );
	this->SetToolRevision( tool->GetToolRevision() );
	this->SetToolSerialNumber( tool->GetToolSerialNumber() );
	this->SetToolPartNumber( tool->GetToolPartNumber() );
	this->SetToolManufacturer( tool->GetToolManufacturer() );
	this->SetToolName( tool->GetToolName() ); 

	this->ModelToToolTransform->DeepCopy( tool->GetModelToToolTransform() );
	//this->ToolToToolReferenceTransform->DeepCopy( tool->GetToolToToolReferenceTransform() );

	this->SetCalibrationMatrix( tool->GetCalibrationMatrix() ); 
	this->SetCalibrationMatrixName( tool->GetCalibrationMatrixName() ); 
	this->SetCalibrationDate( tool->GetCalibrationDate() ); 
	this->SetCalibrationError( tool->GetCalibrationError() ); 

	this->SetEnabled( tool->GetEnabled() ); 

	this->Buffer->DeepCopy( tool->GetBuffer() );

	this->SetFrameNumber( tool->GetFrameNumber() );
}


//-----------------------------------------------------------------------------
PlusStatus vtkTrackerTool::ReadConfiguration(vtkXMLDataElement* config)
{
	LOG_TRACE("vtkTrackerTool::ReadConfiguration"); 
	if ( config == NULL )
	{
		LOG_ERROR("Unable to configure tracker tool! (XML data element is NULL)"); 
		return PLUS_FAIL; 
	}

	const char* toolName = config->GetAttribute("Name"); 
	if ( toolName != NULL ) 
	{
		this->SetToolName(toolName); 
	}
	else
	{
		LOG_ERROR("Unable to find tool name! Name attribute is mandatory in tool definition."); 
		return PLUS_FAIL; 
	}

	const char* typeString = config->GetAttribute("Type"); 
	if ( typeString != NULL ) 
	{
		TRACKER_TOOL_TYPE type;
    if (vtkTracker::ConvertStringToToolType(typeString, type) != PLUS_SUCCESS) {
      return PLUS_FAIL;
    }
		this->SetToolType(type);
	}
	else
	{
		LOG_ERROR("Unable to find tool type! Type attribute is mandatory in tool definition."); 
		return PLUS_FAIL; 
	}

	const char* sendToLink = config->GetAttribute("SendTo"); 
	if ( sendToLink != NULL ) 
	{
		this->SetSendToLink(sendToLink); 
	}

	vtkSmartPointer<vtkXMLDataElement> toolCalibrationDataElement = config->FindNestedElementWithName("Calibration"); 
	if ( toolCalibrationDataElement != NULL ) 
	{
		const char* matrixName = toolCalibrationDataElement->GetAttribute("MatrixName"); 
		if ( matrixName != NULL ) 
		{
			this->SetCalibrationMatrixName(matrixName); 
		}

		double calibrationMatrixValue[16] = {0}; 
		if ( toolCalibrationDataElement->GetVectorAttribute("MatrixValue", 16, calibrationMatrixValue ) )
		{
			this->CalibrationMatrix->DeepCopy(calibrationMatrixValue); 
		}

		const char* calibrationDate = toolCalibrationDataElement->GetAttribute("Date"); 
		if ( calibrationDate != NULL ) 
		{
			this->SetCalibrationDate(calibrationDate); 
		}

		double calibrationError(0); 
		if ( toolCalibrationDataElement->GetScalarAttribute("Error", calibrationError) )
		{
			this->SetCalibrationError(calibrationError); 
		}
	}

	vtkSmartPointer<vtkXMLDataElement> modelDataElement = config->FindNestedElementWithName("Model"); 
	if ( modelDataElement != NULL ) 
	{
		const char* file = modelDataElement->GetAttribute("File");
    if ( (file != NULL) && (STRCASECMP(file, "") != 0) )
		{
			this->SetTool3DModelFileName(file);
		}
    else
    {
      LOG_WARNING("'" << typeString << "' CAD model has not been defined");
    }

    // ModelToToolTransform stays identity if no model file has been found
		double modelToToolTransformMatrixValue[16] = {0};
		if ( (modelDataElement->GetVectorAttribute("ModelToToolTransform", 16, modelToToolTransformMatrixValue )) && (file != NULL) && (STRCASECMP(file, "") != 0) )
		{
			this->ModelToToolTransform->SetMatrix(modelToToolTransformMatrixValue);
		}
	}

	return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------
PlusStatus vtkTrackerTool::WriteConfiguration(vtkXMLDataElement* config)
{

  if ( config == NULL )
  {
    LOG_ERROR("Unable to write calibration result: xml data element is NULL!"); 
    return PLUS_FAIL;
  }

  vtkSmartPointer<vtkXMLDataElement> trackerConfig = config->LookupElementWithName("Tracker"); 

  if ( trackerConfig == NULL )
  {
    LOG_ERROR("Unable to find Tracker xml data element!"); 
    return PLUS_FAIL; 
  }
	
  std::string toolType; 
  if ( vtkTracker::ConvertToolTypeToString(this->GetToolType(), toolType) != PLUS_SUCCESS )
  {
    LOG_ERROR("Failed to get tool type in string format!"); 
    return PLUS_FAIL; 
  }

  // Find tracker tool with type and name
  vtkSmartPointer<vtkXMLDataElement> trackerTool = trackerConfig->FindNestedElementWithNameAndAttribute("Tool", "Type", toolType.c_str() );
	if (trackerTool == NULL) 
  {
    LOG_ERROR("Unable to find tracker tool configuration file for type: " << toolType);
		return PLUS_FAIL;
	}

  vtkSmartPointer<vtkXMLDataElement> tool; 
  vtkSmartPointer<vtkXMLDataElement> calibration = trackerTool->FindNestedElementWithName("Calibration"); 
	if ( calibration == NULL ) 
  {
    // create new element and add to trackerTool 
    calibration = vtkSmartPointer<vtkXMLDataElement>::New(); 
    calibration->SetName("Calibration"); 
    calibration->SetParent(trackerTool); 
    trackerTool->AddNestedElement(calibration); 
  }

  // Set matrix name 
  calibration->SetAttribute("MatrixName", this->GetCalibrationMatrixName()); 

  // Set calibration matrix value
  double matrixValue[16] = {0}; 
  vtkMatrix4x4::DeepCopy(matrixValue, this->GetCalibrationMatrix() ); 
  calibration->SetVectorAttribute("MatrixValue",16, matrixValue); 

  // Set calibration date
  calibration->SetAttribute("Date", this->GetCalibrationDate()); 

  // Set calibration date
  calibration->SetDoubleAttribute("Error", this->GetCalibrationError()); 

  return PLUS_SUCCESS; 
}
