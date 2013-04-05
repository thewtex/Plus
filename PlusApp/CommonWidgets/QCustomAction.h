/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/ 

#ifndef __QCustomAction_h
#define __QCustomAction_h

#include <QAction>
#include <vtkPlusChannel.h>

class QCustomAction : public QAction
{
  Q_OBJECT

public:
  QCustomAction(const QString &text, QObject* parent, bool aIsSeparator = false, vtkPlusChannel* ownerChannel = NULL);
  bool IsSeparator(){ return m_IsSeparator; }

public slots:
  void activated();

signals:
  void channelSelected(vtkPlusChannel* aChannel);

private:
  bool m_IsSeparator;
  vtkPlusChannel* m_OwnerChannel;
};

#endif // __QCustomAction_h