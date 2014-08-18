//*****************************************************************************
// Mishira: An audiovisual production tool for broadcasting live video
//
// Copyright (C) 2014 Lucas Murray <lucas@polyflare.com>
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

#ifndef PROFILE_H
#define PROFILE_H

#include "common.h"
#include "animatedfloat.h"
#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QSize>
#include <QtCore/QVector>

class AudioEncoder;
class AudioMixer;
class LayerGroup;
class Scene;
class Target;
class VideoEncoder;

typedef QVector<Scene *> SceneList;
typedef QVector<Target *> TargetList;
typedef QHash<quint32, VideoEncoder *> VideoEncoderHash;
typedef QHash<quint32, AudioEncoder *> AudioEncoderHash;
typedef QHash<quint32, LayerGroup *> LayerGroupHash;

//=============================================================================
class Profile : public QObject
{
	Q_OBJECT

private: // Constants ---------------------------------------------------------
	static const int	NumTransitionSettings = 3;

private: // Members -----------------------------------------------------------
	QString				m_filename;
	bool				m_fileExists;
	SceneList			m_scenes;
	Scene *				m_activeScene;
	Scene *				m_prevScene; // Used for transitions
	bool				m_useFuzzyWinCapture;
	Fraction			m_videoFramerate;
	AudioMixer *		m_mixer;
	QSize				m_canvasSize;
	PrflAudioMode		m_audioMode;
	VideoEncoderHash	m_videoEncoders;
	AudioEncoderHash	m_audioEncoders;
	LayerGroupHash		m_layerGroups;
	TargetList			m_targets;

	// Transitions
	PrflTransition		m_transition; // Current transition
	AnimatedFloat		m_transitionAni; // Current transition
	VidgfxVertBuf *		m_fadeVertBuf;
	int					m_activeTransition;
	PrflTransition		m_transitions[NumTransitionSettings];
	uint				m_transitionDursMsec[NumTransitionSettings];

public: // Static methods -----------------------------------------------------
	static QVector<QString>	queryAvailableProfiles();
	static QString			getProfileFilePath(const QString &name);
	static bool				doesProfileExist(const QString &name);

public: // Constructor/destructor ---------------------------------------------
	Profile(const QString &filename, QObject *parent = 0);
	~Profile();

public: // Methods ------------------------------------------------------------
	bool			doesFileExist() const;
	void			clear();
	void			loadDefaults();
	void			loadFromDisk();
	bool			saveToDisk();

	QString			getName() const;
	bool			setName(const QString &name);
	void			setCanvasSize(const QSize &size);
	QSize			getCanvasSize() const;
	void			setUseFuzzyWinCapture(bool useFuzzyCapture);
	bool			getUseFuzzyWinCapture() const;
	void			setAudioMode(PrflAudioMode mode);
	PrflAudioMode	getAudioMode() const;
	void			setActiveScene(Scene *scene);
	Scene *			getActiveScene() const;
	void			setTransition(
		int transitionId, PrflTransition mode, uint durationMsec);
	void			getTransition(
		int transitionId, PrflTransition &modeOut, uint &durationMsecOut);
	int				getActiveTransition() const;
	void			setActiveTransition(int transitionId);
	Fraction		getVideoFramerate() const;
	void			setVideoFramerate(Fraction framerate);
	AudioMixer *	getAudioMixer();

	VideoEncoder *		getOrCreateX264VideoEncoder(
		QSize size, SclrScalingMode scaling, GfxFilter scaleFilter,
		const X264Options &opt);
	void				destroyVideoEncoder(VideoEncoder *encoder);
	void				pruneVideoEncoders();
	VideoEncoder *		getVideoEncoderById(quint32 id) const;
	quint32				idOfVideoEncoder(VideoEncoder *encoder) const;
	VideoEncoderHash	getVideoEncoders() const;

	AudioEncoder *		getOrCreateFdkAacAudioEncoder(
		const FdkAacOptions &opt);
	void				destroyAudioEncoder(AudioEncoder *encoder);
	void				pruneAudioEncoders();
	AudioEncoder *		getAudioEncoderById(quint32 id) const;
	quint32				idOfAudioEncoder(AudioEncoder *encoder) const;
	AudioEncoderHash	getAudioEncoders() const;

