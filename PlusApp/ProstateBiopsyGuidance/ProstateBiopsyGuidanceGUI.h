#ifndef ProstateBiopsyGuidanceGUI_H
#define ProstateBiopsyGuidanceGUI_H

#include "PlusConfigure.h"
#include "vtkVideoBuffer.h"
#include <QMainWindow>
#include <QWidget>


namespace Ui {
	class ProstateBiopsyGuidance;
}

class DeviceSetSelectorWidget;

class ProstateBiopsyGuidanceGUI : public QMainWindow
{
	Q_OBJECT

protected:
	// Device Set (Configuration Set) widget object:
	DeviceSetSelectorWidget*	m_DeviceSetSelectorWidget;

public:
	explicit ProstateBiopsyGuidanceGUI(QWidget *parent = 0);
	~ProstateBiopsyGuidanceGUI();

private:
	Ui::ProstateBiopsyGuidance *ui;

protected slots:
		void ConnectToDevicesByConfigFile(std::string aConfigFile);
		void ReadPBGConfigData(std::string aConfigFile);
		void butStop_Click();
		PlusStatus  saveData(vtkVideoBuffer *buffer_RF,std::string outputVideoBufferSequenceFileName);
		PlusStatus  acquireData(vtkVideoBuffer *buffer_RF, int type,std::string savedBufferName);
		PlusStatus  startShaker();
		PlusStatus	initialize();
//		PlusStatus	AcquireBModeData();
//		PlusStatus	SaveBModeData();
//		PlusStatus  StopDataAquisition();
		PlusStatus  startBiopsyProcess();
		PlusStatus	stopDataAquisition();
		PlusStatus	stopShaker();
		PlusStatus	deleteBuffer(vtkVideoBuffer *buffer_RF);

 };

 #endif