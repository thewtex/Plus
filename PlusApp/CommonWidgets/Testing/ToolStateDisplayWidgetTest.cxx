/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/ 

#include "DeviceSetSelectorWidget.h"
#include "ToolStateDisplayWidget.h"
#include "ToolStateDisplayWidgetTest.h"
#include "vtkDataCollector.h"
#include "vtkPlusChannel.h"
#include "vtkPlusDevice.h"
#include "vtkXMLUtilities.h"
#include <QTimer>

//-----------------------------------------------------------------------------

ToolStateDisplayWidgetTest::ToolStateDisplayWidgetTest(QWidget *parent, Qt::WFlags flags)
: QDialog(parent, flags)
, m_DeviceSetSelectorWidget(NULL)
, m_DataCollector(NULL)
, m_SelectedChannel(NULL)
{
  this->setMinimumSize(480, 300);
  this->setMaximumSize(480, 300);

  // Create device set selector widget
  m_DeviceSetSelectorWidget = new DeviceSetSelectorWidget(this);
  m_DeviceSetSelectorWidget->setMinimumWidth(472);
  m_DeviceSetSelectorWidget->setMaximumHeight(220);
  m_DeviceSetSelectorWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
  connect( m_DeviceSetSelectorWidget, SIGNAL( ConnectToDevicesByConfigFileInvoked(std::string) ), this, SLOT( ConnectToDevicesByConfigFile(std::string) ) );

  m_ToolStateDisplayWidget = new ToolStateDisplayWidget(this);
  m_ToolStateDisplayWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  m_ToolStateDisplayWidget->setMinimumHeight(m_ToolStateDisplayWidget->GetDesiredHeight() + 40);
  m_ToolStateDisplayWidget->setMaximumHeight(m_ToolStateDisplayWidget->GetDesiredHeight());

  // Insert widgets into placeholders
  QGridLayout* mainGrid = new QGridLayout(this);
  mainGrid->setMargin(4);
  mainGrid->setSpacing(4);
  mainGrid->addWidget(m_DeviceSetSelectorWidget, 0, 0, 1, 2, Qt::AlignHCenter);
  mainGrid->addWidget(m_ToolStateDisplayWidget, 1, 0, 1, 2, Qt::AlignHCenter | Qt::AlignVCenter);
  this->setLayout(mainGrid);

  // Set up timer for refreshing UI
  QTimer* refreshTimer = new QTimer(this);
  connect(refreshTimer, SIGNAL(timeout()), this, SLOT(RefreshToolDisplay()));
  refreshTimer->start(50);
}

//-----------------------------------------------------------------------------

ToolStateDisplayWidgetTest::~ToolStateDisplayWidgetTest()
{
  if (m_DataCollector != NULL) {
    m_DataCollector->Stop();
  }
  DELETE_IF_NOT_NULL(m_DataCollector);
}

//-----------------------------------------------------------------------------

void ToolStateDisplayWidgetTest::RefreshToolDisplay()
{
  //LOG_TRACE("ToolStateDisplayWidgetTest::RefreshToolDisplay"); 

  if (m_ToolStateDisplayWidget->IsInitialized()) {
    m_ToolStateDisplayWidget->Update();
  }
}

//-----------------------------------------------------------------------------

