///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <plugins/stdmod/StdMod.h>
#include <plugins/stdobj/util/InputHelper.h>
#include <plugins/stdobj/util/OutputHelper.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/app/PluginManager.h>
#include <core/app/Application.h>
#include <core/utilities/units/UnitsManager.h>
#include "FreezePropertyModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(FreezePropertyModifier);
DEFINE_PROPERTY_FIELD(FreezePropertyModifier, sourceProperty);
DEFINE_PROPERTY_FIELD(FreezePropertyModifier, destinationProperty);
DEFINE_PROPERTY_FIELD(FreezePropertyModifier, freezeTime);
SET_PROPERTY_FIELD_LABEL(FreezePropertyModifier, sourceProperty, "Property");
SET_PROPERTY_FIELD_LABEL(FreezePropertyModifier, destinationProperty, "Destination property");
SET_PROPERTY_FIELD_LABEL(FreezePropertyModifier, freezeTime, "Freeze at frame");
SET_PROPERTY_FIELD_UNITS(FreezePropertyModifier, freezeTime, TimeParameterUnit);

IMPLEMENT_OVITO_CLASS(FreezePropertyModifierApplication);
DEFINE_REFERENCE_FIELD(FreezePropertyModifierApplication, property);
DEFINE_REFERENCE_FIELD(FreezePropertyModifierApplication, identifiers);
DEFINE_REFERENCE_FIELD(FreezePropertyModifierApplication, cachedVisElements);
SET_MODIFIER_APPLICATION_TYPE(FreezePropertyModifier, FreezePropertyModifierApplication);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
FreezePropertyModifier::FreezePropertyModifier(DataSet* dataset) : GenericPropertyModifier(dataset), 
	_freezeTime(0)
{
	// Operate on particle properties by default.
	setPropertyClass(static_cast<const PropertyClass*>(
		PluginManager::instance().findClass(QStringLiteral("Particles"), QStringLiteral("ParticleProperty"))));	
}

