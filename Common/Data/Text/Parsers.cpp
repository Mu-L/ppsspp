#include <algorithm>
#include <cstdarg>
#include <climits>
#include <cstdio>
#include <string>

#include "Common/Data/Text/Parsers.h"
#include "Common/Data/Text/I18n.h"
#include "Common/StringUtils.h"

// Not strictly a parser...
void NiceSizeFormat(uint64_t size, char *out, size_t bufSize) {
	int s = 0;
	int frac = 0;
	while (size >= 1024) {
		s++;
		frac = (int)size & 1023;
		size /= 1024;
	}
	float f = (float)size + ((float)frac / 1024.0f);
	if (s == 0)
		snprintf(out, bufSize, "%d B", (int)size);
	else {
		static const char* const sizes[] = { "B","KB","MB","GB","TB","PB","EB" };
		snprintf(out, bufSize, "%3.2f %s", f, sizes[s]);
	}
}

std::string NiceSizeFormat(uint64_t size) {
	char buffer[16];
	NiceSizeFormat(size, buffer, sizeof(buffer));
	return std::string(buffer);
}

std::string NiceTimeFormat(int seconds) {
	auto di = GetI18NCategory(I18NCat::DIALOG);
	if (seconds < 60) {
		return StringFromFormat(di->T_cstr("%d seconds"), seconds);
	} else if (seconds < 60 * 60) {
		int minutes = seconds / 60;
		return StringFromFormat(di->T_cstr("%d minutes"), minutes);
	} else {
		int hours = seconds / 3600;
		return StringFromFormat(di->T_cstr("%d hours"), hours);
	}
}

bool Version::ParseVersionString(std::string str) {
	if (str.empty())
		return false;
	if (str[0] == 'v')
		str = str.substr(1);
	if (3 != sscanf(str.c_str(), "%i.%i.%i", &major, &minor, &sub)) {
		sub = 0;
		if (2 != sscanf(str.c_str(), "%i.%i", &major, &minor))
			return false;
	}
	return true;
}

std::string Version::ToString() const {
	char temp[128];
	snprintf(temp, sizeof(temp), "%i.%i.%i", major, minor, sub);
	return std::string(temp);
}

int Version::ToInteger() const {
	// This allows for ~2000 major versions, ~100 minor versions, and ~10000 sub versions.
	return major * 1000000 + minor * 10000 + sub;
}

bool ParseMacAddress(const std::string &str, uint8_t macAddr[6]) {
	unsigned int mac[6];
	if (6 != sscanf(str.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])) {
		return false;
	}
	for (int i = 0; i < 6; i++) {
		macAddr[i] = mac[i];
	}
	return true;
}

static bool TryParseUnsigned32(const std::string &str, uint32_t *const output) {
	char *endptr = NULL;

	// Holy crap this is ugly.

	// Reset errno to a value other than ERANGE
	errno = 0;

	unsigned long value = strtoul(str.c_str(), &endptr, 0);

	if (!endptr || *endptr)
		return false;

	if (errno == ERANGE)
		return false;

	if (ULONG_MAX > UINT_MAX) {
#ifdef _MSC_VER
#pragma warning (disable:4309)
#endif
		// Note: The typecasts avoid GCC warnings when long is 32 bits wide.
		if (value >= static_cast<unsigned long>(0x100000000ull)
			&& value <= static_cast<unsigned long>(0xFFFFFFFF00000000ull))
			return false;
	}

	*output = static_cast<uint32_t>(value);
	return true;
}

bool TryParse(const std::string &str, uint32_t *const output) {
	if (str[0] != '#') {
		return TryParseUnsigned32(str, output);
	} else {
		// Parse it as "#RGBA" and convert to a ABGR interger
		std::string s = ReplaceAll(str, "#", "0x");
		if (TryParseUnsigned32(s, output)) {
			int a = (*output >> 24) & 0xff;
			int b = (*output >> 16) & 0xff;
			int g = (*output >> 8) & 0xff;
			int r = *output & 0xff;
			*output = (r << 24) | (g << 16) | (b << 8) | a;
			return true;
		} else {
			return false;
		}
	}
}

bool TryParse(const std::string &str, uint64_t *const output) {
	char *endptr = NULL;

	// Holy crap this is ugly.

	// Reset errno to a value other than ERANGE
	errno = 0;

	uint64_t value = strtoull(str.c_str(), &endptr, 0);

	if (!endptr || *endptr)
		return false;

	if (errno == ERANGE)
		return false;

	*output = value;
	return true;
}

bool TryParse(const std::string &str, bool *const output) {
	if ("1" == str || !strcasecmp("true", str.c_str()))
		*output = true;
	else if ("0" == str || !strcasecmp("false", str.c_str()))
		*output = false;
	else
		return false;

	return true;
}

StringWriter &StringWriter::F(const char *format, ...) {
	const size_t remainder = bufSize_ - (p_ - start_);
	if (remainder < 3) {
		return *this;
	}
	va_list args;
	va_start(args, format);
	int wouldHaveBeenWritten = vsnprintf(p_, remainder, format, args);
	p_ += std::min((int)remainder, wouldHaveBeenWritten);
	va_end(args);
	return *this;
}
