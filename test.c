/* test extension for PHP */
/* 导入一些必要的后文件, 也可以自己添加额外的#include 指令*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_test.h"

/* For compatibility(兼容) with older PHP versions */
/* 向前(forward)兼容的宏, 能够让扩展(extension)在php7 之前的的版本中编译*/
#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() \
	ZEND_PARSE_PARAMETERS_START(0, 0) \
	ZEND_PARSE_PARAMETERS_END()
#endif

ZEND_DECLARE_MODULE_GLOBALS(test)


/*
 * PHP_INI_ALL 表明可以在任何时候被修改
 * (in php.ini, in per-directory configuration files and by ini_set() function during script)
 *
 * OnUpdateLong: common callback 设置directive value
 * “zend_test_globals”  保存模块全局变量的结构名称
 * test_globals 非线程安全构建下保存模块全局变量的全局变量
 *  scale -> 全局模块全局变量名称
 * */

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("test.scale1", "1", PHP_INI_ALL, OnUpdateLong, scale, zend_test_globals, test_globals)
PHP_INI_END()

/* {{{ void test_test1()
 */
PHP_FUNCTION(test_test1) /* PHP_FUNCTION 参数是方法名称*/
{
	ZEND_PARSE_PARAMETERS_NONE(); /* 没有参数的*/

	php_printf("The extension %s is loaded and working!\r\n", "test"); /*打印字符串到输出流*/
}
/* }}} */

/* {{{ string test_test2( [ string $var ] )
 */
PHP_FUNCTION(test_test2)
{
	char *var = "World";
	size_t var_len = sizeof("World") - 1;
	zend_string *retval;

	ZEND_PARSE_PARAMETERS_START(0, 1) /*0 需要的参数数量, 1 最多有多少个参数, 说明方法被调用的时候可能没有参数,或者有一个参数*/
		Z_PARAM_OPTIONAL /*分割可选参数和必须参数*/
		Z_PARAM_STRING(var, var_len) /*定义一个字符串参数, 把他复制到var 变量中, 长度放入var_len*/
	ZEND_PARSE_PARAMETERS_END();

	retval = strpprintf(0, "Hello %s", var);

	RETURN_STR(retval);
}
/* }}}*/

static int do_scale(zval *return_value, zval* x, zend_long factor) {

    ZVAL_DEREF(x);
    if (Z_TYPE_P(x) == IS_LONG) { /*Z_TYPE_P  返回zval 类型*/
        RETVAL_LONG(Z_LVAL_P(x) * factor); /* Z_LVAL  返回zval的long integer, 类型必须是IS_LONG*/
    } else if (Z_TYPE_P(x) == IS_DOUBLE) {
        RETVAL_DOUBLE(Z_DVAL_P(x) * factor); /*Z_DVAL_P 返回zval的floating-point number, 类型必须是IS_DOUBLE*/
    } else if (Z_TYPE_P(x) == IS_STRING) {
        /* Z_STRLEN_P 返回字符串长度
         * zend_string_safe_alloc 为zend_string分配len * factor + 0 空间,
         * 给入一个C string(char*)和长度初始化zend_string
         * */
        zend_string *ret = zend_string_safe_alloc(Z_STRLEN_P(x), factor, 0, 0);
        char* p = ZSTR_VAL(ret); /*ZSTR_VAL 返回对应C string(char*) 的指针*/
        while (factor-- > 0) {
            memcpy(p, Z_STRVAL_P(x), Z_STRLEN_P(x)); /*Z_STRVAL_P 返回对应的C string(char*)的指针*/
            p += Z_STRLEN_P(x);
        }
        *p = '\0';
        RETVAL_STR(ret);
    } else if (Z_TYPE_P(x) == IS_ARRAY) {
        zend_array* ret = zend_new_array(zend_array_count(Z_ARR_P(x)));
        zend_ulong idx;
        zend_string *key;
        zval* val, tmp;

        ZEND_HASH_FOREACH_KEY_VAL(Z_ARR_P(x), idx, key, val) {
            if (do_scale(&tmp, val, factor) != SUCCESS) {
                zend_array_destroy(ret);
                return FAILURE;
            }
            if (key) {
                zend_hash_add(ret, key, &tmp);
            } else {
                zend_hash_index_add(ret, idx, &tmp);
            }
        } ZEND_HASH_FOREACH_END();
        RETVAL_ARR(ret);
    }else{
        php_error_docref(NULL, E_WARNING, "unexpected argument type");
        return FAILURE;
    }
    return SUCCESS;
}


