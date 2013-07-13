///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <core/Core.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportManager.h>
#include <core/animation/AnimManager.h>

#include "StructureIdentificationModifier.h"

namespace Viz {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Viz, StructureIdentificationModifier, AsynchronousParticleModifier)
DEFINE_VECTOR_REFERENCE_FIELD(StructureIdentificationModifier, _structureTypes, "StructureTypes", ParticleType)
SET_PROPERTY_FIELD_LABEL(StructureIdentificationModifier, _structureTypes, "Structure types")

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
StructureIdentificationModifier::StructureIdentificationModifier() :
	_structureProperty(new ParticleProperty(0, ParticleProperty::StructureTypeProperty))
{
	INIT_PROPERTY_FIELD(StructureIdentificationModifier::_structureTypes);
}

/******************************************************************************
* Create an instance of the ParticleType class to represent a structure type.
******************************************************************************/
void StructureIdentificationModifier::createStructureType(int id, const QString& name, const Color& color)
{
	OORef<ParticleType> stype(new ParticleType());
	stype->setId(id);
	stype->setName(name);
	stype->setColor(color);
	_structureTypes.push_back(stype);
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void StructureIdentificationModifier::saveToStream(ObjectSaveStream& stream)
{
	AsynchronousParticleModifier::saveToStream(stream);
	stream.beginChunk(0x01);
	_structureProperty.constData()->saveToStream(stream, !storeResultsWithScene());
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void StructureIdentificationModifier::loadFromStream(ObjectLoadStream& stream)
{
	AsynchronousParticleModifier::loadFromStream(stream);
	stream.expectChunk(0x01);
	_structureProperty.data()->loadFromStream(stream);
	stream.closeChunk();
}

/******************************************************************************
* Unpacks the computation results stored in the given engine object.
******************************************************************************/
void StructureIdentificationModifier::retrieveModifierResults(Engine* engine)
{
	StructureIdentificationEngine* eng = static_cast<StructureIdentificationEngine*>(engine);
	if(eng->structures())
		_structureProperty = eng->structures();
}

/******************************************************************************
* This lets the modifier insert the previously computed results into the pipeline.
******************************************************************************/
ObjectStatus StructureIdentificationModifier::applyModifierResults(TimePoint time, TimeInterval& validityInterval)
{
	if(inputParticleCount() != particleStructures().size())
		throw Exception(tr("The number of input particles has changed. The stored analysis results have become invalid."));

	// Get output property object.
	ParticleTypeProperty* structureProperty = static_object_cast<ParticleTypeProperty>(outputStandardProperty(ParticleProperty::StructureTypeProperty));

	// Insert structure types into output property.
	structureProperty->setParticleTypes(structureTypes());

	// Insert results into output property.
	structureProperty->replaceStorage(_structureProperty.data());

	// Build structure type to color map.
	std::vector<Color> structureTypeColors(structureTypes().size());
	std::vector<size_t> typeCounters(structureTypes().size());
	for(ParticleType* stype : structureTypes()) {
		OVITO_ASSERT(stype->id() >= 0);
		if(stype->id() >= structureTypeColors.size()) {
			structureTypeColors.resize(stype->id() + 1);
			typeCounters.resize(stype->id() + 1);
		}
		structureTypeColors[stype->id()] = stype->color();
		typeCounters[stype->id()] = 0;
	}

	// Assign colors to particles based on their structure type.
	ParticlePropertyObject* colorProperty = outputStandardProperty(ParticleProperty::ColorProperty);
	OVITO_ASSERT(colorProperty->size() == particleStructures().size());
	const int* s = particleStructures().constDataInt();
	Color* c = colorProperty->dataColor();
	Color* c_end = c + colorProperty->size();
	for(; c != c_end; ++s, ++c) {
		OVITO_ASSERT(*s >= 0 && *s < structureTypeColors.size());
		*c = structureTypeColors[*s];
		typeCounters[*s]++;
	}
	colorProperty->changed();

	_structureCounts.clear();
	for(ParticleType* stype : structureTypes())
		_structureCounts[stype->id()] = typeCounters[stype->id()];

	return ObjectStatus::Success;
}

/******************************************************************************
* Returns a data item from the list data model.
******************************************************************************/
QVariant StructureListParameterUI::getItemData(RefTarget* target, const QModelIndex& index, int role)
{
	ParticleType* stype = dynamic_object_cast<ParticleType>(target);
	StructureIdentificationModifier* modifier = dynamic_object_cast<StructureIdentificationModifier>(editor()->editObject());

	if(stype && modifier) {
		if(role == Qt::DisplayRole) {
			if(index.column() == 1)
				return stype->name();
			else if(index.column() == 2) {
				auto entry = modifier->structureCounts().find(stype->id());
				if(entry != modifier->structureCounts().end())
					return (int)entry->second;
				else
					return QVariant();
			}
		}
		else if(role == Qt::DecorationRole) {
			if(index.column() == 0)
				return (QColor)stype->color();
		}
	}
	return QVariant();
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool StructureListParameterUI::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject()) {
		if(event->type() == ReferenceEvent::StatusChanged) {
			// Update the structure count column.
			_model->updateColumn(2);
		}
	}
	return RefTargetListParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the user has double-clicked on one of the structure
* types in the list widget.
******************************************************************************/
void StructureListParameterUI::onDoubleClickStructureType(const QModelIndex& index)
{
	// Let the user select a color for the structure type.
	ParticleType* stype = static_object_cast<ParticleType>(selectedObject());
	if(!stype) return;

	QColor oldColor = (QColor)stype->color();
	QColor newColor = QColorDialog::getColor(oldColor, editor()->container());
	if(!newColor.isValid() || newColor == oldColor) return;

	UndoManager::instance().beginCompoundOperation(tr("Change structure type color"));
	stype->setColor(Color(newColor));
	UndoManager::instance().endCompoundOperation();
}

};	// End of namespace
