/***************************************************************************\
*                         __    _                 _ __                      *
*                        / /_  (_)___________  __(_) /_                     *
*                       / __ \/ / ___/ ___/ / / / / __/                     *
*                      / /_/ / (__  ) /__/ /_/ / / /_                       *
*                     /_.___/_/____/\___/\__,_/_/\__/                       *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of biscuit                                           *
*                                                                           *
*  biscuit is free software; you can redistribute it and/or                 *
*  modify it under the terms of the GNU General Public License              *
*  as published by the Free Software Foundation; either version 3           *
*  of the License, or (at your option) any later version.                   *
*                                                                           *
*  This program is distributed in the hope that it will be useful,          *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*  GNU General Public License for more details.                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program; if not, write to the Free Software              *
*  Foundation, Inc., 51 Franklin Street, Fifth Floor,                       *
*  Boston, MA  02110-1301, USA.                                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  Copyright (C) 2024, Guillaume Clercin <guillaume.clercin@billy482.net>   *
\***************************************************************************/

// sscanf, vsnprintf
#include <cstdio>
// mutex
#include <mutex>
// va_list
#include <stdarg.h>

#include "string.hpp"

namespace Biscuit {
	class StringPrivateConcatenate;

	class StringPrivate {
		public:
			virtual ~StringPrivate() = default;

			virtual bool can_concatenate() const;
			static StringPrivate * create(const char * string);
			virtual uint32_t get(uint32_t offset) const = 0;
			inline uint32_t length() const {
				return this->mStringPrivate_length;
			}
			inline uint32_t refs() const {
				return this->mStringPrivate_refs;
			}
			void release() const;
			StringPrivate * share();
			virtual StringPrivate * substring(uint32_t offset);
			virtual StringPrivate * substring(uint32_t offset, uint32_t length);
			virtual StringPrivateConcatenate * to_concatenate();
			inline uint32_t utf8_length() const {
				return this->mStringPrivate_utf8_length;
			}
			static StringPrivate * vsprintf(const char * format, va_list params);

		protected:
			StringPrivate() = default;

			uint32_t mStringPrivate_length = 0;
			uint32_t mStringPrivate_utf8_length = 0;
			mutable uint32_t mStringPrivate_refs = 1;

		private:
			static StringPrivate * vsprintf_heap(const char * format, va_list params);
			static StringPrivate * vsprintf_stack(const char * format, va_list params, uint32_t length);
			static StringPrivate * vsprintf_static(const char * format, va_list params);
	};

	class StringPrivateConcatenate : public StringPrivate {
		public:
			StringPrivateConcatenate() = default;
			virtual ~StringPrivateConcatenate();

			void append(StringPrivate * sp, bool share);
			void append(StringPrivateConcatenate * sp);
			virtual bool can_concatenate() const;
			virtual uint32_t get(uint32_t offset) const;
			virtual StringPrivate * substring(uint32_t offset);
			virtual StringPrivate * substring(uint32_t offset, uint32_t length);
			virtual StringPrivateConcatenate * to_concatenate();

		private:
			class StringPrivateConcatenateNode {
				public:
					StringPrivateConcatenateNode(StringPrivate * string);
					~StringPrivateConcatenateNode();

					inline StringPrivate * data() {
						return this->mStringPrivateConcatenateNode_data;
					}
					inline const StringPrivate * data() const {
						return this->mStringPrivateConcatenateNode_data;
					}
					inline StringPrivateConcatenateNode * next() {
						return this->mStringPrivateConcatenateNode_next;
					}
					inline const StringPrivateConcatenateNode * next() const {
						return this->mStringPrivateConcatenateNode_next;
					}
					inline void setNext(StringPrivateConcatenateNode * next) {
						this->mStringPrivateConcatenateNode_next = next;
					}

				private:
					StringPrivate * mStringPrivateConcatenateNode_data;
					StringPrivateConcatenateNode * mStringPrivateConcatenateNode_next;
			};

			StringPrivateConcatenateNode * mStringPrivateConcatenate_first = nullptr;
			StringPrivateConcatenateNode * mStringPrivateConcatenate_last = nullptr;
	};

