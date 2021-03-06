#pragma once

#include <iterator>
#include <vector>
#include <map>
#include <string>
#include <math.h>
#include <bx/math.h>
#include <bx/bx.h>
#include <bimg/bimg.h>
#include <bx/allocator.h>
#include <bgfx/platform.h>
#include <bgfx/bgfx.h>
#include "log.h"

enum RendererBlendMode {
    BLEND_AVG = 0,
    BLEND_ADD,
    BLEND_SUB,
    BLEND_25P,
    BLEND_NONE
};

enum RendererCullMode {
    DISABLED = 0,
    FRONT,
    BACK
};

enum RendererAlphaFunc {
    NEVER = 0,
    LESS,
    EQUAL,
    LEQUAL,
    GREATER,
    NOTEQUAL,
    GEQUAL,
    ALWAYS
};

enum RendererPrimitiveType
{
    PT_POINTS = 0,
    PT_LINES,
    PT_LINE_LOOP,
    PT_LINE_STRIP,
    PT_TRIANGLES,
    PT_TRIANGLE_STRIP,
    PT_TRIANGLE_FAN,
    PT_QUADS,
    PT_QUAD_STRIP
};

enum RendererTextureType
{
    BGRA = 0,
    YUV
};

enum RendererInternalType
{
    RGBA8 = 0,
    COMPRESSED_RGBA
};

struct RendererCallbacks : public bgfx::CallbackI {
    virtual ~RendererCallbacks()
    {
    }

    virtual void fatal(const char* _filePath, uint16_t _line, bgfx::Fatal::Enum _code, const char* _str) override
    {
        std::string error;

        switch (_code) {
        case bgfx::Fatal::Enum::DebugCheck: error = "Debug Check";
        case bgfx::Fatal::Enum::InvalidShader: error = "Invalid Shader";
        case bgfx::Fatal::Enum::UnableToInitialize: error = "Unable To Initialize";
        case bgfx::Fatal::Enum::UnableToCreateTexture: error = "Unable To Create Texture";
        case bgfx::Fatal::Enum::DeviceLost: error = "Device Lost";
        }

        error("[%s] %s\n", error.c_str(), _str);
    }

    virtual void traceVargs(const char* _filePath, uint16_t _line, const char* _format, va_list _argList) override
    {
        if (renderer_debug)
        {
            char buffer[16 * 1024];

            va_list argListCopy;
            va_copy(argListCopy, _argList);
            vsnprintf(buffer, sizeof(buffer), _format, argListCopy);
            va_end(argListCopy);

            trace("%s", buffer);
        }
    }

    virtual void profilerBegin(const char* _name, uint32_t _abgr, const char* _filePath, uint16_t _line) override
    {
    }

    virtual void profilerBeginLiteral(const char* _name, uint32_t _abgr, const char* _filePath, uint16_t _line) override
    {
    }

    virtual void profilerEnd() override
    {
    }

    virtual uint32_t cacheReadSize(uint64_t _id) override
    {
        // Shader not found
        return 0;
    }

    virtual bool cacheRead(uint64_t _id, void* _data, uint32_t _size) override
    {
        // Rebuild Shader
        return false;
    }

    virtual void cacheWrite(uint64_t _id, const void* _data, uint32_t _size) override
    {
    }

    virtual void screenShot(const char* _filePath, uint32_t _width, uint32_t _height, uint32_t _pitch, const void* _data, uint32_t _size, bool _yflip) override
    {
    }

    virtual void captureBegin(uint32_t _width, uint32_t _height, uint32_t _pitch, bgfx::TextureFormat::Enum _format, bool _yflip) override
    {
    }

    virtual void captureEnd() override
    {
    }

    virtual void captureFrame(const void* _data, uint32_t _size) override
    {
    }
};

class Renderer {
private:
    // Current renderer view
    enum RendererView {
        FRAMEBUFFER = 0,
        POSTPROCESSING
    };

    // Vertex data structure
    struct Vertex
    {
        float x;
        float y;
        float z;
        float w;
        uint bgra;
        float u;
        float v;
    };

    struct RendererState
    {
        uint16_t texHandlers[3];

        bool bDoAlphaTest = false;
        float alphaRef = 0.0f;
        RendererAlphaFunc alphaFunc;

        bool bDoDepthTest = false;
        bool bDoDepthWrite = false;

        bool bIsTLVertex = false;
        bool bIsFBTexture = false;
        bool bIsTexture = false;
        bool bDoTextureFiltering = false;
        bool bInheritTextureAlpha = false;
        bool bIsMovie = false;
        bool bIsMovieFullRange = false;
        bool bIsMovieYUV = false;

        float backendProjMatrix[16];

