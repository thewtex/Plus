/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/ 

#include "PlusConfigure.h"

#include "vtkOpenIGTLinkBroadcaster.h"

#include <sstream>
#include <string>

#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "PlusVideoFrame.h"

#include "vtkTrackerTool.h"

#include "igtlImageMessage.h"
#include "igtlTransformMessage.h"

vtkCxxRevisionMacro( vtkOpenIGTLinkBroadcaster, "$Revision: 1.0 $" );
vtkStandardNewMacro( vtkOpenIGTLinkBroadcaster ); 

//----------------------------------------------------------------------------
vtkOpenIGTLinkBroadcaster::Status vtkOpenIGTLinkBroadcaster::SetDataCollector( vtkDataCollector* dataCollector )
{
  this->DataCollector = dataCollector;

  return this->InternalStatus;
}

//----------------------------------------------------------------------------
vtkOpenIGTLinkBroadcaster::Status vtkOpenIGTLinkBroadcaster::Initialize( std::string &strError )
{
  if (    this->DataCollector == NULL
    || this->DataCollector->GetTracker() == NULL )
  {
    LOG_ERROR( "Tried to initialize vtkOpenIGTLinkBroadcaster without DataCollector." );
    this->InternalStatus = STATUS_NOT_INITIALIZED;
    return this->InternalStatus;
  }


  // Create a socket for all non-reference tools that need to be broadcasted.

  for ( ToolIteratorType it = this->DataCollector->GetTracker()->GetToolIteratorBegin(); it != this->DataCollector->GetTracker()->GetToolIteratorEnd(); ++it)
  {
    if ( STRCASECMP(it->second->GetToolName(), "Reference") == 0 )
    {
      // We never broadcast the reference tool.
      continue; 
    }

    // Check if SendTo address exists for non reference tools.

    vtkTrackerTool* tool = it->second;
    const char* constCharSendTo = tool->GetSendToLink();

    if ( constCharSendTo == NULL ) continue;  // This tool is not broadcasted.

    // Parse the SendTo string to get hostname and port
    std::istringstream iss(constCharSendTo);
    std::string hostname;
    getline(iss, hostname, ':');
    int port=0;
    iss >> port;    

    if ( hostname.empty() || port <= 0 )
    {
      LOG_WARNING( "SendTo address could not be parsed for tool: " << tool->GetToolName()
        << " (hostname=" << hostname << ", port=" << port << ")" );
      continue;
    }      

    LOG_TRACE( "SendTo address for tool " << tool->GetToolName() << ": hostname=" << hostname << ", port=" << port );

    // Check if new socket has to be created.

    bool socketFound = false;
    igtl::ClientSocket::Pointer socket;
    for ( int socketIndex = 0; socketIndex < this->SocketInfos.size(); ++ socketIndex )
    {
      if (    hostname.compare(this->SocketInfos[ socketIndex ].Host) == 0
        && this->SocketInfos[ socketIndex ].Port == port )
      {
        socketFound = true;
        socket = this->SocketInfos[ socketIndex ].Socket;
      }
    }

    if ( socketFound == false )
    {
      socket = igtl::ClientSocket::New();
      int fail = socket->ConnectToServer( hostname.c_str(), port );

      if ( fail )
      {
        LOG_WARNING( "Could not connect to OpenIGTLink host: " << tool->GetSendToLink() );
        continue;
      }

      SocketInfo sInfo;
      sInfo.Host = std::string( hostname );
      sInfo.Port = port;
      sInfo.Socket = socket;

      this->SocketInfos.push_back( sInfo );
    }

    IgtToolInfo info;
    info.Socket = socket;
    info.ToolName = tool->GetToolName();
    info.TrackerPortName = it->second->GetPortName();

    this->NonReferenceToolInfos.push_back( info );
  }


  // Everything worked.

  LOG_DEBUG( "Number of non-reference tools = " << this->NonReferenceToolInfos.size() );


  this->InternalStatus = STATUS_OK;
  return this->InternalStatus;
}



vtkOpenIGTLinkBroadcaster::vtkOpenIGTLinkBroadcaster()
{
  this->DataCollector = NULL;
  this->InternalStatus = STATUS_NOT_INITIALIZED;
  this->ApplyStylusCalibration = false;
}



vtkOpenIGTLinkBroadcaster::~vtkOpenIGTLinkBroadcaster()
{
  for ( int i = 0; i < this->SocketInfos.size(); ++ i )
  {
    this->SocketInfos[ i ].Socket->CloseSocket();
  }

  this->SocketInfos.clear();
}



void vtkOpenIGTLinkBroadcaster::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}



void vtkOpenIGTLinkBroadcaster::SetApplyStylusCalibration( bool apply )
{
  this->ApplyStylusCalibration = apply;
}



