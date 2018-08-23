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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <plugins/crystalanalysis/data/DislocationNetwork.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMesh.h>
#include <plugins/particles/import/ParticleImporter.h>
#include <plugins/particles/import/ParticleFrameData.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief Importer for output files generated by the Crystal Analysis Tool.
 */
class OVITO_CRYSTALANALYSIS_EXPORT CAImporter : public ParticleImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public ParticleImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleImporter::OOMetaClass ::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*"); }
	
		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("Crystal Analysis files"); }
	
		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const override;	
	};
	
	OVITO_CLASS_META(CAImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE CAImporter(DataSet* dataset) : ParticleImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("CA File"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FileSourceImporter::FrameLoader> createFrameLoader(const Frame& frame, const QString& localFilename) override {
		return std::make_shared<FrameLoader>(frame, localFilename);
	}

	/// Creates an asynchronous frame discovery object that scans the input file for contained animation frames.
	virtual std::shared_ptr<FileSourceImporter::FrameFinder> createFrameFinder(const QUrl& sourceUrl, const QString& localFilename) override {
		return std::make_shared<FrameFinder>(sourceUrl, localFilename);
	}

protected:

	/// The format-specific data holder.
	class CrystalAnalysisFrameData : public ParticleFrameData
	{
	public:
		
		struct BurgersVectorFamilyInfo {
			int id = 0;
			QString name;
			Vector3 burgersVector = Vector3::Zero();
			Color color = Color(1,1,1);
		};

		struct PatternInfo {
			int id = 0;
			StructurePattern::StructureType type = StructurePattern::Lattice;
			StructurePattern::SymmetryType symmetryType = StructurePattern::CubicSymmetry;
			QString shortName;
			QString longName;
			Color color = Color(1,1,1);
			QVector<BurgersVectorFamilyInfo> burgersVectorFamilies;
		};

		
	public:

		/// Inherit constructor from base class.
		using ParticleFrameData::ParticleFrameData;
		
		/// Inserts the loaded data into the provided pipeline state structure. This function is
		/// called by the system from the main thread after the asynchronous loading task has finished.
		virtual void handOver(PipelineOutputHelper& poh, const PipelineFlowState& existing, bool isNewFile, FileSource* fileSource) override;

		void addPattern(PatternInfo pattern) {
			_patterns.push_back(std::move(pattern));
		}

		Cluster* createCluster(int patternId) {
			if(!_clusterGraph) _clusterGraph = std::make_shared<ClusterGraph>();
			return _clusterGraph->createCluster(patternId);
		}

		const std::shared_ptr<ClusterGraph>& clusterGraph() const {
			OVITO_ASSERT(_clusterGraph);
			return _clusterGraph;
		}

		const std::shared_ptr<DislocationNetwork>& dislocations() {
			if(!_dislocations) _dislocations = std::make_shared<DislocationNetwork>(clusterGraph());
			return _dislocations;
		}

		const std::shared_ptr<HalfEdgeMesh<>>& defectSurface() {
			if(!_defectSurface) _defectSurface = std::make_shared<HalfEdgeMesh<>>();
			return _defectSurface;
		}

		const std::shared_ptr<PartitionMeshData>& partitionMesh() {
			if(!_partitionMesh) _partitionMesh = std::make_shared<PartitionMeshData>();
			return _partitionMesh;
		}

	protected:

		/// The structure pattern catalog.
		QVector<PatternInfo> _patterns;

		/// The cluster list.
		std::shared_ptr<ClusterGraph> _clusterGraph;

		/// The dislocation segments.
		std::shared_ptr<DislocationNetwork> _dislocations;

		/// The triangle mesh of the defect surface.
		std::shared_ptr<HalfEdgeMesh<>> _defectSurface;
		
		/// The partition mesh.
		std::shared_ptr<PartitionMeshData> _partitionMesh;
	};

	/// The format-specific task object that is responsible for reading an input file in the background.
	class FrameLoader : public FileSourceImporter::FrameLoader
	{
	public:

		/// Inherit constructor from base class.
		using FileSourceImporter::FrameLoader::FrameLoader;

	protected:

		/// Loads the frame data from the given file.
		virtual FrameDataPtr loadFile(QFile& file) override;
	};

	/// The format-specific task object that is responsible for scanning the input file for animation frames. 
	class FrameFinder : public FileSourceImporter::FrameFinder
	{
	public:

		/// Inherit constructor from base class.
		using FileSourceImporter::FrameFinder::FrameFinder;

	protected:

		/// Scans the given file for source frames.
		virtual void discoverFramesInFile(QFile& file, const QUrl& sourceUrl, QVector<FileSourceImporter::Frame>& frames) override;	
	};
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