        std::vector<float> VSFlags;
        std::vector<float> FSAlphaFlags;
        std::vector<float> FSMiscFlags;

        float d3dViewMatrix[16];
        float d3dProjectionMatrix[16];
        float worldViewMatrix[16];

        uint32_t clearColorValue;

        RendererCullMode cullMode = RendererCullMode::DISABLED;
        RendererBlendMode blendMode = RendererBlendMode::BLEND_NONE;
        RendererPrimitiveType primitiveType = RendererPrimitiveType::PT_TRIANGLES;

        uint64_t state = BGFX_STATE_MSAA;
    };

    char shaderTextureBindings[3][6] = {
            "tex", // BGRA or Y share the same binding
            "tex_u",
            "tex_v",
    };

    std::string vertexPath = "shaders/FFNx";
    std::string fragmentPath = "shaders/FFNx";
    std::string vertexPostPath = "shaders/FFNx.post";
    std::string fragmentPostPath = "shaders/FFNx.post";

    bgfx::ViewId backendViewId = RendererView::FRAMEBUFFER;

    std::vector<bgfx::ProgramHandle> backendProgramHandles = { BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE };

    std::vector<bgfx::TextureHandle> backendFrameBufferRT = { BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE };
    bgfx::FrameBufferHandle backendFrameBuffer = BGFX_INVALID_HANDLE;

    bgfx::VertexBufferHandle vertexBufferHandle = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle indexBufferHandle = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout vertexLayout;

    bgfx::TextureHandle emptyTexture = BGFX_INVALID_HANDLE;

    std::map<std::string,uint16_t> bgfxUniformHandles;

    RendererState internalState;

    RendererCallbacks bgfxCallbacks;

    float viewOffsetX = 0.0f;
    float viewOffsetY = 0.0f;
    float viewWidth = 0.0f;
    float viewHeight = 0.0f;

    uint32_t createBGRA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void setCommonUniforms();
    bgfx::RendererType::Enum getRendererType();
    bgfx::ShaderHandle getShader(const char* filePath);

    bgfx::UniformHandle getUniform(std::string uniformName, bgfx::UniformType::Enum uniformType);
    bgfx::UniformHandle setUniform(const char* uniformName, bgfx::UniformType::Enum uniformType, const void* uniformValue);
    void destroyUniforms();
    void destroyAll();

    void renderFrameBuffer();

    void printMatrix(char* name, float* mat);

public:
    void init();
    void shutdown();

    void draw();
    void show();

    void reset();

    void printText(uint16_t x, uint16_t y, uint attr, const char* text);

    // ---

    const bgfx::Caps* getCaps();

    void bindVertexBuffer(struct nvertex* inVertex, uint inCount);
    void bindIndexBuffer(word* inIndex, uint inCount);

    void setScissor(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void setClearFlags(bool doClearColor = false, bool doClearDepth = false);
    void setBackgroundColor(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f);

    uint createTexture(uint8_t* data, size_t width, size_t height, int stride = 0, RendererTextureType type = RendererTextureType::BGRA, bool generateMips = false);
    void deleteTexture(uint16_t texId);
    void useTexture(uint texId, uint slot = 0);
    uint blitTexture(uint x, uint y, uint width, uint height);

    void isMovie(bool flag = false);
    void isTLVertex(bool flag = false);
    void setBlendMode(RendererBlendMode mode = RendererBlendMode::BLEND_NONE);
    void isTexture(bool flag = false);
    void isFBTexture(bool flag = false);
    void isFullRange(bool flag = false);
    void isYUV(bool flag = false);
    void doInheritTextureAlpha(bool flag = false);
    void doTextureFiltering(bool flag = false);

    // Alpha mode emulation
    void setAlphaRef(RendererAlphaFunc func = RendererAlphaFunc::ALWAYS, float ref = 0.0f);
    void doAlphaTest(bool flag = false);

    // Internal states
    void setPrimitiveType(RendererPrimitiveType type = RendererPrimitiveType::PT_TRIANGLES);
    void setCullMode(RendererCullMode mode = RendererCullMode::DISABLED);
    void doDepthTest(bool flag = false);
    void doDepthWrite(bool flag = false);

    // Wireframe mode
    void setWireframeMode(bool flag = false);

    // Viewport
    void setWorldView(struct matrix* matrix);
    void setD3DViweport(struct matrix* matrix);
    void setD3DProjection(struct matrix* matrix);

    // Internal coord calculation
    float getInternalCoordX(float inX);
    float getInternalCoordY(float inY);

    // Hardware capabilities
    bool supportsAutoMips(RendererTextureType type = RendererTextureType::BGRA);
};

extern Renderer newRenderer;
