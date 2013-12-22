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

#ifndef __OVITO_LAMMPS_BINARY_DUMP_IMPORTER_H
#define __OVITO_LAMMPS_BINARY_DUMP_IMPORTER_H

#include <plugins/particles/Particles.h>
#include <core/gui/properties/PropertiesEditor.h>
#include <plugins/particles/importer/InputColumnMapping.h>
#include <plugins/particles/importer/ParticleImporter.h>

namespace Particles {

using namespace Ovito;

/**
 * \brief File parser for binary LAMMPS dump files.
 */
class OVITO_PARTICLES_EXPORT LAMMPSBinaryDumpImporter : public ParticleImporter
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE LAMMPSBinaryDumpImporter(DataSet* dataset) : ParticleImporter(dataset) {}

	/// \brief Returns the file filter that specifies the files that can be imported by this service.
	/// \return A wild-card pattern that specifies the file types that can be handled by this import class.
	virtual QString fileFilter() override { return QStringLiteral("*"); }

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	/// \return A string that describes the file format.
	virtual QString fileFilterDescription() override { return tr("LAMMPS Binary Dump Files"); }

	/// \brief Checks if the given file has format that can be read by this importer.
	virtual bool checkFileFormat(QIODevice& input, const QUrl& sourceLocation) override;

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("LAMMPS Dump File"); }

	/// This method is called by the LinkedFileObject each time a new source
	/// file has been selected by the user.
	virtual bool inspectNewFile(LinkedFileObject* obj) override;

	/// \brief Returns the user-defined mapping between data columns in the input file and
	///        the internal particle properties.
	const InputColumnMapping& columnMapping() const { return _columnMapping; }

	/// \brief Sets the user-defined mapping between data columns in the input file and
	///        the internal particle properties.
	void setColumnMapping(const InputColumnMapping& mapping);

	/// Displays a dialog box that allows the user to edit the custom file column to particle
	/// property mapping.
	void showEditColumnMappingDialog(QWidget* parent);

protected:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class LAMMPSBinaryDumpImportTask : public ParticleImportTask
	{
	public:

		/// Normal constructor.
		LAMMPSBinaryDumpImportTask(const LinkedFileImporter::FrameSourceInformation& frame, const InputColumnMapping& columnMapping)
			: ParticleImportTask(frame), _parseFileHeaderOnly(false), _columnMapping(columnMapping) {}

		/// Constructor used when reading only the file header information.
		LAMMPSBinaryDumpImportTask(const LinkedFileImporter::FrameSourceInformation& frame)
			: ParticleImportTask(frame), _parseFileHeaderOnly(true) {}

		/// Returns the file column mapping used to load the file.
		const InputColumnMapping& columnMapping() const { return _columnMapping; }

	protected:

		/// Parses the given input file and stores the data in this container object.
		virtual void parseFile(FutureInterfaceBase& futureInterface, CompressedTextParserStream& stream) override;

	private:

		bool _parseFileHeaderOnly;
		InputColumnMapping _columnMapping;
	};

protected:

	/// \brief Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// \brief Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// \brief Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) override;

	/// \brief Creates an import task object to read the given frame.
	virtual ImportTaskPtr createImportTask(const FrameSourceInformation& frame) override {
		return std::make_shared<LAMMPSBinaryDumpImportTask>(frame, _columnMapping);
	}

	/// \brief Scans the given input file to find all contained simulation frames.
	virtual void scanFileForTimesteps(FutureInterfaceBase& futureInterface, QVector<LinkedFileImporter::FrameSourceInformation>& frames, const QUrl& sourceUrl, CompressedTextParserStream& stream) override;

private:

	/// Stores the user-defined mapping between data columns in the input file and
	/// the internal particle properties.
	InputColumnMapping _columnMapping;

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief A properties editor for the LAMMPSBinaryDumpImporter class.
 */
class LAMMPSBinaryDumpImporterEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE LAMMPSBinaryDumpImporterEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Is called when the user pressed the "Edit column mapping" button.
	void onEditColumnMapping();

private:

	Q_OBJECT
	OVITO_OBJECT
};


};

#endif // __OVITO_LAMMPS_BINARY_DUMP_IMPORTER_H