void ToolStateDisplayWidgetTest::ConnectToDevicesByConfigFile(std::string aConfigFile)
{
  LOG_TRACE("ToolStateDisplayWidgetTest::ConnectToDevicesByConfigFile");

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // If not empty, then try to connect; empty parameter string means disconnect
  if (STRCASECMP(aConfigFile.c_str(), "") != 0) {
    // Read configuration
    vtkSmartPointer<vtkXMLDataElement> configRootElement = vtkSmartPointer<vtkXMLDataElement>::Take(
      vtkXMLUtilities::ReadElementFromFile(aConfigFile.c_str()));  
    if (configRootElement == NULL) {	
      LOG_ERROR("Unable to read configuration from file " << aConfigFile); 
      return;
    }

    vtkPlusConfig::GetInstance()->SetDeviceSetConfigurationData(configRootElement); 

    // If connection has been successfully created then this action should disconnect
    if (! m_DeviceSetSelectorWidget->GetConnectionSuccessful()) {
      LOG_INFO("Connect to devices"); 

      // Disable main window
      this->setEnabled(false);

      // Create dialog
      QDialog* connectDialog = new QDialog(this, Qt::Dialog);
      connectDialog->setMinimumSize(QSize(360,80));
      connectDialog->setWindowTitle(tr("fCal"));
      connectDialog->setStyleSheet("QDialog { background-color: rgb(224, 224, 224); }");

      QLabel* connectLabel = new QLabel(QString("Connecting to devices, please wait..."), connectDialog);
      connectLabel->setFont(QFont("SansSerif", 16));

      QHBoxLayout* layout = new QHBoxLayout();
      layout->addWidget(connectLabel);

      connectDialog->setLayout(layout);
      connectDialog->show();

      QApplication::processEvents();

      // Connect to devices
      if (this->StartDataCollection() != PLUS_SUCCESS) {
        LOG_ERROR("Unable to start collecting data!");
        m_DeviceSetSelectorWidget->SetConnectionSuccessful(false);
        m_ToolStateDisplayWidget->InitializeTools(NULL, false);

      } else {
        // Successful connection
        m_DeviceSetSelectorWidget->SetConnectionSuccessful(true);

        vtkPlusConfig::GetInstance()->SaveApplicationConfigurationToFile();

        if( this->SelectChannel(m_SelectedChannel) != PLUS_SUCCESS )
        {
          LOG_ERROR("Unable to select channel in ToolStateDisplayWidgetTest");
          return;
        }

        if (m_ToolStateDisplayWidget->InitializeTools(m_SelectedChannel, true)) {
          m_ToolStateDisplayWidget->setMinimumHeight(m_ToolStateDisplayWidget->GetDesiredHeight() + 40);
          m_ToolStateDisplayWidget->setMaximumHeight(m_ToolStateDisplayWidget->GetDesiredHeight());
        }
      }

      // Close dialog
      connectDialog->done(0);
      connectDialog->hide();
      delete connectDialog;

      // Re-enable main window
      this->setEnabled(true);
    }

  } else { // Disconnect
    if ((m_DataCollector != NULL) && (m_DataCollector->GetConnected())) {

      m_DataCollector->Stop();
      m_DataCollector->Disconnect();

      m_DeviceSetSelectorWidget->SetConnectionSuccessful(false);
      m_ToolStateDisplayWidget->InitializeTools(NULL, false);
    }
  }

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------

PlusStatus ToolStateDisplayWidgetTest::StartDataCollection()
{
  LOG_TRACE("ToolStateDisplayWidgetTest::StartDataCollection"); 

  // Stop data collection if already started
  if (m_DataCollector != NULL) {
    m_DataCollector->Stop();
  } else {
    m_DataCollector = vtkDataCollector::New();
  }

  // Initialize data collector and read configuration
  if (m_DataCollector->ReadConfiguration(vtkPlusConfig::GetInstance()->GetDeviceSetConfigurationData()) != PLUS_SUCCESS) {
    return PLUS_FAIL;
  }

  if (m_DataCollector->Connect() != PLUS_SUCCESS) {
    return PLUS_FAIL;
  }

  if (m_DataCollector->Start() != PLUS_SUCCESS) {
    return PLUS_FAIL;
  }

  vtkPlusDevice* aDevice(NULL);
  vtkPlusChannel* aChannel(NULL);
  DeviceCollection aCollection;
  if( m_DataCollector->GetDevices(aCollection) == PLUS_SUCCESS && aCollection.size() > 0 )
  {
    aDevice = aCollection[0];
  }
  else
  {
    LOG_ERROR("No devices in tool state display widget test. Nothing to show!");
    return PLUS_FAIL;
  }

  if( aDevice->OutputChannelCount() == 0 )
  {
    LOG_ERROR("No output channels to acquire data from.");
    return PLUS_FAIL;
  }

  aChannel = *(aDevice->GetOutputChannelsStart());

  if (aChannel->GetTrackingDataAvailable() == false) {
    LOG_INFO("Tracking is not initialized"); 
  }

  if ( !m_DataCollector->GetConnected()) {
    LOG_ERROR("Unable to initialize DataCollector!"); 
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus ToolStateDisplayWidgetTest::SelectChannel(vtkPlusChannel*& aChannel)
{
  vtkPlusDevice* aDevice(NULL);

  DeviceCollection aCollection;
  if( m_DataCollector->GetDevices(aCollection) == PLUS_SUCCESS && aCollection.size() > 0 )
  {
    aDevice = aCollection[0];
  }
  else
  {
    LOG_ERROR("No default selected device defined and no devices to fall back on. Please check configuration.");
    return PLUS_FAIL;
  }

  if( aDevice->GetOutputChannelsStart() != aDevice->GetOutputChannelsEnd() )
  {
    aChannel = *(aDevice->GetOutputChannelsStart());
  }
  else
  {
    LOG_ERROR("Unable to set selected channel to default selected channel when connecting. device id: " << aDevice->GetDeviceId() << ".");
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}
