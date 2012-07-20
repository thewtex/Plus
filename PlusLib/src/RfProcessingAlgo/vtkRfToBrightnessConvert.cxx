/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

#include "PlusConfigure.h"

#include "vtkRfToBrightnessConvert.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMath.h"

#include <math.h>

vtkStandardNewMacro(vtkRfToBrightnessConvert);

//----------------------------------------------------------------------------
vtkRfToBrightnessConvert::vtkRfToBrightnessConvert()
{
  this->ImageType=US_IMG_TYPE_XX;
  this->BrightnessScale=10.0;
  this->NumberOfHilberFilterCoeffs=64; // 128;
  PrepareHilbertTransform();
}

//----------------------------------------------------------------------------
void vtkRfToBrightnessConvert::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int ext[6], int id)
{  
  // Check input and output parameters
  if (inData[0][0]->GetNumberOfScalarComponents() != 1)
  {
    vtkErrorMacro("Expecting 1 components not " << inData[0][0]->GetNumberOfScalarComponents());
    return;
  }
  if (inData[0][0]->GetScalarType() != VTK_SHORT || 
    outData[0]->GetScalarType() != VTK_SHORT)
  {
    vtkErrorMacro("Expecting input and output to be of type short");
    return;
  }

  int wholeExtent[6]={0};
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);  
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

  short *inPtr = static_cast<short*>(inData[0][0]->GetScalarPointerForExtent(ext));
  short *outPtr = static_cast<short*>(outData[0]->GetScalarPointerForExtent(ext));

  vtkIdType inInc0=0;
  vtkIdType inInc1=0;
  vtkIdType inInc2=0;
  inData[0][0]->GetContinuousIncrements(ext, inInc0, inInc1, inInc2);
  vtkIdType outInc0=0;
  vtkIdType outInc1=0;
  vtkIdType outInc2=0;
  outData[0]->GetContinuousIncrements(ext, outInc0, outInc1, outInc2);  

  unsigned long target = static_cast<unsigned long>((ext[5]-ext[4]+1)*(ext[3]-ext[2]+1)/50.0);
  target++;

  // Temporary buffer to hold Hilbert transform results
  int numberOfSamplesInScanline=ext[1]-ext[0]+1;
  short* hilbertTransformBuffer=new short[numberOfSamplesInScanline+1];
  /*
  std::vector<short> hilbertTransformBuffer;
  hilbertTransformBuffer.resize(numberOfSamplesInScanline);
  */

  bool imageTypeValid=true;
  unsigned long count = 0;
  // loop over all the pixels (keeping track of normalized distance to origin.
  for (int idx2 = ext[4]; idx2 <= ext[5]; ++idx2)
  {
    for (int idx1 = ext[2]; !this->AbortExecute && idx1 <= ext[3]; ++idx1)    
    {
      if (id==0)
      {
        // it is the first thread, report progress
        if (!(count%target))
        {
          this->UpdateProgress(count/(50.0*target));
        }
        count++;
      }

      // Modes: IQIQIQ....., IQIQIQIQ..... / IIIIIII..., QQQQQQ.... / IIIII..., IIIII...
      switch (this->ImageType)
      {
      case US_IMG_RF_I_LINE_Q_LINE:
        {
          ComputeHilbertTransform(hilbertTransformBuffer, inPtr, numberOfSamplesInScanline);
          short *phaseShiftedSignal=hilbertTransformBuffer;
          ComputeAmplitudeILineQLine(outPtr, inPtr, phaseShiftedSignal, numberOfSamplesInScanline);
          outPtr += numberOfSamplesInScanline+outInc1;
          ComputeAmplitudeILineQLine(outPtr, inPtr, phaseShiftedSignal, numberOfSamplesInScanline);
          inPtr += 2*(numberOfSamplesInScanline+inInc1);
          outPtr += numberOfSamplesInScanline+outInc1;
          idx1++; // processed two lines
        }
        break;
      case US_IMG_RF_REAL:
        {
          ComputeHilbertTransform(hilbertTransformBuffer, inPtr, numberOfSamplesInScanline);
          ComputeAmplitudeILineQLine(outPtr, inPtr, hilbertTransformBuffer, numberOfSamplesInScanline);
          inPtr += numberOfSamplesInScanline+inInc1;
          outPtr += numberOfSamplesInScanline+outInc1;
        }
        break;
      case US_IMG_RF_IQ_LINE:
        {
          ComputeAmplitudeIqLine(outPtr, inPtr, numberOfSamplesInScanline);
          inPtr += numberOfSamplesInScanline+inInc1;
          outPtr += numberOfSamplesInScanline+outInc1;
        }
        break;
      default:
        imageTypeValid=false;
      }
    }
    inPtr += inInc2;
    outPtr += outInc2;    
  }
  if (!imageTypeValid)
  {
    LOG_ERROR("Unsupported image type for brightness conversion: "<<PlusVideoFrame::GetStringFromUsImageType(this->ImageType));
  }

  delete[] hilbertTransformBuffer;
  hilbertTransformBuffer=NULL;
}

