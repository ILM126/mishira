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

#ifndef SCRIPTTEXTLAYER_H
#define SCRIPTTEXTLAYER_H

#include "textlayer.h"
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtScript/QScriptable>
#include <QtScript/QScriptEngine>

class ScriptThread;

//=============================================================================
class ParentDummy : public QObject
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	ScriptThread *	m_parent;

public: // Constructor/destructor ---------------------------------------------
	ParentDummy(ScriptThread *realParent);
	~ParentDummy();

public: // Methods ------------------------------------------------------------
	ScriptThread *	getRealParent() const;
	int				threadStartTimer(
		int interval, Qt::TimerType timerType = Qt::CoarseTimer);
	void			threadKillTimer(int id);

protected:
	virtual void	timerEvent(QTimerEvent *ev);
};
//=============================================================================

inline ScriptThread *ParentDummy::getRealParent() const
{
	return m_parent;
}

inline int ParentDummy::threadStartTimer(int interval, Qt::TimerType timerType)
{
	return startTimer(interval, timerType);
}

inline void ParentDummy::threadKillTimer(int id)
{
	killTimer(id);
}

//=============================================================================
class ScriptThread : public QThread
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	ParentDummy *				m_dummy;
	QScriptEngine *				m_engine;
	QMutex						m_mutex;
	bool						m_safeToAbort;
	QString						m_script;
	QHash<int, QScriptValue>	m_timeoutTimers;
	QHash<int, QScriptValue>	m_intervalTimers;

public: // Constructor/destructor ---------------------------------------------
	ScriptThread(const QString &script);
	~ScriptThread();

public: // Methods ------------------------------------------------------------
	void			threadTimerEvent(QTimerEvent *ev);
	void			abortScript();

private:
	void			testForException() const;

protected:
	virtual void	run();

public: // Scriptable methods -------------------------------------------------
	void	log(const QString &text);

	int		setTimeout(const QScriptValue &expression, int delay);
	void	clearTimeout(int timerId);
	int		setInterval(const QScriptValue &expression, int delay);
	void	clearInterval(int timerId);

	QString	readFile(const QString &filename);
	QString	readHttpGet(const QString &url);

	void	updateText(const QString &text);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	textUpdated(const QString &text);
};
//=============================================================================

//=============================================================================
class ScriptTextLayer : public TextLayer
{
	friend class ScriptTextLayerFactory;
	Q_OBJECT

private: // Members -----------------------------------------------------------
	ScriptThread *	m_thread;
	QString			m_cachedText;
	bool			m_ignoreUpdate;

	// Settings
	QString	m_script;
	QFont	m_font;
	int		m_fontSize;
	bool	m_fontBold;
	bool	m_fontItalic;
	bool	m_fontUnderline;
	QColor	m_fontColor;

private: // Constructor/destructor --------------------------------------------
	ScriptTextLayer(LayerGroup *parent);
	~ScriptTextLayer();

public: // Methods ------------------------------------------------------------
	void	setScript(const QString &script);
	QString	getScript();
	void	setFont(const QFont &font);
	QFont	getFont();
	void	setFontSize(int size);
	int		getFontSize();
	void	setFontBold(bool bold);
	bool	getFontBold();
	void	setFontItalic(bool italic);
	bool	getFontItalic();
	void	setFontUnderline(bool underline);
	bool	getFontUnderline();
	void	setFontColor(const QColor &color);
	QColor	getFontColor();

	void	resetScript();
	void	stopScript();

public: // Interface ----------------------------------------------------------
	virtual void	initializeResources(VidgfxContext *gfx);
	virtual void	updateResources(VidgfxContext *gfx);
	virtual void	destroyResources(VidgfxContext *gfx);

	virtual quint32	getTypeId() const;

	virtual bool			hasSettingsDialog();
	virtual LayerDialog *	createSettingsDialog(QWidget *parent = NULL);

	virtual void	serialize(QDataStream *stream) const;
	virtual bool	unserialize(QDataStream *stream);

private:
	virtual void	loadEvent();
	virtual void	unloadEvent();

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			textUpdated(const QString &text);
};
//=============================================================================

inline QString ScriptTextLayer::getScript()
{
	return m_script;
}

inline QFont ScriptTextLayer::getFont()
{
	return m_font;
}

inline int ScriptTextLayer::getFontSize()
{
	return m_fontSize;
}

inline bool ScriptTextLayer::getFontBold()
{
	return m_fontBold;
}

inline bool ScriptTextLayer::getFontItalic()
{
	return m_fontItalic;
}

inline bool ScriptTextLayer::getFontUnderline()
{
	return m_fontUnderline;
}

inline QColor ScriptTextLayer::getFontColor()
{
	return m_fontColor;
}

//=============================================================================
class ScriptTextLayerFactory : public LayerFactory
{
public: // Interface ----------------------------------------------------------
	virtual quint32		getTypeId() const;
	virtual QByteArray	getTypeString() const;
	virtual Layer *		createBlankLayer(LayerGroup *parent);
	virtual Layer *		createLayerWithDefaults(LayerGroup *parent);
	virtual QString		getAddLayerString() const;
};
//=============================================================================

#endif // SCRIPTTEXTLAYER_H