	class StringPrivateFourByte : public StringPrivate {
		public:
			StringPrivateFourByte(const char * string, uint32_t length);
			virtual ~StringPrivateFourByte();

			virtual uint32_t get(uint32_t offset) const;

		protected:
			uint32_t * mStringPrivateFourByte_buffer;
	};

	class StringPrivateOneByte : public StringPrivate {
		public:
			StringPrivateOneByte(const char * string, uint32_t length);
			virtual ~StringPrivateOneByte();

			virtual uint32_t get(uint32_t offset) const;

		private:
			uint8_t * mStringPrivateOneByte_buffer;
	};

	class StringPrivateSubstring : public StringPrivate {
		public:
			StringPrivateSubstring(StringPrivate * sp, uint32_t offset);
			StringPrivateSubstring(StringPrivate * sp, uint32_t length, uint32_t offset);
			virtual ~StringPrivateSubstring();

			virtual uint32_t get(uint32_t offset) const;
			virtual StringPrivate * substring(uint32_t offset);
			virtual StringPrivate * substring(uint32_t offset, uint32_t length);

		private:
			StringPrivate * mStringPrivateSubstring_data;
			uint32_t mStringPrivateSubstring_length;
			uint32_t mStringPrivateSubstring_offset;
	};

	class StringPrivateTwoByte : public StringPrivate {
		public:
			StringPrivateTwoByte(const char * string, uint32_t length);
			virtual ~StringPrivateTwoByte();

			virtual uint32_t get(uint32_t offset) const;

		private:
			uint16_t * mStringPrivateTwoByte_buffer;
	};
}

using Biscuit::String;
using Biscuit::StringPrivate;
using Biscuit::StringPrivateConcatenate;
using Biscuit::StringPrivateFourByte;
using Biscuit::StringPrivateOneByte;
using Biscuit::StringPrivateSubstring;
using Biscuit::StringPrivateTwoByte;


String::String(const char * string) : mString_is_null(string == nullptr) {
	if (string != nullptr)
		this->mString_data = StringPrivate::create(string);
}

String::String(StringPrivate * string) : mString_data(string), mString_is_null(string == nullptr) {}

String::String(const String& string) : mString_is_null(string.mString_is_null) {
	if (string.mString_data != nullptr)
		this->mString_data = string.mString_data->share();
}

String::~String() {
	this->clear();
}


void String::clear() {
	this->discard_cache();

	if (this->mString_data != nullptr)
		this->mString_data->release();

	this->mString_data = nullptr;
	this->mString_is_null = true;
}

int8_t String::compare(const String& a, const String& b) {
	if (a.mString_data == nullptr and b.mString_data == nullptr)
		return 0;

	if (a.mString_data == nullptr)
		return -1;

	if (b.mString_data == nullptr)
		return 1;

	const uint32_t length_a = a.length();
	const uint32_t length_b = b.length();
	const uint32_t min_length = length_a < length_b ? length_a : length_b;
	for (uint32_t i = 0; i < min_length; i++) {
		const uint32_t char_a = a.mString_data->get(i);
		const uint32_t char_b = b.mString_data->get(i);

		if (char_a < char_b)
			return -1;
		else if (char_a > char_b)
			return 1;
	}

	if (length_a != length_b)
		return length_a < length_b ? -1 : 1;

	return 0;
}

uint32_t String::count(const String& str, uint32_t offset) const {
	const uint32_t this_length = this->length();
	if (this_length == 0 or offset >= this_length)
		return 0;

	const uint32_t str_length = str.length();
	if (str_length == 0 or this_length < str_length)
		return 0;

	const uint32_t first_char = str.mString_data->get(0);
	uint32_t count = 0;
	for (uint32_t i = offset; i + str_length <= this_length; i++) {
		if (first_char == this->mString_data->get(i)) {
			bool ok = true;

			for (uint32_t j = 1; j < str_length and ok; j++)
				ok = this->mString_data->get(i + j) == str.mString_data->get(j);

			if (ok) {
				count++;
				i += str_length - 1;
			}
		}
	}

	return count;
}

