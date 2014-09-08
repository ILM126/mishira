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

#include "scripttextlayer.h"
#include "constants.h"
#include "layergroup.h"
#include "scripttextlayerdialog.h"
#include <QtCore/QDateTime>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

const QString LOG_CAT_SCENE = QStringLiteral("Scene");
const QString LOG_CAT_SCRIPT = QStringLiteral("Script");

//=============================================================================
// Default example script

QString defaultExampleScript()
{
	QString nowStr = QDateTime::currentDateTime().toString(
		QString("d MMM yyyy h:mm:ss AP"));
	return QString(
		"//--------------------------------------------------------\n"
		"// Example countdown/up timer script\n"
		"\n"
		"// Timer configuration\n"
		"var target = Date.parse(\"%1\"); // Time to count down/up to\n"
		"var countUp = true; // `true` = count up timer, `false = count down timer\n"
		"\n"
		"function updateTimer()\n"
		"{\n"
		"    // Calculate the difference in time between now and the target in seconds\n"
		"    var now = new Date();\n"
		"    var diff;\n"
		"    if(countUp)\n"
		"        diff = Math.max((now - target) / 1000, 0);\n"
		"    else\n"
		"        diff = Math.max((target - now) / 1000, 0);\n"
		"\n"
		"    // Convert seconds to days, hours, minutes and seconds\n"
		"    var days = Math.floor(diff / 60 / 60 / 24);\n"
		"    var hrs  = Math.floor(diff / 60 / 60 - days * 24);\n"
		"    var mins = Math.floor(diff / 60 - (days * 24 + hrs) * 60);\n"
		"    var secs = Math.floor(diff - ((days * 24 + hrs) * 60 + mins) * 60);\n"
		"\n"
		"    // Display the counter\n"
		"    updateText(days + \"d \" + hrs + \"h \" + mins + \"m \" + secs + \"s\");\n"
		"}\n"
		"\n"
		"// Update the text immediately and once every second\n"
		"updateTimer();\n"
		"setInterval(updateTimer, 1000); // 1000 msec\n"
		).arg(nowStr);
}

//=============================================================================
// ParentDummy class

ParentDummy::ParentDummy(ScriptThread *realParent)
	: QObject()
	, m_parent(realParent)
{
}

ParentDummy::~ParentDummy()
{
}

void ParentDummy::timerEvent(QTimerEvent *ev)
{
	// Timers can only be created in the same thread as the QObject which means
	// that we cannot create them in the `ScriptThread` object.
	m_parent->threadTimerEvent(ev);
}

//=============================================================================
// ScriptThread class

