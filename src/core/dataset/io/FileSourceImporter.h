///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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


#include <core/Core.h>
#include <core/dataset/io/FileImporter.h>
#include <core/dataset/pipeline/PipelineStatus.h>
#include <core/utilities/concurrent/Future.h>
#include <core/utilities/concurrent/Task.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

/**
 * \brief Base class for file parsers that can reload a file that has been imported into the scene.
 */
class OVITO_CORE_EXPORT FileSourceImporter : public FileImporter
{
	Q_OBJECT
	OVITO_CLASS(FileSourceImporter)
	
public:

	/// Data structure that stores meta information about a source animation frame.
	struct Frame {

		/// Default constructor.
		Frame() = default;

		/// Initialization constructor.
		Frame(QUrl url, qint64 offset = 0, int linenum = 1, const QDateTime& modTime = QDateTime(), const QString& name = QString(), qint64 parserInfo = 0)	:
			sourceFile(std::move(url)), byteOffset(offset), lineNumber(linenum), lastModificationTime(modTime), label(name), parserData(parserInfo) {}

		/// The source file that contains the data of the animation frame.
		QUrl sourceFile;

		/// The byte offset into the source file where the frame's data is stored.
		qint64 byteOffset = 0;

		/// The line number in the source file where the frame data is stored, if the file has a text-based format.
		int lineNumber = 1;

		/// The last modification time of the source file.
		/// This is used to detect changes of the source file, which let the stored byte offset become invalid.
		QDateTime lastModificationTime;

		/// The name or label of the source frame.
		QString label;

		/// An informational field that can be used by the file parser to store additional info about the frame.
		qint64 parserData = 0; 

		/// Compares two data records.
		bool operator!=(const Frame& other) const {
			return (sourceFile != other.sourceFile) ||
					(byteOffset != other.byteOffset) ||
					(lineNumber != other.lineNumber) ||
					(lastModificationTime != other.lastModificationTime) ||
					(parserData != other.parserData);
		}
	};

	/**
	 * Base class for data structures holding a single frame's data.
	 */
	class OVITO_CORE_EXPORT FrameData 
	{
	public:

		/// Transfers the loaded data into a data collection.
		/// This function is called by the system from the main thread after the asynchronous loading task
		/// has finished. An implementation of this method should try to re-use any existing data objects from the provided data collection.
		virtual OORef<DataCollection> handOver(const DataCollection* existing, bool isNewFile, FileSource* fileSource) = 0;

		/// Returns the status of the load operation.
		const PipelineStatus& status() const { return _status; }

		/// Sets the status of the load operation.
		void setStatus(const QString& statusText) { _status.setText(statusText); }

	private:

		/// Stores additional status information about the load operation.
		PipelineStatus _status;
	};

	/// A managed pointer to a FrameData instance.
	using FrameDataPtr = std::shared_ptr<FrameData>;

	/**
	 * Base class for frame data loader routines.
	 */
	class OVITO_CORE_EXPORT FrameLoader : public AsynchronousTask<FrameDataPtr>
	{
	public:

		/// Constructor.
		FrameLoader(const Frame& frame, const QString& filename) :
			_frame(frame), _localFilename(filename) {}

		/// Returns the source file information.
		const Frame& frame() const { return _frame; }

		/// Fetches the source URL and calls loadFile().
		virtual void perform() override;

	protected:

		/// Loads the frame data from the given file.
		virtual FrameDataPtr loadFile(QFile& file) = 0;

	private:

		/// The source file information.
		Frame _frame;

		/// The local copy of the input file.
		QString _localFilename;		
	};

	/// A managed pointer to a FrameLoader instance.
	using FrameLoaderPtr = std::shared_ptr<FrameLoader>;	

	/**
	 * Base class for frame discovery routines.
	 */
	class OVITO_CORE_EXPORT FrameFinder : public AsynchronousTask<QVector<Frame>> 
	{
	public:

		/// Constructor.
		FrameFinder(const QUrl& sourceUrl, const QString& filename) :
			_sourceUrl(sourceUrl), _localFilename(filename) {}

		/// Returns the source file information.
		const QUrl& sourceUrl() const { return _sourceUrl; }

		/// Scans the source URL for input frames.
		virtual void perform() override;

	protected:

