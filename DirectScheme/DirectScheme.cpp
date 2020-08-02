 
// Fast 2D graphics for Chez Scheme on Windows
// Double Buffered for animation etc.
// Alban Read 2020.

#include "framework.h"
#include "DirectScheme.h"
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <shlwapi.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <dwrite.h>
#include <wincodec.h>
#include <string>
#include <scheme.h>
#include <deque>
#include <exception>
#include <stdexcept>
#include <vector>
#include <wincodec.h>	 
#include <WTypes.h>
#include <Mmsystem.h>
#include <Windows.Foundation.h>
#include <wrl\wrappers\corewrappers.h>
#include <wrl\client.h>

#include <functional>
#include <chrono>
#include <future>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid")
 
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment (lib, "Windowscodecs.lib")
#pragma comment (lib, "runtimeobject.lib")

#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

template <class callable, class... arguments>
void later(int after, bool async, callable f, arguments&&... args) {
	std::function<typename std::result_of < callable(arguments...)>::type() > task(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

	if (async) {
		std::thread([after, task]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(after));
			task();
			}).detach();
	}
	else {
		std::this_thread::sleep_for(std::chrono::milliseconds(after));
		task();
	}
}

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#define WinGetObject GetObjectW
#define WinSendMessage SendMessageW

HWND main_window;
HANDLE g_script_mutex;
HANDLE g_commands_mutex;
HANDLE g_rotation_mutex;

#define CALL0(who) Scall0(Stop_level_value(Sstring_to_symbol(who)))
#define CALL1(who, arg) Scall1(Stop_level_value(Sstring_to_symbol(who)), arg)
#define CALL2(who, arg, arg2) Scall2(Stop_level_value(Sstring_to_symbol(who)), arg, arg2)


#define MAX_LOADSTRING 100

#define bank_size 512


namespace Text
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Text conversions
	// Text (UTF8) processing functions.


	typedef unsigned int UTF32;
	typedef unsigned short UTF16;
	typedef unsigned char UTF8;
	typedef unsigned char Boolean;

	static const unsigned char total_bytes[256] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
	};

	static const char trailing_bytes_for_utf8[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
	};

	static const UTF8 first_byte_mark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

	static const UTF32 offsets_from_utf8[6] = {
		0x00000000UL, 0x00003080UL, 0x000E2080UL,
		0x03C82080UL, 0xFA082080UL, 0x82082080UL
	};

	typedef enum
	{
		conversionOK,
		/* conversion successful */
		sourceExhausted,
		/* partial character in source, but hit end */
		targetExhausted,
		/* insuff. room in target for conversion */
		sourceIllegal /* source sequence is illegal/malformed */
	} ConversionResult;

	typedef enum
	{
		strictConversion = 0,
		lenientConversion
	} conversion_flags;

	/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_BMP (UTF32)0x0000FFFF
#define UNI_MAX_UTF16 (UTF32)0x0010FFFF
#define UNI_MAX_UTF32 (UTF32)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32 (UTF32)0x0010FFFF

	static const int halfShift = 10;
	static const UTF32 halfBase = 0x0010000UL;
	static const UTF32 halfMask = 0x3FFUL;

#define UNI_SUR_HIGH_START (UTF32)0xD800
#define UNI_SUR_HIGH_END (UTF32)0xDBFF
#define UNI_SUR_LOW_START (UTF32)0xDC00
#define UNI_SUR_LOW_END (UTF32)0xDFFF
#define false 0
#define true 1

	/* is c the start of a utf8 sequence? */
#define ISUTF8(c) (((c)&0xC0) != 0x80)
#define ISASCII(ch) ((unsigned char)ch < 0x80)

#define UTFmax 4
	typedef char32_t Rune;
