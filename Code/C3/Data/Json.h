#pragma once
#include "Data/DataType.h"
#include "Memory/Allocator.h"
#include <gason.hpp>
#include <jsonbuilder.hpp>

class JsonWriter {
public:
  JsonWriter(char* buffer, u32 size);
  ~JsonWriter();

  void WriteInt(const char* key, int value);
  void WriteFloat(const char* key, float value);
  void WriteBool(const char* key, bool value);
  void WriteString(const char* key, const char* value);
  
  void BeginWriteObject(const char* name = nullptr);
  void EndWriteObject();
  
  void BeginWriteArray(const char* name = nullptr);
  void WriteIntElement(int value);
  void WriteFloatElement(float value);
  void WriteBoolElement(bool value);
  void WriteStringElement(const char* value);
  void EndWriteArray();

  bool IsValid() const;

private:
  gason::JSonBuilder _builder;
};

enum JsonValueType {
  JSON_VALUE_NUMBER = 0,
  JSON_VALUE_STRING,
  JSON_VALUE_ARRAY,
  JSON_VALUE_OBJECT,
  JSON_VALUE_BOOLEAN,
  JSON_VALUE_NULL = 0xF
};

class JsonReader {
public:
  JsonReader(const char* buffer);
  ~JsonReader();

  bool ReadInt(const char* key, int& out, int default_value = 0);
  bool ReadFloat(const char* key, float& out, float default_value = 0);
  bool ReadBool(const char* key, bool& out, bool default_value = 0);
  bool ReadString(const char* key, char* out, int max_size, const char* default_value = "");

  bool BeginReadObject(const char* name = nullptr);
  bool Peek(char* key, int max_size, JsonValueType* out_type = nullptr);
  void EndReadObject();

  bool BeginReadArray(const char* name = nullptr);
  bool ReadIntElement(int& out, int default_value = 0);
  bool ReadFloatElement(float& out, float default_value = 0);
  bool ReadBoolElement(bool& out, bool default_value = 0);
  bool ReadStringElement(char* out, int max_size, const char* default_value = "");
  void EndReadArray();

  bool IsValid() const;

private:
  void PushStack();
  void PopStack();
  gason::JsonAllocator _json_allocator;
  gason::JsonValue _root;
  gason::JsonValue _current_value;
  gason::JsonNode* _current_node;
  struct Cursor {
    gason::JsonValue _value;
    gason::JsonNode* _node;
  };
  vector<Cursor> _stack;
  gason::JsonParseStatus _status;
};
