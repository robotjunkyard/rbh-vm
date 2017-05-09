#pragma once
#include <string>
#include <algorithm>
#include <cstring>

template <unsigned int SIZE = 8192> class TextBuffer
{
private:
	const char* const _name;
	const char* const _desc;
	char _text[SIZE];
	void zeroText()
	{
		for (int i = 0; i < SIZE; i++)
			_text[i] = 0;
	}

public:
	TextBuffer<SIZE>()
		: _name("Untitled")
	{	zeroText();   }

	TextBuffer<SIZE>(const char* const text, const char* const name = "Untitled", const char* const desc = "")
		: _name(name), _desc(desc)
	{
		zeroText();
		for (int i = 0; i < strlen(text); i++)
			_text[i] = text[i];
	}

	TextBuffer<SIZE>(const std::string& text, const char* const name = "Untitled", const char* const desc = "")
		: _name(name), _desc(desc)
	{
		zeroText();
		for (int i = 0; i < std::min<unsigned int>(SIZE, text.length()); i++)
			_text[i] = text.c_str()[i];
	}

	~TextBuffer<SIZE>() { }

	int capacity() const {
		return SIZE;
	}

	int length() const {
		return strlen(&_text[0]);
	}

	char* text() {
		return &_text[0];
	}

	char& operator[](int index)
	{
		// TODO: bounds checking
		return _text[index];
	}
};