#define RUNE_C(x) x##L
#define Runeself 0x80
#define Runemax RUNE_C(0x10FFFF)

	int runelen(Rune r)
	{
		if (r <= 0x7F)
			return 1;
		else if (r <= 0x07FF)
			return 2;
		else if (r <= 0xD7FF)
			return 3;
		else if (r <= 0xDFFF)
			return 0; /* surrogate character */
		else if (r <= 0xFFFD)
			return 3;
		else if (r <= 0xFFFF)
			return 0; /* illegal character */
		else if (r <= Runemax)
			return 4;
		else
			return 0; /* rune too large */
	}


	int strlen_utf8(const char* s)
	{
		auto i = 0, j = 0;
		while (s[i])
		{
			if ((s[i] & 0xc0) != 0x80) j++;
			i++;
		}
		return j;
	}



	int utf8_string_length(const char* s)
	{
		auto i = 0, j = 0;
		while (s[i])
		{
			if ((s[i] & 0xc0) != 0x80)
				j++;
			i++;
		}
		return j;
	}

	ptr UTF8toSstring(char* s)
	{
		unsigned int byte2;

		if (s == nullptr)
		{
			return Sstring("");
		}
		const auto ll = utf8_string_length(s);
		if (ll == 0)
		{
			return Sstring("");
		}

		const char* cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);

		ptr ss = Sstring_of_length("", ll);

		auto i = 0;

		while (byte != 0 && i < ll)
		{
			// ascii
			if (byte < 0x80)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80))
			{
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);

				if ((byte2 & 0xC0) == 0x80)
				{
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);
				if (byte2 == 0)
				{
					break;
				};
				unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (byte3 == 0)
				{
					break;
				};

				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80))
				{
					byte = (((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			int trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0)
			{
				int ch = byte & (0x3F >> trail);
				do
				{
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80)
					{
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// no match..
			if (i == ll)
			{
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}
		return ss;
	}

	ptr constUTF8toSstring(const char* s)
	{
		// With UTF8 we just have sequences of bytes in a buffer.
		// in scheme we use a single longer integer for a code point.
		// see https://github.com/Komodo/KomodoEdit/blob/master/src/SciMoz/nsSciMoz.cxx
		// passes the greek test.

		unsigned int byte2;

		if (s == nullptr)
		{
			return Sstring("");
		}
		const auto ll = utf8_string_length(s);
		if (ll == 0)
		{
			return Sstring("");
		}

		const char* cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);

		ptr ss = Sstring_of_length("", ll);

		auto i = 0;

		while (byte != 0 && i < ll)
		{
			// ascii
			if (byte < 0x80)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80))
			{
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);

				if ((byte2 & 0xC0) == 0x80)
				{
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80))
				{
					byte = (((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0)
			{
				int ch = byte & (0x3F >> trail);
				do
				{
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80)
					{
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// no match..
			if (i == ll)
			{
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}

		return ss;
	}

	ptr constUTF8toSstringOfLength(const char* s, const int length)
	{
		// With UTF8 we just have sequences of bytes in a buffer.
		// in scheme we use a single longer integer for a code point.
		// see https://github.com/Komodo/KomodoEdit/blob/master/src/SciMoz/nsSciMoz.cxx
		// passes the greek test.

		unsigned int byte2;

		if (s == nullptr)
		{
			return Sstring("");
		}
		const auto ll = utf8_string_length(s);
		if (ll == 0)
		{
			return Sstring("");
		}

		const char* cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);

		ptr ss = Sstring_of_length("", length);

		auto i = 0;

		while (byte != 0 && i < length)
		{
			// ascii
			if (byte < 0x80)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0)
			{
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80))
			{
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);

				if ((byte2 & 0xC0) == 0x80)
				{
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0)
			{
				byte2 = static_cast<unsigned char>(*cptr++);
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80))
				{
					byte = (((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0)
			{
				int ch = byte & (0x3F >> trail);
				do
				{
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80)
					{
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// no match..
			if (i == ll)
			{
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}
		return ss;
	}

	// use windows functions to widen string
	std::wstring Widen(const std::string& in)
	{
		// Calculate target buffer size (not including the zero terminator).
		const auto len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			in.c_str(), in.size(), NULL, 0);
		if (len == 0)
		{
			throw std::runtime_error("Invalid character sequence.");
		}

		std::wstring out(len, 0);
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			in.c_str(), in.size(), &out[0], out.size());
		return out;
	}

	// use windows functions to widen string
	std::wstring widen(const std::string& in)
	{
		// Calculate target buffer size (not including the zero terminator).
		const auto len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			in.c_str(), in.size(), NULL, 0);
		if (len == 0)
		{
			throw std::runtime_error("Invalid character sequence.");
		}

		std::wstring out(len, 0);
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			in.c_str(), in.size(), &out[0], out.size());
		return out;
	}
	// scheme string to c char*
	char* Sstring_to_charptr(ptr sparam)
	{
		ptr bytes = CALL1("string->utf8", sparam);
		const long len = Sbytevector_length(bytes);
		const auto data = Sbytevector_data(bytes);
		const auto text = static_cast<char*>(calloc(len + 1, sizeof(char)));
		memcpy(text, data, len);
		bytes = Snil;
		return text;
	}

	// scheme strings are wider UTF32 than windows wide UTF16 strings.
	// not sure if widestring code points span more than one
	// short int; if they do this is broken.

	ptr wideto_sstring(WCHAR* s)
	{
		if (s == nullptr)
		{
			return Sstring("");
		}
		const int len = wcslen(s);
		if (len == 0)
		{
			return Sstring("");
		}
		auto ss = Sstring_of_length("", len);
		for (auto i = 0; i < len; i++)
		{
			Sstring_set(ss, i, s[i]);
		}
		return ss;
	}



	ptr const_utf8_to_sstring(const char* s) {



		// With UTF8 we just have sequences of bytes in a buffer.
		// in scheme we use a single longer integer for a code point.
		// see https://github.com/Komodo/KomodoEdit/blob/master/src/SciMoz/nsSciMoz.cxx
		// passes the greek test.

		unsigned int byte2;

		if (s == nullptr) {
			return Sstring("");
		}
		const auto ll = Text::strlen_utf8(s);
		if (ll == 0) {
			return Sstring("");
		}

		auto cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);

		auto ss = Sstring_of_length("", ll);

		auto i = 0;

		while (byte != 0 && i < ll) {

			// ascii
			if (byte < 0x80) {
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0) {
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80)) {
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0) {
				byte2 = static_cast<unsigned char>(*cptr++);
				if ((byte2 & 0xC0) == 0x80) {
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0) {
				byte2 = static_cast<unsigned char>(*cptr++);
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80)) {
					byte = (((byte & 0x0F) << 12)
						| ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0) {
				int ch = byte & (0x3F >> trail);
				do {
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80) {
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// no match..
			if (i == ll) {
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}

		return ss;
	}

	ptr const_utf8_to_sstring_of_length(const char* s, const int length) {


		// With UTF8 we just have sequences of bytes in a buffer.
		// in scheme we use a single longer integer for a code point.
		// see https://github.com/Komodo/KomodoEdit/blob/master/src/SciMoz/nsSciMoz.cxx
		// passes the greek test.

		unsigned int byte2;
		if (s == nullptr) {
			return Sstring("");
		}
		const auto ll = Text::strlen_utf8(s);
		if (ll == 0) {
			return Sstring("");
		}

		auto cptr = s;
		unsigned int byte = static_cast<unsigned char>(*cptr++);
		auto ss = Sstring_of_length("", length);
		auto i = 0;

		while (byte != 0 && i < length) {

			// ascii
			if (byte < 0x80) {
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			if (byte < 0xC0) {
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}

			// skip
			while ((byte < 0xC0) && (byte >= 0x80)) {
				byte = static_cast<unsigned char>(*cptr++);
			}

			if (byte < 0xE0) {
				byte2 = static_cast<unsigned char>(*cptr++);
				if ((byte2 & 0xC0) == 0x80) {
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			if (byte < 0xF0) {
				byte2 = static_cast<unsigned char>(*cptr++);
				const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80)) {
					byte = (((byte & 0x0F) << 12)
						| ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
				}
				continue;
			}

			auto trail = total_bytes[byte] - 1; // expected number of trail bytes
			if (trail > 0) {
				int ch = byte & (0x3F >> trail);
				do {
					byte2 = static_cast<unsigned char>(*cptr++);
					if ((byte2 & 0xC0) != 0x80) {
						Sstring_set(ss, i++, byte);
						byte = static_cast<unsigned char>(*cptr++);
						continue;
					}
					ch <<= 6;
					ch |= (byte2 & 0x3F);
					trail--;
				} while (trail > 0);
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
				continue;
			}
			// no match..
			if (i == ll) {
				break;
			}
			byte = static_cast<unsigned char>(*cptr++);
		}
		return ss;
	}

	 
} // namespace Text


namespace Assoc
{
    ptr sstring(const char* symbol, const char* value)
    {
        ptr a = Snil;
        a = CALL2("cons", Sstring_to_symbol(symbol), Text::constUTF8toSstring(value));
        return a;
    }

    ptr sflonum(const char* symbol, const float value)
    {
        ptr a = Snil;
        a = CALL2("cons", Sstring_to_symbol(symbol), Sflonum(value));
        return a;
    }

    ptr sfixnum(const char* symbol, const int value)
    {
        ptr a = Snil;
        a = CALL2("cons", Sstring_to_symbol(symbol), Sfixnum(value));
        return a;
    }

    ptr sptr(const char* symbol, ptr value)
    {
        ptr a = Snil;
        a = CALL2("cons", Sstring_to_symbol(symbol), value);
        return a;
    }

    ptr cons_sstring(const char* symbol, const char* value, ptr l)
    {
        ptr a = Snil;
        a = CALL2("cons", Sstring_to_symbol(symbol), Text::constUTF8toSstring(value));
        l = CALL2("cons", a, l);
        return l;
    }

    ptr cons_sbool(const char* symbol, bool value, ptr l)
    {
        ptr a = Snil;
        if (value) {
            a = CALL2("cons", Sstring_to_symbol(symbol), Strue);
        }
        else
        {
            a = CALL2("cons", Sstring_to_symbol(symbol), Sfalse);
        }
        l = CALL2("cons", a, l);
        return l;
    }

    ptr cons_sptr(const char* symbol, ptr value, ptr l)
    {
        ptr a = Snil;
        a = CALL2("cons", Sstring_to_symbol(symbol), value);
        l = CALL2("cons", a, l);
        return l;
    }

    ptr cons_sfixnum(const char* symbol, const int value, ptr l)
    {
        ptr a = Snil;
        a = CALL2("cons", Sstring_to_symbol(symbol), Sfixnum(value));
        l = CALL2("cons", a, l);
        return l;
    }

    ptr cons_sflonum(const char* symbol, const float value, ptr l)
    {
        ptr a = Snil;
        a = CALL2("cons", Sstring_to_symbol(symbol), Sflonum(value));
        l = CALL2("cons", a, l);
        return l;
    }
} // namespace Assoc
 

extern "C" __declspec(dllexport) ptr EscapeKeyPressed()
{
    if (GetAsyncKeyState(VK_ESCAPE) != 0)
    {
        return Strue;
    }
    return Sfalse;
}

extern "C" __declspec(dllexport) ptr QuitApplication()
{
    ::ExitProcess(0);
    return Strue;
}

HANDLE every_timer_queue = nullptr;
HANDLE h_every_timer = nullptr;
HANDLE after_timer_queue = nullptr;
ptr every_function;

void stop_every() {
    if (every_timer_queue != nullptr && h_every_timer != nullptr) {
        DeleteTimerQueueTimer(every_timer_queue, h_every_timer, NULL);
        every_timer_queue = nullptr; h_every_timer = nullptr;
    }
}

void swap_buffers(int n);
int swap_mode;
VOID CALLBACK run_every(PVOID lpParam, BOOLEAN TimerOrWaitFired) {

    WaitForSingleObject(g_script_mutex, INFINITE);
    try {
  
        if (Sprocedurep(lpParam)) {
            Scall0(lpParam);
        }
    }
    catch (std::exception e)
    {
        ReleaseMutex(g_script_mutex);
   
        stop_every();
        return;
    }
    ReleaseMutex(g_script_mutex);

    // copy active surface to display surface.
    swap_buffers(swap_mode);

}

void start_every(int delay, int period, ptr p) {

    stop_every();

    if (nullptr == every_timer_queue)
    {
        every_timer_queue = CreateTimerQueue();

        try {
            CreateTimerQueueTimer(&h_every_timer, every_timer_queue,
                static_cast<WAITORTIMERCALLBACK>(run_every), p, delay, period, WT_EXECUTEINTIMERTHREAD);
        }
        catch (std::exception e)
        {
            // Display the exception and quit
            MessageBox(nullptr, L"Error", L"Error", MB_ICONERROR);
            return;
        }
    }
}

// only one thing runs every.
ptr every(int delay, int period, int mode, ptr p)
{
    swap_mode = mode;
    if (delay == 0 || period == 0) {
        stop_every();
        Sunlock_object(every_function);
    }
    else {

        Slock_object(p);
        every_function = p;
        if (Sprocedurep(p)) {
            start_every(delay, period, p);
            return Strue;
        }
        else {
            return Sfalse;
        }
    }
    return Strue;
}

VOID CALLBACK run_after(PVOID lpParam, BOOLEAN TimerOrWaitFired) {

    WaitForSingleObject(g_script_mutex, INFINITE);
 
        if (Sprocedurep(lpParam)) {

			WaitForSingleObject(g_rotation_mutex, INFINITE);
            Scall0(lpParam);
            Sunlock_object(lpParam);
			ReleaseMutex(g_rotation_mutex);
        }
 
    ReleaseMutex(g_script_mutex);
}

void start_after(int delay, ptr p) {
    HANDLE h_after_timer = nullptr;
    Slock_object(p);
    try {

        // do not recreate queue
        if (after_timer_queue == nullptr) {
            after_timer_queue = CreateTimerQueue();
        }
        // add event to queue.
        CreateTimerQueueTimer(&h_after_timer, after_timer_queue,
            static_cast<WAITORTIMERCALLBACK>(run_after), p, delay, 0, WT_EXECUTEINTIMERTHREAD);
    }
    catch (std::exception e)
    {
        // Display the exception and quit
        MessageBox(nullptr, L"Error", L"App died..", MB_ICONERROR);
        return;
    }
}

ptr after(int delay, ptr p)
{
    if (Sprocedurep(p)) {
        start_after(delay, p);
        return Strue;
    }
    return Sfalse;
}

ptr set_repaint_timer(const int n)
{
    KillTimer(main_window, 1000);
    if (n > 0) {
        SetTimer(main_window, 1000, n, NULL);
    }
    return Strue;
}

struct {
	int when;
	boolean left;
	boolean right;
	boolean up;
	boolean down;
	boolean ctrl;
	boolean space;
	int key_code;
} graphics_keypressed;

ptr graphics_keys(void) {
	ptr a = Snil;
	a = Assoc::cons_sbool("left", graphics_keypressed.left, a);
	a = Assoc::cons_sbool("right", graphics_keypressed.right, a);
	a = Assoc::cons_sbool("up", graphics_keypressed.up, a);
	a = Assoc::cons_sbool("down", graphics_keypressed.down, a);
	a = Assoc::cons_sbool("ctrl", graphics_keypressed.ctrl, a);
	a = Assoc::cons_sbool("space", graphics_keypressed.space, a);
	a = Assoc::cons_sfixnum("key", graphics_keypressed.key_code, a);
	a = Assoc::cons_sfixnum("recent", GetTickCount() - graphics_keypressed.when, a);
	return a;
}

extern HANDLE g_script_mutex;
char* get_this_path(char* dest, const size_t dest_size)
{
    if (!dest) return nullptr;
    GetModuleFileNameA(nullptr, dest, dest_size);
    if (MAX_PATH > dest_size) return nullptr;
    PathRemoveFileSpecA(dest);
    return dest;
}

void register_boot_file(const char* boot_file, bool& already_loaded)
{
    const auto maxpath = MAX_PATH + 80;
    char dest[maxpath];
    get_this_path(dest, MAX_PATH + 80);
    strcat_s(dest, boot_file);
    if (!already_loaded &&
        !(INVALID_FILE_ATTRIBUTES == GetFileAttributesA(dest) &&
            GetLastError() == ERROR_FILE_NOT_FOUND))
    {
        already_loaded = true;
        Sregister_boot_file(dest);
    }
}

void load_script_ifexists(const char* script_relative)
{
    const auto maxpath = MAX_PATH + 80;
    char dest[maxpath];
    get_this_path(dest, maxpath);
    strcat_s(dest, script_relative);
    if (!(INVALID_FILE_ATTRIBUTES == GetFileAttributesA(dest) &&
        GetLastError() == ERROR_FILE_NOT_FOUND))
    {
        CALL1("load", Sstring(dest));
    }
}

static void custom_init()
{
}

void abnormal_exit()
{
    MessageBox(nullptr,
        L"Sorry to say; the App has died.",
        L"App has died.",
        MB_OK | MB_ICONERROR);
    exit(1);
}


// set up audio
bool audio_available = false;
ComPtr<IMFAttributes> sourceReaderConfiguration;
IXAudio2* pXAudio2 = NULL;
IXAudio2MasteringVoice* masterVoice;

// sound sample banks
struct sound {
	bool loaded=false;
	int AudioBytes=0;
	int audioDataLength=0;
	WAVEFORMATEX* waveFormat = {};
	unsigned int waveLength=0;
	std::vector<BYTE> audioData = {};
	XAUDIO2_BUFFER audioBuffer = {};
} sounds[bank_size];

void init_audio() {

	HRESULT hr;
	hr = MFStartup(MF_VERSION);
	if (FAILED(hr))  return;
	hr = MFCreateAttributes(sourceReaderConfiguration.GetAddressOf(), 1);
	if (FAILED(hr)) return;
	hr = sourceReaderConfiguration->SetUINT32(MF_LOW_LATENCY, true);
	if (FAILED(hr)) return;
	hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr)) return;
	hr = pXAudio2->CreateMasteringVoice(&masterVoice);
	if (FAILED(hr)) return;
	audio_available = true;
}

int load_sound_data_file(const std::wstring& filename, int n)
{
 
	if (n > bank_size - 1) return 0;
	DWORD streamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;
	HRESULT hr;
	ComPtr<IMFSourceReader> sourceReader;
	ComPtr<IMFMediaType> nativeMediaType;

	hr = MFCreateSourceReaderFromURL(filename.c_str(), sourceReaderConfiguration.Get(), sourceReader.GetAddressOf());
	if (FAILED(hr)) return 0;
	hr = sourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, false);
	if (FAILED(hr)) return 0;
	hr = sourceReader->SetStreamSelection(streamIndex, true);
	 
	ComPtr<IMFMediaType> partialType = nullptr;
	hr = MFCreateMediaType(partialType.GetAddressOf());
	if (FAILED(hr)) return 0;
	hr = partialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	if (FAILED(hr)) return 0;
	hr = partialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	if (FAILED(hr)) return 0;
	hr = sourceReader->SetCurrentMediaType(streamIndex, NULL, partialType.Get());
	if (FAILED(hr)) return 0;
 
	// what sound is not compressed??
	ComPtr<IMFMediaType> uncompressedAudioType = nullptr;
	hr = sourceReader->GetCurrentMediaType(streamIndex, uncompressedAudioType.GetAddressOf());
	if (FAILED(hr)) return 0;
	hr = MFCreateWaveFormatExFromMFMediaType(uncompressedAudioType.Get(), &sounds[n].waveFormat, &sounds[n].waveLength);
	if (FAILED(hr)) return 0;
	hr = sourceReader->SetStreamSelection(streamIndex, true);
	if (FAILED(hr)) return 0;
	ComPtr<IMFSample> sample = nullptr;
	ComPtr<IMFMediaBuffer> buffer = nullptr;
 
	while (true)
	{
		DWORD flags = 0;
		hr = sourceReader->ReadSample(streamIndex, 0, nullptr, &flags, nullptr, sample.GetAddressOf());
		if (FAILED(hr)) return 0;
		if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
			break;
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
			break;

		if (sample == nullptr)
			continue;
		hr = sample->ConvertToContiguousBuffer(buffer.GetAddressOf());
		if (FAILED(hr)) return 0;

		BYTE* AudioData = NULL;
		DWORD AudioDataLength = 0;

		hr = buffer->Lock(&AudioData, nullptr, &AudioDataLength);
		if (FAILED(hr)) return 0;
		for (size_t i = 0; i < AudioDataLength; i++)
			sounds[n].audioData.push_back(AudioData[i]);
	 
		hr = buffer->Unlock();
		AudioData = nullptr;
		sounds[n].loaded = true;
		sounds[n].audioBuffer.AudioBytes = sounds[n].audioData.size();
		sounds[n].audioBuffer.pAudioData = &sounds[n].audioData[0];
		sounds[n].audioBuffer.pContext = nullptr;

	}
	return -1;
}

int load_sound_file(const std::wstring fileName, int n)
{
	HRESULT hr = S_OK;
	auto result = load_sound_data_file(fileName, n );
	return -1;
}

ptr load_sound(char* s,  int n) {
	if (n > bank_size - 1) return Sfalse;
	auto ws = Text::Widen(s).c_str();
	load_sound_file(ws, n);
	return Strue;
}

ptr play_sound(int n) {

	if (n > bank_size - 1) return Sfalse;
	IXAudio2SourceVoice* sourceVoice;
	HRESULT hr = pXAudio2->CreateSourceVoice(&sourceVoice, sounds[n].waveFormat);
	sourceVoice->SubmitSourceBuffer(&sounds[n].audioBuffer);
	sourceVoice->Start();
	
	later(10000, true, [=]() { 
		float pVolume;
		sourceVoice->GetVolume(&pVolume);
		
		while (pVolume > 0.0f) {
			pVolume -= 0.001f;
			Sleep(1);
			sourceVoice->SetVolume(pVolume);
		}
		sourceVoice->DestroyVoice(); });
	return Strue;
}


// direct 2D replaces GDI+

#pragma comment(lib, "d2d1.lib")

float prefer_width = 800.0f;
float prefer_height = 600.0f;

// represents the visible surface on the window itself.
ID2D1HwndRenderTarget* pRenderTarget;

// stoke width 
float d2d_stroke_width = 1.3;
ID2D1StrokeStyle* d2d_stroke_style = nullptr;

// colours and brushes used when drawing
ID2D1SolidColorBrush* pColourBrush = nullptr;       // line-color
ID2D1SolidColorBrush* pfillColourBrush = nullptr;   // fill-color
ID2D1BitmapBrush* pPatternBrush = nullptr;          // brush-pattern
ID2D1BitmapBrush* pTileBrush = nullptr;				// tile its U/S

// we draw into this
ID2D1Bitmap* bitmap= nullptr;
ID2D1BitmapRenderTarget* BitmapRenderTarget = nullptr;
ID2D1Bitmap* bitmap2 = nullptr;
ID2D1BitmapRenderTarget* BitmapRenderTarget2 = nullptr;
 
ID2D1Factory* pD2DFactory;



// hiDPI
float g_DPIScaleX = 1.0f;
float g_DPIScaleY = 1.0f;
float _pen_width = static_cast<float>(1.2);

#pragma warning(disable : 4996)

void d2d_CreateOffscreenBitmap()
{
	if (pRenderTarget == nullptr)
	{
		return;
	}
 
	if (BitmapRenderTarget == NULL) {
		pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(prefer_width, prefer_height), &BitmapRenderTarget);
		BitmapRenderTarget->GetBitmap(&bitmap);
	}
	if (BitmapRenderTarget2 == NULL) {
		pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(prefer_width, prefer_height), &BitmapRenderTarget2);
		BitmapRenderTarget2->GetBitmap(&bitmap2);
	}
}

void swap_buffers(int n) {
 
	WaitForSingleObject(g_rotation_mutex, INFINITE);
	ID2D1Bitmap* temp;
	d2d_CreateOffscreenBitmap();
	if (n == 1) {
		BitmapRenderTarget2->BeginDraw();
		BitmapRenderTarget2->DrawBitmap(bitmap, D2D1::RectF(0.0f, 0.0f, prefer_width, prefer_height));
		BitmapRenderTarget2->EndDraw();
	}
	temp = bitmap;
	bitmap = bitmap2;
	bitmap2 = temp;
	ID2D1BitmapRenderTarget* temptarget;
	temptarget = BitmapRenderTarget;
	BitmapRenderTarget = BitmapRenderTarget2;
	BitmapRenderTarget2 = temptarget;
	ReleaseMutex(g_rotation_mutex);
	if (main_window != nullptr) {
		InvalidateRect(main_window, NULL, FALSE);
	}
	Sleep(1);

}

ptr d2d_color(float r, float g, float b, float a) {
	 
	if (BitmapRenderTarget == NULL)
	{
		return Snil;
	}
	SafeRelease(&pColourBrush);
	HRESULT hr = BitmapRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF(r, g, b, a)),
		&pColourBrush
	);
	return Strue;
}

