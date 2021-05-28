/*
 * jscore-test.c
 * 
 * Copyright 2021 chehw <hongwei.che@gmail.com>
 * 
 * The MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of 
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to 
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
 * of the Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 */

/**
 * Compile:
 * $ gcc -std=gnu99 -g -Wall -o jscore-test jscore-test.c `pkg-config --cflags --libs  javascriptcoregtk-4.0`
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <jsc/jsc.h>


static const char * s_js_class_names[] = {
	"CFoo", 
	"CBar",
	NULL
};

#define JSC_VALUE_CHECK_TYPE(value, type) jsc_value_is_##type(value)
void dump_jsc_value(JSCValue * value)
{
	if(jsc_value_is_object(value)) {
		printf("[type=object");
		if(jsc_value_is_array(value)) {
			printf(", array]\n");
			int index = 0;
			JSCValue * item = NULL;
			while((item = jsc_value_object_get_property_at_index(value, index++))){
				if(jsc_value_is_undefined(item)) break;
				printf("item[%d]=", index);
				dump_jsc_value(item);
			}
			return;
		}
		if(jsc_value_is_function(value)) {
			printf(", function");
		}
		
		if(jsc_value_is_constructor(value)) {
			printf(", constructor");
		} 
		
		for(size_t i = 0; i < (sizeof(s_js_class_names) / sizeof(s_js_class_names[0])); ++i) {
			const char * class_name = s_js_class_names[i];
			if(NULL == class_name) break;
			if(jsc_value_object_is_instance_of(value, class_name)) 
			{
				printf(", <class::%s>", class_name);
			}
		}
		
		printf("]\n");
		
		char ** props_array = jsc_value_object_enumerate_properties(value);
		if(props_array) {
			char ** props = props_array;
			char * prop = NULL;
			while((prop = *props++)) {
				printf("property: %s\n", prop);
			}
			
		}
		g_strfreev(props_array);
		return;
	}
	
	if(jsc_value_is_boolean(value)) {
		printf("value: (boolean)%d\n", jsc_value_to_boolean(value));
	}
	if(jsc_value_is_number(value)) {
		printf("value: (number)%g\n", jsc_value_to_double(value));
	}
	if(jsc_value_is_string(value)) {
		char * sz_value = jsc_value_to_string(value);
		printf("value: (string)%s\n", sz_value);
		free(sz_value);
	}
	
	if(jsc_value_is_undefined(value)) {
	//	printf("jsc_value_is_undefined\n");
	}
	
	if(jsc_value_is_null(value)) {
		printf("value: (null)\n");
	}
}

struct CFoo
{
	JSCContext * js;
	int result;
};
static int foo_method_mul(struct CFoo * _this, int a, int b)
{
	int c = a * b;
	return c;
}
int main(int argc, char **argv)
{
	JSCVirtualMachine * jsvm = jsc_virtual_machine_new();
	JSCContext * js = jsc_context_new_with_virtual_machine(jsvm);
	assert(jsvm && js);
	
	//~ GType value_type = jsc_value_get_type();
	JSCClass * foo_class = jsc_context_register_class(js, s_js_class_names[0], NULL, NULL, NULL);
	
	static const char * src_js_array= "var a = ['1', 200, '3']; a;";
	static const char * src_js_object = "var obj = { key1: 'value1', key2: 'value2'}; obj";
	static const char * src_js_func_add = "function add(a,b) { return a+b; }; add;";
	JSCValue * ret = NULL;
	
	JSCValue * js_array = jsc_context_evaluate(js, src_js_array, strlen(src_js_array));
	JSCValue * js_object = jsc_context_evaluate(js, src_js_object, strlen(src_js_object));
	JSCValue * js_func_add = jsc_context_evaluate(js, src_js_func_add, strlen(src_js_func_add));
	
	
	printf("\e[33m== dump array ====\e[39m\n");
	dump_jsc_value(js_array);
	
	printf("\e[33m== dump object ====\e[39m\n");
	dump_jsc_value(js_object);
	
	printf("\e[33m== dump function ====\e[39m\n");
	dump_jsc_value(js_func_add);
	
	
	int values[] = {10, 20};
	JSCValue * params[] = {
		[0] = jsc_value_new_number(js, values[0]),
		[1] = jsc_value_new_number(js, values[1]),
	};
	ret = jsc_value_function_callv(js_func_add, 2, params);
	
	printf("== eval func: add(%d, %d) --> %d ====\n", values[0], values[1], jsc_value_to_int32(ret));
	dump_jsc_value(ret);
	
	
	// add native class
	jsc_class_add_method(foo_class, "mul", 
		G_CALLBACK(foo_method_mul), NULL, NULL, 
		G_TYPE_INT, // return type
		2, // num_params
		G_TYPE_INT, G_TYPE_INT	// param types
	);
	
	
	struct CFoo foo = {
		.js = js,
		.result = 0,
	};
	
	JSCValue * foo_instance = jsc_value_new_object(js, &foo, foo_class);
	printf("\e[33m== dump class, classname=%s ====\e[39m\n", jsc_class_get_name(foo_class));
	dump_jsc_value((JSCValue *)foo_instance);
	
	ret = jsc_value_object_invoke_methodv(foo_instance, "mul", 2, params);
	printf("\e[33m== call native functio CFoo::mul(%d, %d) --> %d ====\e[39m\n", values[0], values[1], jsc_value_to_int32(ret));
	dump_jsc_value(ret);
	
	return 0;
}

