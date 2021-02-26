#include <iostream>

#include "webserver/webserver.h"
#include "Socket.h"

extern "C"
{
#include "ribbon_api.h"

static RibbonApi ribbon;
static ObjectModule* module;

static void expose_function(char* name, int num_params, char** params, NativeFunction function) {
    // Value value = MAKE_VALUE_OBJECT(ribbon.make_native_function_with_params(name, num_params, params, function));
    Value value = Value{VALUE_OBJECT};
    value.as.object = reinterpret_cast<Object*>(ribbon.make_native_function_with_params(name, num_params, params, function));
    ribbon.object_set_attribute_cstring_key((Object*) module, name, value);
}

// static ObjectClass* expose_class(
//     char* name, size_t instance_size, DeallocationFunction dealloc_func,
//     GcMarkFunction gc_mark_func, ObjectFunction* init_func, void* descriptors[][2]) {

//     ObjectClass* klass = ribbon.object_class_native_new(name, instance_size, dealloc_func, gc_mark_func, init_func, descriptors);
//     ribbon.object_set_attribute_cstring_key((Object*) module, name, MAKE_VALUE_OBJECT(klass));
//     return klass;
// }

// static void request_handler(webserver::http_request* r) {
//   Socket s = *(r->s_);

//   std::string title;
//   std::string body;
//   std::string bgcolor="#ffffff";
//   std::string links =
//       "<p><a href='/red'>red</a> "
//       "<br><a href='/blue'>blue</a> "
//       "<br><a href='/form'>form</a> "
//       "<br><a href='/auth'>authentication example</a> [use <b>rene</b> as username and <b>secretGarden</b> as password"
//       "<br><a href='/header'>show some HTTP header details</a> "
//       ;

//   if(r->path_ == "/") {
//     title = "Web Server Example";
//     body  = "<h1>Welcome to Rene's Web Server</h1>"
//             "I wonder what you're going to click"  + links;
//   }
//   else if (r->path_ == "/red") {
//     bgcolor = "#ff4444";
//     title   = "You chose red";
//     body    = "<h1>Red</h1>" + links;
//   }
//   else if (r->path_ == "/blue") {
//     bgcolor = "#4444ff";
//     title   = "You chose blue";
//     body    = "<h1>Blue</h1>" + links;
//   }
//   else if (r->path_ == "/form") {
//     title   = "Fill a form";

//     body    = "<h1>Fill a form</h1>";
//     body   += "<form action='/form'>"
//               "<table>"
//               "<tr><td>Field 1</td><td><input name=field_1></td></tr>"
//               "<tr><td>Field 2</td><td><input name=field_2></td></tr>"
//               "<tr><td>Field 3</td><td><input name=field_3></td></tr>"
//               "</table>"
//               "<input type=submit></form>";


//     for (std::map<std::string, std::string>::const_iterator i = r->params_.begin();
//          i != r->params_.end();
//          i++) {

//       body += "<br>" + i->first + " = " + i->second;
//     }


//     body += "<hr>" + links;

//   }
//   else if (r->path_ == "/auth") {
//     if (r->authentication_given_) {
//       if (r->username_ == "rene" && r->password_ == "secretGarden") {
//          body = "<h1>Successfully authenticated</h1>" + links;
//       }
//       else {
//          body = "<h1>Wrong username or password</h1>" + links;
//          r->auth_realm_ = "Private Stuff";
//       }
//     }
//     else {
//       r->auth_realm_ = "Private Stuff";
//     }
//   }
//   else if (r->path_ == "/header") {
//     title   = "some HTTP header details";
//     body    = std::string ("<table>")                                   +
//               "<tr><td>Accept:</td><td>"          + r->accept_          + "</td></tr>" +
//               "<tr><td>Accept-Encoding:</td><td>" + r->accept_encoding_ + "</td></tr>" +
//               "<tr><td>Accept-Language:</td><td>" + r->accept_language_ + "</td></tr>" +
//               "<tr><td>User-Agent:</td><td>"      + r->user_agent_      + "</td></tr>" +
//               "</table>"                                                +
//               links;
//   }
//   else {
//     r->status_ = "404 Not Found";
//     title      = "Wrong URL";
//     body       = "<h1>Wrong URL</h1>";
//     body      += "Path is : &gt;" + r->path_ + "&lt;"; 
//   }

//   r->answer_  = "<html><head><title>";
//   r->answer_ += title;
//   r->answer_ += "</title></head><body bgcolor='" + bgcolor + "'>";
//   r->answer_ += body;
//   r->answer_ += "</body></html>";
// }

static void request_handler(webserver::http_request* r) {
  /* TODO: Check function calls boolean return value  */

  Value handlers_value;
  ribbon.object_load_attribute_cstring_key(reinterpret_cast<Object*>(module), "_handlers", &handlers_value);

  ObjectTable* handlers = reinterpret_cast<ObjectTable*>(handlers_value.as.object);

  ObjectString* path_as_string = ribbon.object_string_copy_from_null_terminated(r->path_.c_str());
  Value path_as_string_value = Value{VALUE_OBJECT};
  path_as_string_value.as.object = reinterpret_cast<Object*>(path_as_string);
  
  ValueArray path_key_args;
  ribbon.value_array_init(&path_key_args);
  ribbon.value_array_write(&path_key_args, &path_as_string_value);
  Value has_path;
  ribbon.vm_call_attribute_cstring(reinterpret_cast<Object*>(handlers), "has_key", path_key_args, &has_path);

  if (has_path.as.boolean) {
    Value handler_value;
    ribbon.vm_call_attribute_cstring(reinterpret_cast<Object*>(handlers), "@get_key", path_key_args, &handler_value);
    Object* handler = handler_value.as.object;

    Value handler_result;
    ValueArray empty_handler_args = ribbon.value_array_make(0, NULL);
    ribbon.vm_call_object(handler, empty_handler_args, &handler_result);
    ribbon.value_array_free(&empty_handler_args);

    ObjectString* result = reinterpret_cast<ObjectString*>(handler_result.as.object);
    r->answer_ = result->chars;

  } else {
    r->status_ = "404 Not Found";
    r->answer_ = "Not found";
  }

  ribbon.value_array_free(&path_key_args);
}

static bool start_server_method(Object* self, ValueArray args, Value* out) {
  ObjectTable* handlers = reinterpret_cast<ObjectTable*>(args.values[0].as.object);
  Value handlers_value = Value{VALUE_OBJECT};
  handlers_value.as.object = reinterpret_cast<Object*>(handlers);

  ribbon.object_set_attribute_cstring_key(reinterpret_cast<Object*>(module), "_handlers", handlers_value);

  printf("Starting server\n");
  webserver(8080, request_handler);
  printf("Ending server\n");

  *out = Value{VALUE_NIL}; /* Value is discarded anyway, but has to be pushed */
  out->as.number = -1;
  return true;
}

__declspec(dllexport) bool ribbon_module_init(RibbonApi api, ObjectModule* self) {
    ribbon = api;
    module = self;
  
  char* start_function_arg_names[1] = {"handlers"};
  expose_function("start", 1, start_function_arg_names, start_server_method); 

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