uint32_t String::decode_from_utf8(const char * string, bool escape) {
	uint8_t length;
	return String::decode_from_utf8(string, length, escape);
}

uint32_t String::decode_from_utf8(const char * string, uint8_t& length, bool escape) {
	if (string == nullptr)
		return 0;

	const uint8_t * raw_string = reinterpret_cast<const uint8_t *>(string);
	if (raw_string[0] < 0x80) {
		length = 1;
		return raw_string[0];
	} else if (raw_string[0] >> 5 == 0x6) {
		if (raw_string[1] >> 6 == 0x2) {
			length = 2;
			return ((raw_string[0] & 0x1F) << 6) + (raw_string[1] & 0x3F);
		} else if (escape) {
			length = 1;
			return 0xE000 + raw_string[0];
		} else {
			length = 0;
			return 0;
		}
	} else if (raw_string[0] >> 4 == 0xE) {
		if ((raw_string[1] >> 6) == 0x2 and (raw_string[2] >> 6) == 0x2) {
			length = 3;
			return ((raw_string[0] & 0x0F) << 12) + ((raw_string[1] & 0x3F) << 6) + (raw_string[2] & 0x3F);
		} else if (escape) {
			length = 1;
			return 0xE000 + raw_string[0];
		} else {
			length = 0;
			return 0;
		}
	} else if (raw_string[0] >> 3 == 0x1E) {
		if ((raw_string[1] >> 6) == 0x2 and (raw_string[2] >> 6) == 0x2 and (raw_string[3] >> 6) == 0x2) {
			length = 4;
			return ((raw_string[0] & 0x07) << 18) + ((raw_string[1] & 0x3F) << 12) + ((raw_string[2] & 0x3F) << 6) + (raw_string[3] & 0x3F);
		} else if (escape) {
			length = 1;
			return 0xE000 + raw_string[0];
		} else {
			length = 0;
			return 0;
		}
	} else if (escape) {
		length = 1;
		return 0xE000 + raw_string[0];
	} else {
		length = 0;
		return 0;
	}
}

uint8_t String::decode_length_from_utf8(const char * string, bool escape) {
	uint8_t length = 0;
	String::decode_from_utf8(string, length, escape);
	return length;
}

void String::discard_cache() {
	if (this->mString_cache != nullptr)
		delete [] this->mString_cache;
	this->mString_cache = nullptr;
}

int8_t String::encode_to_utf8(char * string, uint32_t length, uint32_t unicode, bool null) {
	return String::encode_to_utf8(string, length, false, unicode, null);
}

int8_t String::encode_to_utf8(char * string, uint32_t length, bool unescaped, uint32_t unicode, bool null) {
	uint32_t character_length = String::utf8_length(unicode, unescaped);

	if ((null and character_length + 1 > length) or character_length > length)
		return -1;

	if (unicode < 0x80) {
		string[0] = static_cast<char>(unicode & 0x7F);
		if (null) {
			string[1] = '\0';
			return 2;
		} else
			return 1;
	} else if (unicode < 0x800) {
		if (unescaped and (unicode >> 8) == 0xE0) {
			string[0] = static_cast<char>(unicode & 0xFF);
			if (null) {
				string[1] = '\0';
				return 2;
			} else
				return 1;
		} else {
			string[0] = static_cast<char>(((unicode & 0x7C0) >> 6) | 0xC0);
			string[1] = static_cast<char>((unicode & 0x3F) | 0x80);
			if (null) {
				string[2] = '\0';
				return 3;
			} else
				return 2;
		}
	} else if (unicode < 0x10000) {
		string[0] = static_cast<char>(((unicode & 0xF000) >> 12) | 0xE0);
		string[1] = static_cast<char>(((unicode & 0xFC0) >> 6) | 0x80);
		string[2] = static_cast<char>((unicode & 0x3F) | 0x80);
		if (null) {
			string[3] = '\0';
			return 4;
		} else
			return 3;
	} else if (unicode < 0x200000) {
		string[0] = static_cast<char>(((unicode & 0x1C0000) >> 18) | 0xF0);
		string[1] = static_cast<char>(((unicode & 0x3F000) >> 12) | 0x80);
		string[2] = static_cast<char>(((unicode & 0xFC0) >> 6) | 0x80);
		string[3] = static_cast<char>((unicode & 0x3F) | 0x80);
		if (null) {
			string[4] = '\0';
			return 5;
		} else
			return 4;
	} else {
		if (null) {
			string[0] = '\0';
			return 1;
		} else
			return 0;
	}
}

