#include <libpostal/libpostal.h>
#include <nan.h>

#define PARSER_USAGE "Usage: parse_address(address[, options])"

void ParseAddress(const Nan::FunctionCallbackInfo<v8::Value> &args) {
    if (args.Length() < 1) {
        Nan::ThrowTypeError(PARSER_USAGE);
        return;
    }

    if (!args[0]->IsString()) {
        Nan::ThrowTypeError(PARSER_USAGE);
    }

    Nan::Utf8String address_utf8(args[0]);
    char *address = *address_utf8;

    if (address == NULL) {
        Nan::ThrowTypeError("Could not convert first arg to string");
        return;
    }

    char *language = NULL;
    char *country = NULL;

    uint64_t i;

    address_parser_options_t options = LIBPOSTAL_ADDRESS_PARSER_DEFAULT_OPTIONS;

    if (args.Length() > 1 && args[1]->IsObject()) {
        v8::Local<v8::Object> props = args[1]->ToObject();
        v8::Local<v8::Array> prop_names = Nan::GetPropertyNames(props).ToLocalChecked();

        for (i = 0; i < prop_names->Length(); i++) {
            v8::Local<v8::Value> key = prop_names->Get(i);

            if (key->IsString()) {
                Nan::Utf8String utf8_key(key);
                char *key_string = *utf8_key;

                if (strcmp(key_string, "language") == 0) {
                    Nan::Utf8String language_utf8(props->Get(key));
                    language = *language_utf8;
                    if (language != NULL) {
                        options.language = language;
                    }
                } else if (strcmp(key_string, "country") == 0) {
                    Nan::Utf8String country_utf8(props->Get(key));
                    country = *country_utf8;
                    if (country != NULL) {
                        options.country = country;
                    }
                }
            }

        }

    }

    address_parser_response_t *response = parse_address(address, options);

    if (response == NULL) {
        Nan::ThrowError("Error parsing address");
        return;
    }

    v8::Local<v8::Array> ret = Nan::New<v8::Array>(response->num_components);

    v8::Local<v8::String> name_key = Nan::New("component").ToLocalChecked();
    v8::Local<v8::String> label_key = Nan::New("label").ToLocalChecked();

    for (i = 0; i < response->num_components; i++) {
        char *component = response->components[i];
        char *label = response->labels[i];

        v8::Local<v8::Object> o = Nan::New<v8::Object>();
        o->Set(name_key, Nan::New(component).ToLocalChecked());
        o->Set(label_key, Nan::New(label).ToLocalChecked());

        ret->Set(i, o);
    }

    address_parser_response_destroy(response);

    args.GetReturnValue().Set(ret);
}

static void cleanup(void*) {
    libpostal_teardown();
    libpostal_teardown_parser();
}

void init(v8::Local<v8::Object> exports) {
    if (!libpostal_setup() || !libpostal_setup_parser()) {
        Nan::ThrowError("Could not load libpostal");
        return;
    }

    exports->Set(Nan::New("parse_address").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ParseAddress)->GetFunction());

    node::AtExit(cleanup);
}

NODE_MODULE(expand, init)
