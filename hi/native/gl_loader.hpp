#pragma once
#include "types.hpp"
#include "out.hpp"

// ------------------- OS-dependent Includes -------------------------
#if defined(__linux__)
    // linux impl
#elif defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   undef MessageBox
#   undef CreateWindow
#   include <gl/GL.h>
#endif

#pragma region macros
#if !defined(NDEBUG) && !defined(_DEBUG)
#   define _DEBUG
#endif

#ifdef _DEBUG
#   include <assert.h>
#else
#   ifndef assert
#       define assert
#   endif
#endif
// ======================= OpenGL Macros ======================================
#if defined(_WIN32)
#   define GL_CALLING_CONVENTION __stdcall *
#else
#   define GL_CALLING_CONVENTION *
#endif
#pragma endregion // macros

namespace gl {
#if defined(_MSC_VER)
    static inline void* OpenglLoader(const char* name) noexcept {
        void* p = (void*)wglGetProcAddress(name);
        if (!p || p == (void*)0x1 ||
            p == (void*)0x2 ||
            p == (void*)0x3 ||
            p == (void*)-1)
        {
            static HMODULE module = LoadLibraryA("opengl32.dll");
            p = (void*)GetProcAddress(module, name);
        }
        return p;
    } // OpenglLoader
#endif

    using LoadProc = void* (*)(const char* name);
    static const LoadProc loader = OpenglLoader;

    // DefaultReturnImpl
    namespace native {
        template <typename T> inline T DefaultReturnImpl() noexcept { return T{}; }
        template <> inline void DefaultReturnImpl<void>() noexcept {}
    } // namespace native
} // namespace gl