ptr d2d_fill_color(float r, float g, float b, float a) {


	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	SafeRelease(&pfillColourBrush);
	HRESULT hr = BitmapRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF(r, g, b, a)),
		&pfillColourBrush
	);
	return Strue;
}

ptr d2d_ellipse(float x, float y, float w, float h) {

 
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}

	if (pColourBrush == nullptr) {
		d2d_color(0.0, 0.0, 0.0, 1.0);
	}
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);

	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->DrawEllipse(ellipse, pColourBrush, d2d_stroke_width);
	BitmapRenderTarget->EndDraw();

	return Strue;
}


ptr d2d_line(float x, float y, float x1, float y1) {


	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}

	if (pColourBrush == nullptr) {
		d2d_color(0.0, 0.0, 0.0, 1.0);
	}
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	auto p1 = D2D1::Point2F(x, y);
	auto p2 = D2D1::Point2F(x1, y1);


	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->DrawLine(p1, p2, pfillColourBrush, stroke_width, stroke_style);
	BitmapRenderTarget->EndDraw();

	return Strue;
}
 
ptr d2d_fill_ellipse(float x, float y, float w, float h) {

	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}

	if (pfillColourBrush == nullptr) {
		d2d_fill_color(0.0, 0.0, 0.0, 1.0);
	}
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->FillEllipse(ellipse, pfillColourBrush);
	BitmapRenderTarget->EndDraw();
	return Strue;
}