vtkOpenIGTLinkBroadcaster::Status vtkOpenIGTLinkBroadcaster::SendMessages()
{
  std::string str;
  return SendMessages( str );
}

//----------------------------------------------------------------------------
vtkOpenIGTLinkBroadcaster::Status vtkOpenIGTLinkBroadcaster::SendMessages( std::string strError )
{
  // Check status and possible errors.

  if ( this->InternalStatus == STATUS_NOT_INITIALIZED )
  {
    LOG_WARNING( "Broadcaster not initialized." );
    return this->InternalStatus;
  }

  if ( this->DataCollector == NULL )
  {
    LOG_WARNING( "Tried to send OpenIGTLink messages without specifying a DataCollector." );
    this->InternalStatus = STATUS_NOT_INITIALIZED;
    return this->InternalStatus;
  }

  if ( ! this->DataCollector->GetTracker()->IsTracking() )
  {
    LOG_WARNING( "Tried to send OpenIGTLink messages without starting the tracker." );
    this->InternalStatus = STATUS_NOT_TRACKING;
    return this->InternalStatus;
  }


  TrackedFrame trackedFrame;
  TrackedFrame trackedFrameCalibrated;
  this->DataCollector->GetTrackedFrame( &trackedFrame, false );
  this->DataCollector->GetTrackedFrame( &trackedFrameCalibrated, true );

  double timestamp = trackedFrame.GetTimestamp();


  // Read the non-reference transforms to be broadcasted
  // relative to the reference from the DataCollector.

  for ( int igtIndex = 0; igtIndex < this->NonReferenceToolInfos.size(); ++ igtIndex )
  {

    // TrackerStatus status = TR_OK;
    vtkSmartPointer< vtkMatrix4x4 > mToolToTracker  = vtkSmartPointer< vtkMatrix4x4 >::New();

    std::string toolPortName = this->NonReferenceToolInfos[ igtIndex ].TrackerPortName;
    const char* toolName = this->NonReferenceToolInfos[ igtIndex ].ToolName.c_str();
    igtl::ClientSocket::Pointer toolSocket = this->NonReferenceToolInfos[ igtIndex ].Socket;
    
    double transform[ 16 ] = { 0 };
    
    // debug
    double c[ 16 ] = { 0 };
    double n[ 16 ] = { 0 };
    trackedFrameCalibrated.GetCustomFrameTransform( toolName, c );
    trackedFrame.GetCustomFrameTransform( toolName, n );
    
    
    if (    this->ApplyStylusCalibration
         && strcmp( toolName, "Stylus" ) == 0 )
    {
      trackedFrameCalibrated.GetCustomFrameTransform( toolName, transform );
    }
    else
    {
      trackedFrame.GetCustomFrameTransform( toolName, transform );
    }
    
    TrackerStatus status = trackedFrame.GetStatus();

    if ( status != TR_OK )
    {
      LOG_INFO( "Tracking data invalid for tool: " << toolPortName );
      continue;
    }


    // Prepare the igtl matrix and timestamp.

    igtl::Matrix4x4 igtlMatrix;


    for ( int row = 0; row < 4; ++ row )
    {
      for ( int col = 0; col < 4; ++ col )
      {
        // igtlMatrix[ row ][ col ] = mToolToTracker->GetElement( row, col );
        igtlMatrix[ row ][ col ] = transform[ row * 4 + col ];
      }
    }

    igtl::TimeStamp::Pointer igtlTime = igtl::TimeStamp::New();
    igtlTime->SetTime( timestamp );


    // Create and send OpenIGTLink Message for non ref transforms.

    igtl::TransformMessage::Pointer transformMessage = igtl::TransformMessage::New();
    transformMessage->SetMatrix( igtlMatrix );
    transformMessage->SetTimeStamp( igtlTime );
    transformMessage->SetDeviceName( toolName );
    transformMessage->Pack();

    int success = toolSocket->Send( transformMessage->GetPackPointer(), transformMessage->GetPackSize() );

    if ( success == 0 )
    {
      LOG_WARNING( "Could not broadcast tool: " << toolPortName );
    }
  }


  // If we should broadcast the image slice too, set up the image container.

  if (    this->DataCollector->GetVideoSource() != NULL
    && this->DataCollector->GetAcquisitionType() != SYNCHRO_VIDEO_NONE )
  {
    this->SendImageMessage( &trackedFrame, strError );
  }


  this->InternalStatus = STATUS_OK;
  return this->InternalStatus;
}

