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

#ifndef LAYERFACTORY_H
#define LAYERFACTORY_H

#include "common.h"

class Layer;
class LayerGroup;

/// <summary>
/// The list of built-in layer type IDs. WARNING: This is used in user
/// profiles so treat it as a public API.
/// </summary>
enum LayerTypeId {
	LyrColorLayerTypeId = 0,
	LyrImageLayerTypeId = 1,
	LyrWindowLayerTypeId = 2,
	LyrTextLayerTypeId = 3,
	LyrSyncLayerTypeId = 4,
	LyrWebcamLayerTypeId = 5,
	LyrSlideshowLayerTypeId = 6,
	LyrMonitorLayerTypeId = 7,
	LyrScriptTextLayerTypeId = 8
};

//=============================================================================
class LayerFactory
{
public: // Static methods -----------------------------------------------------
	static void				registerBuiltInFactories();
	static LayerFactory *	findFactory(quint32 type);
	static LayerFactory *	findFactory(QByteArray type);

public: // Interface ----------------------------------------------------------
	/// <summary>
	/// Returns the unique ID for this layer type. Unofficial layers should
	/// have a randomly generated ID above 100.
	/// </summary>
	virtual quint32 getTypeId() const = 0;

	/// <summary>
	/// Returns the unique ID string for this layer type using PascalCaps
	/// suffixed by "Layer". Unofficial layers should be prefixed with
	/// something unique to the author such as their name. Example strings:
	/// "GregTextLayer", "BobWindowCapLayer", "MishiriumWebLayer".
	/// </summary>
	virtual QByteArray getTypeString() const = 0;

	/// <summary>
	/// Returns a new Layer object with consistent but unremarkable initial
	/// settings. Blank layers are typically used as a base for
	/// unserialization.
	/// </summary>
	virtual Layer *createBlankLayer(LayerGroup *parent) = 0;

	/// <summary>
	/// Similar to `createBlankLayer()` but instead configures the layer with
	/// user-friendly default settings. This is what the user sees when they
	/// create a new layer of this type in the UI.
	/// </summary>
	virtual Layer *createLayerWithDefaults(LayerGroup *parent) = 0;

	/// <summary>
	/// Returns the translated string that will be displayed in the "add layer"
	/// menu in the UI. Should be in the format "Add ____ layer".
	/// </summary>
	virtual QString getAddLayerString() const = 0;
};
//=============================================================================

#endif // LAYERFACTORY_H