bool String::ends_with(const String& end) const {
	if (this->mString_is_null and end.mString_is_null)
		return true;
	if (this->mString_is_null)
		return false;
	if (end.mString_is_null)
		return true;

	const int32_t position = this->length() - end.length();
	if (position < 0)
		return false;

	for (uint32_t this_pos = position, end_pos = 0; end_pos < end.length(); this_pos++, end_pos++)
		if (this->mString_data->get(this_pos) != end.mString_data->get(end_pos))
			return false;

	return true;
}

int32_t String::find(const String& str, uint32_t offset) const {
	const uint32_t this_length = this->length();
	if (this_length == 0 or offset >= this_length)
		return -1;

	const uint32_t str_length = str.length();
	if (str_length == 0)
		return 0;

	if (this_length < str_length)
		return -1;

	const uint32_t first_char = str.mString_data->get(0);
	for (uint32_t i = offset; i < this_length; i++) {
		if (first_char == this->mString_data->get(i)) {
			bool ok = true;

			for (uint32_t j = 1; j < str_length and ok; j++)
				ok = this->mString_data->get(i + j) == str.mString_data->get(j);

			if (ok)
				return i;
		}
	}

	return -1;
}

uint32_t String::get(uint32_t offset) const {
	if (this->length() <= offset)
		return 0;
	else
		return this->mString_data->get(offset);
}

uint64_t String::hash(const char * string) {
	if (string == nullptr)
		return 0;

	uint64_t hash = 0;
	while (*string != '\0') {
		uint8_t length;
		const uint32_t character = String::decode_from_utf8(string, length, true);
		hash = character + (hash << 6) + (hash << 16) - hash;

		string += length;
	}

	return hash;
}

