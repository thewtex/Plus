#ifndef DEVICESETSELECTORWIDGET_H
#define DEVICESETSELECTORWIDGET_H

#include "ui_DeviceSetSelectorWidget.h"

#include "PlusConfigure.h"

#include <QWidget>

//-----------------------------------------------------------------------------

/*!
* \brief Data selector widget class
*/
class DeviceSetSelectorWidget : public QWidget
{
	Q_OBJECT

public:
	/*!
	* \brief Constructor
	* \param aParent parent
	* \param aFlags widget flag
	*/
	DeviceSetSelectorWidget(QWidget* aParent = 0);

	/*!
	* \brief Destructor
	*/
	~DeviceSetSelectorWidget();

	/*!
	* \brief Set configuration directory to search in
	* \param aDirectory Input configuration directory
	* \param aForce If true, it is set even when it is not empty (false if omitted)
	*/
	void SetConfigurationDirectory(std::string aDirectory, bool aForce = false);

	/*!
	* \brief Set connection successful flag
	* \param aConnectionSuccessful If true, Connect button will be disabled until changing another device set
	*/
	void SetConnectionSuccessful(bool aConnectionSuccessful);

signals:
	/*!
	* \brief Emmitted when configuration directory is changed (notifies application)
	* \param Configuration directory path
	*/
	void ConfigurationDirectoryChanged(std::string);

	/*!
	* \brief Emitted when connecting to devices
	* \param Device set configuration file
	*/
	void ConnectToDevicesByConfigFileInvoked(std::string);

protected:
	/*!
	* \brief Fills the combo box with the valid device set configuration files found in input directory
	* \param aDirectory The directory to search in
	*/
	PlusStatus ParseDirectory(QString aDirectory);

protected slots:
	/*!
	* \brief Pops up open directory dialog and saves the selected one into application
	*/
	void OpenConfigurationDirectoryClicked();

	/*!
	* \brief Called when device set selection has been changed
	*/
	void DeviceSetSelected(int);

	/*!
	* \brief Called when Connect button is pushed - connects to devices
	*/
	void InvokeConnect();

protected:
	//! Configuration directory path
	QString	m_ConfigurationDirectory;

	//! Flag telling whether connection has been successful
	bool	m_ConnectionSuccessful;

protected:
	Ui::DeviceSetSelectorWidget ui;
};

#endif 