#define GL_CALL_CUSTOM(_Name, _RetType, _ProcT, _Args_Decl, _Args_Pass)       \
 static inline _RetType _Name _Args_Decl noexcept {                           \
    assert(::gl::loader && "gl::" #_Name " failed: OpenGL loader not set");   \
    static _RetType(GL_CALLING_CONVENTION proc)_ProcT =                      \
reinterpret_cast<_RetType(GL_CALLING_CONVENTION)_ProcT>(::gl::loader("gl" #_Name)); \
    assert(proc && "gl::" #_Name " failed with NULL procedure");              \
    return proc ? proc _Args_Pass                                             \
        : ::gl::native::DefaultReturnImpl<_RetType>();                        \
} // GL_CALL_CUSTOM

// ========================== opengl ==========================================

namespace gl {

#pragma region Types
    using Enum = u32;
    using Boolean = unsigned char;

    // A singleton (instead of an enum class) that contains bit-mask constants
    // used when clearing frame buffers.  
    // These values correspond to the standard OpenGL flags for glClear().  
    // They can be combined using the `|` operator, for example:
    //     gl::Clear(gl::BufferBit.Color | gl::BufferBit.Depth);
    //
    // Each flag indicates which buffer should be cleared before rendering a new frame:
    //   - Color   — clears the color buffer where the final scene pixels are stored.
    //   - Depth   — clears the depth buffer that contains per-pixel depth values.
    //   - Stencil — clears the stencil buffer if stencil testing is used.
    //
    // The minimal structure avoids dragging in large enums and integrates easily
    // into a small or experimental rendering framework.
    IO_CONSTEXPR_VAR struct BufferBit {
        IO_CONSTEXPR_VAR static int Depth = 0x00000100;
        IO_CONSTEXPR_VAR static int Stencil = 0x00000400;
        IO_CONSTEXPR_VAR static int Color = 0x00004000;
    } BufferBit;

    enum class Face : u32 {
        Front = 0x404,
        Back = 0x405,
        FrontAndBack = 0x408,
    };

    enum class Polygon : u32 {
        Point = 0x1B00,
        Line = 0x1B01,
        Fill = 0x1B02,
    };

    enum class Capability : u32 {
        Blend = 0x0BE2,
        CullFace = 0x0B44,
        DepthTest = 0x0B71,
        // others
    };

    enum class BlendFactor : u32 {
        Zero = 0,
        One = 1,
        SrcAlpha = 0x0302,
        OneMinusSrcAlpha = 0x0303,
        // others
    };

    enum class TextureTarget : u32 {
        Texture2D = 0xDE1,
        // others
    };

    enum class TextureParam : u32 {
        MinFilter = 0x2801,
        MagFilter = 0x2800,
        WrapS = 0x2802,
        WrapT = 0x2803,
        // others
    };

    enum class TextureFormat : u32 {
        RGBA = 0x1908,
        RGB = 0x1907,
    };

    enum class DataType : u32 {
        UnsignedByte = 0x1401,
    };

    enum class GetParam : u32 {
        Viewport = 0x0BA2,
        // others
    };

    enum class PrimitiveMode : u32 {
        Triangles = 0x0004,
        TriangleStrip = 0x0005,
        Lines = 0x0001,
        LineStrip = 0x0003,
        Points = 0x0000,
        // others
    };

    enum class DrawElementsType : u32 {
        Byte = 0x1400,
        UnsignedByte = 0x1401,
        Short = 0x1402,
        UnsignedShort = 0x1403,
        Int = 0x1404,
        UnsignedInt = 0x1405,
        Float = 0x1406,
    };
    IO_CONSTEXPR IO_NODISCARD
    u32 getDrawElementsTypeSize(DrawElementsType type) noexcept {
        switch (type) {
        case DrawElementsType::Byte:          IO_FALLTHROUGH;
        case DrawElementsType::UnsignedByte:  return 1;
        case DrawElementsType::Short:         IO_FALLTHROUGH;
        case DrawElementsType::UnsignedShort: return 2;
        case DrawElementsType::Int:           IO_FALLTHROUGH;
        case DrawElementsType::UnsignedInt:   IO_FALLTHROUGH;
        case DrawElementsType::Float:         IO_FALLTHROUGH;
        default:                              return 4;
        }
    }

    enum class BufferTarget : u32 {
        ArrayBuffer = 0x8892,
        ElementArrayBuffer = 0x8893,
        UniformBuffer = 0x8A11,
        // others
    };

    enum class BufferUsage : u32 {
        StaticDraw = 0x88E4,
        DynamicDraw = 0x88E8,
        StreamDraw = 0x88E0
    };

    enum class ShaderType : u32 {
        VertexShader = 0x8B31,
        FragmentShader = 0x8B30,
        // others
    };

    enum class ProgramProperty : u32 {
        LinkStatus = 0x8B82,
        InfoLogLength = 0x8B84
    };

    enum class ShaderProperty : u32 {
        CompileStatus = 0x8B81,
        InfoLogLength = 0x8B84
    };

#pragma endregion



#pragma region core state
    // ----------------------------------------------------------------------------
    // Core state
    // ----------------------------------------------------------------------------
    GL_CALL_CUSTOM(
        /*     name */ GetError,
        /* ret    T */ int,
        /* proc   T */ (),
        /*     decl */ (),
        /*     pass */ ())

    GL_CALL_CUSTOM(
        /*     name */ CullFace,
        /* ret    T */ void,
        /* proc   T */ (u32),
        /*     decl */ (Face face),
        /*     pass */ (static_cast<u32>(face)))

    GL_CALL_CUSTOM(PolygonMode, void,
        /* proc   T */ (u32, u32),
        /*     decl */ (Face face, Polygon mode),
        /*     pass */ (static_cast<u32>(face), static_cast<u32>(mode)))

    GL_CALL_CUSTOM(TexParameterf, void,
        /* proc   T */ (u32, u32, float),
        /*     decl */ (TextureTarget target, TextureParam pname, float param),
        /*     pass */ (static_cast<u32>(target), static_cast<u32>(pname), param))

    GL_CALL_CUSTOM(TexParameteri, void,
        /* proc   T */ (u32, u32, int),
        /*     decl */ (TextureTarget target, TextureParam pname, int param),
        /*     pass */ (static_cast<u32>(target), static_cast<u32>(pname), param))

    GL_CALL_CUSTOM(TexImage2D, void,
        /* proc   T */ (u32,
            int, int, int, int, int,
            u32, u32,
            const void*),
        /*     decl */ (TextureTarget target,
            int level, int internalformat,
            int width, int height, int border,
            TextureFormat format,
            DataType type,
            const void* pixels),
        /*      pass */ (
            static_cast<u32>(target),
            level, internalformat,
            width, height, border,
            static_cast<u32>(format),
            static_cast<u32>(type),
            pixels))

    // ----------------------------------------------------------------------------
    // Example usage in the render loop:
    //
    //   // 1. set background color
    //   gl::ClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    //   // 2. clear color and depth buffers
    //   gl::Clear(gl::BufferBit.Color | gl::BufferBit.Depth);
    //   // 3. after this you can draw your scene geometry
    //
    // A wrapper around glClear(), generated through the GL_CALL_CUSTOM macro.
    // The function takes a bit-mask (e.g. Color | Depth) and calls the actual
    // OpenGL function to clear the corresponding buffers before rendering.
    //
    // In the rendering pipeline, clearing is the first step before drawing a new
    // frame. If you don't clear the buffers:
    //
    //   • The color buffer may hold pixels from the previous frame;
    //   • The depth buffer may contain old depth data, causing new objects to be
    //     incorrectly hidden or mis-sorted by Z;
    //   • The stencil buffer may contain leftover stencil masks.
    // ----------------------------------------------------------------------------
    GL_CALL_CUSTOM(Clear, void,
        /* proc   T */ (u32),
        /*     decl */ (u32 mask),
        /*     pass */ (mask))

    GL_CALL_CUSTOM(ClearColor, void,
        /* proc   T */ (float, float, float, float),
        /*     decl */ (float r, float g, float b, float a),
        /*     pass */ (r, g, b, a))

    GL_CALL_CUSTOM(Disable, void,
        /* proc   T */ (u32),
        /*     decl */ (Capability cap),
        /*     pass */ (static_cast<u32>(cap)))

    GL_CALL_CUSTOM(Enable, void,
        /* proc   T */ (u32),
        /*     decl */ (Capability cap),
        /*     pass */ (static_cast<u32>(cap)))

    GL_CALL_CUSTOM(BlendFunc, void,
        /* proc   T */ (u32, u32),
        /*     decl */ (BlendFactor sfactor, BlendFactor dfactor),
        /*     pass */ (static_cast<u32>(sfactor), static_cast<u32>(dfactor)))

    GL_CALL_CUSTOM(GetFloatv, void,
        /* proc   T */ (u32, float*),
        /*     decl */ (GetParam pname, float* data),
        /*     pass */ (static_cast<u32>(pname), data))

    GL_CALL_CUSTOM(GetString, const unsigned char*,
        /* proc   T */ (u32),
        /*     decl */ (u32 name),
        /*     pass */ (name))

    GL_CALL_CUSTOM(Viewport, void,
        /* proc   T */ (int, int, int, int),
        /*     decl */ (int x, int y, int width, int height),
        /*     pass */ (x, y, width, height))
#pragma endregion


#pragma region 1.1
    // ----------------------------------------------------------------------------
    // GL_VERSION_1_1
    // ----------------------------------------------------------------------------

    GL_CALL_CUSTOM(DrawArrays, void,
        /* proc   T */ (u32, int, int),
        /*     decl */ (PrimitiveMode mode, int first, int count),
        /*     pass */ (static_cast<u32>(mode), first, count))

    GL_CALL_CUSTOM(DrawElements, void,
        /* proc   T */ (u32, int, u32, const void*),
        /*     decl */ (PrimitiveMode mode, int count, DrawElementsType type, const void* indices),
        /*     pass */ (
            static_cast<u32>(mode),
            count,
            static_cast<u32>(type),
            indices
            ))

    GL_CALL_CUSTOM(BindTexture, void,
        /* proc   T */ (u32, u32),
        /*     decl */ (TextureTarget target, u32 texture),
        /*     pass */ (static_cast<u32>(target), texture))

    GL_CALL_CUSTOM(DeleteTextures, void,
        /* proc   T */ (int, const u32*),
        /*     decl */ (int n, const u32* textures),
        /*     pass */ (n, textures))

    GL_CALL_CUSTOM(GenTextures, void,
        /* proc   T */ (int, u32*),
        /*     decl */ (int n, u32* textures),
        /*     pass */ (n, textures))
#pragma endregion


#pragma region 1.3
    // ----------------------------------------------------------------------------
    // GL_VERSION_1_3
    // ----------------------------------------------------------------------------

    GL_CALL_CUSTOM(ActiveTexture, void,
        /* proc   T */ (u32),
        /*     decl */ (TextureTarget texture),
        /*     pass */ (static_cast<u32>(texture)))
#pragma endregion


#pragma region 1.5
    // ----------------------------------------------------------------------------
    // GL_VERSION_1_5
    // ----------------------------------------------------------------------------

    GL_CALL_CUSTOM(BindBuffer, void,
        /* proc   T */ (u32, u32),
        /*     decl */ (BufferTarget target, u32 buffer),
        /*     pass */ (static_cast<u32>(target), buffer))

    GL_CALL_CUSTOM(DeleteBuffers, void,
        /* proc   T */ (int, const u32*),
        /*     decl */ (int n, const u32* buffers),
        /*     pass */ (n, buffers))

    GL_CALL_CUSTOM(GenBuffers, void,
        /* proc   T */ (int, u32*),
        /*     decl */ (int n, u32* buffers),
        /*     pass */ (n, buffers))

    GL_CALL_CUSTOM(BufferData, void,
        /* proc   T */ (u32, intptr_t, const void*, u32),
        /*     decl */ (BufferTarget target, intptr_t size, const void* data, BufferUsage usage),
        /*     pass */ (
            static_cast<u32>(target),
            size,
            data,
            static_cast<u32>(usage)
            ))

    GL_CALL_CUSTOM(BufferSubData, void,
        /* proc   T */ (u32, intptr_t, intptr_t, const void*),
        /*     decl */ (BufferTarget target, intptr_t offset, intptr_t size, const void* data),
        /*     pass */ (
            static_cast<u32>(target),
            offset,
            size,
            data
            ))
#pragma endregion


#pragma region 2.0
    // ----------------------------------------------------------------------------
    // GL_VERSION_2_0
    // ----------------------------------------------------------------------------
    
    GL_CALL_CUSTOM(AttachShader, void,
        /* proc   T */ (u32, u32),
        /*     decl */ (u32 program, u32 shader),
        /*     pass */ (program, shader))
    
    GL_CALL_CUSTOM(CompileShader, void,
        /* proc   T */ (u32),
        /*     decl */ (u32 shader),
        /*     pass */ (shader))
    
    GL_CALL_CUSTOM(CreateProgram, u32,
        /* proc   T */ (void),
        /*     decl */ (),
        /*     pass */ ())
    
    GL_CALL_CUSTOM(CreateShader, u32,
        /* proc   T */ (u32),
        /*     decl */ (ShaderType type),
        /*     pass */ (static_cast<u32>(type)))
    
    GL_CALL_CUSTOM(DeleteProgram, void,
        /* proc   T */ (u32),
        /*     decl */ (u32 program),
        /*     pass */ (program))
    
    GL_CALL_CUSTOM(DeleteShader, void,
        /* proc   T */ (u32),
        /*     decl */ (u32 shader),
        /*     pass */ (shader))
    
    GL_CALL_CUSTOM(EnableVertexAttribArray, void,
        /* proc   T */ (u32),
        /*     decl */ (u32 index),
        /*     pass */ (index))
    
    GL_CALL_CUSTOM(GetProgramiv, void,
        /* proc   T */ (u32, u32, int*),
        /*     decl */ (u32 program, ProgramProperty pname, int* params),
        /*     pass */ (program, static_cast<u32>(pname), params))
    
    GL_CALL_CUSTOM(GetProgramInfoLog, void,
        /* proc   T */ (u32, int, int*, char*),
        /*     decl */ (u32 program, int bufSize, int* length, char* infoLog),
        /*     pass */ (program, bufSize, length, infoLog))
    
    GL_CALL_CUSTOM(GetShaderiv, void,
        /* proc   T */ (u32, u32, int*),
        /*     decl */ (u32 shader, ShaderProperty pname, int* params),
        /*     pass */ (shader, static_cast<u32>(pname), params))
    
    GL_CALL_CUSTOM(GetShaderInfoLog, void,
        /* proc   T */ (u32, int, int*, char*),
        /*     decl */ (u32 shader, int bufSize, int* length, char* infoLog),
        /*     pass */ (shader, bufSize, length, infoLog))
    
    GL_CALL_CUSTOM(GetUniformLocation, int,
        /* proc   T */ (u32, const char*),
        /*     decl */ (u32 program, const char* name),
        /*     pass */ (program, name))
    
    GL_CALL_CUSTOM(LinkProgram, void,
        /* proc   T */ (u32),
        /*     decl */ (u32 program),
        /*     pass */ (program))
    
    GL_CALL_CUSTOM(ShaderSource, void,
        /* proc   T */ (u32, int, const char* const*, const int*),
        /*     decl */ (u32 shader, int count, const char* const* strings, const int* lengths),
        /*     pass */ (shader, count, strings, lengths))
    
    GL_CALL_CUSTOM(UseProgram, void,
        /* proc   T */ (u32),
        /*     decl */ (u32 program),
        /*     pass */ (program))
    
    GL_CALL_CUSTOM(Uniform1i, void,
        /* proc   T */ (int, int),
        /*     decl */ (int location, int v0),
        /*     pass */ (location, v0))
    
    GL_CALL_CUSTOM(UniformMatrix4fv, void,
        /* proc   T */ (int, int, unsigned char, const float*),
        /*     decl */ (int location, int count, bool transpose, const float* value),
        /*     pass */ (location, count, Boolean{ transpose }, value))
    
    GL_CALL_CUSTOM(VertexAttribPointer, void,
        /* proc   T */ (u32, int, u32, unsigned char, int, const void*),
        /*     decl */ (u32 index, int size, DrawElementsType type, bool normalized, int stride, const void* pointer),
        /*     pass */ (index, size, static_cast<u32>(type), Boolean{ normalized }, stride, pointer))
#pragma endregion



#pragma region 3.0
    // ----------------------------------------------------------------------------
    // GL_VERSION_3_0
    // ----------------------------------------------------------------------------

    GL_CALL_CUSTOM(BindBufferBase, void,
        /* proc   T */ (u32, u32, u32),
        /*     decl */ (BufferTarget target, u32 index, u32 buffer),
        /*     pass */ (static_cast<u32>(target), index, buffer))

    GL_CALL_CUSTOM(VertexAttribIPointer, void,
        /* proc   T */ (u32, int, u32, int, const void*),
        /*     decl */ (u32 index, int size, DrawElementsType type, int stride, const void* pointer),
        /*     pass */ (index, size, static_cast<u32>(type), stride, pointer))

    GL_CALL_CUSTOM(BindVertexArray, void,
        /* proc   T */ (u32),
        /*     decl */ (u32 _array),
        /*     pass */ (_array))

    GL_CALL_CUSTOM(DeleteVertexArrays, void,
        /* proc   T */ (int, const u32*),
        /*     decl */ (int n, const u32* arrays),
        /*     pass */ (n, arrays))

    GL_CALL_CUSTOM(GenVertexArrays, void,
        /* proc   T */ (int, u32*),
        /*     decl */ (int n, u32* arrays),
        /*     pass */ (n, arrays))
#pragma endregion

    struct Shader {
    private:
        u32 _id_program{ 0 };

    public:
        // Check compile status with `.failed()`
        // When `#define _DEBUG` automatically prints errors via glGetShaderInfoLog
        inline bool Compile(const char* vert, const char* frag) noexcept {
#if defined(_DEBUG)
            int success;
            char info[512];
#endif
            u32 V; // 1. Vertex shader
            u32 F; // 2. Fragment shader
            u32 P; // 3. shader Program
            // 1. vertex shader
            V = CreateShader(ShaderType::VertexShader);
            ShaderSource(V, 1, &vert, nullptr);
            CompileShader(V);
#if defined(_DEBUG)
            GetShaderiv(V, ShaderProperty::CompileStatus, &success);
            if (!success) {
                GetShaderInfoLog(V, 512, nullptr, info);
                ::io::out << "vertex shader error: " << info << ::io::out.endl;
                DeleteShader(V);
                return false;
            }
#endif
            // 2. fragment shader
            F = CreateShader(ShaderType::FragmentShader);
            ShaderSource(F, 1, &frag, nullptr);
            CompileShader(F);
#if defined(_DEBUG)
            GetShaderiv(F, ShaderProperty::CompileStatus, &success);
            if (!success) {
                GetShaderInfoLog(F, 512, nullptr, info);
                ::io::out << "fragment shader error: " << info << ::io::out.endl;
                DeleteShader(V);
                DeleteShader(F);
                return false;
            }
#endif  
            // 3. shader program
            P = CreateProgram();
            AttachShader(P, V);
            AttachShader(P, F);
            LinkProgram(P);
#if defined(_DEBUG)
            GetProgramiv(P, ProgramProperty::LinkStatus, &success);
            if (!success) {
                GetProgramInfoLog(P, 512, nullptr, info);
                ::io::out << "shader program failed: " << info << ::io::out.endl;
                DeleteShader(V);
                DeleteShader(F);
                DeleteProgram(P);
                return false;
            }
#endif
            // We no longer need shader objects after linking
            DeleteShader(V);
            DeleteShader(F);
            _id_program = P;
            // Freeing the shader program, using RAII, in the destructor.
            return true;
        }

        inline explicit Shader() noexcept = default;
        inline explicit Shader(const char* vert, const char* frag) noexcept { Compile(vert, frag); }
        inline ~Shader() noexcept { DeleteProgram(_id_program); }

        // Non-copyable, non-movable
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) = delete;
        Shader& operator=(Shader&&) = delete;

        inline u32 id() const noexcept { return _id_program; }
        inline bool failed() const noexcept { return _id_program == 0; }

        inline void Use() const noexcept { UseProgram(_id_program); }
}; // struct Shader

struct Buffer {
private:
    unsigned _id{ 0 };
    BufferTarget _target;

public:
    IO_CONSTEXPR Buffer() noexcept = delete;

    inline explicit Buffer(BufferTarget target) noexcept
        : _target(target) {
        GenBuffers(1, &_id);
    }
    ~Buffer() noexcept { if (_id) DeleteBuffers(1, &_id); }

    inline void bind() const noexcept { BindBuffer(_target, _id); }
    inline void data(const void* ptr, size_t bytes, BufferUsage usage)
        const noexcept {
        BufferData(_target, bytes, ptr, usage);
    }
    inline unsigned id() const noexcept { return _id; }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& o) noexcept {
        _id = o._id;
        _target = o._target;
        o._id = 0;
    }
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) {
            if (_id) DeleteBuffers(1, &_id);
            _id = o._id;
            _target = o._target;
            o._id = 0;
        }
        return *this;
    }
};

struct VertexArray {
private:
    unsigned _id{ 0 };

public:
    inline VertexArray() noexcept { GenVertexArrays(1, &_id); }
    inline ~VertexArray() noexcept { if (_id) DeleteVertexArrays(1, &_id); }

    inline void bind() const noexcept { BindVertexArray(_id); }
    inline unsigned id() const noexcept { return _id; }

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& o) noexcept {
        _id = o._id;
        o._id = 0;
    }

    VertexArray& operator=(VertexArray&& o) noexcept {
        if (this != &o) {
            if (_id) DeleteVertexArrays(1, &_id);
            _id = o._id;
            o._id = 0;
        }
        return *this;
    }
};