uint64_t String::hash(const String& string) {
	if (string.length() == 0)
		return 0;

	uint64_t hash = 0;
	for (uint32_t index = 0; index < string.length(); index++) {
		const uint32_t character = string.get(index);
		hash = character + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

uint32_t String::length() const {
	if (this->mString_data == nullptr)
		return 0;
	else
		return this->mString_data->length();
}

String String::middle_ellipsis(uint32_t length, const String& middle) const {
	const uint32_t str_length = this->length();
	if (length == 0)
		return String();

	if (length >= str_length)
		return String(*this);

	const uint32_t left = length / 2;
	const uint32_t right = str_length - length + left + 1;

	StringPrivateConcatenate * sp = new StringPrivateConcatenate;
	sp->append(this->mString_data->substring(0, left), false);
	sp->append(middle.mString_data, true);
	sp->append(this->mString_data->substring(right), false);

	return sp;
}

String String::replace(const String& old_str, const String& new_str) const {
	const uint32_t this_length = this->length();
	if (this_length == 0)
		return *this;

	const uint32_t old_length = old_str.length();
	if (old_length == 0 or this_length < old_length)
		return *this;

	int32_t index = this->find(old_str);
	if (index < 0)
		return *this;

	StringPrivateConcatenate * result = new StringPrivateConcatenate();

	int32_t last_index = 0;
	while (index >= 0) {
		if (index > last_index)
			result->append(this->mString_data->substring(last_index, index - last_index), false);
		result->append(new_str.mString_data, true);

		last_index = index + old_length;
		index = this->find(old_str, last_index);
	}

	if (static_cast<uint32_t>(last_index) < this_length)
		result->append(this->mString_data->substring(last_index), false);

	return result;
}

String String::sprintf(const char * format, ...) {
	va_list params;
	va_start(params, format);
	String result = StringPrivate::vsprintf(format, params);
	va_end(params);

	return result;
}

bool String::starts_with(const String& begin) const {
	if (this->mString_is_null and begin.mString_is_null)
		return true;
	if (this->mString_is_null)
		return false;
	if (begin.mString_is_null)
		return true;

	const uint32_t length = begin.mString_data->length();
	if (this->length() < length)
		return false;

	for (uint32_t i = 0; i < length; i++)
		if (this->mString_data->get(i) != begin.mString_data->get(i))
			return false;

	return true;
}

String String::substring(int32_t offset) const {
	if (this->mString_is_null)
		return String();

	const int32_t length = this->mString_data->length();
	if (length <= 0)
		return String();

	if (length < offset)
		return String();

	if (offset < 0) {
		offset = length - offset;
		if (offset < 0)
			offset = 0;
	}

	if (offset > 0)
		return this->mString_data->substring(offset);
	else
		return *this;
}

String String::substring(int32_t offset, uint32_t length) const {
	if (this->mString_is_null)
		return String();

	const uint32_t str_length = this->mString_data->length();
	if (str_length == 0)
		return String();

	if (static_cast<int32_t>(str_length) < offset)
		return String();

	if (offset < 0) {
		offset = str_length - offset;
		if (offset < 0)
			offset = 0;
	}

	if (str_length < offset + length)
		length = str_length - offset;

	if (offset != 0 or length != str_length)
		return this->mString_data->substring(offset, length);
	else
		return *this;
}

uint32_t String::utf8_length() const {
	if (this->mString_data == nullptr)
		return 0;
	else
		return this->mString_data->utf8_length();
}

uint8_t String::utf8_length(uint32_t unicode, bool unescape) {
	if (unicode < 0x80)
		return 1;
	else if (unicode < 0x800)
		return 2;
	else if (unescape and (unicode >> 8) == 0xE0)
		return 1;
	else if (unicode < 0x10000)
		return 3;
	else if (unicode < 0x200000)
		return 4;
	else
		return 0;
}

String String::vsprintf(const char * format, va_list args) {
	return StringPrivate::vsprintf(format, args);
}


String& String::operator =(const char * string) {
	this->clear();

	if (string != nullptr) {
		this->mString_data = StringPrivate::create(string);
		this->mString_is_null = false;
	} else
		this->mString_is_null = true;

	return *this;
}

String& String::operator =(const String& string) {
	if (this != &string) {
		this->clear();

		if (string.mString_data != nullptr)
			this->mString_data = string.mString_data->share();

		this->mString_is_null = string.mString_is_null;
	}

	return *this;
}

String& String::operator +=(const String& string) {
	if (string.mString_data == nullptr)
		return *this;

	if (this->mString_data == nullptr) {
		this->mString_data = string.mString_data->share();
		this->mString_is_null = string.mString_is_null;
		return *this;
	}

	this->discard_cache();

	StringPrivateConcatenate * sp;
	if (this->mString_data->can_concatenate()) {
		if (this->mString_data->refs() > 1) {
			sp = new StringPrivateConcatenate;
			sp->append(this->mString_data->to_concatenate());
			this->mString_data->release();
			this->mString_data = sp;
		} else
			sp = this->mString_data->to_concatenate();
	} else {
		sp = new StringPrivateConcatenate;
		sp->append(this->mString_data, false);
		this->mString_data = sp;
	}

	if (string.mString_data->can_concatenate())
		sp->append(string.mString_data->to_concatenate());
	else
		sp->append(string.mString_data, true);

	return *this;
}

String String::operator +(const String& string) const {
	if (this->mString_data == nullptr and string.mString_data == nullptr)
		return String();

	if (this->mString_data != nullptr and string.mString_data == nullptr)
		return *this;

	if (this->mString_data == nullptr and string.mString_data != nullptr)
		return string;

	StringPrivateConcatenate * sp = new StringPrivateConcatenate;

	if (this->mString_data->can_concatenate())
		sp->append(this->mString_data->to_concatenate());
	else
		sp->append(this->mString_data, true);

	if (string.mString_data->can_concatenate())
		sp->append(string.mString_data->to_concatenate());
	else
		sp->append(string.mString_data, true);

	return sp;
}

String::operator const char *() const {
	if (this->mString_data == nullptr)
		return nullptr;

	if (this->mString_cache == nullptr) {
		const uint32_t utf8_length = this->mString_data->utf8_length(), length = this->mString_data->length();
		this->mString_cache = new char[utf8_length + 1];
		for (uint32_t index = 0, offset = 0; index < length; index++)
			offset += String::encode_to_utf8(this->mString_cache + offset, utf8_length - offset, this->mString_data->get(index), false);
		this->mString_cache[utf8_length] = '\0';
	}

	return this->mString_cache;
}



bool StringPrivate::can_concatenate() const {
	return false;
}

StringPrivate * StringPrivate::create(const char * string) {
	uint32_t max_character = 0;
	uint32_t length = 0;
	uint32_t offset = 0;

	while (string[offset] != '\0') {
		uint8_t char_length;
		uint32_t character = String::decode_from_utf8(string + offset, char_length, true);

		offset += char_length;
		length++;

		if (max_character < character)
			max_character = character;
	}

	if (max_character == 0)
		return nullptr;
	else if (max_character < 0x100)
		return new StringPrivateOneByte(string, length);
	else if (max_character < 0x10000)
		return new StringPrivateTwoByte(string, length);
	else
		return new StringPrivateFourByte(string, length);
}

void StringPrivate::release() const {
	this->mStringPrivate_refs--;
	if (this->mStringPrivate_refs == 0)
		delete this;
}

StringPrivate * StringPrivate::share() {
	this->mStringPrivate_refs++;
	return this;
}

StringPrivate * StringPrivate::substring(uint32_t offset) {
	if (offset > 0)
		return new StringPrivateSubstring(this->share(), offset);
	else
		return this->share();
}

StringPrivate * StringPrivate::substring(uint32_t offset, uint32_t length) {
	if (offset > 0 or length < this->mStringPrivate_length)
		return new StringPrivateSubstring(this->share(), length, offset);
	else
		return this->share();
}

StringPrivateConcatenate * StringPrivate::to_concatenate() {
	return nullptr;
}

StringPrivate * StringPrivate::vsprintf(const char * format, va_list params) {
	va_list cpyArgs;
	va_copy(cpyArgs, params);
	uint32_t length = vsnprintf(nullptr, 0, format, cpyArgs);
	va_end(cpyArgs);

	if (length < 256)
		return StringPrivate::vsprintf_heap(format, params);
	else if (length < 1048576)
		return StringPrivate::vsprintf_static(format, params);
	else
		return StringPrivate::vsprintf_stack(format, params, length);
}

StringPrivate * StringPrivate::vsprintf_heap(const char * format, va_list params) {
	char buffer[256];
	vsnprintf(buffer, 256, format, params);
	return StringPrivate::create(buffer);
}

StringPrivate * StringPrivate::vsprintf_stack(const char * format, va_list params, uint32_t length) {
	char * buffer = new char[length + 1];
	vsnprintf(buffer, length + 1, format, params);

	StringPrivate * data = StringPrivate::create(buffer);
	delete [] buffer;
	return data;
}

StringPrivate * StringPrivate::vsprintf_static(const char * format, va_list params) {
	static std::mutex l;
	l.lock();

	static char buffer[1048576];
	vsnprintf(buffer, 1048576, format, params);
	StringPrivate * data = StringPrivate::create(buffer);

	l.unlock();

	return data;
}




StringPrivateConcatenate::~StringPrivateConcatenate() {
	while (this->mStringPrivateConcatenate_first != nullptr) {
		this->mStringPrivateConcatenate_last = this->mStringPrivateConcatenate_first->next();
		delete this->mStringPrivateConcatenate_first;
		this->mStringPrivateConcatenate_first = this->mStringPrivateConcatenate_last;
	}
}


void StringPrivateConcatenate::append(StringPrivate * sp, bool share) {
	if (sp->can_concatenate()) {
		this->append(sp->to_concatenate());
		if (not share)
			sp->release();
		return;
	}

	if (share)
		sp = sp->share();

	StringPrivateConcatenateNode * node = new StringPrivateConcatenateNode(sp);

	if (this->mStringPrivateConcatenate_first == nullptr)
		this->mStringPrivateConcatenate_first = this->mStringPrivateConcatenate_last = node;
	else {
		this->mStringPrivateConcatenate_last->setNext(node);
		this->mStringPrivateConcatenate_last = node;
	}

	this->mStringPrivate_length += sp->length();
	this->mStringPrivate_utf8_length += sp->utf8_length();
}

void StringPrivateConcatenate::append(StringPrivateConcatenate * sp) {
	for (StringPrivateConcatenateNode * node = sp->mStringPrivateConcatenate_first; node != nullptr; node = node->next())
		this->append(node->data(), true);
}

bool StringPrivateConcatenate::can_concatenate() const {
	return true;
}

uint32_t StringPrivateConcatenate::get(uint32_t offset) const {
	for (StringPrivateConcatenateNode * node = this->mStringPrivateConcatenate_first; node != nullptr; node = node->next()) {
		const StringPrivate * sp = node->data();

		if (sp->length() <= offset) {
			offset -= sp->length();
			continue;
		} else
			return sp->get(offset);
	}

	return 0;
}

StringPrivate * StringPrivateConcatenate::substring(uint32_t offset) {
	if (offset == 0)
		return this->share();

	StringPrivateConcatenateNode * node = this->mStringPrivateConcatenate_first;
	while (node != nullptr) {
		StringPrivate * data = node->data();

		if (offset < data->length())
			break;

		offset -= data->length();
		node = node->next();
	}

	StringPrivateConcatenate * sp = new StringPrivateConcatenate;

	if (node != nullptr and offset > 0) {
		sp->append(node->data()->substring(offset), false);
		node = node->next();
	}

	while (node != nullptr) {
		sp->append(node->data(), true);
		node = node->next();
	}

	return sp;
}

StringPrivate * StringPrivateConcatenate::substring(uint32_t offset, uint32_t length) {
	if (offset == 0 and length == this->mStringPrivate_length)
		return this->share();

	StringPrivateConcatenateNode * node = this->mStringPrivateConcatenate_first;
	while (node != nullptr and offset > 0) {
		StringPrivate * data = node->data();

		if (offset < data->length())
			break;

		offset -= data->length();
		node = node->next();
	}

	StringPrivateConcatenate * sp = new StringPrivateConcatenate;

	if (node != nullptr and offset > 0) {
		StringPrivate * data = node->data();

		if (data->length() > offset + length) {
			sp->append(data->substring(offset, length), false);
			return sp;
		} else
			sp->append(data->substring(offset), false);

		length -= data->length() - offset;
		if (length == 0)
			return sp;

		node = node->next();
	}

	while (node != nullptr and length > 0) {
		StringPrivate * data = node->data();

		if (data->length() > length) {
			sp->append(data->substring(0, length), false);
			return sp;
		} else
			sp->append(data, true);

		length -= data->length();
		if (length == 0)
			return sp;

		node = node->next();
	}

	return sp;
}

StringPrivateConcatenate * StringPrivateConcatenate::to_concatenate() {
	return this;
}



StringPrivateConcatenate::StringPrivateConcatenateNode::StringPrivateConcatenateNode(StringPrivate * string) : mStringPrivateConcatenateNode_data(string), mStringPrivateConcatenateNode_next(nullptr) {}

StringPrivateConcatenate::StringPrivateConcatenateNode::~StringPrivateConcatenateNode() {
	this->mStringPrivateConcatenateNode_data->release();
}



StringPrivateFourByte::StringPrivateFourByte(const char * string, uint32_t length) : StringPrivate() {
	this->mStringPrivate_length = length;
	this->mStringPrivateFourByte_buffer = new uint32_t[length];

	for (uint32_t from = 0, to = 0; to < length; to++) {
		uint8_t character_length;
		this->mStringPrivateFourByte_buffer[to] = String::decode_from_utf8(string + from, character_length, true);

		from += character_length;
		this->mStringPrivate_utf8_length += character_length;
	}
}

StringPrivateFourByte::~StringPrivateFourByte() {
	delete [] this->mStringPrivateFourByte_buffer;
}


uint32_t StringPrivateFourByte::get(uint32_t offset) const {
	return this->mStringPrivateFourByte_buffer[offset];
}



StringPrivateOneByte::StringPrivateOneByte(const char * string, uint32_t length) : StringPrivate() {
	this->mStringPrivate_length = length;
	this->mStringPrivateOneByte_buffer = new uint8_t[length];

	for (uint32_t from = 0, to = 0; to < length; to++) {
		uint8_t character_length;
		this->mStringPrivateOneByte_buffer[to] = String::decode_from_utf8(string + from, character_length, true);

		from += character_length;
		this->mStringPrivate_utf8_length += character_length;
	}
}

StringPrivateOneByte::~StringPrivateOneByte() {
	delete [] this->mStringPrivateOneByte_buffer;
}


uint32_t StringPrivateOneByte::get(uint32_t offset) const {
	return this->mStringPrivateOneByte_buffer[offset];
}



StringPrivateSubstring::StringPrivateSubstring(StringPrivate * sp, uint32_t offset) : StringPrivate(), mStringPrivateSubstring_data(sp), mStringPrivateSubstring_length(0), mStringPrivateSubstring_offset(offset) {
	const uint32_t length = this->mStringPrivateSubstring_data->length();
	for (uint32_t pos = offset; pos < length; pos++) {
		this->mStringPrivate_length++;
		this->mStringPrivate_utf8_length += String::utf8_length(this->mStringPrivateSubstring_data->get(pos), false);
	}

	this->mStringPrivateSubstring_length = this->mStringPrivate_length;
}

StringPrivateSubstring::StringPrivateSubstring(StringPrivate * sp, uint32_t length, uint32_t offset) : StringPrivate(), mStringPrivateSubstring_data(sp), mStringPrivateSubstring_length(length), mStringPrivateSubstring_offset(offset) {
	for (uint32_t pos = 0; pos < length; pos++) {
		this->mStringPrivate_length++;
		this->mStringPrivate_utf8_length += String::utf8_length(this->mStringPrivateSubstring_data->get(offset + pos), false);
	}
}

StringPrivateSubstring::~StringPrivateSubstring() {
	this->mStringPrivateSubstring_data->release();
}


uint32_t StringPrivateSubstring::get(uint32_t offset) const {
	return this->mStringPrivateSubstring_data->get(this->mStringPrivateSubstring_offset + offset);
}

StringPrivate * StringPrivateSubstring::substring(uint32_t offset) {
	return new StringPrivateSubstring(this->mStringPrivateSubstring_data->share(), this->mStringPrivateSubstring_length - offset, offset + this->mStringPrivateSubstring_offset);
}

StringPrivate * StringPrivateSubstring::substring(uint32_t offset, uint32_t length) {
	return new StringPrivateSubstring(this->mStringPrivateSubstring_data->share(), length, offset + this->mStringPrivateSubstring_offset);
}



StringPrivateTwoByte::StringPrivateTwoByte(const char * string, uint32_t length) : StringPrivate() {
	this->mStringPrivate_length = length;
	this->mStringPrivateTwoByte_buffer = new uint16_t[length];

	for (uint32_t from = 0, to = 0; to < length; to++) {
		uint8_t character_length;
		this->mStringPrivateTwoByte_buffer[to] = String::decode_from_utf8(string + from, character_length, true);

		from += character_length;
		this->mStringPrivate_utf8_length += character_length;
	}
}

StringPrivateTwoByte::~StringPrivateTwoByte() {
	delete [] this->mStringPrivateTwoByte_buffer;
}


uint32_t StringPrivateTwoByte::get(uint32_t offset) const {
	return this->mStringPrivateTwoByte_buffer[offset];
}