void vtkRfToBrightnessConvert::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
PlusStatus vtkRfToBrightnessConvert::ReadConfiguration(vtkXMLDataElement* rfToBrightnessElement)
{
  LOG_TRACE("vtkRfToBrightnessConvert::ReadConfiguration"); 
  if ( rfToBrightnessElement == NULL )
  {
    LOG_DEBUG("Unable to configure vtkRfToBrightnessConvert! (XML data element is NULL)"); 
    return PLUS_FAIL; 
  }
  if (STRCASECMP(rfToBrightnessElement->GetName(), "RfToBrightnessConversion")!=NULL)
  {
    LOG_ERROR("Cannot read vtkRfToBrightnessConvert configuration: RfToBrightnessConversion element is expected"); 
    return PLUS_FAIL;
  }  

  int numberOfHilberFilterCoeffs=0;
  if ( rfToBrightnessElement->GetScalarAttribute("NumberOfHilberFilterCoeffs", numberOfHilberFilterCoeffs)) 
  {
    this->NumberOfHilberFilterCoeffs=numberOfHilberFilterCoeffs; 
  }

  double brightnessScale=0;
  if ( rfToBrightnessElement->GetScalarAttribute("BrightnessScale", brightnessScale)) 
  {
    this->BrightnessScale=brightnessScale; 
  }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------
PlusStatus vtkRfToBrightnessConvert::WriteConfiguration(vtkXMLDataElement* rfToBrightnessElement)
{
  LOG_TRACE("vtkRfToBrightnessConvert::WriteConfiguration"); 
  if ( rfToBrightnessElement == NULL )
  {
    LOG_DEBUG("Unable to write vtkRfToBrightnessConvert: XML data element is NULL"); 
    return PLUS_FAIL; 
  }
  if (STRCASECMP(rfToBrightnessElement->GetName(), "RfToBrightnessConversion")!=NULL)
  {
    LOG_ERROR("Cannot write vtkRfToBrightnessConvert configuration: RfToBrightnessConversion element is expected"); 
    return PLUS_FAIL;
  }  

  rfToBrightnessElement->SetDoubleAttribute("NumberOfHilberFilterCoeffs", this->NumberOfHilberFilterCoeffs);
  rfToBrightnessElement->SetDoubleAttribute("BrightnessScale", this->BrightnessScale);

  return PLUS_SUCCESS;
}


// From http://www.vbforums.com/archive/index.php/t-639223.html

void vtkRfToBrightnessConvert::PrepareHilbertTransform()
{
  this->HilbertTransformCoeffs.resize(this->NumberOfHilberFilterCoeffs+1);
  
  const bool debugOutput=false; // print Hilbert transform coefficients?  
  if (debugOutput)
  {
    std::cout << "hc = [ ";
  }
  for (int i=1; i<=this->NumberOfHilberFilterCoeffs; i++)
  {
    this->HilbertTransformCoeffs[i]=1/((i-this->NumberOfHilberFilterCoeffs/2)-0.5)/vtkMath::Pi();
    if (debugOutput)
    {
      std::cout << this->HilbertTransformCoeffs[i];
      if (i<this->NumberOfHilberFilterCoeffs)
      {
        std::cout << ";";
      }
      if (i%6==0)
      {
        std::cout << "\n";
      }
    }
  }
  if (debugOutput)
  {
    std::cout << "]\n";
  }
}

// time, x: input vectors
// npt: number of input points
// xh, ampl, phase, omega: output vectors
PlusStatus vtkRfToBrightnessConvert::ComputeHilbertTransform(short *hilbertTransformOutput, short *input, int npt)
{
  double pi=vtkMath::Pi();
  double pi2=2*pi;

  if (npt < this->NumberOfHilberFilterCoeffs)
  {
    LOG_ERROR("Insufficient data for performing Hilbert transform");
    return PLUS_FAIL;
  }

  // Compute Hilbert transform by convolution
  for (int l=1; l<=npt-this->NumberOfHilberFilterCoeffs+1; l++) 
  {
    double yt = 0.0;
    for (int i=1; i<=this->NumberOfHilberFilterCoeffs; i++) 
    {
      yt += input[l+i-1]*this->HilbertTransformCoeffs[this->NumberOfHilberFilterCoeffs+1-i];
    }
    hilbertTransformOutput[l] = yt;
  }

  // Shift this->NumberOfHilberFilterCoeffs/1+1/2 points
  for (int i=1; i<=npt-this->NumberOfHilberFilterCoeffs; i++) 
  {
    hilbertTransformOutput[i] = 0.5*(hilbertTransformOutput[i]+hilbertTransformOutput[i+1]);
  }
  for (int i=npt-this->NumberOfHilberFilterCoeffs; i>=1; i--)
  {
    hilbertTransformOutput[i+this->NumberOfHilberFilterCoeffs/2]=hilbertTransformOutput[i];
  }

  // Pad by zeros 
  for (int i=1; i<=this->NumberOfHilberFilterCoeffs/2; i++) 
  {
    hilbertTransformOutput[i] = 0.0;
    hilbertTransformOutput[npt+1-i] = 0.0;
  }

  return PLUS_SUCCESS;
}

void vtkRfToBrightnessConvert::ComputeAmplitudeILineQLine(short *ampl, short *inputSignal, short *inputSignalHilbertTransformed, int npt)
{
  for (int i=0; i<this->NumberOfHilberFilterCoeffs/2+1; i++)
  {
    ampl[i]=0;
  }
  for (int i=this->NumberOfHilberFilterCoeffs/2+1; i<=npt-this->NumberOfHilberFilterCoeffs/2; i++) 
  {
    double xt = inputSignal[i];
    double xht = inputSignalHilbertTransformed[i];
    ampl[i] = sqrt(sqrt(sqrt(xt*xt+xht*xht)))*this->BrightnessScale;
    if (ampl[i]>255.0) ampl[i]=255.0;
    if (ampl[i]<0.0) ampl[i]=0.0;

    /*
    phase[i] = atan2(xht ,xt);
    if (phase[i] < phase[i-1])
    {
    omega[i] = phase[i]-phase[i-1]+pi2;
    }
    else
    {
    omega[i] = phase[i]-phase[i-1];
    }
    */
  }
  for (int i=npt-this->NumberOfHilberFilterCoeffs/2+1; i<npt; i++)
  {
    ampl[i]=0;
  }
}

void vtkRfToBrightnessConvert::ComputeAmplitudeIqLine(short *ampl, short *inputSignal, const int npt)
{
  int inputIndex=0;
  int outputIndex=0;
  for (; inputIndex+1<npt; inputIndex++, outputIndex++) 
  {
    double xt = inputSignal[inputIndex++];
    double xht = inputSignal[inputIndex];
    short outputValue=sqrt(sqrt(sqrt(xt*xt+xht*xht)))*this->BrightnessScale;
    if (outputValue>255.0) outputValue=255.0;
    if (outputValue<0.0) outputValue=0.0;
    ampl[outputIndex++] = outputValue;
    ampl[outputIndex] = outputValue;
  }
}
