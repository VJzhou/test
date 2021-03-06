/* test extension for PHP */

#ifndef PHP_TEST_H
# define PHP_TEST_H

extern zend_module_entry test_module_entry; // 扩展描述结构,  扩展的入口
# define phpext_test_ptr &test_module_entry

# define PHP_TEST_VERSION "0.1.0"

# if defined(ZTS) && defined(COMPILE_DL_TEST)
ZEND_TSRMLS_CACHE_EXTERN() // 本地线程储存缓存入口, 如果这个扩展是被构建为线程安全(thread-safe)并且被编译为共享对象(shared object)
# endif

ZEND_BEGIN_MODULE_GLOBALS(test)
    zend_long scale;
ZEND_END_MODULE_GLOBALS(test)

/*提供一个外部名称访问这个结构*/
ZEND_EXTERN_MODULE_GLOBALS(test)

#define TEST_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(test, v)
#endif	/* PHP_TEST_H */