	LayerGroup *	createLayerGroup(const QString &name);
	LayerGroup *	createLayerGroupSerialized(
		quint32 id, QDataStream *stream);
	void			destroyLayerGroup(LayerGroup *group);
	void			pruneLayerGroups();
	LayerGroup *	getLayerGroupById(quint32 id) const;
	quint32			idOfLayerGroup(LayerGroup *group) const;
	LayerGroupHash	getLayerGroups() const;

	void			reloadAllLayers();
	void			loadAllLayers();
	void			unloadAllInvisibleLayers();

	Scene *			createScene(const QString &name, int before = -1);
	Scene *			createSceneSerialized(
		QDataStream *stream, int before = -1);
	void			destroyScene(Scene *scene);
	int				indexOfScene(Scene *scene) const;
	Scene *			sceneAtIndex(int index) const;
	void			moveSceneTo(Scene *scene, int before);
	SceneList		getScenes() const;

	Target *		createFileTarget(
		const QString &name, const FileTrgtOptions &opt, int before = -1);
	Target *		createRtmpTarget(
		const QString &name, const RTMPTrgtOptions &opt, int before = -1);
	Target *		createTwitchTarget(
		const QString &name, const TwitchTrgtOptions &opt, int before = -1);
	Target *		createUstreamTarget(
		const QString &name, const UstreamTrgtOptions &opt, int before = -1);
	Target *		createHitboxTarget(
		const QString &name, const HitboxTrgtOptions &opt, int before = -1);
	Target *		createTargetSerialized(
		TrgtType type, QDataStream *stream, int before = -1);
	void			destroyTarget(Target *target);
	int				indexOfTarget(Target *target) const;
	void			moveTargetTo(Target *target, int before);
	TargetList		getTargets() const;

private:
	VideoEncoder *	createVideoEncoderSerialized(
		quint32 id, VencType type, QDataStream *stream);
	AudioEncoder *	createAudioEncoderSerialized(
		quint32 id, AencType type, QDataStream *stream);

	void			ensureActiveScene();

	void			setupContext(VidgfxContext *gfx);
	void			render(
		VidgfxContext *gfx, uint frameNum, int numDropped);

	void			serialize(QDataStream *stream) const;
	bool			unserialize(QDataStream *stream);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			sceneAdded(Scene *scene, int before);
	void			destroyingScene(Scene *scene);
	void			sceneMoved(Scene *scene, int before);
	void			sceneChanged(Scene *scene);
	void			activeSceneChanged(Scene *scene, Scene *oldScene);

	void			targetAdded(Target *target, int before);
	void			destroyingTarget(Target *target);
	void			targetChanged(Target *target);
	void			targetMoved(Target *target, int before);

	void			canvasSizeChanged(const QSize &oldSize);
	void			audioModeChanged(PrflAudioMode mode);
	void			frameRendered(
		VidgfxTex *tex, uint frameNum, int numDropped);
	void			transitionsChanged();

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			graphicsContextInitialized(VidgfxContext *gfx);
	void			graphicsContextDestroyed(VidgfxContext *gfx);
	void			queuedFrameEvent(uint frameNum, int numDropped);
	void			targetActiveChanged(Target *target, bool active);
	void			unserializeErrorTimeout();
};
//=============================================================================

inline bool Profile::doesFileExist() const
{
	return m_fileExists;
}

inline QSize Profile::getCanvasSize() const
{
	return m_canvasSize;
}

inline bool Profile::getUseFuzzyWinCapture() const
{
	return m_useFuzzyWinCapture;
}

inline PrflAudioMode Profile::getAudioMode() const
{
	return m_audioMode;
}

inline Scene *Profile::getActiveScene() const
{
	return m_activeScene;
}

inline int Profile::getActiveTransition() const
{
	return m_activeTransition;
}

inline Fraction Profile::getVideoFramerate() const
{
	return m_videoFramerate;
}

inline AudioMixer *Profile::getAudioMixer()
{
	return m_mixer;
}

inline VideoEncoderHash Profile::getVideoEncoders() const
{
	return m_videoEncoders;
}

inline AudioEncoderHash Profile::getAudioEncoders() const
{
	return m_audioEncoders;
}

inline SceneList Profile::getScenes() const
{
	return m_scenes;
}

inline TargetList Profile::getTargets() const
{
	return m_targets;
}

#endif // PROFILE_H