		/// Scans the given file for source frames.
		virtual void discoverFramesInFile(QFile& file, const QUrl& sourceUrl, QVector<Frame>& frames);

	private:

		/// The source file information.
		QUrl _sourceUrl;

		/// The local copy of the file.
		QString _localFilename;
	};

	/// A managed pointer to a FrameFinder instance.
	using FrameFinderPtr = std::shared_ptr<FrameFinder>;

public:

	/// \brief Constructs a new instance of this class.
	FileSourceImporter(DataSet* dataset) : FileImporter(dataset) {}

	///////////////////////////// from FileImporter /////////////////////////////

	/// \brief Asks the importer if the option to replace the currently selected object
	///        with the new file is available.
	virtual bool isReplaceExistingPossible(const QUrl& sourceUrl) override;

	/// \brief Imports the given file into the scene.
	virtual bool importFile(std::vector<QUrl> sourceUrls, ImportMode importMode, bool autodetectFileSequences) override;

	//////////////////////////// Specific methods ////////////////////////////////

	/// This method indicates whether a wildcard pattern should be automatically generated
	/// when the user picks a new input filename. The default implementation returns true.
	/// Subclasses can override this method to disable generation of wildcard patterns.
	virtual bool autoGenerateWildcardPattern() { return true; }

	/// Scans the given external path(s) (which may be a directory and a wild-card pattern,
	/// or a single file containing multiple frames) to find all available animation frames.
	///
	/// \param sourceUrls The list of source files or wild-card patterns to scan for animation frames.
	/// \return A Future that will yield the list of discovered animation frames.
	///
	/// The default implementation of this method checks if the given URLs contain a wild-card pattern.
	/// If yes, it scans the directory to find all matching files.
	virtual Future<QVector<Frame>> discoverFrames(const std::vector<QUrl>& sourceUrls);

	/// Scans the given external path (which may be a directory and a wild-card pattern,
	/// or a single file containing multiple frames) to find all available animation frames.
	///
	/// \param sourceUrl The source file or wild-card patterns to scan for animation frames.
	/// \return A Future that will yield the list of discovered animation frames.
	///
	/// The default implementation of this method checks if the given URLs contain a wild-card pattern.
	/// If yes, it scans the directory to find all matching files.
	virtual Future<QVector<Frame>> discoverFrames(const QUrl& sourceUrl);

	/// \brief Returns the list of files that match the given wildcard pattern.
	static Future<std::vector<QUrl>> findWildcardMatches(const QUrl& sourceUrl, DataSet* dataset);

	/// \brief Sends a request to the FileSource owning this importer to reload the input file.
	void requestReload(int frame = -1);

	/// \brief Sends a request to the FileSource owning this importer to refresh the animation frame sequence.
	void requestFramesUpdate();

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FrameLoaderPtr createFrameLoader(const Frame& frame, const QString& localFilename) = 0;

	/// Creates an asynchronous frame discovery object that scans a file for contained animation frames.
	virtual FrameFinderPtr createFrameFinder(const QUrl& sourceUrl, const QString& localFilename) { return {}; }

protected:

	/// This method is called when the pipeline scene node for the FileSource is created.
	/// It can be overwritten by importer subclasses to customize the initial pipeline, add modifiers, etc.
	/// The default implementation does nothing.
	virtual void setupPipeline(PipelineSceneNode* node, FileSource* importObj) {}

	/// Checks if a filename matches to the given wildcard pattern.
	static bool matchesWildcardPattern(const QString& pattern, const QString& filename);

	/// Determines whether the input file should be scanned to discover all contained frames.
	/// The default implementation returns false.
	virtual bool shouldScanFileForFrames(const QUrl& sourceUrl) { return false; }

	/// Determines whether the URL contains a wildcard pattern.
	static bool isWildcardPattern(const QUrl& sourceUrl);
};

/// \brief Writes an animation frame information record to a binary output stream.
/// \relates FileSourceImporter::Frame
OVITO_CORE_EXPORT SaveStream& operator<<(SaveStream& stream, const FileSourceImporter::Frame& frame);

/// \brief Reads an animation frame information record from a binary input stream.
/// \relates FileSourceImporter::Frame
OVITO_CORE_EXPORT LoadStream& operator>>(LoadStream& stream, FileSourceImporter::Frame& frame);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
