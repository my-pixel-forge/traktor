#include "Render/ICubeTexture.h"
#include "Render/IndexBuffer.h"
#include "Render/IRenderSystem.h"
#include "Render/IRenderView.h"
#include "Render/Shader.h"
#include "Render/VertexBuffer.h"
#include "Render/VertexElement.h"
#include "Render/Context/RenderContext.h"
#include "Resource/IResourceManager.h"
#include "World/IWorldRenderPass.h"
#include "World/WorldContext.h"
#include "World/WorldRenderView.h"
#include "World/Entity/ProbeCapturer.h"
#include "World/Entity/ProbeComponent.h"
#include "World/Entity/ProbeFilterer.h"
#include "World/Entity/ProbeRenderer.h"

namespace traktor
{
	namespace world
	{
		namespace
		{

const resource::Id< render::Shader > c_probeShader(Guid(L"{99BB18CB-A744-D845-9A17-D0E586E4D9EA}"));

#pragma pack(1)
struct Vertex
{
	float position[3];
};
#pragma pack()

render::handle_t s_handleProbeLocal;
render::handle_t s_handleProbeVolumeCenter;
render::handle_t s_handleProbeVolumeExtent;
render::handle_t s_handleProbeTexture;
render::handle_t s_handleProbeTextureMips;
render::handle_t s_handleProbeIntensity;
render::handle_t s_handleMagicCoeffs;
render::handle_t s_handleWorldViewInv;

class ProbeCaptureRenderBlock : public render::RenderBlock
{
public:
	ProbeCapturer* capturer;
	Ref< render::ICubeTexture > texture;
	int32_t face;
	bool* pending;

	virtual void render(render::IRenderView* renderView) const override final
	{
		capturer->render(renderView, texture, face);
		
		// Need to flush renderer as we need to reuse sbuffers for lights
		// in world renderer, sbuffers are being queued in driver.
		renderView->flush();

		*pending = false;
	}
};

class ProbeDownSampleRenderBlock : public render::RenderBlock
{
public:
	ProbeFilterer* filterer;
	Ref< render::ICubeTexture > texture;
	bool* pending;

