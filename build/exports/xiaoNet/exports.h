
#ifndef XIAONET_EXPORT_H
#define XIAONET_EXPORT_H

#ifdef XIAONET_STATIC_DEFINE
#  define XIAONET_EXPORT
#  define XIAONET_NO_EXPORT
#else
#  ifndef XIAONET_EXPORT
#    ifdef xiaoNet_EXPORTS
        /* We are building this library */
#      define XIAONET_EXPORT 
#    else
        /* We are using this library */
#      define XIAONET_EXPORT 
#    endif
#  endif

#  ifndef XIAONET_NO_EXPORT
#    define XIAONET_NO_EXPORT 
#  endif
#endif

#ifndef XIAONET_DEPRECATED
#  define XIAONET_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef XIAONET_DEPRECATED_EXPORT
#  define XIAONET_DEPRECATED_EXPORT XIAONET_EXPORT XIAONET_DEPRECATED
#endif

#ifndef XIAONET_DEPRECATED_NO_EXPORT
#  define XIAONET_DEPRECATED_NO_EXPORT XIAONET_NO_EXPORT XIAONET_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef XIAONET_NO_DEPRECATED
#    define XIAONET_NO_DEPRECATED
#  endif
#endif

#endif /* XIAONET_EXPORT_H */