static int do_scale_ref(zval *x, zend_long factor) {

    ZVAL_DEREF(x);
    switch (Z_TYPE_P(x)) {
        case IS_LONG:
            Z_LVAL_P(x) *= factor;
            break;
        case IS_DOUBLE:
            Z_DVAL_P(x) *=  factor;
            break;
        case IS_STRING:
            {
                size_t len = Z_STRLEN_P(x);
                char *p;
                ZVAL_STR(x, zend_string_safe_realloc(Z_STR_P(x), len, factor, 0, 0));
                p = Z_STRVAL_P(x) + len;
                while (--factor > 0) {
                    memcpy(p, Z_STRVAL_P(x), len);
                    p += len;
                }
                *p = '\0';
                break;
            }
        case IS_ARRAY:
            {
                zval *val;
                SEPARATE_ARRAY(x); // perform  copy on write, 解决多个变量指向同一块内存时, 修改一个,多个都被修改
                ZEND_HASH_FOREACH_VAL(Z_ARR_P(x), val) {
                    if (do_scale_ref(val, factor) != SUCCESS) {
                        return FAILURE;
                    }
                } ZEND_HASH_FOREACH_END();
                break;
            }
        default:
            php_error_docref(NULL, E_WARNING, "unexpected argument type");
            return FAILURE;
    }

    return SUCCESS;
}


/* {{{ double test_scale( double $x )
 */
PHP_FUNCTION(test_scale)
{
//    double x;
//    ZEND_PARSE_PARAMETERS_START(1,1)
//        Z_PARAM_DOUBLE(x)
//    ZEND_PARSE_PARAMETERS_END();
//
//    RETURN_DOUBLE(x * TEST_G(scale));

    zval *x;
    zend_long factor = TEST_G(scale);

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(x)
        Z_PARAM_OPTIONAL /*必要参数与可选参数分割*/
        Z_PARAM_LONG(factor)
    ZEND_PARSE_PARAMETERS_END();
    do_scale(return_value, x, factor);
}
/* }}}*/

PHP_FUNCTION(test_scale_ref) {
        zval *x;
        zend_long factor = TEST_G(scale);

        ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(x)
        Z_PARAM_OPTIONAL /*必要参数与可选参数分割*/
        Z_PARAM_LONG(factor)
        ZEND_PARSE_PARAMETERS_END();
        do_scale_ref(x, factor);
}

static zend_class_entry *scaler_class_entry = NULL;
#define DEFAULT_SCALE_FACTOR 2

PHP_METHOD(Scaler, __construct) {
    zend_long factor = DEFAULT_SCALE_FACTOR;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(factor);
    ZEND_PARSE_PARAMETERS_END();

    if (ZEND_NUM_ARGS() > 0) {
        zend_update_property_long(Z_OBJCE_P(ZEND_THIS), ZEND_THIS, "factor", sizeof("factor") -1, factor);
    }
}

PHP_METHOD(Scaler, scale){
    zval *x, *zv, tmp;
    zend_long factor;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(x)
    ZEND_PARSE_PARAMETERS_END();

    zv = zend_read_property(Z_OBJCE_P(ZEND_THIS), ZEND_THIS, "factor", sizeof("factor")-1, 0, &tmp);
    factor = zval_get_long(zv);

    do_scale_ref(x, factor);
}

ZEND_BEGIN_ARG_INFO(arginfo_scaler_construct, 0)
ZEND_ARG_INFO(0, factor)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_scaler_scale, 0)
ZEND_ARG_INFO(1, x)
ZEND_END_ARG_INFO()

static const zend_function_entry scaler_functions[] = {
        PHP_ME(Scaler, __construct, arginfo_scaler_construct, ZEND_ACC_PUBLIC)
        PHP_ME(Scaler, scale, arginfo_scaler_scale, ZEND_ACC_PUBLIC)
        PHP_FE_END
};


