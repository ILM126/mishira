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

#include "layerfactory.h"
#include "application.h"
#include "Layers/color/colorlayer.h"
#include "Layers/deskcap/monitorlayer.h"
#include "Layers/deskcap/windowlayer.h"
#include "Layers/image/imagelayer.h"
#include "Layers/slideshow/slideshowlayer.h"
#include "Layers/sync/synclayer.h"
#include "Layers/text/scripttextlayer.h"
#include "Layers/text/textlayer.h"
#include "Layers/webcam/webcamlayer.h"

/// <summary>
/// A helper method for registering all of Mishira's built-in layer factories.
/// </summary>
void LayerFactory::registerBuiltInFactories()
{
	// NOTE: The layers are displayed in the UI in the order that they are
	// registered here.
	App->registerLayerFactory(new ColorLayerFactory());
	App->registerLayerFactory(new ImageLayerFactory());
	App->registerLayerFactory(new MonitorLayerFactory());
	App->registerLayerFactory(new ScriptTextLayerFactory());
	App->registerLayerFactory(new SlideshowLayerFactory());
	App->registerLayerFactory(new SyncLayerFactory());
	App->registerLayerFactory(new TextLayerFactory());
	App->registerLayerFactory(new WindowLayerFactory());
	App->registerLayerFactory(new WebcamLayerFactory());
}

LayerFactory *LayerFactory::findFactory(quint32 type)
{
	LayerFactoryList factories = App->getLayerFactories();
	for(int i = 0; i < factories.size(); i++) {
		LayerFactory *factory = factories.at(i);
		if(type == factory->getTypeId())
			return factory;
	}
	return NULL; // Layer type not found
}

LayerFactory *LayerFactory::findFactory(QByteArray type)
{
	LayerFactoryList factories = App->getLayerFactories();
	for(int i = 0; i < factories.size(); i++) {
		LayerFactory *factory = factories.at(i);
		if(type == factory->getTypeString())
			return factory;
	}
	return NULL; // Layer type not found
}
