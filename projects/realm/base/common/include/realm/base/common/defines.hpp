#pragma once

// Enforces this project's supported targets.

#if !defined(RE_PLATFORM_LINUX)
    #error "realm: unsupported platform."
#endif

#if !defined(RE_ARCH_X64)
    #error "realm: unsupported architecture."
#endif

#if !defined(RE_COMPILER_GCC) && !defined(RE_COMPILER_CLANG)
    #error "realm: unsupported compiler."
#endif
#if (defined(RE_COMPILER_GCC) + defined(RE_COMPILER_CLANG)) != 1
    #error "realm: exactly one RE_COMPILER_* must be defined."
#endif

// Feature detection

#if defined(__has_cpp_attribute)
    #define RE_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
    #define RE_HAS_CPP_ATTRIBUTE(x) 0
#endif

#if defined(__has_builtin)
    #define RE_HAS_BUILTIN(x) __has_builtin(x)
#else
    #define RE_HAS_BUILTIN(x) 0
#endif

// Token / string helpers

#define RE_CONCAT_IMPL(a, b) a##b
#define RE_CONCAT(a, b)      RE_CONCAT_IMPL(a, b)

#define RE_STRINGIZE_IMPL(x) #x
#define RE_STRINGIZE(x)      RE_STRINGIZE_IMPL(x)

// Unique identifier for RAII-on-a-line tricks (RE_DEFER, etc.)
#define RE_UNIQUE_NAME(prefix) RE_CONCAT(prefix##_, __LINE__)

#define RE_PRAGMA_IMPL(x) _Pragma(#x)
#define RE_PRAGMA(x)      RE_PRAGMA_IMPL(x)

// Standard C++ attributes

#define RE_NORETURN          [[noreturn]]
#define RE_NODISCARD         [[nodiscard]]
#define RE_MAYBE_UNUSED      [[maybe_unused]]
#define RE_FALLTHROUGH       [[fallthrough]]
#define RE_DEPRECATED(msg)   [[deprecated(msg)]]
#define RE_NO_UNIQUE_ADDRESS [[no_unique_address]]

// Inline control

#if defined(RE_COMPILER_GCC) || defined(RE_COMPILER_CLANG)
    #define RE_FORCE_INLINE inline __attribute__((always_inline))
    #define RE_NO_INLINE    __attribute__((noinline))
#else
    #define RE_FORCE_INLINE
    #define RE_NO_INLINE
#endif

// Thread-local storage

#if defined(RE_COMPILER_GCC) || defined(RE_COMPILER_CLANG)
    #if defined(__ELF__)
        #define RE_THREAD_LOCAL __attribute__((tls_model("initial-exec"))) thread_local
    #else
        #define RE_THREAD_LOCAL thread_local
    #endif
#else
    #define RE_THREAD_LOCAL thread_local
#endif

// Function attributes

#if defined(RE_COMPILER_GCC) || defined(RE_COMPILER_CLANG)
    #define RE_COLD            __attribute__((cold))
    #define RE_HOT             __attribute__((hot))
    #define RE_PURE            __attribute__((pure))
    #define RE_CONST           __attribute__((const))
    #define RE_MALLOC          __attribute__((malloc))
    #define RE_RETURNS_NONNULL __attribute__((returns_nonnull))
    #define RE_FLATTEN         __attribute__((flatten))
    #define RE_NO_RETURN       __attribute__((noreturn))
#else
    #define RE_COLD
    #define RE_HOT
    #define RE_PURE
    #define RE_CONST
    #define RE_MALLOC
    #define RE_RETURNS_NONNULL
    #define RE_FLATTEN
    #define RE_NO_RETURN
#endif

// Optimizer hints (expr must be side-effect free and always true)

#if defined(RE_COMPILER_GCC) || defined(RE_COMPILER_CLANG)
    #if RE_HAS_CPP_ATTRIBUTE(assume)
        #define RE_ASSUME(expr) [[assume(expr)]]
    #elif RE_HAS_BUILTIN(__builtin_assume)
        #define RE_ASSUME(expr) __builtin_assume(expr)
    #else
        #define RE_ASSUME(expr) static_cast<void>(0)
    #endif
#else
    #define RE_ASSUME(expr) static_cast<void>(0)
#endif

#if defined(RE_COMPILER_GCC) || defined(RE_COMPILER_CLANG)
    #define RE_UNREACHABLE() __builtin_unreachable()
#else
    #include <utility>
    #define RE_UNREACHABLE() std::unreachable()
#endif

// Warning control

#if defined(RE_COMPILER_CLANG)
    #define RE_WARNING_PUSH()       _Pragma("clang diagnostic push")
    #define RE_WARNING_POP()        _Pragma("clang diagnostic pop")
    #define RE_WARNING_DISABLE(wrn) RE_PRAGMA(clang diagnostic ignored wrn)
#elif defined(RE_COMPILER_GCC)
    #define RE_WARNING_PUSH()       _Pragma("GCC diagnostic push")
    #define RE_WARNING_POP()        _Pragma("GCC diagnostic pop")
    #define RE_WARNING_DISABLE(wrn) RE_PRAGMA(GCC diagnostic ignored wrn)
#else
    #define RE_WARNING_PUSH()
    #define RE_WARNING_POP()
    #define RE_WARNING_DISABLE(wrn)
#endif
