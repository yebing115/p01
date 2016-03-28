#pragma once

class RefObject;
class RefBase {
protected:
  static bool Unreferenced(RefObject *object);
  static void Increment(RefObject *object);
  static void Decrement(RefObject *object);
};

class RefObject {
public:
  RefObject() : _ref_count(0) {}
  virtual ~RefObject() {}
  RefObject(const RefObject&) : _ref_count(0) {}
  RefObject& operator = (const RefObject&) { _ref_count = 0; }
private:
  unsigned int _ref_count;
  friend class RefBase;
};