ptr d2d_rectangle(float x, float y, float w, float h) {
 
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	if (pColourBrush == nullptr) {
		d2d_color(0.0, 0.0, 0.0, 1.0);
	}
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);

	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->DrawRectangle(&rectangle1, pColourBrush, stroke_width);
	BitmapRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_rounded_rectangle(float x, float y, float w, float h, float rx, float ry) {

	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	if (pColourBrush == nullptr) {
		d2d_color(0.0, 0.0, 0.0, 1.0);
	}
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	D2D1_ROUNDED_RECT rectangle1 = { D2D1::RectF(x, y, w, h), rx, ry };
	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->DrawRoundedRectangle(rectangle1, pfillColourBrush, stroke_width, stroke_style);
	BitmapRenderTarget->EndDraw();
	return Strue;
}



ptr d2d_fill_rectangle(float x, float y, float w, float h) {

	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	if (pfillColourBrush == nullptr) {
		d2d_fill_color(0.0, 0.0, 0.0, 1.0);
	}
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->FillRectangle(&rectangle1, pfillColourBrush);
	BitmapRenderTarget->EndDraw();
	return Strue;
}

// reset matrix
ptr d2d_matrix_identity() {
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	BitmapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	return Strue;
}

ptr d2d_matrix_rotate(float a, float x, float y) {
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	BitmapRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y)));
	return Strue;
}

