#pragma once

#include <EditorPluginScene/Plugin.h>
#include <Tools/EditorPluginScene/ui_DuplicateDlg.h>
#include <QDialog>

class qtDuplicateDlg : public QDialog, public Ui_DuplicateDlg
{
    Q_OBJECT

public:
	qtDuplicateDlg (QWidget *parent = nullptr);

	static ezUInt32 s_uiNumberOfCopies;
	static bool s_bStartAtCenter;
	static bool s_bGroupCopies;
	static ezVec3 s_vTranslation;
	static ezVec3 s_vRotation;
	static ezVec3 s_vTranslationGauss;
	static ezVec3 s_vRotationGauss;
	static int s_iStartAngle;
	static int s_iRevolveAngle;
	static float s_fRevolveRadius;
	static ezVec3 vSize;
	static ezVec3 vTranslate;
	static int s_iRevolveAxis;

public slots:
	virtual void on_PushButtonCancel_clicked ();
	virtual void on_PushButtonOk_clicked ();

	virtual void on_toolButtonTransX_clicked ();
	virtual void on_toolButtonTransY_clicked ();
	virtual void on_toolButtonTransZ_clicked ();

	virtual void on_PushButtonReset_clicked ();
	virtual void on_RefPointCenter_clicked ();
	virtual void on_RefPointMouse_clicked ();

	virtual void on_RevolveNone_clicked ();
	virtual void on_RevolveX_clicked ();
	virtual void on_RevolveY_clicked ();
	virtual void on_RevolveZ_clicked ();

};