template<typename T> struct DrawElementsTypeOf;
template<> struct DrawElementsTypeOf<i8> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::Byte; };
template<> struct DrawElementsTypeOf<u8> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::UnsignedByte; };
template<> struct DrawElementsTypeOf<i16> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::Short; };
template<> struct DrawElementsTypeOf<u16> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::UnsignedShort; };
template<> struct DrawElementsTypeOf<i32> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::Int; };
template<> struct DrawElementsTypeOf<u32> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::UnsignedInt; };
template<> struct DrawElementsTypeOf<float> { static IO_CONSTEXPR_VAR DrawElementsType value = DrawElementsType::Float; };

struct Attr {
private:
    u32 _amount;
    DrawElementsType _type;

public:
    // amount_of_components e.g. 3 for vec3
    // gl_type: Byte, UnsignedByte, Short, UnsignedShort, Int, UnsignedInt, Float.    
    IO_CONSTEXPR Attr(u32 amount_of_components, DrawElementsType gl_type = DrawElementsType::Float) noexcept
        : _amount{ amount_of_components }, _type{ gl_type } {}

    // amount of components, e.g. 3 for vec3
    IO_NODISCARD IO_CONSTEXPR u32 amount() const noexcept { return _amount; }
    IO_NODISCARD IO_CONSTEXPR DrawElementsType type() const noexcept { return _type; }
    IO_NODISCARD IO_CONSTEXPR int size() const noexcept {
        return amount() * getDrawElementsTypeSize(type()); // e.g. `3 * sizeof(float)`
    }
};

template<typename T>
IO_CONSTEXPR const Attr AttrOf(u32 amount) noexcept {
    return Attr(amount, DrawElementsTypeOf<T>::value);
}

struct MeshBinder {
    inline static void setup(
        const VertexArray& vao,
        const Buffer& vbo,
        const io::view<const Attr>& attrs
    ) noexcept
    {
        u32 index, stride;
        size_t offset = 0;
        index = stride = 0;

        vao.bind();
        vbo.bind();

        // Calculate stride, by iterating all elements
        for (auto it = attrs.begin(); it != attrs.end(); ++it)
            stride += it->size();

        // Call glFunctions for each Attr
        for (auto it = attrs.begin(); it != attrs.end(); ++it, ++index) {
            VertexAttribPointer(
                index,        // location
                it->amount(), // amount of comps
                it->type(),   // gl type
                false,        // normalize
                stride,       // stride
                reinterpret_cast<void*>(offset) // attr offset
            );
            EnableVertexAttribArray(index);
            offset += it->size();
        }

        BindVertexArray(0);
        BindBuffer(BufferTarget::ArrayBuffer, 0);
    }
}; // struct MeshBinder


} // namespace gl