ptr d2d_matrix_translate(float x, float y) {
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	BitmapRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Translation(40, 10));
	return Strue;
}

ptr d2d_matrix_rotrans(float a,float x, float y, float x1, float y1) {
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1,y1);
	BitmapRenderTarget->SetTransform(rot * trans);
	return Strue;
}

ptr d2d_matrix_transrot(float a, float x, float y, float x1, float y1) {
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
	BitmapRenderTarget->SetTransform(trans*rot);
	return Strue;
}

ptr d2d_matrix_scale(float x, float y) {
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	BitmapRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Scale(
			D2D1::Size(x, y),
			D2D1::Point2F(prefer_width/2, prefer_height/2))
	);
	return Strue;
}

ptr d2d_matrix_rotscale(float a, float x, float y, float x1, float y1) {
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(x, y),
		D2D1::Point2F(prefer_width / 2, prefer_height / 2));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	BitmapRenderTarget->SetTransform(rot * scale);
	return Strue;
}

ptr d2d_matrix_scalerot(float a, float x, float y, float x1, float y1) {
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(x, y),
		D2D1::Point2F(prefer_width / 2, prefer_height / 2));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	BitmapRenderTarget->SetTransform(rot * scale);
	return Strue;
}

ptr d2d_matrix_scalerottrans(float a, float x, float y, float x1, float y1, float x2, float y2) {
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(x, y),
		D2D1::Point2F(prefer_width / 2, prefer_height / 2));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
	BitmapRenderTarget->SetTransform(scale*rot*trans);
	return Strue;
}