/******************************************************************************
* This method is called by the system when the modifier is being inserted
* into a pipeline.
******************************************************************************/
void FreezePropertyModifier::initializeModifier(ModifierApplication* modApp)
{
	GenericPropertyModifier::initializeModifier(modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull() && propertyClass() && Application::instance()->guiMode()) {	
		const PipelineFlowState& input = modApp->evaluateInputPreliminary();
		for(DataObject* o : input.objects()) {
			if(PropertyObject* property = dynamic_object_cast<PropertyObject>(o)) {
				if(propertyClass()->isMember(property)) {
					setSourceProperty(PropertyReference(property));
					setDestinationProperty(sourceProperty());
					break;
				}
			}
		}
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void FreezePropertyModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	// Whenever the selected property class of this modifier changes, update the property references accordingly.
	if(field == PROPERTY_FIELD(GenericPropertyModifier::propertyClass) && !isBeingLoaded() && !dataset()->undoStack().isUndoingOrRedoing()) {
		setSourceProperty(sourceProperty().convertToPropertyClass(propertyClass()));
		setDestinationProperty(destinationProperty().convertToPropertyClass(propertyClass()));
	}
	GenericPropertyModifier::propertyChanged(field);
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> FreezePropertyModifier::evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Check if we already have the frozen property available.
	if(FreezePropertyModifierApplication* myModApp = dynamic_object_cast<FreezePropertyModifierApplication>(modApp)) {
		if(myModApp->hasFrozenState(freezeTime())) {
			// Perform replacement of the property in the input pipeline state.
			return evaluatePreliminary(time, modApp, std::move(input));
		}
	}
		
	// Request the frozen state from the pipeline.
	return modApp->evaluateInput(freezeTime())
		.then(executor(), [this, time, modApp = QPointer<ModifierApplication>(modApp), input = input](const PipelineFlowState& frozenState) mutable {
			UndoSuspender noUndo(this);
			
			// Extract the input property.
			if(FreezePropertyModifierApplication* myModApp = dynamic_object_cast<FreezePropertyModifierApplication>(modApp.data())) {
				if(myModApp->modifier() == this && !sourceProperty().isNull()) {
					if(PropertyObject* property = sourceProperty().findInState(frozenState)) {

						// Cache the property to be frozen in the ModifierApplication.
						myModApp->updateStoredData(property, 
							propertyClass() ? propertyClass()->findInState(frozenState, PropertyStorage::GenericIdentifierProperty) : nullptr, frozenState.stateValidity());

						// Perform the actual replacement of the property in the input pipeline state.
						return evaluatePreliminary(time, modApp, std::move(input));
					}
					else {
						throwException(tr("The property '%1' is not present in the input state").arg(sourceProperty().name()));
					}
				}
				myModApp->invalidateFrozenState();
			}

			return std::move(input);
		});
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
PipelineFlowState FreezePropertyModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;
	InputHelper ih(dataset(), input);
	OutputHelper oh(dataset(), output);
	
	if(!propertyClass())
		throwException(tr("No property class selected."));
	
	if(sourceProperty().isNull()) {
		output.setStatus(PipelineStatus(PipelineStatus::Warning, tr("No source property selected.")));
		return output;
	}
	if(destinationProperty().isNull())
		throwException(tr("No output property selected."));

	// Retrieve the property values stored in the ModifierApplication.
	FreezePropertyModifierApplication* myModApp = dynamic_object_cast<FreezePropertyModifierApplication>(modApp);
	if(!myModApp || !myModApp->property())
		throwException(tr("No stored property values available."));

	// Get the property that will be overwritten by the stored one.
	PropertyObject* outputProperty;
	if(destinationProperty().type() != PropertyStorage::GenericUserProperty) {
		outputProperty = oh.outputStandardProperty(*propertyClass(), destinationProperty().type(), true);
		if(outputProperty->dataType() != myModApp->property()->dataType()
			|| outputProperty->componentCount() != myModApp->property()->componentCount())
			throwException(tr("Types of source property and output property are not compatible. Cannot restore saved property values."));
	}
	else {
		outputProperty = oh.outputCustomProperty(*propertyClass(), destinationProperty().name(), 
			myModApp->property()->dataType(), myModApp->property()->componentCount(),
			0, true);
	}
	OVITO_ASSERT(outputProperty->stride() == myModApp->property()->stride());
	
	// Check if particle IDs are present and if the order of particles has changed
	// since we took the snapshot of the property values.
	PropertyObject* idProperty = ih.inputStandardProperty(*propertyClass(), PropertyStorage::GenericIdentifierProperty);
	if(myModApp->identifiers() && idProperty && 
			(idProperty->size() != myModApp->identifiers()->size() || 
			!std::equal(idProperty->constDataInt64(), idProperty->constDataInt64() + idProperty->size(), myModApp->identifiers()->constDataInt64()))) {

		// Build ID-to-index map.
		std::unordered_map<qlonglong,size_t> idmap;
		size_t index = 0;
		for(auto id : myModApp->identifiers()->constInt64Range()) {
			if(!idmap.insert(std::make_pair(id,index)).second)
				throwException(tr("Detected duplicate element ID %1 in saved snapshot. Cannot apply saved property values.").arg(id));
			index++;
		}

		// Copy and reorder property data.
		auto id = idProperty->constDataInt64();
		char* dest = static_cast<char*>(outputProperty->data());
		const char* src = static_cast<const char*>(myModApp->property()->constData());
		size_t stride = outputProperty->stride();
		for(size_t index = 0; index < outputProperty->size(); index++, ++id, dest += stride) {
			auto mapEntry = idmap.find(*id);
			if(mapEntry == idmap.end())
				throwException(tr("Detected new element ID %1, which didn't exist when the snapshot was created. Cannot restore saved property values.").arg(*id));
			memcpy(dest, src + stride * mapEntry->second, stride);
		}
	}
	else {
		
		// Make sure the number of elements didn't change when no IDs are defined.
		if(myModApp->property()->size() != outputProperty->size())
			throwException(tr("Number of input element has changed. Cannot restore saved property values. There were %1 elements when the snapshot was created. Now there are %2.").arg(myModApp->property()->size()).arg(outputProperty->size()));

		if(outputProperty->type() == myModApp->property()->type()
				&& outputProperty->name() == myModApp->property()->name()
				&& outputProperty->dataType() == myModApp->property()->dataType()) {
			// Make shallow data copy if input and output property are the same.
			outputProperty->setStorage(myModApp->property()->storage());
		}
		else {
			// Make a full data copy otherwise.
			OVITO_ASSERT(outputProperty->dataType() == myModApp->property()->dataType());
			OVITO_ASSERT(outputProperty->stride() == myModApp->property()->stride());
			OVITO_ASSERT(outputProperty->size() == myModApp->property()->size());
			memcpy(outputProperty->data(), myModApp->property()->constData(), outputProperty->stride() * outputProperty->size());
		}
	}

	// Replace vis elements of output property with cached ones and cache any new elements.
	// This is required to avoid losing the output property's display settings
	// each time the modifier is re-evaluated or when serializing the modifier application.
	QVector<DataVis*> currentVisElements = outputProperty->visElements();
	// Replace with cached vis elements if they are of the same class type.
	for(int i = 0; i < currentVisElements.size() && i < myModApp->cachedVisElements().size(); i++) {
		if(currentVisElements[i]->getOOClass() == myModApp->cachedVisElements()[i]->getOOClass()) {
			currentVisElements[i] = myModApp->cachedVisElements()[i];
		}
	}
	outputProperty->setVisElements(currentVisElements);
	myModApp->setCachedVisElements(std::move(currentVisElements));

	return output;
}

/******************************************************************************
* Makes a copy of the given source property and, optionally, of the provided
* particle identifier list, which will allow to restore the saved property
* values even if the order of particles changes.
******************************************************************************/
void FreezePropertyModifierApplication::updateStoredData(PropertyObject* property, PropertyObject* identifiers, TimeInterval validityInterval)
{
	CloneHelper cloneHelper;
	setProperty(cloneHelper.cloneObject(property, false));
	setIdentifiers(cloneHelper.cloneObject(identifiers, false));
	_validityInterval = validityInterval;
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool FreezePropertyModifierApplication::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TargetChanged) {
		// Invalidate cached state.
		invalidateFrozenState();
	}
	return ModifierApplication::referenceEvent(source, event);
}

}	// End of namespace
}	// End of namespace