QScriptValue jsLog(QScriptContext *context, QScriptEngine *engine)
{
	ScriptThread *obj =
		static_cast<ParentDummy *>(engine->parent())->getRealParent();

	// Verify argument count
	if(context->argumentCount() < 1) {
		context->throwError(QScriptContext::SyntaxError,
			"Function expects 1 argument"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	// Verify argument types
	if(!context->argument(0).isString()) {
		context->throwError(QScriptContext::TypeError,
			"Argument 1 should be a string"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	obj->log(context->argument(0).toString());
	return QScriptValue(QScriptValue::UndefinedValue);
}

QScriptValue jsSetTimeout(QScriptContext *context, QScriptEngine *engine)
{
	ScriptThread *obj =
		static_cast<ParentDummy *>(engine->parent())->getRealParent();

	// Verify argument count
	if(context->argumentCount() < 2) {
		context->throwError(QScriptContext::SyntaxError,
			"Function expects 2 arguments"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	// Verify argument types
	if(!context->argument(0).isString() &&
		!context->argument(0).isFunction())
	{
		context->throwError(QScriptContext::TypeError,
			"Argument 1 should be a string or a function"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}
	if(!context->argument(1).isNumber()) {
		context->throwError(QScriptContext::TypeError,
			"Argument 2 should be a number"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	int ret = obj->setTimeout(
		context->argument(0), context->argument(1).toInt32());
	return QScriptValue(ret);
}

QScriptValue jsClearTimeout(QScriptContext *context, QScriptEngine *engine)
{
	ScriptThread *obj =
		static_cast<ParentDummy *>(engine->parent())->getRealParent();

	// Verify argument count
	if(context->argumentCount() < 1) {
		context->throwError(QScriptContext::SyntaxError,
			"Function expects 1 argument"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	// Verify argument types
	if(!context->argument(0).isNumber()) {
		context->throwError(QScriptContext::TypeError,
			"Argument 1 should be a number"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	obj->clearTimeout(context->argument(0).toInt32());
	return QScriptValue(QScriptValue::UndefinedValue);
}

QScriptValue jsSetInterval(QScriptContext *context, QScriptEngine *engine)
{
	ScriptThread *obj =
		static_cast<ParentDummy *>(engine->parent())->getRealParent();

	// Verify argument count
	if(context->argumentCount() < 2) {
		context->throwError(QScriptContext::SyntaxError,
			"Function expects 2 arguments"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	// Verify argument types
	if(!context->argument(0).isString() &&
		!context->argument(0).isFunction())
	{
		context->throwError(QScriptContext::TypeError,
			"Argument 1 should be a string or a function"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}
	if(!context->argument(1).isNumber()) {
		context->throwError(QScriptContext::TypeError,
			"Argument 2 should be a number"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	int ret = obj->setInterval(
		context->argument(0), context->argument(1).toInt32());
	return QScriptValue(ret);
}

QScriptValue jsClearInterval(QScriptContext *context, QScriptEngine *engine)
{
	ScriptThread *obj =
		static_cast<ParentDummy *>(engine->parent())->getRealParent();

	// Verify argument count
	if(context->argumentCount() < 1) {
		context->throwError(QScriptContext::SyntaxError,
			"Function expects 1 argument"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	// Verify argument types
	if(!context->argument(0).isNumber()) {
		context->throwError(QScriptContext::TypeError,
			"Argument 1 should be a number"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	obj->clearInterval(context->argument(0).toInt32());
	return QScriptValue(QScriptValue::UndefinedValue);
}

QScriptValue jsReadFile(QScriptContext *context, QScriptEngine *engine)
{
	ScriptThread *obj =
		static_cast<ParentDummy *>(engine->parent())->getRealParent();

	// Verify argument count
	if(context->argumentCount() < 1) {
		context->throwError(QScriptContext::SyntaxError,
			"Function expects 1 argument"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	// Verify argument types
	if(!context->argument(0).isString()) {
		context->throwError(QScriptContext::TypeError,
			"Argument 1 should be a string"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	QString ret = obj->readFile(context->argument(0).toString());
	return QScriptValue(ret);
}

QScriptValue jsReadHttpGet(QScriptContext *context, QScriptEngine *engine)
{
	ScriptThread *obj =
		static_cast<ParentDummy *>(engine->parent())->getRealParent();

	// Verify argument count
	if(context->argumentCount() < 1) {
		context->throwError(QScriptContext::SyntaxError,
			"Function expects 1 argument"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	// Verify argument types
	if(!context->argument(0).isString()) {
		context->throwError(QScriptContext::TypeError,
			"Argument 1 should be a string"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	QString ret = obj->readHttpGet(context->argument(0).toString());
	return QScriptValue(ret);
}

QScriptValue jsUpdateText(QScriptContext *context, QScriptEngine *engine)
{
	ScriptThread *obj =
		static_cast<ParentDummy *>(engine->parent())->getRealParent();

	// Verify argument count
	if(context->argumentCount() < 1) {
		context->throwError(QScriptContext::SyntaxError,
			"Function expects 1 argument"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	// Verify argument types
	if(!context->argument(0).isString()) {
		context->throwError(QScriptContext::TypeError,
			"Argument 1 should be a string"); // TODO: `tr()`
		return QScriptValue(QScriptValue::UndefinedValue);
	}

	obj->updateText(context->argument(0).toString());
	return QScriptValue(QScriptValue::UndefinedValue);
}

//-----------------------------------------------------------------------------

ScriptThread::ScriptThread(const QString &script)
	: QThread()
	, m_dummy(NULL)
	, m_engine(NULL)
	, m_mutex(QMutex::Recursive)
	, m_safeToAbort(false)
	, m_script(script)
	, m_timeoutTimers()
	, m_intervalTimers()
{
	m_timeoutTimers.reserve(32);
	m_intervalTimers.reserve(32);
}

ScriptThread::~ScriptThread()
{
	// WARNING: Object is destructed in the main thread
}

void ScriptThread::run()
{
	// As Qt only allows QObjects to be parents of another object that is in
	// the same thread we must create a dummy parent in order to pass a pointer
	// back to this object out-of-band.
	m_dummy = new ParentDummy(this);
	m_engine = new QScriptEngine(m_dummy);

	// Add our callback functions to the global object
	QScriptValue value;
	value = m_engine->newFunction(jsLog);
	m_engine->globalObject().setProperty(
		"log", value,
		QScriptValue::ReadOnly | QScriptValue::Undeletable);
	value = m_engine->newFunction(jsSetTimeout);
	m_engine->globalObject().setProperty(
		"setTimeout", value,
		QScriptValue::ReadOnly | QScriptValue::Undeletable);
	value = m_engine->newFunction(jsClearTimeout);
	m_engine->globalObject().setProperty(
		"clearTimeout", value,
		QScriptValue::ReadOnly | QScriptValue::Undeletable);
	value = m_engine->newFunction(jsSetInterval);
	m_engine->globalObject().setProperty(
		"setInterval", value,
		QScriptValue::ReadOnly | QScriptValue::Undeletable);
	value = m_engine->newFunction(jsClearInterval);
	m_engine->globalObject().setProperty(
		"clearInterval", value,
		QScriptValue::ReadOnly | QScriptValue::Undeletable);
	value = m_engine->newFunction(jsReadFile);
	m_engine->globalObject().setProperty(
		"readFile", value,
		QScriptValue::ReadOnly | QScriptValue::Undeletable);
	value = m_engine->newFunction(jsReadHttpGet);
	m_engine->globalObject().setProperty(
		"readHttpGet", value,
		QScriptValue::ReadOnly | QScriptValue::Undeletable);
	value = m_engine->newFunction(jsUpdateText);
	m_engine->globalObject().setProperty(
		"updateText", value,
		QScriptValue::ReadOnly | QScriptValue::Undeletable);

	m_mutex.lock();
	m_safeToAbort = true;
	m_mutex.unlock();

	// Evaluate the script
	m_engine->evaluate(m_script);
	testForException();

	// Enter the thread main loop
	exec();

	m_mutex.lock();
	m_safeToAbort = false;
	m_mutex.unlock();

	// Clean up
	delete m_engine;
	m_engine = NULL;
	delete m_dummy;
	m_dummy = NULL;
}

void ScriptThread::testForException() const
{
	if(m_engine == NULL)
		return;
	if(m_engine->hasUncaughtException()) {
		int line = m_engine->uncaughtExceptionLineNumber();
		appLog(LOG_CAT_SCRIPT, Log::Warning)
			<< "Uncaught exception at line " << line << ": "
			<< m_engine->uncaughtException().toString();
		m_engine->clearExceptions();
	}
}

void ScriptThread::threadTimerEvent(QTimerEvent *ev)
{
	if(m_dummy == NULL)
		return;

	// Get corresponding timer expression
	int id = ev->timerId();
	QScriptValue expression = m_intervalTimers.value(id);
	if(!expression.isValid()) {
		expression = m_timeoutTimers.value(id);
		if(expression.isValid())
			m_dummy->threadKillTimer(id); // `setTimeout()` only triggers once
	}

	// Evaluate expression
	if(expression.isString())
		m_engine->evaluate(expression.toString());
	else if(expression.isFunction())
		expression.call();

	testForException();
}

void ScriptThread::abortScript()
{
	m_mutex.lock();
	if(!m_safeToAbort) {
		m_mutex.unlock();
		return;
	}
	if(m_engine != NULL)
		m_engine->abortEvaluation();
	m_mutex.unlock();
}

void ScriptThread::log(const QString &text)
{
	// TODO: "Script:0x____" category
	//if(!text.isEmpty())
	appLog(LOG_CAT_SCRIPT) << text;
	// TODO: Status bar as well?
}

int ScriptThread::setTimeout(const QScriptValue &expression, int delay)
{
	if(m_dummy == NULL)
		return -1;
	if(expression.isString() || expression.isFunction()) {
		int timerId = m_dummy->threadStartTimer(delay);
		m_timeoutTimers.insert(timerId, expression);
		return timerId;
	}
	return -1;
}

void ScriptThread::clearTimeout(int timerId)
{
	if(m_dummy == NULL)
		return;
	m_dummy->threadKillTimer(timerId);
	m_timeoutTimers.remove(timerId);
}

int ScriptThread::setInterval(const QScriptValue &expression, int delay)
{
	if(m_dummy == NULL)
		return -1;
	if(expression.isString() || expression.isFunction()) {
		int timerId = m_dummy->threadStartTimer(delay);
		m_intervalTimers.insert(timerId, expression);
		return timerId;
	}
	return -1;
}

void ScriptThread::clearInterval(int timerId)
{
	if(m_dummy == NULL)
		return;
	m_dummy->threadKillTimer(timerId);
	m_intervalTimers.remove(timerId);
}

QString ScriptThread::readFile(const QString &filename)
{
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly)) {
		appLog(LOG_CAT_SCRIPT, Log::Warning) << QStringLiteral(
			"Error reading file \"%1\"")
			.arg(filename);
		// TODO: Status bar as well?
		return QString();
	}
	QString data = QString::fromUtf8(file.readAll());
	file.close();
	return data;
}

QString ScriptThread::readHttpGet(const QString &url)
{
	// Setup the request and transmit it
	QNetworkAccessManager mgr;
	QNetworkRequest req;
	req.setUrl(QUrl(url));
	req.setRawHeader("User-Agent",
		QStringLiteral("%1/%2").arg(APP_NAME).arg(APP_VER_STR).toUtf8());
	QNetworkReply *reply = mgr.get(req);

	// Block until we get a reply
	QEventLoop loop;
	connect(reply, &QNetworkReply::finished,
		&loop, &QEventLoop::quit);
	loop.exec();

	// Read the reply
	if(reply->error() != QNetworkReply::NoError) {
		appLog(LOG_CAT_SCRIPT, Log::Warning) << QStringLiteral(
			"Error during HTTP GET of \"%1\": %2")
			.arg(url)
			.arg(reply->errorString());
		// TODO: Status bar as well?
		delete reply;
		return QString();
	}
	QString ret = QString::fromUtf8(reply->readAll());

	// Clean up and return
	delete reply;
	return ret;
}

void ScriptThread::updateText(const QString &text)
{
	emit textUpdated(text);
}

//=============================================================================
// ScriptTextLayer class

ScriptTextLayer::ScriptTextLayer(LayerGroup *parent)
	: TextLayer(parent)
	, m_thread(NULL)
	, m_cachedText()
	, m_ignoreUpdate(false)

	// Settings
	, m_script(defaultExampleScript())
	, m_font(getDefaultFont())
	, m_fontSize(m_font.pointSize() / 2)
	, m_fontBold(false)
	, m_fontItalic(false)
	, m_fontUnderline(false)
	, m_fontColor(0, 0, 0)
{
}

ScriptTextLayer::~ScriptTextLayer()
{
	stopScript();
}

void ScriptTextLayer::setScript(const QString &script)
{
	if(m_script == script)
		return; // Nothing to do
	m_script = script;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void ScriptTextLayer::setFont(const QFont &font)
{
	if(m_font == font)
		return; // Nothing to do
	m_font = font;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void ScriptTextLayer::setFontSize(int size)
{
	if(m_fontSize == size)
		return; // Nothing to do
	m_fontSize = size;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void ScriptTextLayer::setFontBold(bool bold)
{
	if(m_fontBold == bold)
		return; // Nothing to do
	m_fontBold = bold;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void ScriptTextLayer::setFontItalic(bool italic)
{
	if(m_fontItalic == italic)
		return; // Nothing to do
	m_fontItalic = italic;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void ScriptTextLayer::setFontUnderline(bool underline)
{
	if(m_fontUnderline == underline)
		return; // Nothing to do
	m_fontUnderline = underline;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void ScriptTextLayer::setFontColor(const QColor &color)
{
	if(m_fontColor == color)
		return; // Nothing to do
	m_fontColor = color;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void ScriptTextLayer::resetScript()
{
	// Delete the existing script if it exists
	stopScript();

	// Create the new script
	if(m_script.isEmpty())
		return; // Nothing to do
	m_thread = new ScriptThread(m_script);
	connect(m_thread, &ScriptThread::textUpdated,
		this, &ScriptTextLayer::textUpdated);
	m_thread->start();
}

void ScriptTextLayer::stopScript()
{
	if(m_thread == NULL)
		return; // Already deleted

	// Brute force script termination
	for(int i = 0; i < 40; i++) {
		m_thread->exit();
		m_thread->abortScript();
		m_thread->wait(50); // msec
		if(m_thread->isFinished())
			break;
	}
	if(!m_thread->isFinished()) {
		// WARNING: Terminating may result in crashes!
		appLog(LOG_CAT_SCRIPT, Log::Warning) << QStringLiteral(
			"Terminating script uncleanly, might crash!");
		m_thread->terminate();
		m_thread->wait();
	}

	delete m_thread;
	m_thread = NULL;
}

void ScriptTextLayer::initializeResources(VidgfxContext *gfx)
{
	TextLayer::initializeResources(gfx);
}

void ScriptTextLayer::updateResources(VidgfxContext *gfx)
{
	TextLayer::updateResources(gfx);
	if(!m_ignoreUpdate)
		textUpdated(m_cachedText);
}

void ScriptTextLayer::destroyResources(VidgfxContext *gfx)
{
	TextLayer::destroyResources(gfx);
}

quint32 ScriptTextLayer::getTypeId() const
{
	return (quint32)LyrScriptTextLayerTypeId;
}

bool ScriptTextLayer::hasSettingsDialog()
{
	return true;
}

LayerDialog *ScriptTextLayer::createSettingsDialog(QWidget *parent)
{
	return new ScriptTextLayerDialog(this, parent);
}

void ScriptTextLayer::serialize(QDataStream *stream) const
{
	Layer::serialize(stream);

	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
	//-------------------------------------------------------------------------
	// We inherit from `TextLayer` but we completely reimplement the
	// serialization methods! We do this as we don't want to save temporary
	// settings such as the final document text.
	//-------------------------------------------------------------------------
	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

	// Write data version number
	*stream << (quint32)0;

	// Save our data
	*stream << m_script;
	*stream << m_font;
	*stream << (qint32)m_fontSize;
	*stream << m_fontBold;
	*stream << m_fontItalic;
	*stream << m_fontUnderline;
	*stream << m_fontColor;

	// Save relevant `TextLayer` settings
	*stream << m_strokeSize;
	*stream << m_strokeColor;
	*stream << m_wordWrap;
	*stream << m_scrollSpeed;
}

bool ScriptTextLayer::unserialize(QDataStream *stream)
{
	if(!Layer::unserialize(stream))
		return false;

	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
	//-------------------------------------------------------------------------
	// We inherit from `TextLayer` but we completely reimplement the
	// serialization methods! We do this as we don't want to save temporary
	// settings such as the final document text.
	//-------------------------------------------------------------------------
	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

	// Read data version number
	quint32 version;
	*stream >> version;

	// Read our data
	if(version == 0) {
		bool boolData;
		qint32 int32Data;
		QString stringData;

		//---------------------------------------------------------------------
		// Our data

		*stream >> m_script;
		*stream >> m_font;
		*stream >> int32Data;
		m_fontSize = int32Data;
		*stream >> m_fontBold;
		*stream >> m_fontItalic;
		*stream >> m_fontUnderline;
		*stream >> m_fontColor;

		//---------------------------------------------------------------------
		// Relevant `TextLayer` settings

		*stream >> m_strokeSize;
		*stream >> m_strokeColor;
		*stream >> boolData;
		setWordWrap(boolData);
		*stream >> m_scrollSpeed;

		resetScript();
	} else {
		appLog(LOG_CAT_SCENE, Log::Warning)
			<< "Unknown version number in scriptable text layer serialized "
			<< "data, cannot load settings";
		return false;
	}

	return true;
}

void ScriptTextLayer::loadEvent()
{
	Layer::loadEvent();

	// Reset and start script
	resetScript();
}

void ScriptTextLayer::unloadEvent()
{
	Layer::unloadEvent();

	// Stop script
	stopScript();
}

void ScriptTextLayer::textUpdated(const QString &text)
{
	// Remember text just in case the user changes minor display settings
	m_cachedText = text;

	// Escape HTML
	QString newText = text;
	newText.replace(QChar('&'), QStringLiteral("&amp;"));
	newText.replace(QChar('<'), QStringLiteral("&lt;"));
	newText.replace(QChar('\n'), QStringLiteral("<br />"));

	// Determine text alignment
	QString alignment;
	switch(getAlignment()) {
	default:
	case LyrTopLeftAlign:
	case LyrMiddleLeftAlign:
	case LyrBottomLeftAlign:
		alignment = QStringLiteral("left");
		break;
	case LyrTopCenterAlign:
	case LyrMiddleCenterAlign:
	case LyrBottomCenterAlign:
		alignment = QStringLiteral("center");
		break;
	case LyrTopRightAlign:
	case LyrMiddleRightAlign:
	case LyrBottomRightAlign:
		alignment = QStringLiteral("right");
		break;
	}

	QString html = "<html>"
		"<body style=\"font-family: '%1'; font-size: %2pt; font-weight: %3; font-style: %4; text-decoration: %5; color: rgb(%6, %7, %8);\">"
		"<p align=\"%9\">"
		"%10"
		"</p>"
		"</body>"
		"</html>";
	html = html
		.arg(m_font.family()) // TODO: Do we need to escape quotes?
		.arg(m_fontSize * 2)
		.arg(m_fontBold ? "bold" : "normal")
		.arg(m_fontItalic ? "italic" : "normal")
		.arg(m_fontUnderline ? "underline" : "none")
		.arg(m_fontColor.red())
		.arg(m_fontColor.green())
		.arg(m_fontColor.blue())
		.arg(alignment)
		.arg(newText);
	m_ignoreUpdate = true;
	setDocumentHtml(html);
	m_ignoreUpdate = false;
}

//=============================================================================
// ScriptTextLayerFactory class

quint32 ScriptTextLayerFactory::getTypeId() const
{
	return (quint32)LyrScriptTextLayerTypeId;
}

QByteArray ScriptTextLayerFactory::getTypeString() const
{
	return QByteArrayLiteral("ScriptTextLayer");
}

Layer *ScriptTextLayerFactory::createBlankLayer(LayerGroup *parent)
{
	return new ScriptTextLayer(parent);
}

Layer *ScriptTextLayerFactory::createLayerWithDefaults(LayerGroup *parent)
{
	return new ScriptTextLayer(parent);
}