ptr d2d_matrix_skew(float x, float y) {
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	BitmapRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Skew(
			x,y,
			D2D1::Point2F(prefer_width / 2, prefer_height / 2))
	);
	return Strue;
}

// display current display buffer.
ptr d2d_render(float x, float y) {

	swap_buffers(1);
	if(pRenderTarget == nullptr)
	{
		return Snil;
	}

	pRenderTarget->BeginDraw();
	pRenderTarget->DrawBitmap(bitmap2, D2D1::RectF(x, y, prefer_width, prefer_height));
	pRenderTarget->EndDraw();
	return Strue;
}


void d2d_DPIScale(ID2D1Factory* pFactory)
{
    FLOAT dpiX, dpiY;
    pFactory->GetDesktopDpi(&dpiX, &dpiY);
    g_DPIScaleX = dpiX / 96.0f;
    g_DPIScaleY = dpiY / 96.0f;
}

int d2d_CreateGridPatternBrush(
    ID2D1RenderTarget* pRenderTarget,
    ID2D1BitmapBrush** ppBitmapBrush
)
{

	if (pPatternBrush != nullptr) {
		return 0;
	}
    // Create a compatible render target.
    ID2D1BitmapRenderTarget* pCompatibleRenderTarget = nullptr;
    HRESULT hr = pRenderTarget->CreateCompatibleRenderTarget(
        D2D1::SizeF(10.0f, 10.0f),
        &pCompatibleRenderTarget
    );
    if (SUCCEEDED(hr))
    {
        // Draw a pattern.
        ID2D1SolidColorBrush* pGridBrush = nullptr;
        hr = pCompatibleRenderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF(0.93f, 0.94f, 0.96f, 1.0f)),
            &pGridBrush
        );

		// create offscreen bitmap
		d2d_CreateOffscreenBitmap();

        if (SUCCEEDED(hr))
        {
            pCompatibleRenderTarget->BeginDraw();
            pCompatibleRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, 10.0f, 1.0f), pGridBrush);
            pCompatibleRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.1f, 1.0f, 10.0f), pGridBrush);
            pCompatibleRenderTarget->EndDraw();

            // Retrieve the bitmap from the render target.
            ID2D1Bitmap* pGridBitmap = nullptr;
            hr = pCompatibleRenderTarget->GetBitmap(&pGridBitmap);
            if (SUCCEEDED(hr))
            {
                // Choose the tiling mode for the bitmap brush.
                D2D1_BITMAP_BRUSH_PROPERTIES brushProperties =
                    D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP);

                // Create the bitmap brush.
                hr = pRenderTarget->CreateBitmapBrush(pGridBitmap, brushProperties, ppBitmapBrush);

				SafeRelease(&pGridBitmap);
            }

			SafeRelease(&pGridBrush);
    
        }

		SafeRelease(&pCompatibleRenderTarget);
  
    }

    return hr;
}

void d2d_make_default_stroke() {
    HRESULT r = pD2DFactory->CreateStrokeStyle(
        D2D1::StrokeStyleProperties(
            D2D1_CAP_STYLE_FLAT,
            D2D1_CAP_STYLE_FLAT,
            D2D1_CAP_STYLE_ROUND,
            D2D1_LINE_JOIN_MITER,
            1.0f,
            D2D1_DASH_STYLE_SOLID,
            0.0f),
        nullptr, 0,
        &d2d_stroke_style
    );
}

