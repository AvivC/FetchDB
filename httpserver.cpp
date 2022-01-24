#include <iostream>
#include <cmath>
#include <mutex>
#include <cassert>

#include "webserver/webserver.h"
#include "Socket.h"

extern "C"
{
#include "ribbon_api.h"

struct ObjectInstanceHttpRequest {
    ObjectInstance base;
    webserver::http_request* request = nullptr;
};

static RibbonApi ribbon;
static ObjectModule* module;

static ObjectClass* http_request_class;

// TODO: This shouldn't really be a module level static variable. Support multiple handlers (per port?) somehow
static Object* handler_callable;

// The interpreter is of course seriously not thread safe, and it seems that the webserver C++ library
// we're using starts a new thread per request. This seemingly causes code to run concurrently in the interpreter
// and we promptly crash.
// This static global mutex is probably no the best solution, but hopefully it's good enough.
static std::mutex mutex;

static Value string_to_value(const std::string& string) {
    ObjectString* string_object = ribbon.object_string_copy_from_null_terminated(string.c_str());
    Value result;
    result.type = VALUE_OBJECT;
    result.as.object = reinterpret_cast<Object*>(string_object);
    return result;
}

static Value make_value_string(const std::string& string) {
    Value result;
    result.type = VALUE_OBJECT;
    result.as.object = reinterpret_cast<Object*>(ribbon.object_string_copy_from_null_terminated(string.c_str()));
    return result;
}

static bool http_request_descriptor_get(Object* self, ValueArray args, Value* out) {
    if (!ribbon.arguments_valid(args, "oHttpRequest oString")) {
        return false;
    }

    Object* object = args.values[0].as.object;
    assert(ribbon.is_instance_of_class(object, "HttpRequest"));
    ObjectInstanceHttpRequest* request = reinterpret_cast<ObjectInstanceHttpRequest*>(object);
    webserver::http_request* raw_request = request->request;
    if (raw_request == nullptr) {
        FAIL("raw_request is nullptr");
    }

    assert(ribbon.object_value_is(args.values[1], OBJECT_STRING));
    ObjectString* attr_name = reinterpret_cast<ObjectString*>(args.values[1].as.object);

    const char* attr_name_cstring = attr_name->chars;
    const int attr_name_length = attr_name->length;

    if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "method", 6)) {

        *out = string_to_value(raw_request->method_.c_str());

    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "path", 4)) {

        *out = string_to_value(raw_request->path_.c_str());

    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "params", 6)) {

        ObjectTable* table = ribbon.object_table_new_empty();

        const std::map<std::string, std::string>& params = raw_request->params_;
        for (const std::pair<std::string, std::string>& entry : params) {
            Value key = make_value_string(entry.first);
            Value value = make_value_string(entry.second);
            ValueArray set_key_args = ribbon.value_array_make(0, nullptr);
            ribbon.value_array_write(&set_key_args, &key);
            ribbon.value_array_write(&set_key_args, &value);
            Value set_key_result;
            ribbon.vm_call_attribute_cstring(reinterpret_cast<Object*>(table), "@set_key", set_key_args, &set_key_result);
            // Do something with result?
            ribbon.value_array_free(&set_key_args);
        }

    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "accept", 6)) {

        *out = string_to_value(raw_request->accept_.c_str());

    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "accept_language", 15)) {

        *out = string_to_value(raw_request->accept_language_.c_str());

    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "accept_encoding", 15)) {

        *out = string_to_value(raw_request->accept_encoding_.c_str());

    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "user_agent", 10)) {

        *out = string_to_value(raw_request->user_agent_.c_str());

    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "status", 6)) {

        *out = string_to_value(raw_request->status_.c_str());

    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "auth_realm", 10)) {

        *out = string_to_value(raw_request->auth_realm_.c_str());
        
    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "answer", 6)) {

        *out = string_to_value(raw_request->answer_.c_str());
        
    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "authentication_given", 20)) {

        Value result;
        result.type = VALUE_BOOLEAN;
        result.as.boolean = raw_request->authentication_given_;
        *out = result;
        
    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "username", 8)) {

        *out = string_to_value(raw_request->username_.c_str());
        
    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "password", 8)) {

        *out = string_to_value(raw_request->password_.c_str());
        
    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "payload", 7)) {

        *out = string_to_value(raw_request->payload_.c_str());
        
    } else {

        FAIL("HttpRequest descriptor get method received invalid attr name");

    }

    return true;
}

