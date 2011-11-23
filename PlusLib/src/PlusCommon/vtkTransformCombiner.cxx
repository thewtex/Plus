/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/

#include "PlusConfigure.h"
#include "vtkTransformCombiner.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkTransformCombiner);

//----------------------------------------------------------------------------
vtkTransformCombiner::TransformInfo::TransformInfo()
{
  m_Transform=vtkTransform::New();
  m_IsValid=true;
  m_IsComputed=false;
}

//----------------------------------------------------------------------------
vtkTransformCombiner::TransformInfo::~TransformInfo()
{
  if (m_Transform!=NULL)
  {
    m_Transform->Delete();
    m_Transform=NULL;
  }
}

//----------------------------------------------------------------------------
vtkTransformCombiner::TransformInfo::TransformInfo(const TransformInfo& obj)
{
  m_Transform=obj.m_Transform;
  if (m_Transform!=NULL)
  {
    m_Transform->Register(NULL);
  }
  m_IsComputed=obj.m_IsComputed;
  m_IsValid=obj.m_IsValid;
}
//----------------------------------------------------------------------------
vtkTransformCombiner::TransformInfo& vtkTransformCombiner::TransformInfo::operator=(const TransformInfo& obj) 
{
  if (m_Transform!=NULL)
  {
    m_Transform->Delete();
    m_Transform=NULL;
  }
  m_Transform=obj.m_Transform;
  if (m_Transform!=NULL)
  {
    m_Transform->Register(NULL);
  }
  m_IsComputed=obj.m_IsComputed;
  m_IsValid=obj.m_IsValid;
  return *this;
}

//----------------------------------------------------------------------------
vtkTransformCombiner::vtkTransformCombiner()
{
}

//----------------------------------------------------------------------------
vtkTransformCombiner::~vtkTransformCombiner()
{
}

//----------------------------------------------------------------------------
void vtkTransformCombiner::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  for (CoordinateFrameListType::iterator coordFrame=this->CoordinateFrames.begin(); coordFrame!=this->CoordinateFrames.end(); ++coordFrame)
  {
    os << indent << coordFrame->first << " coordinate frame transforms:\n";
    for (TransformListType::iterator transformInfo=coordFrame->second.m_Transforms.begin(); transformInfo!=coordFrame->second.m_Transforms.end(); ++transformInfo)
    {
      os << indent << "  To " << transformInfo->first << ": " 
        << (transformInfo->second.m_IsValid?"valid":"invalid") << ", " 
        << (transformInfo->second.m_IsComputed?"computed":"original") << "\n";
      if (transformInfo->second.m_Transform!=NULL && transformInfo->second.m_Transform->GetMatrix()!=NULL)
      {
        vtkMatrix4x4* transformMx=transformInfo->second.m_Transform->GetMatrix();
        os << indent << "     " << transformMx->Element[0][0] << " " << transformMx->Element[0][1] << " " << transformMx->Element[0][2] << " " << transformMx->Element[0][3] << " "<< "\n";
        os << indent << "     " << transformMx->Element[1][0] << " " << transformMx->Element[1][1] << " " << transformMx->Element[1][2] << " " << transformMx->Element[1][3] << " "<< "\n";
        os << indent << "     " << transformMx->Element[2][0] << " " << transformMx->Element[2][1] << " " << transformMx->Element[2][2] << " " << transformMx->Element[2][3] << " "<< "\n";
        os << indent << "     " << transformMx->Element[3][0] << " " << transformMx->Element[3][1] << " " << transformMx->Element[3][2] << " " << transformMx->Element[3][3] << " "<< "\n";
      }
      else
      {
        os << indent << "     No transform is available\n";
      }      
    }
  }
  /*
  os << indent << "Member: " << this->Member << "\n";
  this->Member2->PrintSelf(os,indent.GetNextIndent());
  */
}

//----------------------------------------------------------------------------
vtkTransformCombiner::TransformInfo* vtkTransformCombiner::GetInputTransform(const char* fromCoordFrameName, const char* toCoordFrameName)
{
  CoordinateFrameInfo& fromCoordFrame=this->CoordinateFrames[fromCoordFrameName];
  CoordinateFrameInfo& toCoordFrame=this->CoordinateFrames[toCoordFrameName];

  // Check if the transform already exist
  TransformListType::iterator fromToTransformInfoIt=fromCoordFrame.m_Transforms.find(toCoordFrameName);
  if (fromToTransformInfoIt!=fromCoordFrame.m_Transforms.end())
  {
    // transform is found
    return &(fromToTransformInfoIt->second);
  }
  // transform is not found
  return NULL;
}