// direct write
IDWriteFactory* pDWriteFactory;
IDWriteTextFormat* TextFont;
ID2D1SolidColorBrush* pBlackBrush;

  ptr d2d_write_text(float x, float y, char* s) {

	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	if (pfillColourBrush == nullptr) {
		d2d_fill_color(0.0, 0.0, 0.0, 1.0);
	}

	const auto text = Text::widen(s);
	const auto len = text.length();
 
	D2D1_RECT_F layoutRect = D2D1::RectF(x, y, prefer_width-x, prefer_height-y);

	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->DrawTextW(text.c_str(), len, TextFont, layoutRect, pfillColourBrush);
	BitmapRenderTarget->EndDraw();
	return Strue;
}


ptr d2d_set_font(char* s, float size) {

	SafeRelease(&TextFont);
	auto face = Text::Widen(s);
	HRESULT
		hr = pDWriteFactory->CreateTextFormat(
			face.c_str(),
			NULL,
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			size,
			L"en-us",
			&TextFont
		);
	if (SUCCEEDED(hr)) {
		return Strue;
	}
	return Snil;
}


// images bank
ID2D1Bitmap* pSpriteSheet[bank_size];
 
void d2d_sprite_loader(char* filename, int n)
{
	if (n > bank_size-1) {
		return;
	}
	HRESULT hr;
	CoInitialize(NULL);
	IWICImagingFactory* wicFactory = NULL;
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory,
		(LPVOID*)&wicFactory);

	if (FAILED(hr)) {
		return;
	}
	//create a decoder
	IWICBitmapDecoder* wicDecoder = NULL;
	std::wstring fname = Text::Widen(filename);
	hr = wicFactory->CreateDecoderFromFilename(
		fname.c_str(),       
		NULL,           
		GENERIC_READ,   
		WICDecodeMetadataCacheOnLoad,
		&wicDecoder);

	IWICBitmapFrameDecode* wicFrame = NULL;
	hr = wicDecoder->GetFrame(0, &wicFrame);

	// create a converter
	IWICFormatConverter* wicConverter = NULL;
	hr = wicFactory->CreateFormatConverter(&wicConverter);

	// setup the converter
	hr = wicConverter->Initialize(
		wicFrame,                     
		GUID_WICPixelFormat32bppPBGRA,  
		WICBitmapDitherTypeNone,      
		NULL,             
		0.0,           
		WICBitmapPaletteTypeCustom      
	);
	if (SUCCEEDED(hr))
	{
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			wicConverter,
			NULL,
			&pSpriteSheet[n]
		);
	}
	SafeRelease(&wicFactory);
	SafeRelease(&wicDecoder);
	SafeRelease(&wicConverter);
	SafeRelease(&wicFrame);
}

ptr d2d_load_sprites(char* filename, int n) {
	if (n > bank_size - 1) {
		return Snil;
	}
	d2d_sprite_loader(filename,  n);
	return Strue;
}


// from sheet n; at sx, sy to dx, dy, w,h
ptr d2d_render_sprite(int n, float dx, float dy) {

	if (n > bank_size - 1) {
		return Snil;
	}
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, dx+size.width, dy+size.height);
	const auto opacity = 1.0f;
	BitmapRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Scale(
			D2D1::Size(1.0, 1.0),
			D2D1::Point2F(size.width / 2, size.height / 2)));
	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->DrawBitmap(sprite_sheet, dest);
	BitmapRenderTarget->EndDraw();
	return Strue;
}


ptr d2d_render_sprite_rotscale(int n, float dx, float dy, float a, float s) {

	if (n > bank_size - 1) {
		return Snil;
	}
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
	const auto opacity = 1.0f;

	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(s, s),
		D2D1::Point2F(dx,dy));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(dx+(size.width/2), dy+(size.height/2)));

	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->SetTransform(rot * scale);
	BitmapRenderTarget->DrawBitmap(sprite_sheet, dest);
	BitmapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	BitmapRenderTarget->EndDraw();
	return Strue;
}

// from sheet n; at sx, sy to dx, dy, w,h scale up
ptr d2d_render_sprite_sheet(int n, float dx, float dy, float dw, float dh,
							 float sx, float sy, float sw, float sh, float scale) {

	if (n > bank_size - 1) {
		return Snil;
	}

	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}

	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, scale*(dx + dw), scale*(dy + dh));
	const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
	const auto opacity = 1.0f;
	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f, 
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,source);
	BitmapRenderTarget->EndDraw();
	return Strue;
}

// from sheet n; at sx, sy to dx, dy, w,h scale up
ptr d2d_render_sprite_sheet_rot_scale(int n, float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, float scale, float a, float x2, float y2) {

	if (n > bank_size - 1) {
		return Snil;
	}
	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}

	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
	const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
	const auto opacity = 1.0f;
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x2,y2));
	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->SetTransform(rot);
	BitmapRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
	BitmapRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	BitmapRenderTarget->EndDraw();
	return Strue;
}



HRESULT Create_D2D_Device_Dep(HWND h) //before drawing
{
	if (pRenderTarget == NULL) //if it exists, do not create again
	{

		if (pD2DFactory == NULL) {
			HRESULT hr = D2D1CreateFactory(
				D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2DFactory);
			d2d_DPIScale(pD2DFactory);
		}
		if (pDWriteFactory == NULL) {
			HRESULT hr = DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown**>(&pDWriteFactory)
			);
		}
 
		RECT rc;
		GetClientRect(h, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		HRESULT hr = pD2DFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(h,
				D2D1::SizeU((UINT32)rc.right, (UINT32)rc.bottom)),
			&pRenderTarget);

		d2d_CreateGridPatternBrush(pRenderTarget, &pPatternBrush);

		hr = pDWriteFactory->CreateTextFormat(
			L"Consolas",                 
			NULL,                       
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			32.0f,
			L"en-us",
			&TextFont
		);

		hr = pRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Black),
			&pBlackBrush
		);
		return hr;
	}
	return 0;
}
 


