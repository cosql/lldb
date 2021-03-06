//===-- main.c --------------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstring>

template <typename T>
class Foo
{
public:
    Foo () : object() {}
    Foo (T x) : object(x) {}
    T getObject() { return object; }
private:
    T object;
};


int main (int argc, char const *argv[])
{
    Foo<int> foo_x('a');
    Foo<wchar_t> foo_y(L'a');
    const wchar_t *mazeltov = L"מזל טוב";
    wchar_t *ws_NULL = nullptr;
    wchar_t *ws_empty = L"";
  	wchar_t array[200], * array_source = L"Hey, I'm a super wchar_t string, éõñž";
  	memcpy(array, array_source, 39 * sizeof(wchar_t));
    return 0; // Set break point at this line.
}
