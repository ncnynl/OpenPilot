/**
 ******************************************************************************
 *
 * @file       configrevohwwidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Revolution hardware configuration panel
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include "configrevohwwidget.h"

#include <QDebug>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/generalsettings.h>
#include "hwsettings.h"
#include <QDesktopServices>
#include <QUrl>


ConfigRevoHWWidget::ConfigRevoHWWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_ui = new Ui_RevoHWWidget();
    m_ui->setupUi(this);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();
    if(!settings->useExpertMode()) {
        m_ui->saveTelemetryToRAM->setEnabled(false);
        m_ui->saveTelemetryToRAM->setVisible(false);
    }

    addApplySaveButtons(m_ui->saveTelemetryToRAM, m_ui->saveTelemetryToSD);

    addUAVObjectToWidgetRelation("HwSettings","RM_FlexiPort",m_ui->cbFlexi);
    addUAVObjectToWidgetRelation("HwSettings","RM_MainPort",m_ui->cbMain);
    addUAVObjectToWidgetRelation("HwSettings","RM_RcvrPort",m_ui->cbRcvr);

    addUAVObjectToWidgetRelation("HwSettings","USB_HIDPort",m_ui->cbUSBHIDFunction);
    addUAVObjectToWidgetRelation("HwSettings","USB_VCPPort",m_ui->cbUSBVCPFunction);
    addUAVObjectToWidgetRelation("HwSettings","ComUsbBridgeSpeed",m_ui->cbUSBVCPSpeed);

    addUAVObjectToWidgetRelation("HwSettings","TelemetrySpeed",m_ui->cbFlexiTelemSpeed);
    addUAVObjectToWidgetRelation("HwSettings","GPSSpeed",m_ui->cbFlexiGPSSpeed);
    addUAVObjectToWidgetRelation("HwSettings","ComUsbBridgeSpeed",m_ui->cbFlexiComSpeed);

    addUAVObjectToWidgetRelation("HwSettings","TelemetrySpeed",m_ui->cbMainTelemSpeed);
    addUAVObjectToWidgetRelation("HwSettings","GPSSpeed",m_ui->cbMainGPSSpeed);
    addUAVObjectToWidgetRelation("HwSettings","ComUsbBridgeSpeed",m_ui->cbMainComSpeed);

    addUAVObjectToWidgetRelation("HwSettings","RadioPort",m_ui->cbModem);

    connect(m_ui->cchwHelp,SIGNAL(clicked()),this,SLOT(openHelp()));

    setupCustomCombos();
    enableControls(true);
    populateWidgets();
    refreshWidgetsValues();
    forceConnectedState();
}

ConfigRevoHWWidget::~ConfigRevoHWWidget()
{
    // Do nothing
}

void ConfigRevoHWWidget::setupCustomCombos()
{
    m_ui->cbUSBType->addItem(tr("HID"), USB_HID);
    m_ui->cbUSBType->addItem(tr("VCP"), USB_VCP);
    connect(m_ui->cbUSBType, SIGNAL(currentIndexChanged(int)), this, SLOT(usbTypeChanged(int)));

    m_ui->cbSonar->addItem(tr("Disabled"));
    m_ui->cbSonar->setCurrentIndex(0);
    m_ui->cbSonar->setEnabled(false);

    connect(m_ui->cbFlexi, SIGNAL(currentIndexChanged(int)), this, SLOT(flexiPortChanged(int)));
    connect(m_ui->cbMain, SIGNAL(currentIndexChanged(int)), this, SLOT(mainPortChanged(int)));
    connect(m_ui->cbModem, SIGNAL(currentIndexChanged(int)), this, SLOT(modemPortChanged(int)));

}

void ConfigRevoHWWidget::refreshWidgetsValues(UAVObject *obj)
{
    ConfigTaskWidget::refreshWidgetsValues(obj);
    HwSettings *hwSettings = HwSettings::GetInstance(getObjectManager());
    HwSettings::DataFields data = hwSettings->getData();
    if(data.USB_HIDPort != HwSettings::USB_HIDPORT_DISABLED){
        m_ui->cbUSBType->setCurrentIndex(m_ui->cbUSBType->findData(USB_HID));
    }
    else {
        m_ui->cbUSBType->setCurrentIndex(m_ui->cbUSBType->findData(USB_VCP));
    }
    usbTypeChanged(m_ui->cbUSBType->currentIndex());
    mainPortChanged(0);
    flexiPortChanged(0);
}

void ConfigRevoHWWidget::updateObjectsFromWidgets()
{
    ConfigTaskWidget::updateObjectsFromWidgets();

    HwSettings *hwSettings = HwSettings::GetInstance(getObjectManager());
    HwSettings::DataFields data = hwSettings->getData();

    // If any port is configured to be GPS port, enable GPS module if it is not enabled.
    // Otherwise disable GPS module.
    if(m_ui->cbFlexi->currentIndex() == HwSettings::RM_FLEXIPORT_GPS || m_ui->cbMain->currentIndex() == HwSettings::RM_MAINPORT_GPS) {
        if(data.OptionalModules[HwSettings::OPTIONALMODULES_GPS] != HwSettings::OPTIONALMODULES_ENABLED) {
            data.OptionalModules[HwSettings::OPTIONALMODULES_GPS] = HwSettings::OPTIONALMODULES_ENABLED;
        }
    }
    else if(data.OptionalModules[HwSettings::OPTIONALMODULES_GPS] != HwSettings::OPTIONALMODULES_DISABLED) {
        data.OptionalModules[HwSettings::OPTIONALMODULES_GPS] = HwSettings::OPTIONALMODULES_DISABLED;
    }

    // If any port is configured to be ComBridge port, enable UsbComBridge module if it is not enabled.
    // Otherwise disable UsbComBridge module.
    if(m_ui->cbFlexi->currentIndex() == HwSettings::RM_FLEXIPORT_COMBRIDGE || m_ui->cbMain->currentIndex() == HwSettings::RM_MAINPORT_COMBRIDGE ||
            m_ui->cbUSBVCPFunction->currentIndex() == HwSettings::USB_VCPPORT_COMBRIDGE) {
        if(data.OptionalModules[HwSettings::OPTIONALMODULES_COMUSBBRIDGE] != HwSettings::OPTIONALMODULES_ENABLED) {
            data.OptionalModules[HwSettings::OPTIONALMODULES_COMUSBBRIDGE] = HwSettings::OPTIONALMODULES_ENABLED;
        }
    }
    else if(data.OptionalModules[HwSettings::OPTIONALMODULES_COMUSBBRIDGE] != HwSettings::OPTIONALMODULES_DISABLED) {
        data.OptionalModules[HwSettings::OPTIONALMODULES_COMUSBBRIDGE] = HwSettings::OPTIONALMODULES_DISABLED;
    }

    hwSettings->setData(data);
}

void ConfigRevoHWWidget::usbTypeChanged(int index)
{
    Q_UNUSED(index);

    bool hid = m_ui->cbUSBType->itemData(m_ui->cbUSBType->currentIndex()) == USB_HID;
    m_ui->cbUSBHIDFunction->setVisible(hid);
    m_ui->cbUSBVCPFunction->setVisible(!hid);

    m_ui->lblUSBVCPSpeed->setVisible(!hid);
    m_ui->cbUSBVCPSpeed->setVisible(!hid);

    if(hid){
        m_ui->cbUSBVCPFunction->setCurrentIndex(HwSettings::USB_VCPPORT_DISABLED);
        m_ui->cbUSBHIDFunction->setCurrentIndex(HwSettings::USB_HIDPORT_USBTELEMETRY);
    }
    else {
        m_ui->cbUSBHIDFunction->setCurrentIndex(HwSettings::USB_HIDPORT_DISABLED);
        m_ui->cbUSBVCPFunction->setCurrentIndex(HwSettings::USB_VCPPORT_USBTELEMETRY);
    }

    if(m_ui->cbFlexi->currentIndex() == HwSettings::RM_FLEXIPORT_COMBRIDGE) {
        m_ui->cbFlexi->setCurrentIndex(HwSettings::RM_FLEXIPORT_DISABLED);
    }
    m_ui->cbFlexi->model()->setData(m_ui->cbFlexi->model()->index(HwSettings::RM_FLEXIPORT_COMBRIDGE, 0),
                                    hid ? QVariant(0) : QVariant(1|32), Qt::UserRole - 1);

    if(m_ui->cbMain->currentIndex() == HwSettings::RM_MAINPORT_COMBRIDGE) {
        m_ui->cbMain->setCurrentIndex(HwSettings::RM_MAINPORT_DISABLED);
    }
    m_ui->cbMain->model()->setData(m_ui->cbMain->model()->index(HwSettings::RM_MAINPORT_COMBRIDGE, 0),
                                   hid ? QVariant(0) : QVariant(1|32), Qt::UserRole - 1);
}


void ConfigRevoHWWidget::flexiPortChanged(int index)
{
    Q_UNUSED(index);

    m_ui->cbFlexiTelemSpeed->setVisible(false);
    m_ui->cbFlexiGPSSpeed->setVisible(false);
    m_ui->cbFlexiComSpeed->setVisible(false);
    m_ui->lblFlexiSpeed->setVisible(true);

    switch(m_ui->cbFlexi->currentIndex())
    {
        case HwSettings::RM_FLEXIPORT_TELEMETRY:
            m_ui->cbFlexiTelemSpeed->setVisible(true);
            if(m_ui->cbMain->currentIndex() == HwSettings::RM_MAINPORT_TELEMETRY) {
                m_ui->cbMain->setCurrentIndex(HwSettings::RM_MAINPORT_DISABLED);
            }
            if(m_ui->cbModem->currentIndex() == HwSettings::RADIOPORT_TELEMETRY) {
                m_ui->cbModem->setCurrentIndex(HwSettings::RADIOPORT_DISABLED);
            }
            break;
        case HwSettings::RM_FLEXIPORT_GPS:
            m_ui->cbFlexiGPSSpeed->setVisible(true);
            if(m_ui->cbMain->currentIndex() == HwSettings::RM_MAINPORT_GPS) {
                m_ui->cbMain->setCurrentIndex(HwSettings::RM_MAINPORT_DISABLED);
            }
            break;
        case HwSettings::RM_FLEXIPORT_COMBRIDGE:
            m_ui->cbFlexiComSpeed->setVisible(true);
            if(m_ui->cbMain->currentIndex() == HwSettings::RM_MAINPORT_COMBRIDGE) {
                m_ui->cbMain->setCurrentIndex(HwSettings::RM_MAINPORT_DISABLED);
            }
            break;
        default:
            m_ui->lblFlexiSpeed->setVisible(false);
            break;
    }
}

void ConfigRevoHWWidget::mainPortChanged(int index)
{
    Q_UNUSED(index);

    m_ui->cbMainTelemSpeed->setVisible(false);
    m_ui->cbMainGPSSpeed->setVisible(false);
    m_ui->cbMainComSpeed->setVisible(false);
    m_ui->lblMainSpeed->setVisible(true);

    switch(m_ui->cbMain->currentIndex())
    {
        case HwSettings::RM_MAINPORT_TELEMETRY:
            m_ui->cbMainTelemSpeed->setVisible(true);
            if(m_ui->cbFlexi->currentIndex() == HwSettings::RM_FLEXIPORT_TELEMETRY) {
                m_ui->cbFlexi->setCurrentIndex(HwSettings::RM_FLEXIPORT_DISABLED);
            }
            if(m_ui->cbModem->currentIndex() == HwSettings::RADIOPORT_TELEMETRY) {
                m_ui->cbModem->setCurrentIndex(HwSettings::RADIOPORT_DISABLED);
            }
            break;
        case HwSettings::RM_MAINPORT_GPS:
            m_ui->cbMainGPSSpeed->setVisible(true);
            if(m_ui->cbFlexi->currentIndex() == HwSettings::RM_FLEXIPORT_GPS) {
                m_ui->cbFlexi->setCurrentIndex(HwSettings::RM_FLEXIPORT_DISABLED);
            }
            break;
        case HwSettings::RM_MAINPORT_COMBRIDGE:
            m_ui->cbMainComSpeed->setVisible(true);
            if(m_ui->cbFlexi->currentIndex() == HwSettings::RM_FLEXIPORT_COMBRIDGE) {
                m_ui->cbFlexi->setCurrentIndex(HwSettings::RM_FLEXIPORT_DISABLED);
            }
            break;
        default:
            m_ui->lblMainSpeed->setVisible(false);
            break;
    }
}

void ConfigRevoHWWidget::modemPortChanged(int index)
{
    Q_UNUSED(index);

    if(m_ui->cbModem->currentIndex()== HwSettings::RADIOPORT_TELEMETRY) {
        if(m_ui->cbMain->currentIndex() == HwSettings::RM_MAINPORT_TELEMETRY) {
            m_ui->cbMain->setCurrentIndex(HwSettings::RM_MAINPORT_DISABLED);
        }
        if(m_ui->cbFlexi->currentIndex() == HwSettings::RM_FLEXIPORT_TELEMETRY) {
            m_ui->cbFlexi->setCurrentIndex(HwSettings::RM_FLEXIPORT_DISABLED);
        }
    }
}

void ConfigRevoHWWidget::openHelp()
{
    QDesktopServices::openUrl( QUrl("http://wiki.openpilot.org/x/GgDBAQ", QUrl::StrictMode) );
}
