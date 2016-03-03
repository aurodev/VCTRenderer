#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "deferred_handler.h"

#include <oglplus/vertex_attrib.hpp>
#include <oglplus/bound/texture.hpp>

#include "../core/assets_manager.h"
#include "../types/geometry_buffer.h"
#include "../programs/lighting_program.h"
#include "../programs/geometry_program.h"

std::unique_ptr<GeometryBuffer> DeferredHandler::geometryBuffer = nullptr;

DeferredHandler::DeferredHandler()
{
    LoadShaders();
    CreateFullscreenQuad();
}

void DeferredHandler::CreateFullscreenQuad() const
{
    using namespace oglplus;
    // bind vao for full screen quad
    fsQuadVertexArray.Bind();
    // data for fs quad
    static const std::array<float, 20> fsQuadVertexBufferData =
    {
        // X    Y    Z     U     V
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // vertex 0
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // vertex 1
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // vertex 2
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // vertex 3
    };
    // bind vertex buffer and fill
    fsQuadVertexBuffer.Bind(Buffer::Target::Array);
    Buffer::Data(Buffer::Target::Array, fsQuadVertexBufferData);
    // set up attrib points
    VertexArrayAttrib(VertexAttribSlot(0)).Enable() // position
    .Pointer(3, DataType::Float, false, 5 * sizeof(float),
             reinterpret_cast<const GLvoid *>(0));
    VertexArrayAttrib(VertexAttribSlot(1)).Enable() // uvs
    .Pointer(2, DataType::Float, false, 5 * sizeof(float),
             reinterpret_cast<const GLvoid *>(12));
    // data for element buffer array
    static const std::array<unsigned int, 6> indexData =
    {
        0, 1, 2, // first triangle
        2, 1, 3, // second triangle
    };
    // bind and fill element array
    fsQuadElementBuffer.Bind(Buffer::Target::ElementArray);
    Buffer::Data(Buffer::Target::ElementArray, indexData);
    // unbind vao
    NoVertexArray().Bind();
}

GeometryProgram &DeferredHandler::GeometryPass() const
{
    return *geometryProgram;
}

LightingProgram &DeferredHandler::LightingPass() const
{
    return *lightingProgram;
}

void DeferredHandler::DrawFullscreenQuad() const
{
    static oglplus::Context gl;
    fsQuadVertexArray.Bind();
    gl.DrawElements(
        oglplus::PrimitiveType::Triangles, 6,
        oglplus::DataType::UnsignedInt
    );
}

void DeferredHandler::LoadShaders()
{
    static auto &assets = AssetsManager::Instance();
    // get program shaders from assets manager
    geometryProgram = static_cast<GeometryProgram *>
                      (assets->programs[AssetsManager::GeometryPass].get());
    lightingProgram = static_cast<LightingProgram *>
                      (assets->programs[AssetsManager::LightPass].get());
    geometryProgram->Link();
    lightingProgram->Link();
    // geometry pass shader source code and compile
    geometryProgram->ExtractUniforms();
    //// light pass shader source code and compile
    lightingProgram->ExtractUniforms();
}

void DeferredHandler::SetupGeometryBuffer(unsigned int windowWidth,
        unsigned int windowHeight)
{
    using namespace oglplus;
    static Context gl;

    if (!geometryBuffer)
    {
        geometryBuffer = std::make_unique<GeometryBuffer>();
    }
    // already setup the geometry buffer, need to delete previous to resetup
    else { return; }

    // initialize geometry buffer
    geometryBuffer->Bind(FramebufferTarget::Draw);
    // build textures -- normal
    gl.Bound(TextureTarget::_2D,
             geometryBuffer->RenderTarget(GeometryBuffer::Normal))
    .Image2D(0, PixelDataInternalFormat::RGB8SNorm, windowWidth, windowHeight,
             0, PixelDataFormat::RGB, PixelDataType::Float, nullptr)
    .MinFilter(TextureMinFilter::Nearest)
    .MagFilter(TextureMagFilter::Nearest);
    geometryBuffer->AttachTexture(GeometryBuffer::Normal, FramebufferTarget::Draw);
    // build textures -- albedo
    gl.Bound(TextureTarget::_2D,
             geometryBuffer->RenderTarget(GeometryBuffer::Albedo))
    .Image2D(0, PixelDataInternalFormat::RGB8, windowWidth, windowHeight,
             0, PixelDataFormat::RGB, PixelDataType::Float, nullptr)
    .MinFilter(TextureMinFilter::Nearest)
    .MagFilter(TextureMagFilter::Nearest);
    geometryBuffer->AttachTexture(GeometryBuffer::Albedo, FramebufferTarget::Draw);
    // build textures -- specular
    gl.Bound(TextureTarget::_2D,
             geometryBuffer->RenderTarget(GeometryBuffer::Specular))
    .Image2D(0, PixelDataInternalFormat::RGB8, windowWidth, windowHeight,
             0, PixelDataFormat::RGB, PixelDataType::Float, nullptr)
    .MinFilter(TextureMinFilter::Nearest)
    .MagFilter(TextureMagFilter::Nearest);
    geometryBuffer->AttachTexture(GeometryBuffer::Specular,
                                  FramebufferTarget::Draw);
    // attach depth texture for depth testing
    gl.Bound(TextureTarget::_2D,
             geometryBuffer->RenderTarget(GeometryBuffer::Depth))
    .Image2D(0, PixelDataInternalFormat::DepthComponent24, windowWidth,
             windowHeight, 0, PixelDataFormat::DepthComponent,
             PixelDataType::Float, nullptr)
    .MinFilter(TextureMinFilter::Nearest)
    .MagFilter(TextureMagFilter::Nearest);
    geometryBuffer->AttachTexture(GeometryBuffer::Depth, FramebufferTarget::Draw);
    // set draw buffers
    geometryBuffer->DrawBuffers();

    // check if success building frame buffer
    if (!Framebuffer::IsComplete(FramebufferTarget::Draw))
    {
        auto status = Framebuffer::Status(FramebufferTarget::Draw);
        Framebuffer::HandleIncompleteError(FramebufferTarget::Draw, status);
    }

    Framebuffer::Bind(Framebuffer::Target::Draw, FramebufferName(0));
}

const GeometryBuffer &DeferredHandler::GBuffer()
{
    return *geometryBuffer;
}

DeferredHandler::~DeferredHandler()
{
    // geometry buffer has context dependant components
    // its deletion needs to be included with the destructor
    // so it can be called before the context ceases to exist.
    delete geometryBuffer.release();
}