/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(test) /*回调函数(callback), 在每个请求start-up 调用, 只初始化thread-local storage cache
 * todo 生命周期? 早一点调用比较好? */
{
#if defined(ZTS) && defined(COMPILE_DL_TEST)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}
/* }}} */

static PHP_GINIT_FUNCTION(test)
{
#if defined(COMPILE_DL_BCMATH) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
    test_globals->scale = 1;
}

PHP_MINIT_FUNCTION(test){ /*扩展必须实现MINIT() callback, 参数是扩展名称,
 * 这个function应该返回SUCCESS已便将php扩展链接到php核心开启所有方法和其他的特性.
 * 这个方法只初始化thread-local storage cache
 * 如果要执行某些操作和访问模块全局变量(module global variables)或者通用全局变量(common global variables), 最好代码移动到MINIT 开头
 * MINIT callback 适合添加各种各样的扩展实体(extension entities) 像内部常量(internal constant)
 * */

#ifdef defined(ZTS) && defined(COMPLILE_DL_TEST)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    zend_class_entry ce;

    REGISTER_INI_ENTRIES();

    REGISTER_LONG_CONSTANT("TEST_SCALE_FACTOR", 2, CONST_CS | CONST_PERSISTENT); /*first argument: 名称, second: 参数值, third: 常量标记(constant flag) {
 * CONST_CS : 常量名称大小写敏感, CONST_PERSISTENT: 固定常量(persistent constant)} */
    /*
     * REGISTER_NULL_CONSTANT(NAME, FLAG): CONSTANT VALUE BE NULL
     * REGISTER_BOOL_CONSTANT(NAME, BVAL. FLAG): ANY LONG VALUE
     * REGISTER_DOUBLE_CONSTANT(NAME, DVAL, FLAG): A DOUBLE VALUE
     * REGISTER_STRING_CONSTANT(NAME, STR, FLAG): A ZERO TERMINATED STRING
     * REGISTER_STRINGL_CONSTANT(NAME, STR, LEN, FLAG): A STRING (WITH LENGTH)
     * similar REGISTER_NS_...() group of macros, follow see:
     * https://github.com/php/php-src/blob/PHP-7.4/Zend/zend_constants.h
     * */

    INIT_CLASS_ENTRY(ce, "Scaler", scaler_functions);
    scaler_class_entry = zend_register_internal_class(&ce);

    zend_declare_class_constant_long(scaler_class_entry, "DEFAULT_FACTOR", sizeof("DEFAULT_FACTOR")-1, DEFAULT_SCALE_FACTOR);

    zend_declare_property_long(scaler_class_entry, "factor", sizeof("factor")-1, DEFAULT_SCALE_FACTOR, ZEND_ACC_PRIVATE);

    return SUCCESS;
}


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(test) /*回调方法, 调用php的phpinfo() 方法时被调用, 打印扩展相关信息*/
{
	php_info_print_table_start();
	php_info_print_table_header(2, "test support", "enabled");
	php_info_print_table_end();

}
/* }}} */

/* {{{ arginfo
 */
ZEND_BEGIN_ARG_INFO(arginfo_test_test1, 0) /*test_test1 的参数信息, test_test1() 没有参数*/
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_test_test2, 0) /*test_test2 参数信息,*/
	ZEND_ARG_INFO(0, str) /*单个可选参数按值传递*/
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_test_scale, 0) /* 第二个参数被忽略,(php5 通过引用传递其余参数)每个参数
 * 被ZEND_ARG_INFO宏定义, 通过引用传递(pass by reference)获取值和参数名称*/
    ZEND_ARG_INFO(0, x)
    ZEND_ARG_INFO(0, factor)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_test_scale_ref, 0) /* 第二个参数被忽略,(php5 通过引用传递其余参数)每个参数
 * 被ZEND_ARG_INFO宏定义, 通过引用传递(pass by reference)获取值和参数名称*/
ZEND_ARG_INFO(1, x)
ZEND_ARG_INFO(0, factor)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ test_functions[]
 */
static const zend_function_entry test_functions[] = { /*test_function 是所有扩展函数以及函数参数信息列表*/
	PHP_FE(test_test1,		arginfo_test_test1)
	PHP_FE(test_test2,		arginfo_test_test2)
	PHP_FE(test_scale,		arginfo_test_scale)
	PHP_FE(test_scale_ref,		arginfo_test_scale_ref)
	PHP_FE_END /*terminated macro*/
};
/* }}} */



/* {{{ test_module_entry
 */
zend_module_entry test_module_entry = { /*扩展主要入口结构, PHP核心从这些结构中获取所以的相关信息*/
	STANDARD_MODULE_HEADER,
	"test",					/* Extension name */
	test_functions,			/* zend_function_entry 被定义的php方法列表 */
	PHP_MINIT(test),							/* PHP_MINIT - Module initialization */
	NULL,							/* PHP_MSHUTDOWN - Module shutdown */
	NULL,			/* PHP_RINIT - Request initialization */
	NULL,							/* PHP_RSHUTDOWN - Request shutdown */
	PHP_MINFO(test),			/* PHP_MINFO - Module info */
	PHP_TEST_VERSION,		/* Version */
	PHP_MODULE_GLOBALS(test),  /* module globals*/
	PHP_GINIT(test),           /*  PHP_GINIT – Globals initialization*/
	NULL,                   /* PHP_GSHUTDOWN – Globals shutdown */
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
	/*回调发生在php启动(MINIT), php终止(MSHUTDOWN),每个请求开始(RINIT), 每个请求结束(RSHUTDOWN)和phpinfo()(MINFO)*/
};
/* }}} */





/* 几个动态链接定义*/
#ifdef COMPILE_DL_TEST
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(test)
#endif
