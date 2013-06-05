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

/**
 * \file Modifier.h
 * \brief Contains the definition of the Ovito::Modifier class.
 */

#ifndef __OVITO_MODIFIER_H
#define __OVITO_MODIFIER_H

#include <core/Core.h>
#include <core/reference/RefTarget.h>
#include <core/reference/EvaluationStatus.h>
#include "PipelineFlowState.h"

namespace Ovito {

class ModifierApplication;	// defined in ModifierApplication.h
class PipelineObject;		// defined in PipelineObject.h
class Viewport;				// defined in Viewport.h
class ObjectNode;			// defined in ObjectNode.h

/**
 * \brief Modifies an input object in some way.
 *
 * This is the abstract base class for modifier objects that can be applied to
 * SceneObject objects. A Modifier takes a SceneObject derived object, modifies
 * in some specific way and finally produces a result object.
 *
 * A Modifier is applied to an object using ObjectNode::applyModifier().
 */
class Modifier : public RefTarget
{
protected:

	/// \brief Default constructor.
	Modifier();

public:

	/// \brief This modifies the input object in a specific way.
	/// \param[in] time The animation at which the modifier is applied.
	/// \param[in] modApp The application object for this modifier. It describes this particular usage of the
	///               modifier in the geometry pipeline.
	/// \param[in,out] state The object flowing down the geometry pipeline. It contains the input object
	///                      when the method is called and is filled with the resulting object by the method.
	virtual EvaluationStatus modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) = 0;

	/// \brief Asks the modifier for its validity interval at the given time.
	/// \param time The animation at which the validity interval should be computed.
	/// \return The maximum time interval that contains \a time and during which the modifier's
	///         parameters do not change. This does not include the validity interval of the
	///         modifier's input object.
	virtual TimeInterval modifierValidity(TimePoint time) = 0;

	/// \brief Informs the modifier that its input object has changed.
	/// \param modApp Specifies the particular application of this modifier in a geometry pipeline.
	////
	/// This method is called by the system when an item in the modifier stack before this modifier sends a
	/// TargetChanged event via RefTarget::notifyDependents().
	/// This allows the modifier to clear its internal caches that rely on the last state of the input object.
	///
	/// The default implementation does nothing.
	virtual void onInputChanged(ModifierApplication* modApp) {}

#if 0
	/// \brief Makes the modifier render itself into the viewport.
	/// \param time The animation time at which to render the modifier.
	/// \param contextNode The node context used to render the modifier.
	/// \param modApp The modifier application specifies the particular application of this modifier in a geometry pipeline.
	/// \param vp The viewport to render the modifier in.
	///
	/// The viewport transformation is already set up, when this method is called by the
	/// system. The modifier has to be rendered in the local object coordinate system.
	///
	/// The default implementation does nothing.
	virtual void renderModifier(TimePoint time, ObjectNode* contextNode, ModifierApplication* modApp, Viewport* vp) {}
#endif

	/// \brief Returns whether this modifier is currently enabled.
	/// \return \c true if the modifier is currently enabled, i.e. applied.
	///         \c false if the modifier is disabled and skipped in the geometry pipeline.
	bool isModifierEnabled() const { return _isModifierEnabled; }

	/// \brief Enables or disables this modifier.
	/// \param enabled Controls the state of the modifier.
	///
	/// A disabled modifier is skipped in the geometry pipeline and is not applied to the
	/// input object.
	///
	/// \undoable
	void setModifierEnabled(bool enabled) { _isModifierEnabled = enabled; }

	/// \brief Returns the list of applications of this modifier in pipelines.
	/// \return The list of ModifierApplication objects that describe the particular applications of this Modifier.
	///
	/// One and the same modifier instance can be applied multiple times in different geometry pipelines of several scene objects.
	/// Each application of the modifier instance is associated with a instance of the ModifierApplication class.
	/// This method can be used to determine all applications of this Modifier instance.
	QVector<ModifierApplication*> modifierApplications() const;

	/// \brief Returns the input object of this modifier for each application of the modifier.
	/// \param time The animation for which the geometry pipelines should be evaluated.
	/// \return A map that contains for each particular application of this modifier instance the
	///         object as it comes out of the geometry pipeline up to the modifier.
	///
	/// This method evaluates the geometry pipeline up this modifier. It can be used to work with
	/// the input objects outside of a normal call to modifyObject().
	///
	/// \note This method might return empty result objects in some cases when the modifier stack
	///       cannot be evaluated because of an invalid modifier.
	QMap<ModifierApplication*, PipelineFlowState> getModifierInputs(TimeTicks time) const;

	/// \brief Returns the input object of the modifier assuming that it has been applied only in a single geometry pipeline.
	/// \return The object that comes out of the geometry pipeline when it is evaluated up the application of this modifier.
	///
	/// This is the same function as above but now using the current animation time as
	/// evaluation time and only returning the input object for the first application
	/// of this modifier.
	///
	/// This method can be used to work with the input object outside of a normal call to modifyObject().
	PipelineFlowState getModifierInput() const;

	/// \brief This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	/// \param pipeline The ModifiedObject into which the modifier has been inserted.
	/// \param modApp The ModifiedApplication object that has been created for the modifier.
	virtual void initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp) {}

public:

	Q_PROPERTY(bool isModifierEnabled READ isModifierEnabled WRITE setModifierEnabled)

private:

	/// Flag that indicates whether the modifier is enabled.
	PropertyField<bool, bool, MODIFIER_ENABLED> _isModifierEnabled;

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_PROPERTY_FIELD(_isModifierEnabled);
};


};

#endif // __OVITO_MODIFIER_H