static bool http_request_descriptor_set(Object* self, ValueArray args, Value* out) {
    if (!ribbon.arguments_valid(args, "oHttpRequest oString oString")) {
        return false;
    }

    Object* object = args.values[0].as.object;
    assert(ribbon.is_instance_of_class(object, "HttpRequest"));
    ObjectInstanceHttpRequest* request = reinterpret_cast<ObjectInstanceHttpRequest*>(object);
    webserver::http_request* raw_request = request->request;
    if (raw_request == nullptr) {
        FAIL("raw_request is nullptr");
    }

    assert(ribbon.object_value_is(args.values[1], OBJECT_STRING));
    ObjectString* attr_name = reinterpret_cast<ObjectString*>(args.values[1].as.object);

    assert(ribbon.object_value_is(args.values[2], OBJECT_STRING));
    ObjectString* value = reinterpret_cast<ObjectString*>(args.values[2].as.object);

    char* attr_name_cstring = attr_name->chars;
    int attr_name_length = attr_name->length;

    if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "answer", 6)) {
        raw_request->answer_ = value->chars;
    } else if (ribbon.cstrings_equal(attr_name_cstring, attr_name_length, "status", 6)) {
        raw_request->status_ = value->chars;
    } else {
        FAIL("HttpRequest descriptor set method received invalid attr name");
    }

    out->type = VALUE_NIL;
    out->as.number = -1;
    return true;
}

static ObjectClass* expose_class(
    char* name, size_t instance_size, DeallocationFunction dealloc_func,
    GcMarkFunction gc_mark_func, ObjectFunction* init_func, void* descriptors[][2]) {

    ObjectClass* klass = ribbon.object_class_native_new(name, instance_size, dealloc_func, gc_mark_func, init_func, descriptors);
    Value klass_as_object = Value{VALUE_OBJECT};
    klass_as_object.as.object = reinterpret_cast<Object*>(klass);
    ribbon.object_set_attribute_cstring_key(reinterpret_cast<Object*>(module), name, klass_as_object);
    return klass;
}

static void expose_function(char* name, int num_params, char** params, NativeFunction function) {
    // Value value = MAKE_VALUE_OBJECT(ribbon.make_native_function_with_params(name, num_params, params, function));
    Value value = Value{VALUE_OBJECT};
    value.as.object = reinterpret_cast<Object*>(ribbon.make_native_function_with_params(name, num_params, params, function));
    ribbon.object_set_attribute_cstring_key((Object*) module, name, value);
}

// static void request_handler(webserver::http_request* r) {
//     /* TODO: Check function calls boolean return value    */

//     Value handlers_value;
//     ribbon.object_load_attribute_cstring_key(reinterpret_cast<Object*>(module), "_handlers", &handlers_value);

//     ObjectTable* handlers = reinterpret_cast<ObjectTable*>(handlers_value.as.object);

//     ObjectString* path_as_string = ribbon.object_string_copy_from_null_terminated(r->path_.c_str());
//     Value path_as_string_value = Value{VALUE_OBJECT};
//     path_as_string_value.as.object = reinterpret_cast<Object*>(path_as_string);
    
//     ValueArray path_key_args;
//     ribbon.value_array_init(&path_key_args);
//     ribbon.value_array_write(&path_key_args, &path_as_string_value);
//     Value has_path;
//     ribbon.vm_call_attribute_cstring(reinterpret_cast<Object*>(handlers), "has_key", path_key_args, &has_path);

//     if (has_path.as.boolean) {
//         Value handler_value;
//         ribbon.vm_call_attribute_cstring(reinterpret_cast<Object*>(handlers), "@get_key", path_key_args, &handler_value);
//         Object* handler = handler_value.as.object;

