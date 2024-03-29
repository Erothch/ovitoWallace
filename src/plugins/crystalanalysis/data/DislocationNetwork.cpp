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
#include "Microstructure.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Copy constructor.
******************************************************************************/
DislocationNetwork::DislocationNetwork(const DislocationNetwork& other) :
	_clusterGraph(other.clusterGraph())
{
	for(int segmentIndex = 0; segmentIndex < other.segments().size(); segmentIndex++) {
		DislocationSegment* oldSegment = other.segments()[segmentIndex];
		OVITO_ASSERT(oldSegment->replacedWith == nullptr);
		OVITO_ASSERT(oldSegment->id == segmentIndex);
		DislocationSegment* newSegment = createSegment(oldSegment->burgersVector);
		newSegment->line = oldSegment->line;
		newSegment->coreSize = oldSegment->coreSize;
		OVITO_ASSERT(newSegment->id == oldSegment->id);
	}

	for(int segmentIndex = 0; segmentIndex < other.segments().size(); segmentIndex++) {
		DislocationSegment* oldSegment = other.segments()[segmentIndex];
		DislocationSegment* newSegment = segments()[segmentIndex];
		for(int nodeIndex = 0; nodeIndex < 2; nodeIndex++) {
			DislocationNode* oldNode = oldSegment->nodes[nodeIndex];
			if(oldNode->isDangling()) continue;
			DislocationNode* oldSecondNode = oldNode->junctionRing;
			DislocationNode* newNode = newSegment->nodes[nodeIndex];
			newNode->junctionRing = segments()[oldNode->junctionRing->segment->id]->nodes[oldSecondNode->isForwardNode() ? 0 : 1];
		}
	}

#ifdef OVITO_DEBUG
	for(int segmentIndex = 0; segmentIndex < other.segments().size(); segmentIndex++) {
		DislocationSegment* oldSegment = other.segments()[segmentIndex];
		DislocationSegment* newSegment = segments()[segmentIndex];
		for(int nodeIndex = 0; nodeIndex < 2; nodeIndex++) {
			OVITO_ASSERT(oldSegment->nodes[nodeIndex]->countJunctionArms() == newSegment->nodes[nodeIndex]->countJunctionArms());
		}
	}
#endif
}

/******************************************************************************
* Conversion constructor.
******************************************************************************/
DislocationNetwork::DislocationNetwork(const Microstructure& other, const SimulationCell& cell) :
	_clusterGraph(other.clusterGraph())
{
	// This is used to keep tracke which input edges have already been converted to a dislocation
	// line. For each visited edge, we store the ID of the output dislocation line.
	// The sign of the ID number indicates whether the input edge and the output line
	// have the same or reverse orientation.
	std::unordered_map<Microstructure::Edge*, int> visitedEdges;

	for(Microstructure::Vertex* inputVertex : other.vertices()) {
		for(Microstructure::Edge* inputEdge = inputVertex->edges(); inputEdge != nullptr; inputEdge = inputEdge->nextVertexEdge()) {
			// Ignore helper edges.
			if(!inputEdge->isDislocation()) continue;
			// Start at an arbitrary segment of the input network which has not been converted yet.
			if(visitedEdges.find(inputEdge) != visitedEdges.end()) continue;
			// Create a new line in the output network.
			DislocationSegment* outputSegment = createSegment(ClusterVector(inputEdge->face()->burgersVector(), inputEdge->face()->cluster()));
			outputSegment->line.push_back(inputEdge->vertex1()->pos());
			outputSegment->coreSize.push_back(3);
			// Extend output line along one direction until we hit a higher order node or an already converted segment.
			Microstructure::Edge* currentEdge = inputEdge;
			for(;;) {
				Point3 unwrappedPos2 = outputSegment->line.back() + cell.wrapVector(currentEdge->vertex2()->pos() - outputSegment->line.back());
				outputSegment->line.push_back(unwrappedPos2);
				outputSegment->coreSize.push_back(3);
				visitedEdges.emplace(currentEdge, outputSegment->id + 1);
				visitedEdges.emplace(currentEdge->oppositeEdge(), -(outputSegment->id + 1));
				Microstructure::Edge* nextEdge = nullptr;
				int armCount = 0;
				for(Microstructure::Edge* e = currentEdge->vertex2()->edges(); e != nullptr; e = e->nextVertexEdge()) {
					if(!e->isDislocation()) continue;
					armCount++;
					if(e != currentEdge->oppositeEdge()) nextEdge = e;
				}
				if(armCount != 2) break;
				auto edgeInfo = visitedEdges.find(nextEdge);
				if(edgeInfo != visitedEdges.end()) {
					// It must be a closed loop.
					if(edgeInfo->second != outputSegment->id + 1)
						throw Exception("Invalid dislocation network topology.");
					outputSegment->forwardNode().connectNodes(&outputSegment->backwardNode());
					break;
				}
				currentEdge = nextEdge;
			}
			// Extend output line along opposite direction until we hit a higher order node.
			currentEdge = inputEdge->oppositeEdge();
			OVITO_ASSERT(inputEdge->isDislocation());
			for(;;) {
				Microstructure::Edge* nextEdge = nullptr;
				int armCount = 0;
				for(Microstructure::Edge* e = currentEdge->vertex2()->edges(); e != nullptr; e = e->nextVertexEdge()) {
					if(!e->isDislocation()) continue;
					armCount++;
					if(e != currentEdge->oppositeEdge()) nextEdge = e;
				}
				if(armCount != 2) break;
				auto edgeInfo = visitedEdges.find(nextEdge);
				if(edgeInfo != visitedEdges.end()) {
					if(edgeInfo->second != -(outputSegment->id + 1))
						throw Exception("Invalid dislocation network topology (2).");
					break;
				}
				currentEdge = nextEdge;
				Point3 unwrappedPos2 = outputSegment->line.front() + cell.wrapVector(currentEdge->vertex2()->pos() - outputSegment->line.front());
				outputSegment->line.push_front(unwrappedPos2);
				outputSegment->coreSize.push_front(3);
				visitedEdges.emplace(currentEdge, -(outputSegment->id + 1));
				visitedEdges.emplace(currentEdge->oppositeEdge(), outputSegment->id + 1);
			}
		}
	}

	// Join dislocation lines at nodes with three or more arms.
	for(Microstructure::Vertex* vertex : other.vertices()) {
		if(vertex->countDislocationArms() >= 3) {
			DislocationNode* headNode = nullptr;
			for(Microstructure::Edge* edge = vertex->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
				if(!edge->isDislocation()) continue;
				OVITO_ASSERT(visitedEdges.find(edge) != visitedEdges.end());
				int edgeInfo = visitedEdges[edge];
				DislocationNode* otherNode = (edgeInfo > 0) ? &segments()[edgeInfo - 1]->backwardNode() : &segments()[-edgeInfo - 1]->forwardNode();
				OVITO_ASSERT(cell.wrapPoint(otherNode->position()).equals(cell.wrapPoint(vertex->pos())));
				if(!headNode)
					headNode = otherNode;
				else
					headNode->connectNodes(otherNode);
			}
		}
	}
}

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

