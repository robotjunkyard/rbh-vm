#pragma once
#include <string>
#include <algorithm>
#include <cstring>

// this quick-n-dirty class mainly exists because I'm uneasy about directly
// using char* or std::string.c_str() with ImGui's text input widgets
class TextBuffer
{
private:
    const int         _capacity;
	const char* const _name;
	const char* const _desc;
	char*             _text;

	void zeroText()
	{
        memset(&_text[0], 0, _capacity);
	}

public:
    TextBuffer(TextBuffer&& other)
        : _capacity(other._capacity), _name(other._name), _desc(other._desc), _text(other._text)
    {

    }

	explicit TextBuffer(int capacity = 8192)
		: _capacity(capacity), _name("Untitled"), _desc("")
	{	zeroText();   }

	explicit TextBuffer(const char* const text, const char* const name = "Untitled", const char* const desc = "")
		: _capacity(strlen(text) + 8192), _name(name), _desc(desc)
	{
        _text = new char[_capacity];
		zeroText();
		this->set(text);
	}

	explicit TextBuffer(const std::string& text, const char* const name = "Untitled", const char* const desc = "")
		: _capacity(text.size() + 8192), _name(name), _desc(desc)
	{
        _text = new char[_capacity];
		zeroText();
		this->set(text);
	}

	~TextBuffer() { if (_text) delete[] _text; }

	int capacity() const {
		return _capacity;
	}

	int length() const {
		return strlen(&_text[0]);
	}

	char* text() {
		return &_text[0];
	}

	const char* const name() const { return _name; }
	const char* const desc() const { return _desc; }

	char& operator[](int index)
	{
		// TODO: bounds checking
		return _text[index];
	}

	void set(const std::string& text)
	{
        int bytes = std::min<unsigned int>(text.length(), this->capacity() - 1);
        memcpy(this->text(), text.c_str(), bytes);
        _text[bytes] = '\0';
	}

	TextBuffer& operator=(const std::string& text)
	{
        set(text);
        return *this;
	}
};

