///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <plugins/mesh/Mesh.h>
#include <plugins/stdmod/modifiers/ReplicateModifier.h>

namespace Ovito { namespace Mesh {

/**
 * \brief Delegate for the ReplicateModifier that operates on surface meshes.
 */
class SurfaceMeshReplicateModifierDelegate : public ReplicateModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ReplicateModifierDelegate::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using ReplicateModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("surfaces"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(SurfaceMeshReplicateModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Surfaces");

public:

	/// Constructor.
	Q_INVOKABLE SurfaceMeshReplicateModifierDelegate(DataSet* dataset) : ReplicateModifierDelegate(dataset) {}

	/// Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;
};

}	// End of namespace
}	// End of namespace