//----------------------------------------------------------------------------
PlusStatus vtkTransformCombiner::SetTransform(const char* fromCoordFrameName, const char* toCoordFrameName, vtkMatrix4x4* matrix, TransformStatus status /* = KEEP_CURRENT_STATUS*/ )
{
  if (fromCoordFrameName==NULL)
  {
    LOG_ERROR("From coordinate frame name is invalid");
    return PLUS_FAIL;
  }
  if (toCoordFrameName==NULL)
  {
    LOG_ERROR("To coordinate frame name is invalid");
    return PLUS_FAIL;
  }

  // Check if the transform already exist
  TransformInfo* fromToTransformInfo=GetInputTransform(fromCoordFrameName, toCoordFrameName);
  if (fromToTransformInfo!=NULL)
  {
    // Transform already exists
    if (fromToTransformInfo->m_IsComputed)
    {
      // The transform already exists and it is computed (not original), so reject the transformation update
      LOG_ERROR("The "<<fromCoordFrameName<<"To"<<toCoordFrameName<<" transform cannot be set, as the inverse ("
        <<toCoordFrameName<<"To"<<fromCoordFrameName<<") transform already exists");
      return PLUS_FAIL;
    }
    // This is an original transform that already exists, just update it
    // Update the matrix (the inverse matrix is automatically updated using vtkTransform pipeline)
    if (matrix!=NULL)
    {
      fromToTransformInfo->m_Transform->SetMatrix(matrix);
    }
    // Update the status
    if (status!=KEEP_CURRENT_STATUS)
    {
      // Set the status of the original transform
      fromToTransformInfo->m_IsValid=(status==TRANSFORM_VALID);
      // Set the same status for the computed inverse transform
      TransformInfo* toFromTransformInfo=GetInputTransform(toCoordFrameName, fromCoordFrameName);
      if (toFromTransformInfo==NULL)
      {
        LOG_ERROR("The computed "<<toCoordFrameName<<"To"<<fromCoordFrameName<<" transform is missing. Cannot set its status");
        return PLUS_FAIL;
      }
      toFromTransformInfo->m_IsValid=(status==TRANSFORM_VALID);      
    }
    return PLUS_SUCCESS;
  }
  // The transform does not exist yet, add it now
  // :TODO: add circle check
  // Create the from->to transform
  CoordinateFrameInfo& fromCoordFrame=this->CoordinateFrames[fromCoordFrameName];
  fromCoordFrame.m_Transforms[toCoordFrameName].m_IsComputed=false;
  if (matrix!=NULL)
  {
    fromCoordFrame.m_Transforms[toCoordFrameName].m_Transform->SetMatrix(matrix);
  }
  if (status!=KEEP_CURRENT_STATUS)
  {
    fromCoordFrame.m_Transforms[toCoordFrameName].m_IsValid=(status==TRANSFORM_VALID);
  }
  // Create the to->from inverse transform
  CoordinateFrameInfo& toCoordFrame=this->CoordinateFrames[toCoordFrameName];
  toCoordFrame.m_Transforms[fromCoordFrameName].m_IsComputed=true;
  toCoordFrame.m_Transforms[fromCoordFrameName].m_Transform->SetInput(fromCoordFrame.m_Transforms[toCoordFrameName].m_Transform);
  toCoordFrame.m_Transforms[fromCoordFrameName].m_Transform->Inverse();
  if (status!=KEEP_CURRENT_STATUS)
  {
    toCoordFrame.m_Transforms[toCoordFrameName].m_IsValid=(status==TRANSFORM_VALID);
  }
  return PLUS_SUCCESS;
}
  
//----------------------------------------------------------------------------
PlusStatus vtkTransformCombiner::SetTransformStatus(const char* fromCoordFrameName, const char* toCoordFrameName, TransformStatus status)
{
  return SetTransform(fromCoordFrameName, toCoordFrameName, NULL, status);
}
  