//         Value handler_result;
//         ValueArray empty_handler_args = ribbon.value_array_make(0, NULL);
//         ribbon.vm_call_object(handler, empty_handler_args, &handler_result);
//         ribbon.value_array_free(&empty_handler_args);

//         ObjectString* result = reinterpret_cast<ObjectString*>(handler_result.as.object);
//         r->answer_ = result->chars;

//     } else {
//         r->status_ = "404 Not Found";
//         r->answer_ = "Not found";
//     }

//     ribbon.value_array_free(&path_key_args);
// }

static void request_handler(webserver::http_request* request) {
    std::lock_guard<std::mutex> lock(mutex);

    Value instance_value;
    ValueArray instantiate_request_args = ribbon.value_array_make(0, nullptr);
    if (ribbon.vm_instantiate_class(http_request_class, instantiate_request_args, &instance_value) != CALL_RESULT_SUCCESS) {
        FAIL("Failed to instantiate HttpRequest");
    }
    ribbon.value_array_free(&instantiate_request_args);

    if (!ribbon.object_value_is(instance_value, OBJECT_INSTANCE)) {
        FAIL("Instantiating HttpRequest returned a non-instance");
    }
    if (!ribbon.is_value_instance_of_class(instance_value, "HttpRequest")) {
        FAIL("Instantiating HttpRequest seems to have returned a non-HttpRequest value");
    }

    ObjectInstanceHttpRequest* request_object = reinterpret_cast<ObjectInstanceHttpRequest*>(instance_value.as.object);
    request_object->request = request;

    ValueArray call_handler_args = ribbon.value_array_make(0, nullptr);
    ribbon.value_array_write(&call_handler_args, &instance_value);
    Value handler_result;
    if (ribbon.vm_call_object(handler_callable, call_handler_args, &handler_result) != CALL_RESULT_SUCCESS) {
        FAIL("Handler failed");
    }

    ribbon.value_array_free(&call_handler_args);

    if (handler_result.type != VALUE_NIL) {
        FAIL("Expected handler to return nil");
    }
}

static bool start_server_method(Object* self, ValueArray args, Value* out) {
    if (args.values[0].type != VALUE_NUMBER || args.values[1].type != VALUE_OBJECT) {
        return false;
    }

    const double port = args.values[0].as.number;
    double dummy;
    if (std::modf(port, &dummy) != 0) {
        return false;
    }

    handler_callable = args.values[1].as.object;

    Value handler_callable_value;
    handler_callable_value.type = VALUE_OBJECT;
    handler_callable_value.as.object = handler_callable;

    // Save it as an attribute of the module object, otherwise GC will collect the function object
    ribbon.object_set_attribute_cstring_key(reinterpret_cast<Object*>(module), "_handler_callable", handler_callable_value);

    webserver(static_cast<unsigned int>(port), request_handler);

    *out = Value{VALUE_NIL}; /* Value is discarded anyway, but has to be pushed */
    out->as.number = -1;
    return true;
}

// static bool start_server_method(Object* self, ValueArray args, Value* out) {
//     ObjectTable* handlers = reinterpret_cast<ObjectTable*>(args.values[0].as.object);
//     Value handlers_value = Value{VALUE_OBJECT};
//     handlers_value.as.object = reinterpret_cast<Object*>(handlers);

//     ribbon.object_set_attribute_cstring_key(reinterpret_cast<Object*>(module), "_handlers", handlers_value);

//     printf("Starting server\n");
//     webserver(8080, request_handler);
//     printf("Ending server\n");

//     *out = Value{VALUE_NIL}; /* Value is discarded anyway, but has to be pushed */
//     out->as.number = -1;
//     return true;
// }

static void http_request_class_deallocate(ObjectInstance* instance) {
    /* Currently nothing to do here */
}

