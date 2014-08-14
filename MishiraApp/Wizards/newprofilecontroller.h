//*****************************************************************************
// Mishira: An audiovisual production tool for broadcasting live video
//
// Copyright (C) 2014 Lucas Murray <lmurray@undefinedfire.com>
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//*****************************************************************************

#ifndef NEWPROFILECONTROLLER_H
#define NEWPROFILECONTROLLER_H

#include "fraction.h"
#include "wizardcontroller.h"

class CPUUsage;
class LayerGroup;
class TextLayer;
class VideoEncoder;
class QAbstractButton;
class QMessageBox;

//=============================================================================
class NewProfileController : public WizardController
{
	Q_OBJECT

private: // Datatypes ---------------------------------------------------------
	struct TestResults {
		X264Preset	vidPreset;
		QSize		vidSize;
		Fraction	framerate;
		int			bitrate;
		float		totalAvgCpu;
		float		ourAvgCpu;
		bool		recommended;
	};
	typedef QVector<TestResults> TestResultsList;

private: // Members -----------------------------------------------------------
	WizPage				m_curPage;
	WizTargetSettings *	m_targetDefaults;
	WizTargetSettings	m_targetSettings;
	QMessageBox *		m_exitDialog;
	bool				m_createBlankProfile;
	bool				m_vidHasHighAction;
	int					m_recMaxVideoBitrate;
	int					m_recMaxAudioBitrate;
	float				m_maxCpuUsage;
	QString				m_prevProfileName;
	int					m_chosenResultIndex;
	bool				m_useTutorialScene;

	// Benchmark state
	bool				m_benchmarkActive;
	int					m_numTotalTests;
	int					m_numTestsDone;
	int					m_curFrameNum;
	int					m_curVidSizeIndex;
	int					m_curPresetIndex;
	VideoEncoder *		m_curEncoder;
	CPUUsage *			m_curUsage;
	TestResultsList		m_testResults;

public: // Constructor/destructor ---------------------------------------------
	NewProfileController(
		WizController type, WizardWindow *wizWin, WizShared *shared);
	~NewProfileController();

private: // Methods -----------------------------------------------------------
	void			setPageBasedOnTargetType();

	void			resetWelcomePage();
	void			resetNewProfilePage();
	void			resetTargetTypePage();
	void			resetUploadPage();
	void			resetPrepareBenchmarkPage();
	void			resetBenchmarkPage();
	void			resetBenchmarkResultsPage();
	void			resetSceneTemplatePage();
	void			resetProfileCompletePage();
	void			resetFileTargetPage();
	void			resetRTMPTargetPage();
	void			resetTwitchTargetPage();
	void			resetUstreamTargetPage();
	void			resetHitboxTargetPage();

	void			nextWelcomePage();
	void			nextNewProfilePage();
	void			nextTargetTypePage();
	void			nextUploadPage();
	void			nextPrepareBenchmarkPage();
	void			nextBenchmarkPage();
	void			nextBenchmarkResultsPage();
	void			nextSceneTemplatePage();
	void			nextProfileCompletePage();
	void			nextFileTargetPage();
	void			nextRTMPTargetPage();
	void			nextTwitchTargetPage();
	void			nextUstreamTargetPage();
	void			nextHitboxTargetPage();

	void			leavingTempProfileArea();
	void			createProfile();
	LayerGroup *	createBlankScenes();
	void			createTutorialScenes();
	void			startBenchmark();
	void			endBenchmark();
	void			doNextBenchmarkTest();
	void			showExitDialog();

	// Tutorial scene helpers
	TextLayer *		createTextLayer(
		LayerGroup *group, const QString &name, const QString &text,
		const QRect &rect, LyrAlignment align);

public: // Interface ----------------------------------------------------------
	virtual void	resetWizard();
	virtual bool	cancelWizard();
	virtual void	resetPage();
	virtual void	backPage();
	virtual void	nextPage();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			exitDialogClosed(QAbstractButton *button);
	void			queuedFrameEvent(uint frameNum, int numDropped);
	void			realTimeTickEvent(int numDropped, int lateByUsec);
};
//=============================================================================

#endif // NEWPROFILECONTROLLER_H