//----------------------------------------------------------------------------
PlusStatus vtkTransformCombiner::GetTransform(const char* fromCoordFrameName, const char* toCoordFrameName, vtkMatrix4x4* matrix, TransformStatus* status /*=NULL*/ )
{
  if (fromCoordFrameName==NULL)
  {
    LOG_ERROR("From coordinate frame name is invalid");
    return PLUS_FAIL;
  }
  if (toCoordFrameName==NULL)
  {
    LOG_ERROR("To coordinate frame name is invalid");
    return PLUS_FAIL;
  }
  // Check if we can find the transform directly among the user-defined input transforms
  TransformInfo* fromToTransformInfo=GetInputTransform(fromCoordFrameName, toCoordFrameName);
  if (fromToTransformInfo!=NULL)
  {
    // transform found
    if (matrix!=NULL)
    {
      matrix->DeepCopy(fromToTransformInfo->m_Transform->GetMatrix());
    }
    if (status!=NULL)
    {
      (*status)=fromToTransformInfo->m_IsValid?TRANSFORM_VALID:TRANSFORM_INVALID;
    }
    return PLUS_SUCCESS;
  }
  // Check if we can find the transform by combining the input transforms (could be cached to improve performance)
  TransformInfoListType transformInfoList;
  if (FindPath(fromCoordFrameName, toCoordFrameName, transformInfoList)!=PLUS_SUCCESS)
  {
    LOG_ERROR("Not enough transforms defined to get the transformation from "<<fromCoordFrameName<<" to "<<toCoordFrameName);
    return PLUS_FAIL;
  }
  // Create transform chain
  vtkSmartPointer<vtkTransform> combinedTransform=vtkSmartPointer<vtkTransform>::New();
  bool combinedTransformValid=true;
  for (TransformInfoListType::iterator transformInfo=transformInfoList.begin(); transformInfo!=transformInfoList.end(); ++transformInfo)
  {
    combinedTransform->Concatenate((*transformInfo)->m_Transform);
    if (!(*transformInfo)->m_IsValid)
    {
      combinedTransformValid=false;
    }
  }
  if (matrix!=NULL)
  {
    matrix->DeepCopy(combinedTransform->GetMatrix());
  }
  if (status!=NULL)
  {
    (*status)=combinedTransformValid?TRANSFORM_VALID:TRANSFORM_INVALID;
  }
  return PLUS_SUCCESS;
}
  
//----------------------------------------------------------------------------
PlusStatus vtkTransformCombiner::GetTransformStatus(const char* fromCoordFrameName, const char* toCoordFrameName, TransformStatus &status)
{
  return GetTransform(fromCoordFrameName, toCoordFrameName, NULL, &status);
}

PlusStatus vtkTransformCombiner::FindPath(const char* fromCoordFrameName, const char* toCoordFrameName, TransformInfoListType &transformInfoList, const char* skipCoordFrameName /*=NULL*/, bool silent /*=false*/)
{
  TransformInfo* fromToTransformInfo=GetInputTransform(fromCoordFrameName, toCoordFrameName);
  if (fromToTransformInfo!=NULL)
  {
    // found a transform
    transformInfoList.push_back(fromToTransformInfo);
    return PLUS_SUCCESS;
  }
  // not found, so try to find a path through all the connected coordinate frames
  CoordinateFrameInfo& fromCoordFrame=this->CoordinateFrames[fromCoordFrameName];
  for (TransformListType::iterator transformInfoIt=fromCoordFrame.m_Transforms.begin(); transformInfoIt!=fromCoordFrame.m_Transforms.end(); ++transformInfoIt)
  {
    if (skipCoordFrameName!=NULL && transformInfoIt->first.compare(skipCoordFrameName)==0)
    {
      // coordinate frame shall be ignored
      // (probably it would just go back to the previous coordinate frame where we come from)
      continue;
    }
    if (FindPath(transformInfoIt->first.c_str(), toCoordFrameName, transformInfoList, fromCoordFrameName, true /*silent*/)==PLUS_SUCCESS)
    {
      transformInfoList.push_back(&(transformInfoIt->second));
      return PLUS_SUCCESS;      
    }
  }
  if (!silent)
  {
    LOG_ERROR("Path not found from "<<fromCoordFrameName<<" to "<<toCoordFrameName);
  }
  return PLUS_FAIL;
}