DWORD WINAPI execstartup(LPVOID cmd)
{
	try
	{
		Sscheme_init(abnormal_exit);
		bool register_petite = false;
		bool register_cs = false;
		register_boot_file("\\boot\\petite.boot", register_petite);
		register_boot_file("petite.boot", register_petite);
		register_boot_file("\\boot\\scheme.boot", register_cs);
		register_boot_file("scheme.boot", register_cs);


		if (!register_cs && !register_petite)
		{
			MessageBox(nullptr,
				L"The BOOT FILES could not be loaded.",
				L"Looking for petite.boot and cs.boot in boot",
				MB_OK | MB_ICONERROR);
			exit(1);
		}


		Sbuild_heap("SchemeDirect.exe", custom_init);
		Sforeign_symbol("EscapeKeyPressed", static_cast<ptr>(EscapeKeyPressed));
		Sforeign_symbol("set_repaint_timer", static_cast<ptr>(set_repaint_timer));
		Sforeign_symbol("graphics_keys", static_cast<ptr>(graphics_keys));
		Sforeign_symbol("every", static_cast<ptr>(every));
		Sforeign_symbol("after", static_cast<ptr>(after));

		Sforeign_symbol("d2d_matrix_identity", static_cast<ptr>(d2d_matrix_identity));
		Sforeign_symbol("d2d_matrix_rotate", static_cast<ptr>(d2d_matrix_rotate));

		Sforeign_symbol("d2d_render_sprite", static_cast<ptr>(d2d_render_sprite));
		Sforeign_symbol("d2d_render_sprite_rotscale", static_cast<ptr>(d2d_render_sprite_rotscale));


		Sforeign_symbol("d2d_render_sprite_sheet", static_cast<ptr>(d2d_render_sprite_sheet));
		Sforeign_symbol("d2d_render_sprite_sheet_rot_scale", static_cast<ptr>(d2d_render_sprite_sheet_rot_scale));
		Sforeign_symbol("d2d_load_sprites", static_cast<ptr>(d2d_load_sprites));
		Sforeign_symbol("d2d_write_text", static_cast<ptr>(d2d_write_text));
		Sforeign_symbol("d2d_set_font", static_cast<ptr>(d2d_set_font));
		Sforeign_symbol("d2d_color", static_cast<ptr>(d2d_color));
		Sforeign_symbol("d2d_fill_color", static_cast<ptr>(d2d_fill_color));
		Sforeign_symbol("d2d_rectangle", static_cast<ptr>(d2d_rectangle));
		Sforeign_symbol("d2d_fill_rectangle", static_cast<ptr>(d2d_fill_rectangle));
		Sforeign_symbol("d2d_ellipse", static_cast<ptr>(d2d_ellipse));
		Sforeign_symbol("d2d_fill_ellipse", static_cast<ptr>(d2d_fill_ellipse));
		Sforeign_symbol("d2d_render", static_cast<ptr>(d2d_render));

		init_audio();
		Sforeign_symbol("play_sound", static_cast<ptr>(play_sound));
		Sforeign_symbol("load_sound", static_cast<ptr>(load_sound));

		Sforeign_symbol("QuitApplication", static_cast<ptr>(QuitApplication));

		

		// load scripts
		load_script_ifexists("\\scripts\\base.ss");
		load_script_ifexists("\\scripts\\init.ss");
		load_script_ifexists("\\scripts\\env.ss");

		CALL1("suppress-greeting", Strue);
		CALL1("waiter-prompt-string", Sstring(""));
		load_script_ifexists("\\scripts\\appstart.ss");

	}
	catch (std::exception e)
	{
		MessageBox(nullptr, L"Error", L"App died..", MB_ICONERROR);
		return 0;
	}
	return 0;
}




HINSTANCE hInst;                                 
WCHAR szTitle[MAX_LOADSTRING];                
WCHAR szWindowClass[MAX_LOADSTRING];             

 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

 
	// Initialize the Windows Runtime.
	RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
	if (FAILED(initialize))
	{
		exit(-1);
	}


    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DIRECTSCHEME, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

	g_script_mutex = CreateMutex(nullptr, FALSE, nullptr);
	g_rotation_mutex = CreateMutex(nullptr, FALSE, nullptr);
 

	later(100, true, [=]() {
		execstartup((LPVOID)L"");});

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DIRECTSCHEME));



    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}


 
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIRECTSCHEME));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DIRECTSCHEME);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

 
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;  

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, (int)prefer_width, 50+(int)prefer_height, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   main_window = hWnd;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}
 
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

	case WM_KEYDOWN:
		graphics_keypressed.ctrl = false;
		graphics_keypressed.left = false;
		graphics_keypressed.right = false;
		graphics_keypressed.down = false;
		graphics_keypressed.up = false;
		graphics_keypressed.space = false;
		graphics_keypressed.key_code = wParam;
		graphics_keypressed.when = GetTickCount();
		switch (wParam) {

		case VK_CONTROL:
			graphics_keypressed.ctrl = true;
			break;
		case VK_LEFT:
			graphics_keypressed.left = true;
			break;
		case VK_RIGHT:
			graphics_keypressed.right = true;
			break;
		case VK_UP:
			graphics_keypressed.up = true;
			break;
		case VK_DOWN:
			graphics_keypressed.down = true;
			break;
		case VK_SPACE:
			graphics_keypressed.space = true;
			break;

		}
		break;

	case WM_TIMER:
	 

    case WM_PAINT:
        {

			WaitForSingleObject(g_rotation_mutex, INFINITE);
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
            HRESULT  

            hr = Create_D2D_Device_Dep(hWnd);
            if (SUCCEEDED(hr))
            {

                pRenderTarget->Resize(size);
                pRenderTarget->BeginDraw();
                D2D1_SIZE_F renderTargetSize = pRenderTarget->GetSize();
           
				if (bitmap2 == nullptr) {
					pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::LightGray));

					pRenderTarget->FillRectangle(
						D2D1::RectF(0.0f, 0.0f, renderTargetSize.width, renderTargetSize.height), pPatternBrush);
				}
				if (bitmap2 != nullptr) {
					pRenderTarget->DrawBitmap(bitmap2, D2D1::RectF(0.0f, 0.0f, prefer_width, prefer_height));
				}
                hr = pRenderTarget->EndDraw();
            }

            if (hr == D2DERR_RECREATE_TARGET)
            {
				SafeRelease(&pRenderTarget);
            }
            else
            {
                ::ValidateRect(hWnd, NULL);
            }
       
            EndPaint(hWnd, &ps);
			ReleaseMutex(g_rotation_mutex);
        }
        break;

	case WM_ERASEBKGND:
		return TRUE;
		break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

