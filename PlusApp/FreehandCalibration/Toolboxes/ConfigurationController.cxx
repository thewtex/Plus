#include "PlusConfigure.h"

#include "ConfigurationController.h"

#include "vtkFreehandController.h"

//-----------------------------------------------------------------------------

ConfigurationController *ConfigurationController::m_Instance = NULL;

//-----------------------------------------------------------------------------

ConfigurationController* ConfigurationController::GetInstance() {
	if (! m_Instance) {
		m_Instance = new ConfigurationController();
	}
	return m_Instance;
}

//-----------------------------------------------------------------------------

ConfigurationController::ConfigurationController()
	: AbstractToolboxController()
	, m_IsToolDisplayDetached(false)
{
}

//-----------------------------------------------------------------------------

ConfigurationController::~ConfigurationController()
{
}

//-----------------------------------------------------------------------------

PlusStatus ConfigurationController::Initialize()
{
	LOG_DEBUG("Initialize StylusCalibrationController");

	// Set state to idle
	if (m_State == ToolboxState_Uninitialized) {
		m_State = ToolboxState_Idle;
	}

	return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus ConfigurationController::Clear()
{
	LOG_DEBUG("Clear ConfigurationController");

	m_Toolbox->Clear();

	return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

bool ConfigurationController::IsToolDisplayDetached()
{
	return m_IsToolDisplayDetached;
}

//-----------------------------------------------------------------------------

void ConfigurationController::SetToolDisplayDetached(bool aDetached)
{
	m_IsToolDisplayDetached = aDetached;
}

//-----------------------------------------------------------------------------

PlusStatus ConfigurationController::DoAcquisition()
{
	return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

PlusStatus ConfigurationController::Start()
{
	LOG_WARNING("ConfigurationController cannot be started");
	return PLUS_FAIL;
}

//-----------------------------------------------------------------------------

PlusStatus ConfigurationController::Stop()
{
	LOG_WARNING("ConfigurationController cannot be started (or stopped)");
	return PLUS_FAIL;
}