/******************************************************************************
* Smoothens and coarsens the dislocation lines.
******************************************************************************/
bool DislocationNetwork::smoothDislocationLines(int lineSmoothingLevel, FloatType linePointInterval, PromiseState& promise)
{
	promise.setProgressMaximum(segments().size());

	for(DislocationSegment* segment : segments()) {
		if(!promise.incrementProgressValue())
			return false;
		if(segment->coreSize.empty())
			continue;
		std::deque<Point3> line;
		std::deque<int> coreSize;
		coarsenDislocationLine(linePointInterval, segment->line, segment->coreSize, line, coreSize, segment->isClosedLoop(), segment->isInfiniteLine());
		smoothDislocationLine(lineSmoothingLevel, line, segment->isClosedLoop());
		segment->line = std::move(line);
		segment->coreSize.clear();
	}

	return !promise.isCanceled();
}

/******************************************************************************
* Removes some of the sampling points from a dislocation line.
******************************************************************************/
void DislocationNetwork::coarsenDislocationLine(FloatType linePointInterval, const std::deque<Point3>& input, const std::deque<int>& coreSize, std::deque<Point3>& output, std::deque<int>& outputCoreSize, bool isClosedLoop, bool isInfiniteLine)
{
	OVITO_ASSERT(input.size() >= 2);
	OVITO_ASSERT(input.size() == coreSize.size());

	if(linePointInterval <= 0) {
		output = input;
		outputCoreSize = coreSize;
		return;
	}

	// Special handling for infinite lines.
	if(isInfiniteLine && input.size() >= 3) {
		int coreSizeSum = std::accumulate(coreSize.cbegin(), coreSize.cend() - 1, 0);
		int count = input.size() - 1;
		if(coreSizeSum * linePointInterval > count * count) {
			// Make it a straight line.
			Vector3 com = Vector3::Zero();
			for(auto p = input.cbegin(); p != input.cend() - 1; ++p)
				com += *p - input.front();
			output.push_back(input.front() + com / count);
			outputCoreSize.push_back(coreSizeSum / count);
			output.push_back(input.back() + com / count);
			outputCoreSize.push_back(coreSizeSum / count);
			return;
		}
	}

	// Special handling for very short segments.
	if(input.size() < 4) {
		output = input;
		outputCoreSize = coreSize;
		return;
	}

	// Always keep the end points of linear segments fixed to not break junctions.
	if(!isClosedLoop) {
		output.push_back(input.front());
		outputCoreSize.push_back(coreSize.front());
	}

	// Resulting line must contain at least two points (the end points).
	int minNumPoints = 2;

	// If the dislocation forms a loop, keep at least four points, because two points do not make a proper loop.
	if(input.front().equals(input.back(), CA_ATOM_VECTOR_EPSILON))
		minNumPoints = 4;

	auto inputPtr = input.cbegin();
	auto inputCoreSizePtr = coreSize.cbegin();

	int sum = 0;
	int count = 0;

	// Average over a half interval, starting from the beginning of the segment.
	Vector3 com = Vector3::Zero();
	do {
		sum += *inputCoreSizePtr;
		com += *inputPtr - input.front();
		count++;
		++inputPtr;
		++inputCoreSizePtr;
	}
	while(2*count*count < (int)(linePointInterval * sum) && count+1 < input.size()/minNumPoints/2);

	// Average over a half interval, starting from the end of the segment.
	auto inputPtrEnd = input.cend() - 1;
	auto inputCoreSizePtrEnd = coreSize.cend() - 1;
	OVITO_ASSERT(inputPtr < inputPtrEnd);
	while(count*count < (int)(linePointInterval * sum) && count < input.size()/minNumPoints) {
		sum += *inputCoreSizePtrEnd;
		com += *inputPtrEnd - input.back();
		count++;
		--inputPtrEnd;
		--inputCoreSizePtrEnd;
	}
	OVITO_ASSERT(inputPtr < inputPtrEnd);

	if(isClosedLoop) {
		output.push_back(input.front() + com / count);
		outputCoreSize.push_back(sum / count);
	}

	while(inputPtr < inputPtrEnd)
	{
		int sum = 0;
		int count = 0;
		Vector3 com = Vector3::Zero();
		do {
			sum += *inputCoreSizePtr++;
			com.x() += inputPtr->x();
			com.y() += inputPtr->y();
			com.z() += inputPtr->z();
			count++;
			++inputPtr;
		}
		while(count*count < (int)(linePointInterval * sum) && count+1 < input.size()/minNumPoints && inputPtr != inputPtrEnd);
		output.push_back(Point3::Origin() + com / count);
		outputCoreSize.push_back(sum / count);
	}

	if(!isClosedLoop) {
		// Always keep the end points of linear segments to not break junctions.
		output.push_back(input.back());
		outputCoreSize.push_back(coreSize.back());
	}
	else {
		output.push_back(input.back() + com / count);
		outputCoreSize.push_back(sum / count);
	}

	OVITO_ASSERT(output.size() >= minNumPoints);
	OVITO_ASSERT(!isClosedLoop || isInfiniteLine || output.size() >= 3);
}