__declspec(dllexport) bool ribbon_module_init(RibbonApi api, ObjectModule* self) {
    ribbon = api;
    module = self;
  
    char* start_function_arg_names[2] = {"port", "handler"};
    expose_function("start", 2, start_function_arg_names, start_server_method); 

    ObjectInstance* http_request_descriptor = ribbon.object_descriptor_new_native(http_request_descriptor_get, http_request_descriptor_set);
    void* http_request_descriptors[][2] = {
        {static_cast<void*>(const_cast<char*>("path")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("method")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("status")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("user_agent")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("accept_encoding")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("accept_language")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("accept")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("auth_realm")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("answer")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("authentication_given")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("password")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("payload")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("username")), http_request_descriptor}, 
        {static_cast<void*>(const_cast<char*>("params")), http_request_descriptor}, 
        {nullptr, nullptr}
    };

    // Should we pass a constructor here?
    http_request_class = expose_class(
        "HttpRequest", sizeof(ObjectInstanceHttpRequest), http_request_class_deallocate, nullptr, nullptr, http_request_descriptors);

    return true;
}

// __declspec(dllexport) bool ribbon_module_init(RibbonApi api, ObjectModule* module) {
//     ribbon = api;
//     this = module;

//     /* Init internal stuff */
    
//     owned_string_counter = 0;
//     owned_strings = ribbon.object_table_new_empty();
//     ribbon.object_set_attribute_cstring_key((Object*) this, "_owned_strings", MAKE_VALUE_OBJECT(owned_strings));

//     cached_strings_counter = 0;
//     cached_strings = ribbon.object_table_new_empty();
//     ribbon.object_set_attribute_cstring_key((Object*) this, "_cached_strings", MAKE_VALUE_OBJECT(cached_strings));

//     /* Init and expose classes */

//     texture_class = expose_class("Texture", sizeof(ObjectInstanceTexture), texture_class_deallocate, NULL,
//                 ribbon.object_make_constructor(2, (char*[]) {"renderer", "filename"}, texture_init), NULL);
//     window_class = expose_class("Window", sizeof(ObjectInstanceWindow), window_class_deallocate, NULL, 
//                 ribbon.object_make_constructor(6, (char*[]) {"title", "x", "y", "w", "h", "flags"}, window_init), NULL);
//     renderer_class = expose_class("Renderer", sizeof(ObjectInstanceRenderer), renderer_class_deallocate, renderer_class_gc_mark,
//                 ribbon.object_make_constructor(3, (char*[]) {"window", "index", "flags"}, renderer_init), NULL);

//     ObjectInstance* rect_descriptor = ribbon.object_descriptor_new_native(rect_descriptor_get, rect_descriptor_set);
//     void* rect_descriptors[][2] = {
//         {"x", rect_descriptor}, {"y", rect_descriptor}, {"w", rect_descriptor}, {"h", rect_descriptor}, {NULL, NULL}
//     };

//     rect_class = expose_class("Rect", sizeof(ObjectInstanceRect), rect_class_deallocate, NULL, 
//                 ribbon.object_make_constructor(4, (char*[]) {"x", "y", "w", "h"}, rect_init), rect_descriptors);

//     ObjectInstance* event_descriptor = ribbon.object_descriptor_new_native(event_descriptor_get, event_descriptor_set);
//     void* event_descriptors[][2] = {
//         {"type", event_descriptor}, {"scancode", event_descriptor}, {"repeat", event_descriptor}, {NULL, NULL}
//     };

//     event_class = expose_class("Event", sizeof(ObjectInstanceEvent), event_class_deallocate,
//         NULL, ribbon.object_make_constructor(3, (char*[]) {"type", "scancode", "repeat"}, event_init), event_descriptors);

//     /* Init and explose function */