	virtual void render(render::IRenderView* renderView) const override final
	{
		filterer->render(renderView, texture);
		*pending = false;
	}
};

		}

T_IMPLEMENT_RTTI_CLASS(L"traktor.world.ProbeRenderer", ProbeRenderer, IEntityRenderer)

ProbeRenderer::ProbeRenderer(
	resource::IResourceManager* resourceManager,
	render::IRenderSystem* renderSystem,
	const TypeInfo& worldRendererType
)
:	m_captureFace(0)
,	m_capturePending(false)
,	m_capturing(false)
{
	m_probeCapturer = new ProbeCapturer(resourceManager, renderSystem, worldRendererType);
	m_probeCapturer->create();

	m_probeFilterer = new ProbeFilterer(resourceManager, renderSystem);
	m_probeFilterer->create();

	resourceManager->bind(c_probeShader, m_probeShader);

	s_handleProbeLocal = render::getParameterHandle(L"World_ProbeLocal");
	s_handleProbeVolumeCenter = render::getParameterHandle(L"World_ProbeVolumeCenter");
	s_handleProbeVolumeExtent = render::getParameterHandle(L"World_ProbeVolumeExtent");
	s_handleProbeTexture = render::getParameterHandle(L"World_ProbeTexture");
	s_handleProbeTextureMips = render::getParameterHandle(L"World_ProbeTextureMips");
	s_handleProbeIntensity = render::getParameterHandle(L"World_ProbeIntensity");
	s_handleMagicCoeffs = render::getParameterHandle(L"World_MagicCoeffs");
	s_handleWorldViewInv = render::getParameterHandle(L"World_WorldViewInv");

	AlignedVector< render::VertexElement > vertexElements;
	vertexElements.push_back(render::VertexElement(render::DuPosition, render::DtFloat3, offsetof(Vertex, position), 0));
	T_ASSERT_M (render::getVertexSize(vertexElements) == sizeof(Vertex), L"Incorrect size of vertex");

	m_vertexBuffer = renderSystem->createVertexBuffer(vertexElements, (4 + 8) * sizeof(Vertex), false);
	T_ASSERT_M (m_vertexBuffer, L"Unable to create vertex buffer");

	Vector4 extents[8];
	Aabb3(Vector4(-1.0f, -1.0f, -1.0f, 1.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f)).getExtents(extents);

	Vertex* vertex = static_cast< Vertex* >(m_vertexBuffer->lock());
	T_ASSERT(vertex);

	// Quad vertices.
	vertex[0].position[0] = -1.0f; vertex[0].position[1] =  1.0f; vertex[0].position[2] = 0.0f;
	vertex[1].position[0] =  1.0f; vertex[1].position[1] =  1.0f; vertex[1].position[2] = 0.0f;
	vertex[2].position[0] =  1.0f; vertex[2].position[1] = -1.0f; vertex[2].position[2] = 0.0f;
	vertex[3].position[0] = -1.0f; vertex[3].position[1] = -1.0f; vertex[3].position[2] = 0.0f;
	vertex += 4;

	// Unit cube vertices.
	for (uint32_t i = 0; i < sizeof_array(extents); ++i)
	{
		vertex->position[0] = extents[i].x();
		vertex->position[1] = extents[i].y();
		vertex->position[2] = extents[i].z();
		vertex++;
	}

	m_vertexBuffer->unlock();

	m_indexBuffer = renderSystem->createIndexBuffer(render::ItUInt16, (2 * 3 + 6 * 2 * 3) * sizeof(uint16_t), false);
	T_ASSERT_M (m_indexBuffer, L"Unable to create index buffer");

	uint16_t* index = static_cast< uint16_t* >(m_indexBuffer->lock());
	T_ASSERT(index);
	
	// Quad faces.
	*index++ = 0;
	*index++ = 3;
	*index++ = 1;
	*index++ = 2;
	*index++ = 1;
	*index++ = 3;

	// Unit cube faces.
	const int32_t* faces = Aabb3::getFaces();
	for (uint32_t i = 0; i < 6; ++i)
	{
		*index++ = faces[i * 4 + 0] + 4;
		*index++ = faces[i * 4 + 1] + 4;
		*index++ = faces[i * 4 + 3] + 4;
		*index++ = faces[i * 4 + 1] + 4;
		*index++ = faces[i * 4 + 2] + 4;
		*index++ = faces[i * 4 + 3] + 4;
	}

	m_indexBuffer->unlock();
}

const TypeInfoSet ProbeRenderer::getRenderableTypes() const
{
	return makeTypeInfoSet< ProbeComponent >();
}

void ProbeRenderer::render(
	WorldContext& worldContext,
	WorldRenderView& worldRenderView,
	const IWorldRenderPass& worldRenderPass,
	Object* renderable
)
{
	ProbeComponent* probeComponent = mandatory_non_null_type_cast< ProbeComponent* >(renderable);
	if (probeComponent->getLocal())
	{
		const Transform& transform = probeComponent->getTransform();

		Matrix44 worldView = worldRenderView.getView() * transform.toMatrix44();

		Vector4 center = worldView * probeComponent->getVolume().getCenter().xyz1();
		Scalar radius = probeComponent->getVolume().getExtent().length();

		if (worldRenderView.getCullFrustum().inside(center, radius) == Frustum::IrOutside)
			return;
	}
	m_probeComponents.push_back(probeComponent);
}

void ProbeRenderer::flush(
	WorldContext& worldContext,
	WorldRenderView& worldRenderView,
	const IWorldRenderPass& worldRenderPass,
	Entity* rootEntity
)
{
	// Ignore recursive calls to flush.
	if (m_capturing)
		return;

	render::RenderContext* renderContext = worldContext.getRenderContext();
	T_ASSERT(renderContext);

	// Render captures.
	if ((worldRenderPass.getPassFlags() & world::IWorldRenderPass::PfFirst) != 0)
	{
		if (!m_capturing)
		{
			m_capturing = true;

			// Get dirty probe which needs to be updated.
			if (!m_capture)
			{
				for (auto probeComponent : m_probeComponents)
				{
					if (probeComponent->getDirty())
					{
						m_capture = probeComponent;
						m_captureFace = 0;
						m_capturePending = false;
						break;
					}
				}
			}

			// Progress updating probe, ensure last captured side is finished before progressing.
			// Note the possible added latency if multiple queued frames are being used due to threaded rendering.
			if (m_capture && !m_capturePending)
			{
				if (m_captureFace < 6)
				{
					// Build probe context.
					m_probeCapturer->build(
						worldContext.getEntityRenderers(),
						rootEntity,
						m_capture->getTransform().translation().xyz1(),
						m_captureFace
					);

					// Chain probe render as render block.
					auto renderBlock = worldContext.getRenderContext()->alloc< ProbeCaptureRenderBlock >();
					renderBlock->capturer = m_probeCapturer;
					renderBlock->texture = m_capture->getTexture();
					renderBlock->face = m_captureFace;
					renderBlock->pending = &m_capturePending;
					renderContext->enqueue(renderBlock);

					m_capturePending = true;
					m_captureFace++;
				}
				else if (m_captureFace == 6)
				{
					// Filter rest of mips.
					auto renderBlock = worldContext.getRenderContext()->alloc< ProbeDownSampleRenderBlock >();
					renderBlock->filterer = m_probeFilterer;
					renderBlock->texture = m_capture->getTexture();
					renderBlock->pending = &m_capturePending;
					renderContext->enqueue(renderBlock);

					m_capturePending = true;
					m_captureFace++;
				}
				else
				{
					// Probe capture is finished.
					m_capture->setDirty(false);
					m_capture = nullptr;
				}
			}

			m_capturing = false;
		}
	}

	if (!m_probeShader)
		return;

	const Matrix44& projection = worldRenderView.getProjection();
	const Matrix44& view = worldRenderView.getView();

	const Scalar p11 = projection.get(0, 0);
	const Scalar p22 = projection.get(1, 1);
	const Vector4 magicCoeffs(1.0f / p11, 1.0f / p22, 0.0f, 0.0f);

	for (auto probeComponent : m_probeComponents)
	{
		if (probeComponent->getDirty())
			continue;

		render::Shader* shader = m_probeShader;
		T_ASSERT(shader);

		worldRenderPass.setShaderTechnique(shader);
		worldRenderPass.setShaderCombination(shader);

		shader->setCombination(s_handleProbeLocal, probeComponent->getLocal());

		render::IProgram* program = shader->getCurrentProgram();
		if (!program)
			continue;

		const Transform& transform = probeComponent->getTransform();

		Matrix44 worldView = view * transform.toMatrix44();
		Matrix44 worldViewInv = worldView.inverse();

		render::IndexedRenderBlock* renderBlock = renderContext->alloc< render::IndexedRenderBlock >("Probe");

		renderBlock->distance = 0.0f;
		renderBlock->program = program;
		renderBlock->programParams = renderContext->alloc< render::ProgramParameters >();
		renderBlock->indexBuffer = m_indexBuffer;
		renderBlock->vertexBuffer = m_vertexBuffer;
		renderBlock->primitive = render::PtTriangles;
		
		if (!probeComponent->getLocal())
		{
			renderBlock->offset = 0;
			renderBlock->count = 2;
			renderBlock->minIndex = 0;
			renderBlock->maxIndex = 3;
		}
		else
		{
			renderBlock->offset = 6;
			renderBlock->count = 12;
			renderBlock->minIndex = 4;
			renderBlock->maxIndex = 11;
		}

		renderBlock->programParams->beginParameters(renderContext);

		worldRenderPass.setProgramParameters(
			renderBlock->programParams,
			transform,
			transform,
			probeComponent->getBoundingBox()
		);

		if (probeComponent->getLocal())
		{
			renderBlock->programParams->setVectorParameter(s_handleProbeVolumeCenter, probeComponent->getVolume().getCenter());
			renderBlock->programParams->setVectorParameter(s_handleProbeVolumeExtent, probeComponent->getVolume().getExtent());
		}

		renderBlock->programParams->setFloatParameter(s_handleProbeIntensity, probeComponent->getIntensity());
		renderBlock->programParams->setFloatParameter(s_handleProbeTextureMips, probeComponent->getTexture() != nullptr ? (float)probeComponent->getTexture()->getMips() : 0.0f);
		renderBlock->programParams->setVectorParameter(s_handleMagicCoeffs, magicCoeffs);
		renderBlock->programParams->setMatrixParameter(s_handleWorldViewInv, worldViewInv);
		renderBlock->programParams->setTextureParameter(s_handleProbeTexture, probeComponent->getTexture());

		renderBlock->programParams->endParameters(renderContext);

		renderContext->draw(render::RpOverlay, renderBlock);
	}

	// Flush all queued decals.
	m_probeComponents.resize(0);
}

	}
}
