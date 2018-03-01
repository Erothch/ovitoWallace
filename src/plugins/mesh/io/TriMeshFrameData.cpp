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

#include <plugins/mesh/Mesh.h>
#include <plugins/mesh/tri/TriMeshObject.h>
#include <plugins/mesh/tri/TriMeshVis.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "TriMeshFrameData.h"

namespace Ovito { namespace Mesh {

/******************************************************************************
* Inserts the loaded data into the provided container object.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
PipelineFlowState TriMeshFrameData::handOver(DataSet* dataset, const PipelineFlowState& existing, bool isNewFile, FileSource* fileSource)
{
	PipelineFlowState output;

	// Create a TriMeshObject.
	OORef<TriMeshObject> triMeshObj = new TriMeshObject(dataset);
	triMeshObj->mesh().swap(mesh());

	// Assign a TriMeshVis to the TriMeshObject.
	// Re-use existing vis element if possible.
	OORef<DataVis> triMeshVis;
	if(TriMeshObject* previousTriMeshObj = existing.findObject<TriMeshObject>())
		triMeshVis = previousTriMeshObj->visElement();
	if(!triMeshVis) {
		triMeshVis = new TriMeshVis(dataset);
		triMeshVis->loadUserDefaults();
	}
	triMeshObj->setVisElement(triMeshVis);

	// Put object into output state.
	output.addObject(triMeshObj);
	return output;
}

}	// End of namespace
}	// End of namespace
