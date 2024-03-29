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


#include <core/Core.h>
#include <core/utilities/concurrent/Task.h>
#include "Modifier.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief Base class for modifiers that compute their results in a background thread.
 */
class OVITO_CORE_EXPORT AsynchronousModifier : public Modifier
{
	Q_OBJECT
	OVITO_CLASS(AsynchronousModifier)
	
public:

	/**
	 * Abstract base class for compute engines.
	 */
	class OVITO_CORE_EXPORT ComputeEngine
	{
	public:

		/// Constructor.
		ComputeEngine(const TimeInterval& validityInterval = TimeInterval::infinite()) : 
			_validityInterval(validityInterval),
			_task(std::make_shared<ComputeEngineTask>(this)) {}

		/// Destructor.
		virtual ~ComputeEngine() = default;

		/// This method is called by the system after the computation was successfully completed.
		/// Subclasses should override this method in order to release working memory and any references to the input data. 
		virtual void cleanup();

		/// Computes the modifier's results.
		virtual void perform() = 0;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) = 0;

		/// Returns the validity period of the stored results.
		const TimeInterval& validityInterval() const { return _validityInterval; }

		/// Changes the stored validity period of the results.
		void setValidityInterval(const TimeInterval& iv) { _validityInterval = iv; }

		/// Returns the AsynchronousTask object associated with this engine.
		const std::shared_ptr<AsynchronousTask<>>& task() const { return _task; }

	private:

		/// Asynchronous task class for compute engines.
		class OVITO_CORE_EXPORT ComputeEngineTask : public AsynchronousTask<>
		{
		public:

			/// Constructor.
			ComputeEngineTask(ComputeEngine* engine) : _engine(engine) {}

			/// Is called when the asynchronous task begins to run.
			virtual void perform() override;

		private:

			/// Pointer to the compute engine that owns this task object.
			ComputeEngine* _engine;
		};

		/// The validity period of the stored results.
		TimeInterval _validityInterval;

		/// The asynchronous task object associated with this engine.
		std::shared_ptr<AsynchronousTask<>> _task;
	};

	/// A managed pointer to a ComputeEngine instance.
	using ComputeEnginePtr = std::shared_ptr<ComputeEngine>;

public:

	/// Constructor.
	AsynchronousModifier(DataSet* dataset);

	/// Asks the object for the result of the data pipeline.
	virtual Future<PipelineFlowState> evaluate(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Modifies the input data in an immediate, preliminary way.
	virtual void evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Suppress preliminary viewport updates when a parameter of the asynchronous modifier changes.
	virtual bool performPreliminaryUpdateAfterChange() override { return false; }
	
	/// This method indicates whether outdated computation results should be immediately discarded
	/// whenever the inputs of the modifier changes. By default, the method returns false to indicate
	/// that the results should be kept as long as a new computation is in progress. During this 
	/// transient phase, the old resuls may still be used by the pipeline for preliminary viewport updates.
	virtual bool discardResultsOnInputChange() const { return false; }

	/// This method indicates whether cached computation results of the modifier should be discarded whenever
	/// a parameter of the modifier changes. The default implementation returns true. Subclasses can override
	/// this method if the asynchronous computation results do not depend on certain parameters and their change
	/// should not trigger a recomputation.
	virtual bool discardResultsOnModifierChange(const PropertyFieldEvent& event) const { return true; }
	
protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) = 0;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
