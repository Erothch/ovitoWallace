///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include "DislocationNetwork.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Allocates a new dislocation segment terminated by two nodes.
******************************************************************************/
DislocationSegment* DislocationNetwork::createSegment(const ClusterVector& burgersVector)
{
	DislocationNode* forwardNode  = _nodePool.construct();
	DislocationNode* backwardNode = _nodePool.construct();

	DislocationSegment* segment = _segmentPool.construct(burgersVector, forwardNode, backwardNode);
	segment->id = _segments.size();
	_segments.push_back(segment);

	return segment;
}

/******************************************************************************
* Removes a segment from the list of segments.
******************************************************************************/
void DislocationNetwork::discardSegment(DislocationSegment* segment)
{
	OVITO_ASSERT(segment != nullptr);
	auto i = std::find(_segments.begin(), _segments.end(), segment);
	OVITO_ASSERT(i != _segments.end());
	_segments.erase(i);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

