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

#pragma once


#include <plugins/stdobj/StdObj.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/oo/RefTarget.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Stores a selection set of particles or other elements and provides corresponding modification functions.
 *
 * This class is used by some modifiers to store the selection state of particles and other elements.
 *
 * This selection state can either be stored in an index-based fashion using a bit array,
 * or as a list of unique identifiers. The second storage scheme is less efficient,
 * but supports situations where the order or the number of elements changes.
 */
class OVITO_STDOBJ_EXPORT ElementSelectionSet : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(ElementSelectionSet)

public:

	/// Controls the mode of operation of the setSelection() method.
	enum SelectionMode {
		SelectionReplace,		//< Replace the selection with the new selection set.
		SelectionAdd,			//< Add the selection set to the existing selection.
		SelectionSubtract		//< Subtracts the selection set from the existing selection.
	};

public:

	/// Constructor.
	Q_INVOKABLE ElementSelectionSet(DataSet* dataset) : RefTarget(dataset), _useIdentifiers(true) {}

	/// Returns the stored selection set as a bit array.
	const boost::dynamic_bitset<>& selection() const { return _selection; }

	/// Adopts the selection set from the given input property container.
	void resetSelection(const PropertyContainer* container);

	/// Clears the selection set.
	void clearSelection(const PropertyContainer* container);

	/// Selects all elements in the given container.
	void selectAll(const PropertyContainer* container);

	/// Toggles the selection state of a single element.
	void toggleElement(const PropertyContainer* container, size_t elementIndex);

	/// Toggles the selection state of a single element.
	void toggleElementById(qlonglong elementId);

	/// Toggles the selection state of a single element.
	void toggleElementByIndex(size_t elementIndex);

	/// Replaces the selection.
	void setSelection(const PropertyContainer* container, const boost::dynamic_bitset<>& selection, SelectionMode mode = SelectionReplace);

	/// Copies the stored selection set into the given output selection property.
	PipelineStatus applySelection(PropertyObject* outputSelectionProperty, const PropertyObject* identifierProperty);

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) const override;

private:

	/// Stores the selection set as a bit array.
	boost::dynamic_bitset<> _selection;

	/// Stores the selection as a list of element identifiers.
	QSet<qlonglong> _selectedIdentifiers;

	/// Controls whether the object should store the identifiers of selected elements (when available).
	DECLARE_PROPERTY_FIELD(bool, useIdentifiers);

	friend class ReplaceSelectionOperation;
};

}	// End of namespace
}	// End of namespace