//----------------------------------------------------------------------------
void vtkOpenIGTLinkBroadcaster::SendImageMessage( TrackedFrame* trackedFrame, std::string strError )
{
  // Get the socket information for the default tool;

  // TODO: change this, see assemble ticket plus - #388
  vtkTrackerTool* imageTool = NULL; 
  if ( this->DataCollector->GetTracker()->GetTool("Probe", imageTool) != PLUS_SUCCESS )
  {
    LOG_ERROR("Failed to send image message - unable to find tool 'Probe'!"); 
    return; 
  }

  igtl::ClientSocket::Pointer defaultSocket;
  bool found = false;
  for ( int i = 0; i < this->NonReferenceToolInfos.size(); ++ i )
  {
    if ( STRCASECMP(imageTool->GetPortName(), this->NonReferenceToolInfos[ i ].TrackerPortName.c_str() ) == 0  )
    {
      defaultSocket = this->NonReferenceToolInfos[ i ].Socket;
      found = true;
    }
  }
  if ( ! found )
  {
    LOG_DEBUG( "No SendTo address found for default port." );
    return;
  }

  // Read the actual image data with transform.
  // vtkSmartPointer< vtkImageData > frameImage = vtkSmartPointer< vtkImageData >::New();
  // double timestamp = 0.0;
  double timestamp = trackedFrame->GetTimestamp();
  // TrackerStatus status = TR_OK;
  vtkSmartPointer< vtkMatrix4x4 > mProbeToReference = vtkSmartPointer< vtkMatrix4x4 >::New();
  // PlusStatus pStatus = this->DataCollector->GetTrackedFrame( frameImage, mProbeToReference, status, timestamp, defaultTool, true );

  vtkImageData* frameImage = trackedFrame->GetImageData()->GetVtkImage();
  TrackerStatus status = trackedFrame->GetStatus();

  if ( status != TR_OK )
  {
    LOG_INFO( "Tracked frame status not OK." );
    this->InternalStatus = STATUS_MISSING_TRACKED_FRAME;
    return;
  }


  igtl::TimeStamp::Pointer igtlFrameTime = igtl::TimeStamp::New();
  igtlFrameTime->SetTime( timestamp );

  /*
  //debug
  //imageMessage->Get
  std::stringstream ss;
  ss << "_vtk_" << igtlFrameTime->GetSecond() << "-" << igtlFrameTime->GetNanosecond() << ".bmp";
  PlusVideoFrame::SaveImageToFile(frameImage, ss.str().c_str()); 
  */


  // Convert matrix and time formats.

  igtl::Matrix4x4 igtlMatrix;

  /*
  for ( int row = 0; row < 4; ++ row )
  {
  for ( int col = 0; col < 4; ++ col )
  {
  igtlMatrix[ row ][ col ] = mProbeToReference->GetElement( row, col );
  }
  }
  */

  /*
  //debug
  std::cerr << "ProbeToReference:" << std::endl;
  for ( int row = 0; row < 4; ++ row )
  {
  for ( int col = 0; col < 4; ++ col )
  {
  std::cerr << mProbeToReference->GetElement( row, col ) << "   ";
  }
  std::cerr << std::endl;
  }
  std::cerr << std::endl;
  */



  // Create and send the image message.

  int    imageSizePixels[ 3 ] = { 0, 0, 0 };
  double imageSpacingMm[ 3 ] = { 0, 0, 0 };
  int    subSizePixels[ 3 ] = { 0, 0, 0 };
  int    subOffset[ 3 ] = { 0, 0, 0 };
  int    scalarType = igtl::ImageMessage::TYPE_UINT8;

  frameImage->GetDimensions( imageSizePixels );
  frameImage->GetSpacing( imageSpacingMm );
  frameImage->GetDimensions( subSizePixels );

  float spacingFloat[ 3 ];
  for ( int i = 0; i < 3; ++ i ) spacingFloat[ i ] = (float)imageSpacingMm[ i ];

  igtl::ImageMessage::Pointer imageMessage = igtl::ImageMessage::New();
  imageMessage->SetDimensions( imageSizePixels );
  imageMessage->SetSpacing( spacingFloat );
  imageMessage->SetScalarType( scalarType );
  imageMessage->SetDeviceName( "Ultrasound" );
  imageMessage->SetSubVolume( subSizePixels, subOffset );
  imageMessage->AllocateScalars();

  unsigned char* igtlImagePointer = (unsigned char*)( imageMessage->GetScalarPointer() );
  unsigned char* vtkImagePointer = (unsigned char*)( frameImage->GetScalarPointer() );

  memcpy(igtlImagePointer, vtkImagePointer, imageMessage->GetImageSize());

  imageMessage->SetMatrix( igtlMatrix );
  imageMessage->SetTimeStamp( igtlFrameTime );


  imageMessage->Pack();

  int success = defaultSocket->Send( imageMessage->GetPackPointer(), imageMessage->GetPackSize() );

  if ( success == 0 )
  {
    LOG_ERROR( "Could send image through OpenIGTLink port" );
    this->InternalStatus = STATUS_SEND_ERROR;
    return;
  }
  else
  {
    this->InternalStatus = STATUS_OK;
  }
}
