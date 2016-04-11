#include "C3PCH.h"
#include "Json.h"

JsonWriter::JsonWriter(char* buffer, u32 size): _builder(buffer, size) {

}

JsonWriter::~JsonWriter() {
}

void JsonWriter::WriteInt(const char* key, int value) {
  _builder.addValue(key, value);
}

void JsonWriter::WriteFloat(const char* key, float value) {
  _builder.addValue(key, value);
}

void JsonWriter::WriteBool(const char* key, bool value) {
  _builder.addValue(key, value);
}

void JsonWriter::WriteString(const char* key, const char* value) {
  _builder.addValue(key, value);
}

void JsonWriter::BeginWriteObject(const char* name) {
  _builder.startObject(name);
}

void JsonWriter::EndWriteObject() {
  _builder.endObject();
}

void JsonWriter::BeginWriteArray(const char* name) {
  _builder.startArray(name);
}

void JsonWriter::WriteIntElement(int value) {
  _builder.addValue(value);
}

void JsonWriter::WriteFloatElement(float value) {
  _builder.addValue(value);
}

void JsonWriter::WriteBoolElement(bool value) {
  _builder.addValue(value);
}

void JsonWriter::WriteStringElement(const char* value) {
  _builder.addValue(value);
}

void JsonWriter::EndWriteArray() {
  _builder.endArray();
}

bool JsonWriter::IsValid() const {
  return _builder.isBufferAdequate();
}

//////////////////////////////////////////////////////////////////////////

JsonReader::JsonReader(const char* buffer) {
  _status = gason::jsonParse((char*)buffer, _root, _json_allocator);
  _current_value = _root;
  _current_node = _current_value.toNode();
}

JsonReader::~JsonReader() {

}

bool JsonReader::ReadInt(const char* key, int& out, int default_value) {
  if (_current_value.isObject()) {
    auto child = _current_value.child(key);
    if (child.isNumber()) {
      out = child.toInt();
      return true;
    }
  }
  out = default_value;
  return false;
}

bool JsonReader::ReadFloat(const char* key, float& out, float default_value /*= 0*/) {
  if (_current_value.isObject()) {
    auto child = _current_value.child(key);
    if (child.isNumber()) {
      out = child.toNumber();
      return true;
    }
  }
  out = default_value;
  return false;
}

bool JsonReader::ReadBool(const char* key, bool& out, bool default_value /*= 0*/) {
  if (_current_value.isObject()) {
    auto child = _current_value.child(key);
    if (child.isBoolean()) {
      out = child.toBool();
      return true;
    }
  }
  out = default_value;
  return false;
}

bool JsonReader::ReadString(const char* key, char* out, int max_size, const char* default_value /*= ""*/) {
  if (_current_value.isObject()) {
    auto child = _current_value.child(key);
    if (child.isString()) {
      strncpy(out, child.toString(), max_size);
      return true;
    }
  }
  strncpy(out, default_value, max_size);
  return false;
}

bool JsonReader::BeginReadObject(const char* name) {
  if (name) {
    auto child = _current_value.child(name);
    if (child.isObject()) {
      _stack.push_back(_current_value);
      _current_value = child;
      _current_node = _current_value.toNode();
      return true;
    }
  } else if (_current_node && _current_node->value.isObject()) {
    _stack.push_back(_current_value);
    _current_value = _current_node->value;
    _current_node = _current_value.toNode();
    return true;
  }
  return false;
}

bool JsonReader::Peek(char* key, int max_size, JsonValueType* out_type) {
  if (!_current_node) return false;
  strncpy(key, _current_node->key, max_size);
  if (out_type) {
    auto tag = _current_node->value.getTag();
    switch (tag) {
    case gason::JSON_NUMBER:
      *out_type = JSON_VALUE_NUMBER;
      break;
    case gason::JSON_STRING:
      *out_type = JSON_VALUE_STRING;
      break;
    case gason::JSON_ARRAY:
      *out_type = JSON_VALUE_ARRAY;
      break;
    case gason::JSON_OBJECT:
      *out_type = JSON_VALUE_OBJECT;
      break;
    case gason::JSON_TRUE:
    case gason::JSON_FALSE:
      *out_type = JSON_VALUE_BOOLEAN;
      break;
    case gason::JSON_NULL:
    default:
      *out_type = JSON_VALUE_NULL;
    }
  }
  return true;
}

void JsonReader::EndReadObject() {
  _current_value = _stack.back();
  _current_node = _current_value.toNode();
  _stack.pop_back();
}

bool JsonReader::BeginReadArray(const char* name) {
  if (name) {
    auto child = _current_value.child(name);
    if (child.isArray()) {
      _stack.push_back(_current_value);
      _current_value = child;
      _current_node = _current_value.toNode();
      return true;
    }
  } else if (_current_node && _current_node->value.isArray()) {
    _stack.push_back(_current_value);
    _current_value = _current_node->value;
    _current_node = _current_value.toNode();
    return true;
  }
  return false;
}

bool JsonReader::ReadIntElement(int& out, int default_value) {
  if (_current_node) {
    if (_current_node->value.isNumber()) {
      out = _current_node->value.toInt();
      _current_node = _current_node->next;
      _current_value = _current_node ? _current_node->value : gason::JsonValue();
      return true;
    }
  }
  out = default_value;
  return false;
}

bool JsonReader::ReadFloatElement(float& out, float default_value) {
  if (_current_node) {
    if (_current_node->value.isNumber()) {
      out = _current_node->value.toNumber();
      _current_node = _current_node->next;
      _current_value = _current_node ? _current_node->value : gason::JsonValue();
      return true;
    }
  }
  out = default_value;
  return false;
}

bool JsonReader::ReadBoolElement(bool& out, bool default_value) {
  if (_current_node) {
    if (_current_node->value.isBoolean()) {
      out = _current_node->value.toBool();
      _current_node = _current_node->next;
      _current_value = _current_node ? _current_node->value : gason::JsonValue();
      return true;
    }
  }
  out = default_value;
  return false;
}

bool JsonReader::ReadStringElement(char* out, int max_size, const char* default_value) {
  if (_current_node) {
    if (_current_node->value.isString()) {
      strncpy(out, _current_node->value.toString(), max_size);
      _current_node = _current_node->next;
      _current_value = _current_node ? _current_node->value : gason::JsonValue();
      return true;
    }
  }
  strncpy(out, default_value, max_size);
  return false;
}

void JsonReader::EndReadArray() {
  _current_value = _stack.back();
  _current_node = _current_value.toNode();
  _stack.pop_back();
}

bool JsonReader::IsValid() const {
  return _status == gason::JSON_PARSE_OK;
}
