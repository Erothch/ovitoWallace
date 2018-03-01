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

/**
 * \file
 * \brief Contains forward declarations of OVITO's core classes and namespaces.
 */

#pragma once


// Sub-namespaces are only used for the documentation generated by Doxygen,
// because current C++ compilers do not fully support C++11 inline namespaces yet.
#define OVITO_BEGIN_INLINE_NAMESPACE(ns)
#define OVITO_END_INLINE_NAMESPACE

namespace Ovito {

	class Application;
	OVITO_BEGIN_INLINE_NAMESPACE(Util)
		OVITO_BEGIN_INLINE_NAMESPACE(IO)
			class FileManager;
			class ObjectSaveStream;
			class ObjectLoadStream;
			class CompressedTextReader;
			class CompressedTextWriter;
			OVITO_BEGIN_INLINE_NAMESPACE(Internal)
				class VideoEncoder;
				class SftpJob;
			OVITO_END_INLINE_NAMESPACE
		OVITO_END_INLINE_NAMESPACE
		OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)
			class PromiseBase;
			class FutureBase;
			class PromiseState;
			class TaskManager;
			class PromiseWatcher;
			class AsynchronousTaskBase;
			class TrackingPromiseState;
			template<typename tuple_type> class DirectContinuationPromiseState;
			template<typename... R> class Future;
			template<typename... R> class SharedFuture;
			using PromiseStatePtr = std::shared_ptr<PromiseState>;
		OVITO_END_INLINE_NAMESPACE
		OVITO_BEGIN_INLINE_NAMESPACE(Mesh)
			class TriMesh;
			template<typename> struct EmptyHalfEdgeMeshStruct;
			template<template<typename> class EdgeBase = EmptyHalfEdgeMeshStruct, template<typename> class FaceBase = EmptyHalfEdgeMeshStruct, template<typename> class VertexBase = EmptyHalfEdgeMeshStruct>
			class HalfEdgeMesh;
		OVITO_END_INLINE_NAMESPACE
		OVITO_BEGIN_INLINE_NAMESPACE(Math)
		OVITO_END_INLINE_NAMESPACE
	OVITO_END_INLINE_NAMESPACE
	OVITO_BEGIN_INLINE_NAMESPACE(Anim)
		class Controller;
		class AnimationSettings;
		class LookAtController;
		class KeyframeController;
		class PRSTransformationController;
	OVITO_END_INLINE_NAMESPACE
	OVITO_BEGIN_INLINE_NAMESPACE(PluginSystem)
		class Plugin;
		class PluginManager;
		class ApplicationService;
		OVITO_BEGIN_INLINE_NAMESPACE(Internal)
			class NativePlugin;
		OVITO_END_INLINE_NAMESPACE
	OVITO_END_INLINE_NAMESPACE
	OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)
		class OvitoObject;
		class OvitoClass;
		using OvitoClassPtr = const OvitoClass*;
		class CloneHelper;
		class RefMaker;
		class RefMakerClass;
		class RefTarget;
		class PropertyFieldDescriptor;
		class PropertyFieldBase;
		template<typename property_data_type> class RuntimePropertyField;
		template<typename property_data_type> class PropertyField;
		class SingleReferenceFieldBase;
		template<typename RefTargetType> class ReferenceField;
		class VectorReferenceFieldBase;
		template<typename RefTargetType> class VectorReferenceField;
		class DataSet;
		class DataSetContainer;
		OVITO_BEGIN_INLINE_NAMESPACE(Units)
			class ParameterUnit;
		OVITO_END_INLINE_NAMESPACE
		OVITO_BEGIN_INLINE_NAMESPACE(Undo)
			class UndoStack;
			class UndoableOperation;
		OVITO_END_INLINE_NAMESPACE
		OVITO_BEGIN_INLINE_NAMESPACE(Scene)
			class SceneNode;
			class DataObject;
			class RootSceneNode;
			class SelectionSet;
			class Modifier;
			class ModifierClass;
			using ModifierClassPtr = const ModifierClass*;
			class ModifierApplication;
			class PipelineSceneNode;
			class PipelineFlowState;
			class PipelineObject;
			class PipelineCache;
			class CachingPipelineObject;
			class DataVis;
			class TransformingDataVis;
			class StaticSource;
			class ModifierDelegate;
			class DelegatingModifier;
			class MultiDelegatingModifier;
			OVITO_BEGIN_INLINE_NAMESPACE(StdObj)
				class AbstractCameraObject;
			OVITO_END_INLINE_NAMESPACE
		OVITO_END_INLINE_NAMESPACE
	OVITO_END_INLINE_NAMESPACE
	OVITO_BEGIN_INLINE_NAMESPACE(Rendering)
		class SceneRenderer;
		class ObjectPickInfo;
		class RenderSettings;
		class FrameBuffer;
		OVITO_BEGIN_INLINE_NAMESPACE(Internal)
		OVITO_END_INLINE_NAMESPACE
	OVITO_END_INLINE_NAMESPACE
	OVITO_BEGIN_INLINE_NAMESPACE(View)
		class Viewport;
		class ViewportConfiguration;
		class ViewportSettings;
		struct ViewProjectionParameters;
		class ViewportOverlay;
	OVITO_END_INLINE_NAMESPACE
	OVITO_BEGIN_INLINE_NAMESPACE(DataIO)
		class FileImporter;
		class FileImporterClass;
		class FileExporter;
		class FileExporterClass;
		class FileSource;
		class FileSourceImporter;
	OVITO_END_INLINE_NAMESPACE
	OVITO_BEGIN_INLINE_NAMESPACE(Gui)
		class MainWindow;
	OVITO_END_INLINE_NAMESPACE

	// This should only be visible to Doxygen:
#ifdef ONLY_FOR_DOXYGEN
	using namespace Util;
	using namespace Util::IO;
	using namespace Util::Math;
	using namespace Util::Mesh;
	using namespace Util::Concurrency;
	using namespace Rendering;
	using namespace View;
	using namespace DataIO;
	using namespace Anim;
	using namespace PluginSystem;
	using namespace ObjectSystem;
	using namespace ObjectSystem::Units;
	using namespace ObjectSystem::Undo;
	using namespace ObjectSystem::Scene;
	using namespace ObjectSystem::Scene::StdObj;
#endif
}