/******************************************************************************
* Smoothes the sampling points of a dislocation line.
******************************************************************************/
void DislocationNetwork::smoothDislocationLine(int smoothingLevel, std::deque<Point3>& line, bool isLoop)
{
	if(smoothingLevel <= 0)
		return;	// Nothing to do.

	if(line.size() <= 2)
		return;	// Nothing to do.

	if(line.size() <= 4 && line.front().equals(line.back(), CA_ATOM_VECTOR_EPSILON))
		return;	// Do not smooth loops consisting of very few segments.

	// This is the 2d implementation of the mesh smoothing algorithm:
	//
	// Gabriel Taubin
	// A Signal Processing Approach To Fair Surface Design
	// In SIGGRAPH 95 Conference Proceedings, pages 351-358 (1995)

	FloatType k_PB = 0.1f;
	FloatType lambda = 0.5f;
	FloatType mu = 1.0f / (k_PB - 1.0f/lambda);
	const FloatType prefactors[2] = { lambda, mu };

	std::vector<Vector3> laplacians(line.size());
	for(int iteration = 0; iteration < smoothingLevel; iteration++) {

		for(int pass = 0; pass <= 1; pass++) {
			// Compute discrete Laplacian for each point.
			auto l = laplacians.begin();
			if(isLoop == false)
				(*l++).setZero();
			else
				(*l++) = ((*(line.end()-2) - *(line.end()-3)) + (*(line.begin()+1) - line.front())) * FloatType(0.5);

			auto p1 = line.cbegin();
			auto p2 = line.cbegin() + 1;
			for(;;) {
				auto p0 = p1;
				++p1;
				++p2;
				if(p2 == line.cend())
					break;
				*l++ = ((*p0 - *p1) + (*p2 - *p1)) * FloatType(0.5);
			}

			*l++ = laplacians.front();
			OVITO_ASSERT(l == laplacians.end());

			auto lc = laplacians.cbegin();
			for(Point3& p : line) {
				p += prefactors[pass] * (*lc++);
			}
		}
	}
}

/******************************************************************************
* Computes the location of a point along the segment line.
******************************************************************************/
Point3 DislocationSegment::getPointOnLine(FloatType t) const
{
	if(line.empty())
		return Point3::Origin();

	t *= calculateLength();

	FloatType sum = 0;
	auto i1 = line.begin();
	for(;;) {
		auto i2 = i1 + 1;
		if(i2 == line.end()) break;
		Vector3 delta = *i2 - *i1;
		FloatType len = delta.length();
		if(sum + len >= t && len != 0) {
			return *i1 + (((t - sum) / len) * delta);
		}
		sum += len;
		i1 = i2;
	}

	return line.back();
}


}	// End of namespace
}	// End of namespace
}	// End of namespace