//     expose_function("init", 1, (char*[]) {"flags"}, init);
//     expose_function("set_hint", 2, (char*[]) {"name", "value"}, set_hint);
//     expose_function("create_renderer", 3, (char*[]) {"window", "index", "flags"}, create_renderer);
//     expose_function("create_window", 6, (char*[]) {"title", "x", "y", "w", "h", "flags"}, create_window);
//     expose_function("img_init", 1, (char*[]) {"flags"}, img_init);
//     expose_function("show_cursor", 1, (char*[]) {"toggle"}, show_cursor);
//     expose_function("destroy_renderer", 1, (char*[]) {"renderer"}, destroy_renderer);
//     expose_function("destroy_window", 1, (char*[]) {"window"}, destroy_window);
//     expose_function("quit", 0, NULL, quit);
//     expose_function("log_message", 3, (char*[]) {"category", "priority", "message"}, log_message);
//     expose_function("img_load_texture", 2, (char*[]) {"renderer", "filename"}, img_load_texture);
//     expose_function("query_texture", 2, (char*[]) {"texture", "out_table"}, query_texture);
//     expose_function("set_render_draw_color", 5, (char*[]) {"renderer", "r", "g", "b", "a"}, set_render_draw_color);
//     expose_function("render_clear", 1, (char*[]) {"renderer"}, render_clear);
//     expose_function("render_present", 1, (char*[]) {"renderer"}, render_present);
//     expose_function("render_copy", 4, (char*[]) {"renderer", "texture", "src_rect", "dst_rect"}, render_copy);
//     expose_function("poll_event", 0, NULL, poll_event);
//     expose_function("get_ticks", 0, NULL, get_ticks);
//     expose_function("delay", 1, (char*[]) {"ms"}, delay);
//     expose_function("render_draw_line", 5, (char*[]) {"renderer", "x1", "y1", "x2", "y2"}, render_draw_line);
//     expose_function("set_render_draw_blend_mode", 2, (char*[]) {"renderer", "blend_mode"}, set_render_draw_blend_mode);
//     expose_function("set_texture_blend_mode", 2, (char*[]) {"texture", "blend_mode"}, set_texture_blend_mode);
//     expose_function("set_texture_color_mod", 4, (char*[]) {"texture", "r", "g", "b"}, set_texture_color_mod);
//     expose_function("set_texture_alpha_mod", 2, (char*[]) {"texture", "alpha"}, set_texture_alpha_mod);

//     /* Init and expose constants */

//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_WINDOWPOS_UNDEFINED", MAKE_VALUE_NUMBER(SDL_WINDOWPOS_UNDEFINED));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_INIT_VIDEO", MAKE_VALUE_NUMBER(SDL_INIT_VIDEO));
//     ribbon.object_set_attribute_cstring_key((Object*) this, 
//                         "SDL_HINT_RENDER_SCALE_QUALITY",
//                         MAKE_VALUE_OBJECT(ribbon.object_string_copy_from_null_terminated(SDL_HINT_RENDER_SCALE_QUALITY)));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_RENDERER_ACCELERATED", MAKE_VALUE_NUMBER(SDL_RENDERER_ACCELERATED));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "IMG_INIT_PNG", MAKE_VALUE_NUMBER(IMG_INIT_PNG));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "IMG_INIT_JPG", MAKE_VALUE_NUMBER(IMG_INIT_JPG));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_LOG_CATEGORY_APPLICATION", MAKE_VALUE_NUMBER(SDL_LOG_CATEGORY_APPLICATION));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_LOG_PRIORITY_INFO", MAKE_VALUE_NUMBER(SDL_LOG_PRIORITY_INFO));

//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_QUIT", MAKE_VALUE_NUMBER(SDL_QUIT));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_KEYUP", MAKE_VALUE_NUMBER(SDL_KEYUP));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_KEYDOWN", MAKE_VALUE_NUMBER(SDL_KEYDOWN));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_SCANCODE_UP", MAKE_VALUE_NUMBER(SDL_SCANCODE_UP));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_SCANCODE_DOWN", MAKE_VALUE_NUMBER(SDL_SCANCODE_DOWN));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_SCANCODE_LEFT", MAKE_VALUE_NUMBER(SDL_SCANCODE_LEFT));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_SCANCODE_RIGHT", MAKE_VALUE_NUMBER(SDL_SCANCODE_RIGHT));
//     ribbon.object_set_attribute_cstring_key((Object*) this, "SDL_SCANCODE_SPACE", MAKE_VALUE_NUMBER(SDL_SCANCODE_SPACE));

//     return true;
// }
}